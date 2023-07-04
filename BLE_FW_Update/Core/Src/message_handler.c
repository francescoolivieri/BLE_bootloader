/*
 * msg_handler.c
 *
 *  Created on: 24 apr 2023
 *      Author: Francesco Olivieri
 */

#include <message_handler.h>
#include <string.h>
#include "flash_manager.h"
#include "rand_generator.h"
#include "cmox_crypto.h"

#include "tim.h"

#include <stdlib.h>
#include <time.h>

ConnectionStatus connection_status = IDLE;

uint64_t data_to_flash[(PAYLOAD_LEN * 16)/8 ]; /* BE CAREFUL WITH PADDING */
uint8_t cont_byte_pos = 0;
uint16_t consd = -1;

uint16_t total_pck = 0;
uint16_t count_pck = 0;

bool next_ack = true;

/* CRYPTO UTILS */
const uint8_t Key[] =
{
  0x46, 0x3b, 0x41, 0x29, 0x11, 0x76, 0x7d, 0x57, 0xa0, 0xb3, 0x39, 0x69, 0xe6, 0x74, 0xff, 0xe7,
  0x84, 0x5d, 0x31, 0x3b, 0x88, 0xc6, 0xfe, 0x31, 0x2f, 0x3d, 0x72, 0x4b, 0xe6, 0x8e, 0x1f, 0xca
};

// IV ATTENZIONE
#define IV_LEN 12

uint8_t IV[] =
{
  0x61, 0x1c, 0xe6, 0xf9, 0xa6, 0x88, 0x07, 0x50, 0xde, 0x7d, 0xa6, 0xcb
};

/*
 * ENCRYPTION
 * TODO: handle properly the counter
 * TODO: import correctly cmox library
 * TODO: what if I receive a non crypted cmd ?!?=""£$)$+ù  X
 * TODO: padding data?
 * TODO: num_ack variable
 * TODO: nak from pc
*/

