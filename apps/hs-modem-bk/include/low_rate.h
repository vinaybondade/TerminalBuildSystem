#ifndef __LINUX_LOW_RATE_H
#define __LINUX_LOW_RATE_H

/* ----------------------------
 *           includes
 * ----------------------------
 */
#include <linux/types.h>
#include "air_mode.h"
#include "air_interface.h"
/* ----------------------------
 *            types
 * ----------------------------
 */

typedef struct __attribute__ ((__packed__)) lrHeader_t
{
	unsigned typeHeader	: 3;
	unsigned mtID		: 12;
	unsigned seqNum		: 8;
	unsigned ebn0		: 6;
	unsigned numOfSeg	: 3;
}lrHeader_t;					
						
typedef struct __attribute__ ((__packed__)) lrSegHeader_t
{
	union
	{
		struct __attribute__ ((__packed__)){
			unsigned fragEn			: 1;
			unsigned tbd			: 2;
			unsigned msgType		: 5;
		}segHeaderBits;
		uint8_t	segHeaderByte;

		struct __attribute__ ((__packed__)){
			unsigned fragEn			: 1;
			unsigned firstLast		: 2;
			unsigned msgType		: 5;
		}fragHeaderBits;
		uint8_t	fragHeaderByte;
	};
	uint8_t	segLen;
}lrSegHeader_t;

typedef struct __attribute__ ((__packed__)) lrFragHeader_t
{
	lrSegHeader_t	segHeader;
	uint8_t 		fragSeqRemain;
}lrFragHeader_t;

typedef struct __attribute__ ((__packed__)) lrAirMsg_t
{
	lrHeader_t header;
	uint8_t pData[];
}lrAirMsg_t;

typedef struct __attribute__ ((__packed__)) lrSegMsg_t
{
	lrSegHeader_t header;
	uint8_t pData[];
}lrSegMsg_t;

typedef struct __attribute__ ((__packed__)) lrFragMsg_t
{
	lrFragHeader_t header;
	uint8_t pData[];
}lrFragMsg_t;

/* ----------------------------
 *        Declarations
 * ----------------------------
 */
void low_rate_init_header(uint8_t* satMsg,
						uint16_t mtID,
						uint8_t ebn0,
						uint8_t seqNum);
						
void low_rate_empty_msg(uint8_t* satMsg, airMsgInfo_t* info, int size);

void low_rate_add_segment(uint8_t* satMsg,
						sUDmsg* userMsg,
						airMsgInfo_t* info);
						
void low_rate_add_fragment(uint8_t* satMsg,
						sUDmsg* userMsg,
						airMsgInfo_t* info,
						int firstLast);
						
int low_rate_parse_header(uint8_t* satMsg, uint16_t mtID, uint8_t* remEbn0);


int low_rate_get_num_of_segments(uint8_t* satMsg);
uint8_t* low_rate_get_next_segment(uint8_t* pCurrSegment);

int low_rate_parse_segment(int satMsgLength,
						uint8_t* pSeg,
						airMsgHandler_t* airMsgBuf);

#endif /* __LINUX_LOW_RATE_H */