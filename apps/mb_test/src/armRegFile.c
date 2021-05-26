/*
 * armRegFile.c
 *
 *  Created on: Jun 5, 2019
 *      Author: amit
 */

#include "../include/armRegFile.h"
#include "../include/generalDef.h"
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <string.h> // for using 'memcpy'
#include <pthread.h>

static volatile void* armRegFilemMap = NULL;
static pthread_mutex_t armMsgMutex;
//static pthread_mutexattr_t armMsgMutexAttr;

inline void armRegFileWriteReg(uint32_t offset, uint32_t data)
{
	if(armRegFilemMap) {
		writeRegister(armRegFilemMap, offset << 2, data);
	}
}

inline uint32_t armRegFileReadReg(uint32_t offset)
{
	if(armRegFilemMap) {
		return readRegister(armRegFilemMap, offset << 2);
	}
	return 0;
}

int armRegFileInit(off_t baseAddr, size_t len)
{
	off_t pa_offset;
	int memfd = open("/dev/mem", O_RDWR | O_SYNC);
	if (memfd == -1) {
		printf("Can't open /dev/mem.\n");
		return 0;
	}
	pa_offset = baseAddr & ~(sysconf(_SC_PAGE_SIZE) - 1);

	armRegFilemMap = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, pa_offset);
	close(memfd);

	if (armRegFilemMap == MAP_FAILED) {
		return 0;
	}

	//	pthread_mutexattr_init(&armMsgMutexAttr);
	//	pthread_mutexattr_settype(&armMsgMutexAttr, PTHREAD_MUTEX_RECURSIVE);
	//	pthread_mutex_init(&armMsgMutex, &armMsgMutexAttr);
	pthread_mutex_init(&armMsgMutex, NULL);


	return 1;
}

int armRegFileCleanup (size_t len)
{
	if (armRegFilemMap != NULL)
		munmap ((void *)armRegFilemMap, len);

	return 0;
}

//************************************************
//*********	Read message from MB	**************
//************************************************
int readMsgFromMB(void* data, size_t dataSize, uint8_t* opcode)
{
	int i;
	const uint8_t preambleVal[4] = {
			LOW_BYTE(LOW_WORD(ARM_MB_PREAMBLE)),
			HIGH_BYTE(LOW_WORD(ARM_MB_PREAMBLE)),
			LOW_BYTE(HIGH_WORD(ARM_MB_PREAMBLE)),
			HIGH_BYTE(HIGH_WORD(ARM_MB_PREAMBLE))
	};

	const int US_TO_WAIT = 300;
	int timeOut = 5;

	sMbMsg mbMsg = {};
	uint8_t readVal;

	cs8_t recvCs;
	cs8_t calcCs = 0;

	int pckDataLen = 0;
	int preambleCheckOffset = 0;

	int retVal = ARM_MB_PREABMLE_NOT_FOUND;

	if((dataSize > 0 && !data) || !opcode) {
		return ARM_MB_INVALID_ARG;
	}

	*opcode = 0; // empty opcode

	if(getMbMsgOccupancy() < sizeof(mbMsg.header)) {
		return 0;
	}
	pthread_mutex_lock(&armMsgMutex);
	i = 0;
	while((getMbMsgOccupancy() >= (MIN_ARM_MB_PCK - preambleCheckOffset))
			&& (retVal == ARM_MB_PREABMLE_NOT_FOUND))
	{
		/* Search for PREAMBLE */
		for (i = preambleCheckOffset; i < sizeof(mbMsg.header.preamble); ++i)
		{
			readVal = getByteFromMb();
			if(readVal != preambleVal[i]) {
				break;
			}
			*(((uint8_t*)&mbMsg) +i) = preambleVal[i];
			calcCs += preambleVal[i];
		}
		if(mbMsg.header.preamble != ARM_MB_PREAMBLE) {
			if(readVal == preambleVal[0]) {
				*((uint8_t*)&mbMsg) = preambleVal[0];
				calcCs = preambleVal[0];
				preambleCheckOffset = 1;
				continue;
			}
			mbMsg.header.preamble = 0;
			preambleCheckOffset = 0;
			calcCs = 0;
			continue;
		}
		/* PREAMBLE passed */
		/* Receive dataLen*/
		readVal =  getByteFromMb();
		calcCs += readVal;
		mbMsg.header.pckLen = readVal;

		readVal =  getByteFromMb();
		calcCs += readVal;
		mbMsg.header.pckLen += (readVal << 8);

		/* Wait for packet */
		if(mbMsg.header.pckLen)
		{
			while((getMbMsgOccupancy() < mbMsg.header.pckLen) && timeOut) {
				usleep(US_TO_WAIT);
				timeOut--;
			}
			if(!timeOut) {
				retVal = ARM_MB_READ_TIMEOUT;
				break;
			}

			/* There is no timeout so read the packet */
			mbMsg.header.opcode = getByteFromMb();
			calcCs += mbMsg.header.opcode;

			pckDataLen = mbMsg.header.pckLen - sizeof(mbMsg.header.opcode) - sizeof(cs8_t);
			if(mbMsg.header.pckLen < MIN_ARM_MB_PCKLEN || pckDataLen > MAX_ARM_MB_DATA) {
				printf("Critical error\n");
			}
			/* Receive data or empty data if there is an error */
			if(dataSize < pckDataLen) {
				retVal = ARM_MB_MSG_TOO_LONG;
				for (i = 0; i < pckDataLen; ++i) {
					getByteFromMb();
				}
				break;
			} else {
				for (i = 0; i < pckDataLen; ++i) {
					*(mbMsg.data +i) = getByteFromMb();
					calcCs += (*(mbMsg.data +i));
				}
			}
		}
		/* Receive and check CS */
		recvCs = getByteFromMb();
		if(calcCs == recvCs) {
			*opcode = mbMsg.header.opcode;
			memcpy(data, mbMsg.data, pckDataLen);
			/*****************************/
			/* Return dataLen as success */
			/*****************************/
			retVal = pckDataLen;
		} else {
			retVal = ARM_MB_CS_ERROR;
		}
	}

	pthread_mutex_unlock(&armMsgMutex);

	return retVal;
}
//**************************************************
//*******	Write message to MB		****************
//**************************************************
int writeMsg2MB(const void* msg, size_t msgSize, uint8_t opcode)
{
	int timeOut = 5;
	const int US_TO_WAIT = 300;

	int i;
	cs8_t cs = 0;
	const int size = sizeof(mbMsgHeader_t) + msgSize + sizeof(cs8_t);

	mbMsgHeader_t header;
	header.preamble = ARM_MB_PREAMBLE;
	header.pckLen = size - sizeof(header.preamble) - sizeof(header.pckLen);
	header.opcode = opcode;

	pthread_mutex_lock(&armMsgMutex);

	/* Wait for enough free space or timeout */
	while((size > getArmMsgVacancy()) && timeOut) {
		usleep(US_TO_WAIT);
		timeOut--;
	}
	if(!timeOut) {
		pthread_mutex_unlock(&armMsgMutex);
		return ARM_MB_WRITE_TIMEOUT;
	}

	/* Write the message Header */
	for (i = 0; i < sizeof(header); ++i) {
		cs += *(((uint8_t*)&header) +i);
		writeByteToMb(*(((uint8_t*)&header) +i));
	}
	/* Write the message Data */
	for (i = 0; i < msgSize; ++i) {
		cs += *(((uint8_t*)msg) +i);
		writeByteToMb(*(((uint8_t*)msg) +i));
	}

	/* Write the CS */
	writeByteToMb(cs);

	pthread_mutex_unlock(&armMsgMutex);
	/* Return size as success */
	return size;
}