void data_handler(uint8_t *data_buffer, uint8_t num_bytes){
	//HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);

	int i;
	for(i=0 ; i<num_bytes && num_bytes<=CHAT_DATA_LEN ; i++){
		printf("%02x", data_buffer[i]);
	}
	printf("\n\r");

	switch(connection_status){
		case CONNECTED:   // receive START FLASH MODE pck
			if(verify_data_type(data_buffer, num_bytes, START_FLASH_MODE) == true){
				total_pck = (data_buffer[1] << 8) + data_buffer[2];
				printf("I expect %d packets \n\r", total_pck);  // TODO: CHECK SPACE

				// prepare memory for the new FW
				Erase_Application_Memory();

				send_ack(true, WRITE_FLASH_PACKET_RESPONSE);

				connection_status = RECEVEING_RAW_FW;
				printf("READY TO RECEIVE FW\n\r");

			}else if(verify_data_type(data_buffer, num_bytes, START_SECURE_FLASH_MODE) == true){
				total_pck = (data_buffer[1] << 8) + data_buffer[2]; // la prima volta ricevo il numero completo
				printf("I expect %d packets \n\r", total_pck);

				init_crypto();
				init_rand_generator(__HAL_TIM_GetCounter(&htim1));

				// prepare memory for the new FW
				Erase_Application_Memory();

				send_ack(true, WRITE_SECURE_FLASH_PACKET_RESPONSE);
				connection_status = RECEVEING_SECURE_FW;

			}else{
				send_ack(false, WRITE_FLASH_PACKET_RESPONSE);

				printf("Received an init pck that I can't handle or wrong\n\r");
			}

			break;
		case RECEVEING_RAW_FW:

			if(verify_data_type(data_buffer, num_bytes, WRITE_FLASH_PACKET)){
				uint16_t pck_num = (data_buffer[1] << 8) + data_buffer[2];

				if(pck_num == count_pck){
					// save msg, it will be written in the FLASH

					for(int i=3 ; i<num_bytes-1 ; i++){
						if(cont_byte_pos != 0){
							data_to_flash[consd] += ((uint64_t)data_buffer[i] << cont_byte_pos*8) ; // %8 -> 8 uint8_t * 8 = 1 uint64_t
						}else{
							consd++; // NOTICE it's initialized as (-1) -> OK also for first time
							data_to_flash[consd] = ((uint64_t)data_buffer[i] << cont_byte_pos*8) ; // %8 -> 8 uint8_t * 8 = 1 uint64_t
						}
						cont_byte_pos++;
						cont_byte_pos %= 8;
					}

				}else{
					printf("Wrong pck_num %d, %d \n\r", pck_num, count_pck);
					next_ack = false;
				}

			}else{

				printf("Ricevuto pckt sbagliato \n\r");
				next_ack = false;
			}
			count_pck++;

			if(count_pck%RAW_NUM_CUMULATIVE_ACK == 0 || count_pck == total_pck){

				send_ack(next_ack, WRITE_FLASH_PACKET_RESPONSE);

				if(next_ack == false){
					count_pck -= RAW_NUM_CUMULATIVE_ACK;    // set back the counter packet
				}else{

					Write_FW_to_flash(data_to_flash, consd+1); // you can save properly since no error found in pckts
				}

				if(count_pck == total_pck){
					printf("RAW FW Received!\n\r");
					connection_status = CLOSING_CONNECTION;
				}

				consd = -1;
				next_ack = true;

			}

			break;

		case RECEVEING_SECURE_FW:
			if(verify_data_type(data_buffer, num_bytes, WRITE_SECURE_FLASH_PACKET)){
				// if SECURE
				uint16_t pck_num = ((data_buffer[0] & 0xf) << 8) + data_buffer[1];
				decrypt_data(&data_buffer[2], 18, data_buffer, 2);

				if(pck_num == count_pck){
					// save msg, it will be written in the FLASH
					for(int i=2 ; i<num_bytes-4 ; i++){
						if(cont_byte_pos != 0){
							data_to_flash[consd] += ((uint64_t)data_buffer[i] << cont_byte_pos*8) ; // %8 -> 8 uint8_t * 8 = 1 uint64_t
						}else{
							consd++; // NOTICE it's initialized as (-1) -> OK also for first time
							data_to_flash[consd] = ((uint64_t)data_buffer[i] << cont_byte_pos*8) ; // %8 -> 8 uint8_t * 8 = 1 uint64_t
						}
						cont_byte_pos++;
						cont_byte_pos %= 8;
					}

				}else{
					printf("Wrong pck_num %d, %d \n\r", pck_num, count_pck);
					next_ack = false;
				}

			}else{
				printf("Ricevuto pckt sbagliato \n\r");
				next_ack = false;
			}
			count_pck++;

			if(count_pck%SECURE_NUM_CUMULATIVE_ACK == 0 || count_pck == total_pck){

				send_ack(next_ack, WRITE_SECURE_FLASH_PACKET_RESPONSE);

				if(next_ack == false){
					count_pck -= SECURE_NUM_CUMULATIVE_ACK;    // set back the counter packet
				}else{

					Write_FW_to_flash(data_to_flash, consd+1); // you can save properly since no error found in pckts
				}

				if(count_pck == total_pck){
					printf("SECURE FW Received!\n\r");
					connection_status = CLOSING_CONNECTION;
				}

				consd = -1;
				next_ack = true;

			}

			break;

		default:
			printf("Connection status handle not found\n\r");

			break;
	}
	fflush(stdout);

}

