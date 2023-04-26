/*
 * msg_handler.c
 *
 *  Created on: 24 apr 2023
 *      Author: Francesco Olivieri
 */

#include <stdio.h>
#include "msg_handler.h"

#include "flash_manager.h"

ConnectionStatus connection_status = IDLE;

uint64_t data_to_flash[160];
uint16_t consd = -1;

uint16_t total_pck = 0;
uint16_t count_pck = 0;

bool next_ack = true;

void data_handler(uint8_t *data_buffer, uint8_t num_bytes){
	//HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);

	int i;
	for(i=0 ; i<num_bytes && num_bytes<=CHAT_DATA_LEN ; i++){
		printf("%02x", data_buffer[i]);
	}
	printf("\n\r");

	switch(connection_status){
		case CONNECTED:   // receive START FLASH MODE pck
			if(verify_data(data_buffer, num_bytes, START_FLASH_MODE) == true){
				total_pck = (data_buffer[1] << 8) + data_buffer[2];
				printf("I expect %d packets \n\r", total_pck);


				send_ack(true, WRITE_FLASH_PACKET_RESPONSE);
				connection_status = RECEVEING_FW;
				printf("READY TO RECEIVE FW\n\r");
			}else{
				send_ack(false, WRITE_FLASH_PACKET_RESPONSE);

				printf("Received an init pck that I can't handle or wrong\n\r");
			}

			break;
		case RECEVEING_FW:
			if(verify_data(data_buffer, num_bytes, WRITE_FLASH_PACKET)){
				uint16_t pck_num = (data_buffer[1] << 8) + data_buffer[2];

				if(pck_num == count_pck){
					// save msg
					for(int i=3 ; i<num_bytes-1 ; i++){
						if((i-3)%8 != 0){
							data_to_flash[consd] += ((uint64_t)data_buffer[i] << ((i-3)%8)*8 );
						}else{
							consd++;
							data_to_flash[consd] = ((uint64_t)data_buffer[i] << ((i-3)%8)*8 );
						}
					}
					count_pck++;
				}else{
					printf("Wrong pck_num %d, %d \n\r", pck_num, count_pck);
					next_ack = false;
				}

				if(count_pck%10 == 0 || count_pck == total_pck){

					Write_FW_to_flash(data_to_flash, consd+1);
					consd = -1;

					send_ack(next_ack, WRITE_FLASH_PACKET_RESPONSE);

					if(next_ack == false){
						// reset the buffer properly
					}else{
						// you can save properly
					}
					next_ack = true;

				}
			}else{
				printf("Ricevuto pckt sbagliato \n\r");
				next_ack = false;
			}

			break;

		default:
			printf("Connection status handle not found\n\r");

			break;
	}
	fflush(stdout);

}

bool verify_data(uint8_t *data_buffer, uint8_t num_bytes, PckType expected_pck){
	uint8_t rcv_crc = data_buffer[num_bytes-1];;

	uint8_t checksum = 0;

	switch(expected_pck){
		case START_FLASH_MODE:

			/* check cmd and lenght of the msg */
			if(data_buffer[0] != START_FLASH_MODE_CMD || num_bytes != START_FLASH_MODE_LEN)
				return false;

			checksum = sum_payload(data_buffer, 1, num_bytes-2); // -2 because sum_payload include end index
			/*
			for(int i=0; i<16 && i+1<num_bytes-1 ; i++){
				checksum += data_buffer[i+1];
			}*/
			break;

		case WRITE_FLASH_PACKET:
			// add controls
			checksum = sum_payload(data_buffer, 3, num_bytes-2);
			/*
			for(int i=0; i<16 && i+3<num_bytes-1 ; i++){
				checksum += data_buffer[i+3];
			}*/
			break;

		default:
			printf("Can't verify this data\n\r");

			return false;
	}

	//printf("ck rcv: %d, clc: %d", rcv_crc, checksum);

	if(checksum == rcv_crc){
		return true;
	}else{
		return false;
	}
}


void send_ack(bool ack, PckType reply_type){
	uint8_t ack_msg;

	if(ack == true){
		ack_msg = 0x00;
	}else{
		ack_msg = 0x01;
	}

	uint8_t msg_len = 0;
	switch(reply_type){
		case WRITE_FLASH_PACKET_RESPONSE:
			msg_len = WRITE_FLASH_PACKET_RESPONSE_LEN;
			uint8_t msg[WRITE_FLASH_PACKET_RESPONSE_LEN];

			msg[0] = ack_msg;
			msg[1] = 0x00; // CMD ?!
			msg[2] = (count_pck >> 8) & 0xff;
			msg[3] = (count_pck & 0xff);
			msg[4] = sum_payload(msg, 2, 3);

			//printf("%02x", msg);

			Update_TX_Char(msg, msg_len);
			break;

		default:
			printf("Can't handle the reply_type\n\r");
			return;
			break;
	}

}


uint8_t sum_payload(uint8_t *payload, uint8_t start, uint8_t end){
	uint8_t sum = 0;

	for(int i=start; i<=end ;i++){
		sum += payload[i];
	}
	sum = sum & 0xff;

	return sum;
}

