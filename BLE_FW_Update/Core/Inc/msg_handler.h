/*
 * msg_handler.h
 *
 *  Created on: 24 apr 2023
 *      Author: Francesco Olivieri
 */

#ifndef INC_MSG_HANDLER_H_
#define INC_MSG_HANDLER_H_


/* ----- UTILS ----- */

typedef enum{
	false,
	true
}bool;


/*
 * !!! IT SUMS ALSO THE "end" INDEX !!!
 */
uint8_t sum_payload(uint8_t *payload, uint8_t start, uint8_t end);



/* ----- PACKETS UTILS ----- */
// length in bytes
#define CHAT_DATA_LEN 20

#define START_FLASH_MODE_CMD 0xbb

#define START_FLASH_MODE_LEN 4
#define WRITE_FLASH_PACKET_RESPONSE_LEN 5

typedef enum{
	START_FLASH_MODE,
	WRITE_FLASH_PACKET,
	WRITE_FLASH_PACKET_RESPONSE
}PckType;

/*
 * Checks if the packet received is the one expected
 */
bool verify_data(uint8_t *data_buffer, uint8_t num_bytes, PckType expected_pck);


/* ----- CONNECTION UTILS ----- */

typedef enum{
	IDLE,
	CONNECTED,
	RECEVEING_FW,
	CLOSING_CONNECTION
}ConnectionStatus;

void data_handler(uint8_t *data_buffer, uint8_t num_bytes);
void send_ack(bool ack, PckType reply_type);
extern void Update_TX_Char(uint8_t *data_buffer, uint8_t num_bytes);


#endif /* INC_MSG_HANDLER_H_ */
