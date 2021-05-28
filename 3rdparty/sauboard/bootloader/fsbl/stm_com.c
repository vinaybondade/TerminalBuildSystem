/*
 * stm_com.c
 *
 *  Created on: 12 Jul 2020
 *      Author: amit
 */

#include "stm_com.h"
#include "stdbool.h"
#include "unistd.h"
#include "xuartlite_l.h"
#include "generalDef.h"
#include "crc.h"

void XUartLite_trySendByte(UINTPTR BaseAddress, u8 Data, uint32_t retry);
u8 XUartLite_tryRecvByte(UINTPTR BaseAddress, uint32_t retry);

static void axiUart_write_msg(uint32_t baseAdder, void *msg, size_t size)
{
	size_t i;
	for (i = 0; i < size; ++i)
	{
		XUartLite_trySendByte(baseAdder, ((uint8_t*)msg)[i], 500);
	}
}

static void stm_send_msg(uint32_t uartBaseAddr, msg_t *msg)
{
	size_t size = sizeof(msg->preamble) + sizeof(msg->opc) +
			sizeof(msg->length) + msg->length +1; // +1 for crc8


	axiUart_write_msg(uartBaseAddr, msg, size);
}

static bool is_msg_valid(struct msg_t *msg)
{
	uint8_t calc_crc = crc8(&msg->opc, sizeof(msg->opc) + sizeof(msg->length) + msg->length);
	if(calc_crc == msg->buf[msg->length]) {
		return true;
	}
	return false;
}

static enum state_t
{
	IDLE,
	RCV_PREAMBLE_B2,
	RCV_PREAMBLE_B3,
	RCV_FULL_PREAMBLE,
	RCV_OPC,
	RCV_LEN,
	RCV_DATA,
	RCV_CS
}state  = IDLE;
static inline void parse_reset_state()
{
	state  = IDLE;
}
static int parse_msg(uint8_t *buf, size_t bufLen, struct msg_t *msg)
{
	static size_t i;
	size_t j;
	uint8_t readData;

	for (j = 0; j < bufLen; ++j)
	{
		readData = buf[j];

		switch(state)
		{
		case IDLE:
			if(readData == LOW_BYTE(STM_PREAMBLE))
			{
				*((uint8_t*)&(msg->preamble) +0) = readData;
				i = 1;
				state = RCV_PREAMBLE_B2;
			}
			break;
		case RCV_PREAMBLE_B2:
			if(readData == HIGH_BYTE(STM_PREAMBLE))
			{
				*((uint8_t*)&(msg->preamble) +i) = readData;
				i++;
				state = RCV_PREAMBLE_B3;
			}
			else if(readData != LOW_BYTE(STM_PREAMBLE))
			{
				state = IDLE;
			}
			break;
		case RCV_PREAMBLE_B3:
			if(readData == LOW_BYTE(HIGH_WORD(STM_PREAMBLE)))
			{
				*((uint8_t*)&(msg->preamble) +i) = readData;
				i++;
				state = RCV_FULL_PREAMBLE;
			}
			else if(readData == LOW_BYTE(STM_PREAMBLE))
			{
				state = RCV_PREAMBLE_B2;
			}
			else
			{
				state = IDLE;
			}
			break;
		case RCV_FULL_PREAMBLE:
			*(((uint8_t*)&(msg->preamble)) +i) = readData;
			if(msg->preamble == STM_PREAMBLE)
			{
				state = RCV_OPC;
			}
			else if(readData == LOW_BYTE(STM_PREAMBLE))
			{
				state = RCV_PREAMBLE_B2;
			}
			else
			{
				state = IDLE;
			}
			break;
		case RCV_OPC:
			msg->opc = readData;
			state = RCV_LEN;
			break;
		case RCV_LEN:
			msg->length = readData;
			i = 0;
			if(msg->length) {
				state = RCV_DATA;
			} else {
				state = RCV_CS;
			}
			break;
		case RCV_DATA:
			msg->buf[i++] = readData;
			if(i == msg->length) {
				state = RCV_CS;
			}
			break;
		case RCV_CS:
			msg->buf[i] = readData;
			state = IDLE;
			if(is_msg_valid(msg)) {
				return 1;
			}
			break;
		}
	}
	return 0;
}

int stm_status_req(uint32_t uartBaseAddr, uint8_t *status0, uint8_t *status1)
{
	int ret = 0;
	size_t offset = 0;
	msg_t rxMsg = {0};
	uint8_t buf[sizeof(rxMsg)] = {0};
	int timeout = 50;

	msg_t txMsg = {
			.preamble = STM_PREAMBLE,
			.opc = STM32_STATUS_REQ,
			.length = 0,
	};

	txMsg.buf[0] = crc8(&txMsg.opc, sizeof(txMsg.opc) +
			sizeof(txMsg.length) + txMsg.length);

	stm_send_msg(uartBaseAddr, &txMsg);

	do
	{
		buf[offset] = XUartLite_tryRecvByte(uartBaseAddr, 500);
		ret = parse_msg(&buf[offset], 1, &rxMsg);
		offset++;
		if(offset == sizeof(buf)) {
			parse_reset_state();
			return -1;
		}
		timeout--;
		usleep(10000);
	} while(timeout && (rxMsg.opc != STM32_STATUS_REP || !ret));

	if(timeout && ret && rxMsg.opc == STM32_STATUS_REP)
	{
		if(status0) {
			*status0 = rxMsg.buf[0];
		}
		if(status1 && rxMsg.length == 2) {
			*status1 = rxMsg.buf[1];
		}
		return 0;
	}

	parse_reset_state();
	return -1;
}

void XUartLite_trySendByte(UINTPTR BaseAddress, u8 Data, uint32_t retry)
{
	while (XUartLite_IsTransmitFull(BaseAddress) && (--retry)) {
		usleep(10);
	}

	if(!XUartLite_IsTransmitFull(BaseAddress)) {
		XUartLite_WriteReg(BaseAddress, XUL_TX_FIFO_OFFSET, Data);
	}
}

u8 XUartLite_tryRecvByte(UINTPTR BaseAddress, uint32_t retry)
{
	u8 ret = 0;
	while (XUartLite_IsReceiveEmpty(BaseAddress) && (--retry)) {
		usleep(10);
	}

	if(!XUartLite_IsReceiveEmpty(BaseAddress)) {
		ret = (u8)XUartLite_ReadReg(BaseAddress, XUL_RX_FIFO_OFFSET);
	}

	return ret;
}
