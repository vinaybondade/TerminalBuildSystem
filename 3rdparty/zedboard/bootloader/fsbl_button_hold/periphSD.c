/*
 * periphSD.c
 *
 *  Created on: Mar 20, 2018
 *      Author: amit
 */

#include "periphSD.h"
#include "sd.h"
#include "ff.h"
#include "fsbl.h"
#include "xparameters.h"
#include "xstatus.h"
#include "errors.h"
#include "portab.h"

static FIL fil;		/* File object */
static FATFS fatfs;
static char buffer[32];
static char *boot_file = buffer;

static void get_srec_line (uint8_t *buf);

/****************************************************************************
* Initialize file from SD card.
*
* @param	fileName - The name of the file. Must be English letters only! and must have suffix of ".bin".
*
* @return	Return XST_SUCCESS if succeed, XST_FAILURE if failed.
*
* @note		Must be English letters only! and must have suffix of ".bin" !!!!
******************************************************************************/
u32 initSDFile(const char *fileName)
{

	FRESULT rc;
	TCHAR *path = "0:/"; /* Logical drive number is 0 */

	/* Register volume work area, initialize device */
	rc = f_mount(&fatfs, path, 0);
	fsbl_printf(DEBUG_INFO,"SD: rc= %.8x\n\r", rc);

	if (rc != FR_OK) {
		return XST_FAILURE;
	}

	strcpy_rom(buffer, fileName);
	boot_file = (char *)buffer;

	rc = f_open(&fil, boot_file, FA_READ);
	f_lseek(&fil, 0);
	if (rc) {
		fsbl_printf(DEBUG_GENERAL,"SD: Unable to open file %s: %d\n", boot_file, rc);
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/****************************************************************************
* Loading a boot file (the same file from last call of "initSDFile" function).
*
* @param	transferFunc - pointer to transferFunc for transfer the file to the correct memory.
*
* @return	Return 1 for success, otherwise return 0.
*
* @note		none.
******************************************************************************/
int loadBootFile(transferFunc_t transferFunc)
{
	int8_t done = 0;
	static srec_info_t srinfo;
	static uint8_t sr_buf[SREC_MAX_BYTES];
	static uint8_t sr_data_buf[SREC_DATA_MAX_BYTES];
	uint8 rc;

	// Here need to decode the elf file and set him at the DDR correctly - currently Ill set it in the same DDr as ARM
	srinfo.sr_data = sr_data_buf;
	while (!done)
	{
		get_srec_line (sr_buf);
		rc = decode_srec_line (sr_buf, &srinfo);
		if(rc) {
			return 0;
		}

		switch (srinfo.type)
		{
		case SREC_TYPE_0:
			break;
		case SREC_TYPE_1:
		case SREC_TYPE_2:
		case SREC_TYPE_3:
			if(transferFunc == NULL)
			{
				memcpy ((void*)srinfo.addr, (void*)srinfo.sr_data, srinfo.dlen);
			}
			else
			{
				if((u32)srinfo.addr >= (u32)MB_MIG_BASEADDR)
				{
					if(transferFunc(&srinfo)) {
						return 0;
					}
				}
				else
				{
					xil_printf("Error! Address is out of MB_MIG's rage: 0x%x  \r\n", (u32)srinfo.addr);
				}
			}
			break;
		case SREC_TYPE_5:
			break;
		case SREC_TYPE_7:
		case SREC_TYPE_8:
		case SREC_TYPE_9:
			//laddr = (void (*)())srinfo.addr; // for testing the function only if it run on the same CPU !!!!!
			done = 1;
			xil_printf("Done loading the MB boot file\r\nMicroBlaze will start at address 0x%x\r\n",(u32)srinfo.addr);
			if((u32)srinfo.addr != (u32)MB_MIG_BASEADDR){
				xil_printf("Error! MicroBlaze must start at MB_MIG_BASEADDR: 0x%x\r\n", (u32)MB_MIG_BASEADDR);
				return 0;
			}
			break;
		}
	}
	f_close(&fil);

	return 1;
}

/****************************************************************************
 * Get one line of the .SREC file.
 ******************************************************************************/
static void get_srec_line (uint8_t *buf)
{
	int count = 0;
	UINT br;
	u8 tmpByte;
	FRESULT rc;

	while(count < SREC_MAX_BYTES)
	{
		rc = f_read(&fil, &tmpByte, 1, &br);
		if(rc)
			xil_printf("error: cant read the file\r\n");
		if(tmpByte == '\r')
		{
			f_read(&fil, &tmpByte, 1, &br); // skip '\n'
			//xil_printf("finish seq\r\n");
			return;
		}
		*buf++ = tmpByte;
		count++;
	}
}
