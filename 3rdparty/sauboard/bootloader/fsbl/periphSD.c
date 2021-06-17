/*
 * periphSD.c
 *
 *  Created on: Mar 20, 2018
 *      Author: amit
 */

#include "periphSD.h"
#include "ff.h"
#include "fsbl.h"
#include "xparameters.h"
#include "xstatus.h"
#include "errors.h"
#include "portab.h"
#include "ext4.h"

static ext4_file file;
static struct ext4_blockdev *bd;
static char buffer[32];
static char *boot_file = buffer;
static uint8_t *loadaddr = DDR_TEMP_START_ADDR;

static void get_srec_line (uint8_t *buf);
static FRESULT load_srecfile(void);

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

	/* Register volume work area, initialize device */
	fsbl_printf(DEBUG_INFO,"SD: rc= %.8x\n\r", rc);

	if (rc != FR_OK) {
		return XST_FAILURE;
	}

	strcpy_rom(buffer, fileName);
	boot_file = (char *)buffer;

	rc = ext4_fopen(&file, fileName, "rb");
	if (rc) {
		fsbl_printf(DEBUG_GENERAL,"SD: Unable to open file %s: %d\n", boot_file, rc);
		return XST_FAILURE;
	}

	// Load the file to DDR RAM to read SREC data line by line.
	load_srecfile();

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
			xil_printf("Failed to decode srec.\r\n");
			ReleaseSD();
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
						xil_printf("Failed to transfer srec.\r\n");
						ReleaseSD();
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
				ReleaseSD();
				return 0;
			}
			break;
		}
	}
	ReleaseSD();

	return 1;
}

/****************************************************************************
 * Get one line of the .SREC file.
 ******************************************************************************/
static void get_srec_line (uint8_t *buf)
{
	int count = 0;
	FRESULT rc;

	while(count < SREC_MAX_BYTES)
	{
		if(*loadaddr == '\r')
		{
			loadaddr += 2; // skip '\r\n'
			return;
		}
		*buf++ = *loadaddr++;
		count++;
	}
}

static FRESULT load_srecfile(void)
{	
	FRESULT rc;
	UINT br;
	size_t fileSize = 0;
	rc = ext4_fseek(&file, 0L, SEEK_SET);
	if(rc){
		xil_printf("Failed to set seek pointer to start.\r\n");
		return rc;
	}

	rc = ext4_fseek(&file, 0L, SEEK_END);
	if(rc){
		xil_printf("Failed to set seek pointer to end.\r\n");
		return rc;
	}

	fileSize = ext4_ftell(&file);
	if(fileSize < 0){
		xil_printf("Failed to get file size.\r\n");
		return fileSize;
	}
	xil_printf("filesize - %d\r\n", fileSize);

	// Reset the read pointer.
	rc = ext4_fseek(&file, 0L, SEEK_SET);

	rc = ext4_fread(&file, loadaddr, fileSize, &br);
	if(rc){
		xil_printf("error: cant read the file\r\n");
		return rc;
	}

	return FR_OK;
}