bool verify_data_type(uint8_t *data_buffer, uint8_t num_bytes, PckType expected_pck){
	uint8_t rcv_crc = data_buffer[num_bytes-1];;

	uint8_t checksum = 0;

	switch(expected_pck){
		case START_FLASH_MODE:
			/* check cmd and lenght of the msg */
			if(data_buffer[0] != START_FLASH_MODE_CMD || num_bytes != START_FLASH_MODE_LEN)
				return false;

			checksum = sum_payload(data_buffer, 1, num_bytes-2); // -2 because sum_payload include end index

			break;

		case START_SECURE_FLASH_MODE:
			/* check cmd and lenght of the msg */
			if(data_buffer[0] != START_SECURE_FLASH_MODE_CMD || num_bytes != START_SECURE_FLASH_MODE_LEN)
				return false;

			checksum = sum_payload(data_buffer, 1, num_bytes-2); // -2 because sum_payload include end index

			break;

		case WRITE_FLASH_PACKET:
			// add controls
			if(data_buffer[0] != WRITE_FLASH_PACKET_CMD)
				return false;

			checksum = sum_payload(data_buffer, 3, num_bytes-2);

			break;

		case WRITE_SECURE_FLASH_PACKET:
			if((data_buffer[0] >> 4) != WRITE_SECURE_FLASH_PACKET_CMD)
				return false;

			return true;
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

uint8_t rand_IV[3] = {0, 0, 0};

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

		case WRITE_SECURE_FLASH_PACKET_RESPONSE:
			msg_len = WRITE_SECURE_FLASH_PACKET_RESPONSE_LEN;
			uint8_t msg_crypted[WRITE_SECURE_FLASH_PACKET_RESPONSE_LEN];

			uint8_t plaintext[5] = { (uint8_t) ((count_pck >> 8) & 0xff), (uint8_t) (count_pck & 0xff)};

			for(int i=2 ; i<5 ; i++){
				rand_IV[i-2] = get_rand_byte();
				plaintext[i] = rand_IV[i-2];
				printf("%d ", rand_IV[i-2]);
			}
			printf("\n\r");

			uint8_t ciphertext[5];
			uint8_t tag[AES_TAG_LEN];

			encrypt_data(plaintext, sizeof(plaintext), &ack_msg, 1 , ciphertext, tag);

			msg_crypted[0] = ack_msg;
			msg_crypted[1] = ciphertext[0];
			msg_crypted[2] = ciphertext[1];
			msg_crypted[3] = ciphertext[2];
			msg_crypted[4] = ciphertext[3];
			msg_crypted[5] = ciphertext[4];
			msg_crypted[6] = tag[0];
			msg_crypted[7] = tag[1];
			msg_crypted[8] = tag[2];
			msg_crypted[9] = tag[3];

			// printf("%02x", msg);

			Update_TX_Char(msg_crypted, msg_len);
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

void init_crypto(){

	if (cmox_initialize(NULL) != CMOX_INIT_SUCCESS)
	{
		printf("CRYPTO ERROR\r\n");
	}
}

/* should receive payload  and tag */
void decrypt_data(uint8_t *ciphertext, uint8_t num_bytes, uint8_t *add_data, uint8_t add_data_len){
	cmox_cipher_retval_t retval;
	uint8_t Computed_Plaintext[num_bytes];
	size_t computed_size;

	// calc new IV could use also SHA
	IV[(rand_IV[0] >> 4)%IV_LEN] = (IV[(rand_IV[0] >> 4)%IV_LEN] + (rand_IV[0] & 0x0f)) % 256;
	IV[(rand_IV[1] >> 4)%IV_LEN] = (IV[(rand_IV[1] >> 4)%IV_LEN] + (rand_IV[1] & 0x0f)) % 256;
	IV[(rand_IV[2] >> 4)%IV_LEN] = (IV[(rand_IV[2] >> 4)%IV_LEN] + (rand_IV[2] & 0x0f)) % 256;

	//printf("%d, %d, %d e val: %d\n\r",rand_IV[0] ,(rand_IV[0] >> 4)%IV_LEN, (rand_IV[0] & 0x0f) , IV[(rand_IV[0] >> 4)%IV_LEN]);
	//printf("%d, %d, %d e val: %d\n\r",rand_IV[1] ,(rand_IV[1] >> 4)%IV_LEN, (rand_IV[1] & 0x0f) , IV[(rand_IV[1] >> 4)%IV_LEN]);
	//printf("%d, %d, %d e val: %d\n\r",rand_IV[2] ,(rand_IV[2] >> 4)%IV_LEN, (rand_IV[2] & 0x0f) , IV[(rand_IV[2] >> 4)%IV_LEN]);


	retval = cmox_aead_decrypt(CMOX_AES_GCM_DEC_ALGO,                  /* Use AES GCM algorithm */
	                             ciphertext, num_bytes,     /* Ciphertext + tag to decrypt and verify */
	                             4,                   /* Authentication tag size */
	                             Key, sizeof(Key),                       /* AES key to use */
	                             IV, sizeof(IV),                         /* Initialization vector */
	                             add_data, add_data_len,               /* Additional authenticated data */
	                             Computed_Plaintext, &computed_size);    /* Data buffer to receive generated plaintext */

	  /* Verify API returned value -> decryption and tag verification successfull */
	  if (retval != CMOX_CIPHER_AUTH_SUCCESS)
	  {
		  printf("Probabilmente tag fallito\n\r");
	  }

	  /* Verify generated data size is the expected one */
	  if (computed_size != num_bytes - AES_TAG_LEN)
	  {
	      printf("IROR\n\r");
	  }

	  memcpy(ciphertext, Computed_Plaintext, computed_size);
}


/*
 * NOTE: ciphertext should be long: sizeof(plaintext) while tag: AES_TAG_LEN
 *
 * 		UNITA PLAINTEXT
 */
void encrypt_data(uint8_t *plaintext, uint8_t plaintext_len, uint8_t *add_data, uint8_t add_data_len, uint8_t *ciphertext, uint8_t *tag){
	cmox_cipher_retval_t retval;
	uint8_t Computed_Ciphertext[plaintext_len + AES_TAG_LEN];
	size_t computed_size;

	if(plaintext_len + add_data_len + AES_TAG_LEN > 20){
		printf("Message Length Not Supported by BLE\n\r");
		return;
	}

	if(plaintext_len < 16){
		for(int i=plaintext_len ; i<16 ; i++){
			plaintext[i] = 0x00;
		}
	}

	retval = cmox_aead_encrypt(CMOX_AES_GCM_ENC_ALGO,                  /* Use AES GCM algorithm */
	                             plaintext, plaintext_len,           /* Plaintext to encrypt */
								 AES_TAG_LEN,                   /* Authentication tag size */
	                             Key, sizeof(Key),                       /* AES key to use */
	                             IV, sizeof(IV),                         /* Initialization vector */
	                             add_data, add_data_len,               /* Additional authenticated data */
	                             Computed_Ciphertext, &computed_size);   /* Data buffer to receive generated ciphertext
	                                                                        and authentication tag */


	/* Verify API returned value -> encryption and tag generation successful */
	if (retval != CMOX_CIPHER_SUCCESS)
	{
	    printf("Encryption and tag generation failed");
	}

	/* Verify generated data size is the expected one */
	if (computed_size != (plaintext_len + AES_TAG_LEN))
	{
	  printf("Error in the computation of ciphertext and tag\n\r");
	}

	  memcpy(ciphertext, Computed_Ciphertext, plaintext_len);
	  memcpy(tag, &Computed_Ciphertext[plaintext_len], AES_TAG_LEN);

	  /*
	  printf("ciphertext:");
	  	for(int i=0; i<plaintext_len ; i++){
	  		printf("%02x", ciphertext[i]);
	  	}
	  	printf(" add_data:");
	  	for(int i=0; i<add_data_len ; i++){
	  		printf("%02x", add_data[i]);
	  	}
	  	printf(" tag:");
	  	for(int i=0; i<AES_TAG_LEN ; i++){
	  		printf("%02x", tag[i]);
	  	}
	  	printf(" iv:");
	  		for(int i=0; i<IV_LEN ; i++){
	  			printf("%02x", IV[i]);
	  		}
	  	printf("\n\r");
	  	*/
}
