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

#define PS_DDR_SW_UPDATE_ADDR			0x10000000   /* cant load fpga.bin || uboot.bin from emmc failed */
#define SW_UPDATE_NEEDED			0xfeedabba
#define SW_UPDATE_NOT_NEEDED			0x00000000

#define PS_DDR_RECOVERY_ADDR			0x10000004 
#define SW_RECOVERY_NEEDED			0xfeedabba
#define SW_RECOVERY_NOT_NEEDED			0x00000000

#define PS_DDR_IN_BOOTLOADER_ADDR		0x10000008
#define PS_DDR_IN_BOOTLOADER_VALUE		0xfeedabba
#define PS_DDR_IN_APP_VALUE			0x00000000

#define PS_DDR_MBBOOT_FILE_ADDR			0x1000000c
#define PS_DDR_MBBOOT_FILE_NOT_EXIST		0xfeedabba
#define PS_DDR_MBBOOT_FILE_EXIST		0x00000000

#define PS_DDR_MBBOOT_LOAD_ADDR			0x10000010
#define PS_DDR_MBBOOT_LOAD_FAILED		0xfeedabba
#define PS_DDR_MBBOOT_LOAD_SUCCESS		0x00000000

#define PS_DDR_BITSTREAM_VALID_ADDR		0x10000014
#define PS_DDR_BITSTREAM_NOT_VALID		0xfeedabba
#define PS_DDR_BITSTREAM_VALID			0x00000000

#define PS_DDR_STM_ERR_ADDR			0x10000018
#define PS_DDR_STM_ERR				0xfeedabba
#define PS_DDR_STM_NO_ERR			0x00000000

#if PS_DDR_SW_UPDATE_ADDR < DDR_START_ADDR || PS_DDR_SW_UPDATE_ADDR > DDR_END_ADDR
#error "PS_DDR_SW_UPDATE_ADDR is not in DDR address rage"
#endif

#if PS_DDR_RECOVERY_ADDR < DDR_START_ADDR || PS_DDR_RECOVERY_ADDR > DDR_END_ADDR
#error "PS_DDR_RECOVERY_ADDR is not in DDR address rage"
#endif

#if PS_DDR_IN_BOOTLOADER_ADDR < DDR_START_ADDR || PS_DDR_IN_BOOTLOADER_ADDR > DDR_END_ADDR
#error "PS_DDR_IN_BOOTLOADER_ADDR is not in DDR address rage"
#endif

#if PS_DDR_MBBOOT_FILE_ADDR < DDR_START_ADDR || PS_DDR_MBBOOT_FILE_ADDR > DDR_END_ADDR
#error "PS_DDR_MBBOOT_FILE_ADDR is not in DDR address rage"
#endif

#if PS_DDR_MBBOOT_LOAD_ADDR < DDR_START_ADDR || PS_DDR_MBBOOT_LOAD_ADDR > DDR_END_ADDR
#error "PS_DDR_MBBOOT_LOAD_ADDR is not in DDR address rage"
#endif

#if PS_DDR_BITSTREAM_VALID_ADDR < DDR_START_ADDR || PS_DDR_BITSTREAM_VALID_ADDR > DDR_END_ADDR
#error "PS_DDR_BITSTREAM_VALID_ADDR is not in DDR address rage"
#endif

#if PS_DDR_STM_ERR_ADDR < DDR_START_ADDR || PS_DDR_STM_ERR_ADDR > DDR_END_ADDR
#error "PS_DDR_STM_ERR_ADDR is not in DDR address rage"
#endif
#endif /* SRC_MAIN_H_ */
