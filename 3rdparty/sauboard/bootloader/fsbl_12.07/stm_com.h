/*
 * stm_com.h
 *
 *  Created on: 12 Jul 2020
 *      Author: amit
 */

#ifndef SRC_STM_COM_H_
#define SRC_STM_COM_H_

#include "stdint.h"
#include "stddef.h"

#define STM_PREAMBLE			0xDEADBEEF
#define STM_RECOVERY_REQ		0xAA
#define STM_RECOVERY_NOT_REQ	0x55

#define STM32_STATUS_REQ		0x0F
#define STM32_STATUS_REP		0x0F

typedef struct __attribute__ ((__packed__)) msg_t
{
	uint32_t preamble;
	uint8_t opc;
	uint8_t length;
	uint8_t buf[256];// max len of 255 + 1 byte for crc8
} msg_t;

int stm_status_req(uint32_t uartBaseAddr, uint8_t *status0, uint8_t *status1);

#endif /* SRC_STM_COM_H_ */
