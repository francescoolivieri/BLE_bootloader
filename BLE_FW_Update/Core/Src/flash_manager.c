/*
 * flash_manager.c
 *
 *  Created on: 24 apr 2023
 *      Author: Francesco Olivieri
 */
#include <stdio.h>
#include "stm32g0xx_hal.h"
#include "flash_manager.h"

uint32_t StartPageAddress = FLASH_APP_ADDR;
void Write_FW_to_flash(uint64_t *data_to_flash, uint16_t num_bytes){
	HAL_StatusTypeDef ret;

	ret = HAL_FLASH_Unlock();
	if(ret != HAL_OK){
		printf("ERROR Unlock!");
	}
	HAL_FLASH_OB_Unlock();

	uint8_t sofar = 0;
	uint8_t num_dwords = num_bytes;
	while(sofar < num_dwords){
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, StartPageAddress, data_to_flash[sofar]) == HAL_OK){
			StartPageAddress += 8; // doubleword
			sofar++;
		}else{
			printf("EROR!\n\r");
			return;
		}
	}

	HAL_FLASH_OB_Lock();
	HAL_FLASH_Lock();
}

void Erase_Application_Memory(void){
	HAL_StatusTypeDef ret;
	uint32_t PageError;

	//printf("before : %x \n\r", *(volatile uint16_t*)0x08040000);

	ret = HAL_FLASH_Unlock();
	if(ret != HAL_OK){
		printf("ERROR Unlock!");
	}
	HAL_FLASH_OB_Unlock();

	FLASH_EraseInitTypeDef pEraseInit;

	/* NOT WORKING
	pEraseInit.Banks     = FLASH_BANK_1;
	pEraseInit.NbPages   = 0xc800/FLASH_PAGE_SIZE;
	pEraseInit.Page      = 50;
	pEraseInit.TypeErase = FLASH_TYPEERASE_PAGES;

	ret               = HAL_FLASHEx_Erase(&pEraseInit, &PageError);
	*/

	/* Clear BANK_2 */
    pEraseInit.Banks     = FLASH_BANK_2;
    pEraseInit.NbPages   = 128;
    pEraseInit.Page      = 0;
    pEraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
    ret = HAL_FLASHEx_Erase(&pEraseInit, &PageError);

	if(ret != HAL_OK)
		printf("NO\n\r");

	fflush(stdout);

	HAL_FLASH_OB_Lock();
	HAL_FLASH_Lock();


	//printf("after: %x \n\r", *(volatile uint16_t*)0x08040000);

	StartPageAddress = FLASH_APP_ADDR;
}

void go2App(void){
	uint32_t JumpAddress;
	pFunction Jump_TO_Application;

	//if(((*(uint32_t *) FLASH_APP_ADDR) & 0x2FFD8000) == 0x20000000){
		JumpAddress =  *(__IO uint32_t *) (FLASH_APP_ADDR + 4);
		Jump_TO_Application = (pFunction) JumpAddress;


		HAL_RCC_DeInit();
		HAL_DeInit();

		SysTick->CTRL = 0;
		SysTick->LOAD = 0;
		SysTick->VAL  = 0;

		__set_MSP(*(uint32_t *)FLASH_APP_ADDR);
		Jump_TO_Application();

}
