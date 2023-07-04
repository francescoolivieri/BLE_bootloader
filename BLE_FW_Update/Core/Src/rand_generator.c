/*
 * rand_generator.c
 *
 *  Created on: 2 lug 2023
 *      Author: Francesco Olivieri
 */
#include "rand_generator.h"

uint16_t reg;

void init_rand_generator(uint16_t initializer){
	reg = initializer & 0xFFFF;
}

uint8_t get_rand_byte(){
	uint8_t i = 0;
	uint8_t num = 0;

	while(i < 8){
		num += (reg & 1) << i;

	    reg = (reg >> 1) | (((reg >> 16 & 1) ^ (reg >> 15 & 1) ^ (reg >> 13 & 1) ^ (reg >> 4 & 1) ^ (reg >> 0 & 1) )<<15);

	    i++;
	}

	return num;
}

