/*
 * periphCdma..c
 *
 *  Created on: Mar 25, 2018
 *      Author: amit
 */
#include "periphCdma.h"
#include "xparameters.h"
#include "xaxicdma.h"
#include "xil_cache.h"
#include "xil_io.h"
#include "srec.h"

/* Global for this file only */
XAxiCdma axiCdmaInstance;	/* Instance of the XAxiCdma */
XAxiCdma_Config *cdmaCfgPtr;
/* End Global for this file only */

/****************************************************************************
* Initialize CDMA peripheral.
*
* @param	deviceId - the device id base on "xparameters.h"
*
* @return	Return XST_SUCCESS if succeed, XST_FAILURE if failed.
*
* @note		none.
******************************************************************************/
int initCDMA(u32 deviceId)
{
	int Status;

	cdmaCfgPtr = XAxiCdma_LookupConfig(deviceId);
	if (!cdmaCfgPtr) {
		return XST_FAILURE;
	}

	Status = XAxiCdma_CfgInitialize(&axiCdmaInstance, cdmaCfgPtr, cdmaCfgPtr->BaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/****************************************************************************
* Transfer one sequence with the CDMA device.
*
* @param	srcPtr  - pointer to the source address.
*			destPtr - pointer to the destination address.
*			length  - length in bytes!
*			retries - how many times we want to try the transference if the CDMA failed to transfer 
*
* @return	Return XST_SUCCESS if succeed, XST_FAILURE if failed.
*
* @note		none.
******************************************************************************/
int cdmaSimplePollTransfer(u8* srcPtr, u8* destPtr, int length, int retries)
{
	const int CDMA_BUSY_TIMEOUT = 100;
	int cdmaBusyTimeOut = CDMA_BUSY_TIMEOUT;	
	int status = XST_SUCCESS;
	int error = 0;	/* Dma Error occurs */

	/* Flush the SrcBuffer before the DMA transfer, in case the Data Cache
	 * is enabled
	 */
	Xil_DCacheFlushRange((UINTPTR)&srcPtr, length);

	/* Try to start the DMA transfer
	 */
	while (retries)
	{
		retries -= 1;

		status = XAxiCdma_SimpleTransfer(&axiCdmaInstance, (UINTPTR)srcPtr,	(UINTPTR)destPtr, length, NULL, NULL);

		if (status == XST_SUCCESS) {
			break;
		}
	}

	/* Return failure if failed to submit the transfer
	 */
	if (!retries) {
		return XST_FAILURE;
	}

	/* Wait until the DMA transfer is done
	 */
	while (XAxiCdma_IsBusy(&axiCdmaInstance) && cdmaBusyTimeOut > 0)
	{
		cdmaBusyTimeOut--;
		if(cdmaBusyTimeOut < CDMA_BUSY_TIMEOUT/2) {
			xil_printf("%d: CDMA still busy!\r\n", cdmaBusyTimeOut);
		}
		/* Wait */
	}

	if(cdmaBusyTimeOut == 0) {
		return XST_FAILURE;
	}

	/* If the hardware has errors, this example fails
	 * This is a poll example, no interrupt handler is involved.
	 * Therefore, error conditions are not cleared by the driver.
	 */
	error = XAxiCdma_GetError(&axiCdmaInstance);
	if (error != 0x0) {
		int timeOut = 10;

		/* Need to reset the hardware to restore to the correct state
		 */
		XAxiCdma_Reset(&axiCdmaInstance);

		while (timeOut) {
			if (XAxiCdma_ResetIsDone(&axiCdmaInstance)) {
				break;
			}
			timeOut -= 1;
		}

		/* Reset has failed, print a message to notify the user
		 */
		return XST_FAILURE;
	}
	/* Transfer completes successfully, check data
	 */

	#ifndef __aarch64__
		Xil_DCacheInvalidateRange((UINTPTR)destPtr, length);
	#endif

	return XST_SUCCESS;
}

/****************************************************************************
* This function made for transfer data from the ZYNQ DDR to the Microblaze DDR.
*
* @param	srec_info_t - pointer to srec_info_t
*
* @return	none.
*
* @note		none.
******************************************************************************/
int cdmaZyncToMicroBlaze(srec_info_t* srinfo)
{
	if(cdmaSimplePollTransfer(srinfo->sr_data, srinfo->addr, srinfo->dlen , 10) != XST_SUCCESS)
	{
		xil_printf("Error with cdmaZyncToMicroBlaze function\r\n");
		return XST_FAILURE;
	}
	
	return XST_SUCCESS;
}
