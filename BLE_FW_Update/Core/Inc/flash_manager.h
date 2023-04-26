/*
 * flash_manager.h
 *
 *  Created on: 24 apr 2023
 *      Author: Francesco Olivieri
 */

#ifndef INC_FLASH_MANAGER_H_
#define INC_FLASH_MANAGER_H_

typedef void (*pFunction)(void);

#define FLASH_APP_ADDR 0x08040000

void Write_FW_to_flash(uint64_t *data_to_flash, uint16_t num_bytes);
void Erase_Application_Memory(void);

void go2App(void);

#endif /* INC_FLASH_MANAGER_H_ */
