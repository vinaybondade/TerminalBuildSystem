/******************************************************************************
*
* Copyright (C) 2012 - 2014 Xilinx, Inc.  All rights reserved.
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal 
* in the Software without restriction, including without limitation the rights 
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell  
* copies of the Software, and to permit persons to whom the Software is 
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in 
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications: 
* (a) running on a Xilinx device, or 
* (b) that interact with a Xilinx device through a bus or interconnect.  
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in 
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*****************************************************************************
*
* @file fsbl_hooks.c
*
* This file provides functions that serve as user hooks.  The user can add the
* additional functionality required into these routines.  This would help retain
* the normal FSBL flow unchanged.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who  Date        Changes
* ----- ---- -------- -------------------------------------------------------
* 3.00a np   08/03/12 Initial release
* </pre>
*
* @note
*
******************************************************************************/


#include "fsbl.h"
#include "xstatus.h"
#include "fsbl_hooks.h"


#include "xparameters.h"
#include "xgpio.h"
#include "xgpiops.h"
#include "periphSD.h"
#include "periphCdma.h"
#include "main.h"
#include "stm_com.h"
/************************** Variable Definitions *****************************/
#define PS_EMIO_PIN_OFFSET			54
#define LEDEN1_PS_MIO_PIN			10
#define BITSTREAM_RUN_PS_EMIO_PIN 	(58 + PS_EMIO_PIN_OFFSET)

/************************** Function Prototypes ******************************/
int clear_ps_led(uint32_t pinNum);
void FsblHandoff_Bitstream();
u32 Validate_Bitstream_Run();
/******************************************************************************
* This function is the hook which will be called  before the bitstream download.
* The user can add all the customized code required to be executed before the
* bitstream download to this routine.
*
* @param None
*
* @return
*		- XST_SUCCESS to indicate success
*		- XST_FAILURE.to indicate failure
*
****************************************************************************/
u32 FsblHookBeforeBitstreamDload(void)
{
	u32 Status;

	Status = XST_SUCCESS;
	clear_ps_led(LEDEN1_PS_MIO_PIN);
	/*
	 * User logic to be added here. Errors to be stored in the status variable
	 * and returned
	 */
	fsbl_printf(DEBUG_INFO,"In FsblHookBeforeBitstreamDload function \r\n");

	return (Status);
}

/******************************************************************************
* This function is the hook which will be called  after the bitstream download.
* The user can add all the customized code required to be executed after the
* bitstream download to this routine.
*
* @param None
*
* @return
*		- XST_SUCCESS to indicate success
*		- XST_FAILURE.to indicate failure
*
****************************************************************************/
u32 FsblHookAfterBitstreamDload(void)
{
	u32 Status;
	uint8_t stm_status0 = 0;
	uint8_t stm_status1 = 0;

	Status = XST_SUCCESS;

	/*
	 * User logic to be added here.
	 * Errors to be stored in the status variable and returned
	 */
	fsbl_printf(DEBUG_INFO, "In FsblHookAfterBitstreamDload function \r\n");

	FsblHandoff_Bitstream();

	/* Validate Bitstream run */
	if(Validate_Bitstream_Run() == XST_SUCCESS)
	{
		/* Check for recovery Request by STM32  via AXI UART */
		if(!stm_status_req(XPAR_AXI_UART_STM32_BASEADDR, &stm_status0, &stm_status1))
		{
			if(stm_status1 == STM_RECOVERY_REQ) {
				Xil_Out32(PS_DDR_RECOVERY_ADDR, SW_RECOVERY_NEEDED);
			}
			
			if(stm_status0 != 1/*MAIN APP running*/) {
				Xil_Out32(PS_DDR_IN_BOOTLOADER_ADDR, PS_DDR_IN_BOOTLOADER_VALUE);
				fsbl_printf(DEBUG_INFO, "write 0xfeedabba - reason st not in main\r\n");
			}

			fsbl_printf(DEBUG_INFO, "stm_status0:%x \r\n", stm_status0);
			fsbl_printf(DEBUG_INFO, "stm_status1:%x \r\n", stm_status1);
		}
	}
	else {
		Xil_Out32(PS_DDR_STM_ERR_ADDR, PS_DDR_STM_ERR);
	}

	return (Status);
}

