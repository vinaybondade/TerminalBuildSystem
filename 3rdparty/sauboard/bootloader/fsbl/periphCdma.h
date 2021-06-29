/*
 * periphCdma.h
 *
 *  Created on: Mar 25, 2018
 *      Author: amit
 */

#ifndef PERIPHCDMA_H_
#define PERIPHCDMA_H_

#include "xil_types.h"
#include "xaxicdma.h"
#include "periphSD.h"

#define MB_MIG_BASEADDR 0x80000000


int initCDMA(u32 deviceId);

int cdmaSimplePollTransfer(u8* srcPtr, u8* destPtr, int length, int retries);

int cdmaZyncToMicroBlaze(srec_info_t* srinfo);

#endif /* PERIPHCDMA_H_ */