void writeReqToMb(uint8_t opcode)
{
	int retVal =  writeMsg2MB(NULL, 0, opcode);
	if(retVal < 0) {
		printf("writeMsg2MB return %d\n", retVal);
	}
}

//**********************************************************************
//**************	Write RF COMMAND message to MB		****************
//**********************************************************************
#define IOT_TX_PA_CMD 118
void writeRfCmndToMb(uint8_t opcode, uint8_t* data, uint16_t length)
{
#ifndef BaseStation
#if MT_MODE == MT_DYNAMIC
	if(opcode == IOT_TX_PA_CMD) {
		return;
	}
#endif
#endif

	int retVal = writeMsg2MB(data, length, opcode);
	if(retVal) {
		printf("writeMsg2MB return %d\n", retVal);
	}
}

int write_modem_power(uint32_t power)
{
	mbRegFileRwReq_t mbRegFileRwReq;

	mbRegFileRwReq.reqOpc = 0; // write
	mbRegFileRwReq.address = 0x71;
	mbRegFileRwReq.data = power;

	return writeMsg2MB(&mbRegFileRwReq, sizeof(mbRegFileRwReq), 159);
}

uint32_t read_modem_power()
{
	int timeout = 5;
	mbRegFileRwReq_t mbRegFileRwReq;
	struct mbReadData_t
	{
		uint8_t opcode;
		uint8_t data[MAX_ARM_MB_DATA];
	}mbReadData;
	int mbMsgSize;

	mbRegFileRwReq.reqOpc = 1; // read
	mbRegFileRwReq.address = 0x71;
	mbRegFileRwReq.data = 0;
	writeMsg2MB(&mbRegFileRwReq, sizeof(mbRegFileRwReq), 159);

	while(timeout--)
	{
		mbMsgSize = readMsgFromMB(mbReadData.data, sizeof(mbReadData.data), &mbReadData.opcode);
		if(mbMsgSize > 0 || mbReadData.opcode)
		{
			if(mbReadData.opcode == 159)
			{
				if(((mbRegFileRwReq_t *)mbReadData.data)->address == 0x71)
				{
					return ((mbRegFileRwReq_t *)mbReadData.data)->data;
				}
			}
		}
		usleep(10000);
	}
	return ((uint32_t)-1);
}