/******************************************************************************
* This function is the hook which will be called  before the FSBL does a handoff
* to the application. The user can add all the customized code required to be
* executed before the handoff to this routine.
*
* @param None
*
* @return
*		- XST_SUCCESS to indicate success
*		- XST_FAILURE.to indicate failure
*
****************************************************************************/
u32 FsblHookBeforeHandoff(void)
{
	u32 Status;
	XGpio mbResetGpio;

	Status = XST_SUCCESS;

	fsbl_printf(DEBUG_INFO,"In FsblHookBeforeHandoff function \r\n");
	/*
	 * User logic to be added here.
	 * Errors to be stored in the status variable and returned
	 */

	/* Validate Bitstream run */
	if(Validate_Bitstream_Run() != XST_SUCCESS) {
		return XST_FAILURE;
	}

	Status = initCDMA(XPAR_AXI_CDMA_0_DEVICE_ID);
	if(Status != XST_SUCCESS) {
		xil_printf("Could not init cDMA\r\n");
		return (Status);
	}

	if(initSDFile("mb.bin") != XST_SUCCESS) {
		xil_printf("\rSD/MMC INIT_FAIL - mb.bin\r\n");
		Xil_Out32(PS_DDR_MBBOOT_FILE_ADDR, PS_DDR_MBBOOT_FILE_NOT_EXIST);
	}
	else
	{
		xil_printf("\rSD/MMC INIT_Done (Loading) - mb.bin\r\n");
		if(loadBootFile(cdmaZyncToMicroBlaze))
		{
			/*
			 * Stop Reset MicroBlaze CPU
			 */
			if(XGpio_Initialize(&mbResetGpio, XPAR_AXI_GPIO_0_DEVICE_ID) == XST_SUCCESS) {
				XGpio_DiscreteWrite(&mbResetGpio, 1, 0);
				xil_printf("stop MB reset\r\n");
			}
			else {
				xil_printf("could not stop MB reset\r\n");
			}
		}
		else
		{
			xil_printf("Load MB failed\r\n");
			Xil_Out32(PS_DDR_MBBOOT_LOAD_ADDR, PS_DDR_MBBOOT_LOAD_FAILED);
		}
	}

	return (Status);
}


/******************************************************************************
* This function is the hook which will be called in case FSBL fall back
*
* @param None
*
* @return None
*
****************************************************************************/
void FsblHookFallback(void)
{
	/*
	 * User logic to be added here.
	 * Errors to be stored in the status variable and returned
	 */
	fsbl_printf(DEBUG_INFO,"In FsblHookFallback function \r\n");
	while(1);
}

int clear_ps_led(uint32_t pinNum)
{
	XGpioPs_Config *configPtr;
	XGpioPs gpioPs;

	configPtr = XGpioPs_LookupConfig(XPAR_XGPIOPS_0_DEVICE_ID);
	if (configPtr == NULL) {
		return XST_FAILURE;
	}
	XGpioPs_CfgInitialize(&gpioPs, configPtr, configPtr->BaseAddr);

	XGpioPs_SetDirectionPin(&gpioPs, pinNum, 1);
	XGpioPs_SetOutputEnablePin(&gpioPs, pinNum, 1);
	XGpioPs_WritePin(&gpioPs, pinNum, 0x0);

	return XST_SUCCESS;
}



void FsblHandoff_Bitstream()
{
	/*
	 * Enable level shifter
	 */
	/*
	 * FSBL will not enable the level shifters for a NON PS instantiated
	 * Bitstream
	 * CR# 671028
	 * This flag can be set during compilation for a NON PS instantiated
	 * bitstream
	 */
#ifndef NON_PS_INSTANTIATED_BITSTREAM
#ifdef PS7_POST_CONFIG
	ps7_post_config();
	/*
	 * Unlock SLCR for SLCR register write
	 */
	SlcrUnlock();
#else
	/*
	 * Set Level Shifters DT618760
	 */
	Xil_Out32(PS_LVL_SHFTR_EN, LVL_PL_PS);
	fsbl_printf(DEBUG_INFO,"Enabling Level Shifters PL to PS "
			"Address = 0x%x Value = 0x%x \n\r",
			PS_LVL_SHFTR_EN, Xil_In32(PS_LVL_SHFTR_EN));

	/*
	 * Enable AXI interface
	 */
	Xil_Out32(FPGA_RESET_REG, 0);
	fsbl_printf(DEBUG_INFO,"AXI Interface enabled \n\r");
	fsbl_printf(DEBUG_INFO, "FPGA Reset Register "
			"Address = 0x%x , Value = 0x%x \r\n",
			FPGA_RESET_REG ,Xil_In32(FPGA_RESET_REG));
#endif
#endif
}

u32 Validate_Bitstream_Run()
{
	XGpioPs_Config *configPtr;
	XGpioPs gpioPs;
	configPtr = XGpioPs_LookupConfig(XPAR_XGPIOPS_0_DEVICE_ID);
	if (configPtr == NULL) {
		return XST_FAILURE;
	}
	XGpioPs_CfgInitialize(&gpioPs, configPtr, configPtr->BaseAddr);
	XGpioPs_SetOutputEnablePin(&gpioPs, BITSTREAM_RUN_PS_EMIO_PIN, 0);
	XGpioPs_SetDirectionPin(&gpioPs, BITSTREAM_RUN_PS_EMIO_PIN, 0);

	if(XGpioPs_ReadPin(&gpioPs, BITSTREAM_RUN_PS_EMIO_PIN) == 0) {
		/* No valid bitstream */
		Xil_Out32(PS_DDR_BITSTREAM_VALID_ADDR, PS_DDR_BITSTREAM_NOT_VALID);
		return XST_FAILURE;
	}
	
	return XST_SUCCESS;
}
