/*
 * periphSD.h
 *
 *  Created on: Mar 20, 2018
 *      Author: amit
 */

#ifndef PERIPHSD_H_
#define PERIPHSD_H_

#include "xil_types.h"
#include "srec.h"
#include "periphCdma.h"


typedef int (*transferFunc_t)(srec_info_t* srinfo);

u32 initSDFile(const char *fileName);

int loadBootFile(transferFunc_t deliverFunc);

#endif /* PERIPHSD_H_ */
