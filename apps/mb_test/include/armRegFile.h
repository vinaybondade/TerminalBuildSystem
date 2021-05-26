/*
 * armRegFile.h
 *
 *  Created on: Jun 5, 2019
 *      Author: amit
 */

#ifndef ARMREGFILE_H_
#define ARMREGFILE_H_

#include "armRegFile_hw.h"
#include <stdint.h>
#include <stdio.h>

#define ARM_MB_FIFO_DEPTH 4090 //6 Bytes reduce for safety reasons

#define ARM_MB_PREAMBLE ((uint32_t)0xDEADBEEF)
/***********************
 * Protocol pattern is:
 * *********************
 * 4-Bytes @preamble | 1-Byte @pckLen | 1-Byte @opcode | N-Bytes @data | 1-Byte @cs8
 *
 * @preamble - ARM_MB_PREAMBLE(0xDEADBEEF).
 * @pckLen - contain: @opcode + @data + @cs8
 * @opcode - message opcode
 * @data - message data
 * @cs8 - 8bit of checksum. cs8 contain: @preamble + @pckLen + @opcode + @data
 */
typedef uint8_t cs8_t;

typedef struct __attribute__ ((__packed__)) mbMsgHeader_t
{
	uint32_t	preamble;
	uint16_t	pckLen;
	uint8_t		opcode;
} mbMsgHeader_t;
typedef struct __attribute__ ((__packed__)) mbMsg_t
{
	mbMsgHeader_t	header;
	uint8_t*		data;
} mbMsg_t;

typedef struct __attribute__ ((__packed__)) mbRegFileRwReq_t
{
	uint8_t reqOpc;
	uint32_t address;
	uint32_t data;
} mbRegFileRwReq_t;

#define MAX_ARM_MB_DATA (ARM_MB_FIFO_DEPTH - sizeof(mbMsgHeader_t) - sizeof(cs8_t))
#define MIN_ARM_MB_PCK (sizeof(mbMsgHeader_t) + sizeof(cs8_t))
#define MIN_ARM_MB_PCKLEN (sizeof(uint8_t/*opcode type*/) + sizeof(cs8_t))

typedef struct __attribute__ ((__packed__)) sMbMsg
{
	mbMsgHeader_t	header;
	uint8_t	data[MAX_ARM_MB_DATA];
}sMbMsg,*pMbMsg;

#define ARM_MB_MSG_TOO_LONG			(-1)
#define ARM_MB_CS_ERROR 			(-2)
#define ARM_MB_READ_TIMEOUT			(-3)
#define ARM_MB_PREABMLE_NOT_FOUND	(-4)
#define ARM_MB_INVALID_ARG			(-5)

#define ARM_MB_WRITE_TIMEOUT		(-1)

int armRegFileInit(off_t baseAddr, size_t len);


void armRegFileWriteReg(uint32_t offset, uint32_t data);
uint32_t armRegFileReadReg(uint32_t offset);

int readMsgFromMB(void* data, size_t dataSize, uint8_t* opcode);
int writeMsg2MB(const void* msg, size_t msgSize, uint8_t opcode);
void writeReqToMb(uint8_t opcode);
void writeRfCmndToMb(uint8_t opcode, uint8_t* data, uint16_t length);

int write_modem_power(uint32_t power);
uint32_t read_modem_power();

#define getByteFromMb() armRegFileReadReg(MB2ARM_DATA)
#define getMbMsgOccupancy() armRegFileReadReg(MB2ARM_DATA_CNT)

#define writeByteToMb(byte) armRegFileWriteReg(ARM2MB_DATA, (byte))
#define getArmMsgOccupancy() armRegFileReadReg(ARM2MB_DATA_CNT)
#define getArmMsgVacancy() (ARM_MB_FIFO_DEPTH - getArmMsgOccupancy())

#define setLedColor(color) armRegFileWriteReg(MT_PANEL_LED, color)

#endif /* ARMREGFILE_H_ */
