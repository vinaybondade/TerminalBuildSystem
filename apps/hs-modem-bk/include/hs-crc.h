/*
 * crc.h
 *
 *  Created on: Feb 1, 2017
 *      Author: yevgenyk
 */
/*
 * crc.h
 *
 *  Created on: Jan 29, 2014
 *      Author: yevgenyk

 * Copyright (c) 2000 by Michael Barr.  This software is placed into
 * the public domain and may be used for any purpose.  However, this
 * notice must not be changed or removed and no warranty is either
 * expressed or implied by its publication or distribution.
 */

#ifndef CRC_H_
#define CRC_H_
#define FALSE	0
#define TRUE	!FALSE

/*
 * Select the CRC standard from the list that follows.
 */
#define CRC16

#define MAXIMUM_MESSAGE_SIZE 166

typedef unsigned short  crc;

#define CRC_NAME			"CRC-16"
#define POLYNOMIAL			0x8005
#define INITIAL_REMAINDER		0xFFFF
#define FINAL_XOR_VALUE			0x0000
#define CHECK_VALUE			0xBB3D



void  crcInit(void);
//crc   crcSlow(unsigned char const message[], int nBytes);
unsigned short   crcSlow( unsigned char* , int nBytes);
crc   crcFast(unsigned char const message[], int nBytes);

#endif /* CRC_H_ */

