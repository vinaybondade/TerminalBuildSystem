/*
 * main.h
 *
 *  Created on: 15 Jun 2020
 *      Author: amit
 */

#ifndef SRC_MAIN_H_
#define SRC_MAIN_H_

#include "xparameters.h"
#include "fsbl.h"

#define UBOOT_PARTITION_QSPI 0
#define UBOOT_OFFSET_QSPI 0x40000

#define PS_DDR_SW_UPDATE_ADDR	0x10000000
#define SW_UPDATE_NEEDED		0xfeedabba
#if PS_DDR_SW_UPDATE_ADDR < DDR_START_ADDR || PS_DDR_SW_UPDATE_ADDR > DDR_END_ADDR
#error "PS_DDR_SW_UPDATE_ADDR is not in DDR address rage"
#endif

#endif /* SRC_MAIN_H_ */
