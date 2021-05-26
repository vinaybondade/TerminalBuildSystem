#ifndef __LINUX_AIR_MODE_H
#define __LINUX_AIR_MODE_H
/* ----------------------------
 *           includes
 * ----------------------------
 */
#include <linux/types.h>

/* ----------------------------
 *    Definitions and macros
 * ----------------------------
 */
#define	MIN_FRAGMENT_SIZE		2
/*
 * Define constants for fragment messages
 */
#define SAT_FIRST_FRAG			1
#define SAT_CONT_FRAG			2 // Continues fragment message
#define SAT_LAST_FRAG			3

/*
 * Constant value for empty messages
 */
#define SAT_EMPTY_VAL			0xFF

/*
 * Satellite message ID for each type header
 */
// Common IDs
enum satMsgId_t
{
	SAT_MSG_ID_DATA			= 0x00,
	SAT_MSG_ID_TABLE		= 0x01,
	SAT_MSG_ID_DBG_STR		= 0x1e,
	SAT_MSG_ID_EMPTY		= 0x1f,
};
#define SAT_MSG_ID_TYPES		2

#define MAX_SAT_TYPE_HEADER 8 // correspond to HSMODE_HEADER.typeHeader
#define HS_MODEM_MAX_SEQNUM 255

/* ----------------------------
 *            types
 * ----------------------------
 */

typedef enum
{
	LOW_RATE_TYPE_HEADER  = 0,
	HIGH_RATE_TYPE_HEADER = 1,
	__SATHEADERTYPES_END  = 2
}satHeaderTypes_t;

typedef struct airMsgInfo_t
{
	satHeaderTypes_t airMode;   // Current air protocol.
	uint16_t satMsgLenTx;		// Current satellite message length for TX
	uint16_t satMsgLenRx;		// Current satellite message length for RX

	int maxDataOneSeg;			// The maximum data that can be filled in case the satellite message filled with only one segment
	int maxDataOneFrag;			// The maximum data that can be filled in case the satellite message filled with only one fragment

	int minDataToBeOneSeg;		// The minimum data for one segment and not worth it anymore to add another segment/fragment
	int minDataToBeOneFrag;		// The minimum data for one fragment and not worth it anymore to add another segment/fragment(In case it's the last fragment)

	int freeSpace;				// Free space in DATA section in the satellite message
	int wrIndx;					// Write index in section DATA in the satellite message
}airMsgInfo_t;

typedef struct sUDmsg
{
	uint8_t* data;
	uint16_t len;
	uint8_t msgType;

	uint8_t seqNum;
	uint8_t fragFlag;
	uint16_t reIndx;
	uint8_t fragSec;
}sUDmsg;

typedef struct airMsgBuf_t
{
	uint32_t len;
	uint32_t fragByteCnt;
	uint32_t fragSeqNum;
	uint8_t done;
	uint8_t need_free;
	uint8_t isFirst;
	uint8_t *pData;
	uint32_t pDataSize; // tmp
} airMsgBuf_t;

typedef struct airMsgHandler_t
{
	uint8_t msgType;
	uint32_t rcv_timestamp;
	airMsgBuf_t buf[SAT_MSG_ID_TYPES];
} airMsgHandler_t;

typedef struct airModeFunc_t
{
	/* Common */
	int (*get_num_of_segments) (uint8_t* satMsg);

	/* For TX path */
	void (*init_header) (uint8_t* satMsg, uint16_t mtID,
							uint8_t ebn0, uint8_t seqNum);
	void (*add_segment) (uint8_t* satMsg, sUDmsg* userMsg, airMsgInfo_t* info);
	void (*add_fragment) (uint8_t* satMsg, sUDmsg* userMsg,
							airMsgInfo_t* info, int firstLast);
	void (*empty_msg) (uint8_t* satMsg, airMsgInfo_t* info, int size);

	/* For RX path */
	int (*parse_header) (uint8_t* satMsg, uint16_t mtID, uint8_t* remEbn0);
	uint8_t* (*get_next_segment) (uint8_t* pCurrSegment);
	int (*parse_segment) (int satMsgLength, uint8_t* pSeg,
							airMsgHandler_t* airMsgBuf);
} airModeFunc_t;

typedef struct airModeInfo_t
{
	uint32_t headerSize;		// Size of Header
	uint32_t fragHeaderSize;	// Size of fragment header
	uint32_t segHeaderSize;		// Size of segment header
	uint32_t maxFragments;		// max frag that can be sent using the protocol
}airModeInfo_t;

typedef struct airProtCtx_t
{
	airModeFunc_t f;
	airModeInfo_t i;
}airProtCtx_t;

/* ----------------------------
 *        Declarations
 * ----------------------------
 */


#endif /* __LINUX_AIR_MODE_H */