#ifndef __LINUX_AIRINTERFACE_H
#define __LINUX_AIRINTERFACE_H
/* ----------------------------
 *           includes
 * ----------------------------
 */
#include <linux/types.h>
#include "air_mode.h"
#include "hs_modem_stat.h"
/* ----------------------------
 *    Definitions and macros
 * ----------------------------
 */
#define MAX_MT_ID_VALID 0xffe
 /* ----------------------------
 *            types
 * ----------------------------
 */
typedef uint16_t crc16_t;

typedef struct airContext_t
{
	struct dbg
	{
		char* constMsg;      // const msg for build
		size_t constMsgLen;  // const msg len
		size_t msgPreSize;   // msg prefix size
		size_t msgLen;       // total msg len
		uint8_t msgNum;      // prefix for msg
		int sendConstMsg;    // flag if need to send the msg
		int32_t tdd_self_trig;
	}dbg;

	uint16_t mtID;
	airProtCtx_t* ap;
	airMsgInfo_t a_info;

	uint8_t* satMsgTx;
	sUDmsg userMsg;

	uint8_t* satMsgRx;
	airMsgHandler_t msgHandler;
	uint8_t remEbN0;
	
	uint32_t msg_timestamp; // first fragment timestamp
}airContext_t;

/* ----------------------------
 *        Declarations
 * ----------------------------
 */
int is_air_msg_valid(airContext_t* ctx);
int calc_num_of_frags(int maxDataOneFragment, int netoFreeSpace, int dataSize);
void destroy_air_protocol(airContext_t* ctx);
int __must_check init_air_protocol(airContext_t* ctx,
						satHeaderTypes_t airMode,
						uint16_t satMsgLenTx,
						uint16_t satMsgLenRx);
					
int air_msg_build(airContext_t* ctx, uint8_t modemEbN0data6bit);

int parse_air_msg(airContext_t* ctx, airProtStat_t* stat);

int get_user_msg(airContext_t* ctx, airMsgInfo_t* info);

int register_air_protocol(airProtCtx_t* ap, satHeaderTypes_t airMode);
void unregister_air_protocol(satHeaderTypes_t airMode);

#endif /* __LINUX_AIRINTERFACE_H */