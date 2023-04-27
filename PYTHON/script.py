import asyncio
import binascii

import bincopy

from bleak import BleakScanner
from bleak import BleakClient
import time
import tkinter as tk

from enum import Enum

address = "06:05:04:03:02:01"

TNS_UUID = "984f3988-b8ae-49f4-9d89-fc6b2b987b88"
RCV_UUID = "984f3988-b8ae-4af4-9d89-fc6b2b987b88"


""""
window = tk.Tk()
window.geometry("600x500")
window.title("PitoneBLE")
window.resizable(False, False)
window.grid_columnconfigure(0, weight=1)

address_label = tk.Label(window, text="Insert Device Address: ", font=("Helvetica", 15))
address_label.grid(row=0, column=0, sticky="N", padx=20, pady=10)

address_input = tk.Entry(window)
address_input.insert(0, address)
address_input.grid(row=0, column=1, padx=10)

dv_info_button = tk.Button(text="Device Info", command=print_device_info)
dv_info_button.grid(row=1, column=0)
"""

#if __name__ == "__main__":
    #loop = asyncio.get_event_loop()
    #loop.run_until_complete(print_device_info())
    #window.mainloop()

ack_rcv = False
is_ack = True


def split_file_into_chunk(data, chunk_size=32):
    for i in range(0, len(data), chunk_size):
        yield data[i:i+chunk_size]


async def read_response(sender, data):
    global ack_rcv
    global is_ack

    str = binascii.hexlify(bytearray(data))
    print(str)
    if data[0] != 0x00:
        is_ack = False
        print("NAK RECEIVED!")
    else:
        is_ack = True

    ack_rcv = True
        

class ConnectionStatus(Enum):
    IDLE = 0
    INIT_PHASE = 1
    SENDING_DATA = 2
    CLOSING_PHASE = 3


async def send_new_fw():
    sbaglia = True
    connection_status = ConnectionStatus.IDLE
    async with BleakClient(address) as client:
        print("Connected")
        connection_status = ConnectionStatus.IDLE

        global ack_rcv  # notifies when an ack is received
        global is_ack

        # prepare new firmware
        f = bincopy.BinFile()
        f.add_srec_file("Test.srec")
        data = f.as_binary()
        data = bytearray(data)

        # prepare chunks of 16 bytes
        chunk_size = 16
        chunks = [data[i:i + chunk_size] for i in range(0, len(data), chunk_size)]
        num_chunks = len(chunks)  # 952 for Test.srec

        await client.start_notify(RCV_UUID, read_response)  # client ready to receive data (ack/nak)

        print("Settings completed")
        connection_status = ConnectionStatus.INIT_PHASE

        # !!! SHOULD CHECK IF CONNECTED !!!
        # start logic of the program
        while connection_status != ConnectionStatus.CLOSING_PHASE:
            if connection_status == ConnectionStatus.INIT_PHASE:   # START FLASH MODE PACKET
                # send start flash mode message
                msg = bytearray()
                msg += bytearray.fromhex("bb")
                msg += num_chunks.to_bytes(2, byteorder='big')
                checksum = (msg[-2] + msg[-1]) & 0xFF
                msg += checksum.to_bytes(1, byteorder='big')

                # send pck
                await client.write_gatt_char(TNS_UUID, msg, response=False)

                # wait for ACK
                while not ack_rcv:
                    await asyncio.sleep(0.005)
                    pass

                ack_rcv = False
                # CHECK IF ACK OR NAK
                await asyncio.sleep(0.005)
                connection_status = ConnectionStatus.SENDING_DATA

            elif connection_status == ConnectionStatus.SENDING_DATA: # send new FW
                cont_chunck = 0
                while cont_chunck < num_chunks:
                    chunk = chunks[cont_chunck]

                    msg = bytearray()
                    msg += bytearray.fromhex("bb")
                    msg += cont_chunck.to_bytes(2, byteorder='big')
                    msg += chunk
                    checksum = sum(msg[-len(chunk):]) & 0xFF
                    if cont_chunck == 15 and sbaglia:
                        msg += 0x01.to_bytes(1, byteorder='big')
                        sbaglia = False
                    else:
                        msg += checksum.to_bytes(1, byteorder='big')

                    await client.write_gatt_char(TNS_UUID, msg, response=False)
                    # print("Sent message")
                    cont_chunck += 1

                    if cont_chunck%10 == 0 or cont_chunck == num_chunks:
                        # wait for ACK
                        while not ack_rcv:
                            await asyncio.sleep(0.005)
                            pass

                        if not is_ack:
                            cont_chunck -= 10   # send again the last 10 chuncks of FW

                        ack_rcv = False
                    else:
                        await asyncio.sleep(0.008)  # time to the board to process the received data

                connection_status = ConnectionStatus.CLOSING_PHASE

        # FW sent, now I can end the connection
        await client.stop_notify(RCV_UUID)
        await client.disconnect()
        connection_status = ConnectionStatus.IDLE
        print("Disconnected")


async def scan():
    async with BleakScanner() as scanner:
        devices = await scanner.discover()
        for dev in devices:
            print(dev)


async def is_device_active(address):
    async with BleakScanner() as scanner:
        boh = await scanner.find_device_by_address(address)
        if(boh == None):
            print(f"Device with address: ({address}) not found!")
            return False
        else:
            print("Device found!")
            return True


asyncio.run(send_new_fw())
