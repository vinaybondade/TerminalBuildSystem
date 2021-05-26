/* ----------------------------
 *           includes
 * ----------------------------
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/errno.h>

#include "hs_modem_dbg.h"
#include "air_interface.h"
#include "hs-crc.h"

#include "hs-fifo.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amit Ginat");
/* ----------------------------
 *           globals
 * ----------------------------
 */
extern airProtCtx_t* apctx[];

/* ----------------------------
 *        implementation
 * ----------------------------
 */
int is_air_msg_valid(airContext_t* ctx)
{
	crc16_t calcCrc;
	crc16_t* recCrc;
	char* satMsg = ctx->satMsgRx;

	calcCrc = crcSlow(satMsg, ctx->a_info.satMsgLenRx - sizeof(crc16_t));
	recCrc = (crc16_t*)(satMsg + ctx->a_info.satMsgLenRx - sizeof(crc16_t));
	hsModem_dbg(5, "0x%x, 0x%x\n", (uint16_t)calcCrc, (uint16_t)*recCrc);
	return (calcCrc == (*recCrc));
}	

int calc_num_of_frags(int maxDataOneFragment, int netoFreeSpace, int dataSize)
{
	const int dataOverFullSatMsg = dataSize - netoFreeSpace;
	int fragSeqNeed = dataOverFullSatMsg / maxDataOneFragment;

	if(dataOverFullSatMsg % maxDataOneFragment) {
		fragSeqNeed++;
	}
	return fragSeqNeed;
}
EXPORT_SYMBOL_GPL(calc_num_of_frags);

int __must_check init_air_protocol(airContext_t* ctx,
						satHeaderTypes_t airMode,
						uint16_t satMsgLenTx,
						uint16_t satMsgLenRx)
{
	airProtCtx_t* ap = apctx[airMode];
	airMsgInfo_t* a_info;
	
	if(!ctx) {
		return -EINVAL;
	}
	if(!ap) {
		return -EUNATCH;
	}
	
	ctx->ap = ap;
	a_info = &(ctx->a_info);
	
	a_info->freeSpace = satMsgLenTx - ap->i.headerSize - sizeof(crc16_t);
	a_info->maxDataOneSeg = a_info->freeSpace - ap->i.segHeaderSize;
	a_info->maxDataOneFrag = a_info->freeSpace - ap->i.fragHeaderSize;
	
	if(a_info->freeSpace <= 0 ||
		a_info->maxDataOneSeg <= 0 ||
		a_info->maxDataOneFrag <= 0)
	{
		return -EINVAL;
	}
	a_info->airMode = airMode;
	a_info->satMsgLenTx = satMsgLenTx;
	a_info->minDataToBeOneFrag = a_info->maxDataOneFrag - MIN_FRAGMENT_SIZE;
	a_info->minDataToBeOneSeg = a_info->maxDataOneSeg - MIN_FRAGMENT_SIZE;
	
	hsModem_dbg(1, "satMsgLenTx = %u\n", satMsgLenTx);
	ctx->satMsgTx = kmalloc(satMsgLenTx, GFP_KERNEL);
	if(!ctx->satMsgTx) {
		return -ENOMEM;
	}

	a_info->satMsgLenRx = satMsgLenRx;
	hsModem_dbg(1, "satMsgLenRx = %u\n", satMsgLenRx);
	ctx->satMsgRx = kmalloc(satMsgLenRx, GFP_KERNEL);
	if(!ctx->satMsgRx) {
		return -ENOMEM;
	}
	
	return 0;
}

void destroy_air_protocol(airContext_t* ctx)
{
	if(ctx->satMsgTx)
	{
		kfree(ctx->satMsgTx);
		ctx->satMsgTx = NULL;
	}
	if(ctx->satMsgRx)
	{
		kfree(ctx->satMsgRx);
		ctx->satMsgRx = NULL;
	}
}

int __must_check register_air_protocol(airProtCtx_t* ap,
										satHeaderTypes_t airMode)
{
	if(!ap) {
		return -EINVAL;
	}
	
	if(apctx[airMode]) {
		return -EADDRINUSE;
	}
	
	printk(KERN_INFO "airMode(%u) registered\n", (uint32_t)airMode);
	
	apctx[airMode] = ap;
	
	return 0;
}
EXPORT_SYMBOL_GPL(register_air_protocol);

void unregister_air_protocol(satHeaderTypes_t airMode)
{
	apctx[airMode] = NULL;
	printk(KERN_INFO "unregister airMode(%u)\n", (uint32_t)airMode);
}
EXPORT_SYMBOL_GPL(unregister_air_protocol);

static void fill_remain_sat_msg(airContext_t* ctx)
{
	int msgIsReady = 0;
	int msgLen;
	airProtCtx_t* ap = ctx->ap;
	airMsgInfo_t* a_info = &(ctx->a_info);
	uint8_t* satMsg = ctx->satMsgTx;
	sUDmsg* userMsg = &(ctx->userMsg);
	
	hsModem_dbg(9, "freeSpace = %u\n", a_info->freeSpace);
	
	while(!msgIsReady)
	{
		if(a_info->freeSpace < ap->i.fragHeaderSize + MIN_FRAGMENT_SIZE)
		{
			hsModem_dbg(9, "not enough freeSpace(%u) \n", a_info->freeSpace);
			memset(satMsg + ap->i.headerSize + a_info->wrIndx,
					SAT_EMPTY_VAL, a_info->freeSpace);
			msgIsReady = 1;
			continue;
		}

		msgLen = get_user_msg(ctx, a_info);
		hsModem_dbg(9, "new msgLen = %d\n", msgLen);
		if(msgLen)
		{
			// A case in which exist additional user packet in the
			// queue message.
			if(msgLen <= a_info->freeSpace - ap->i.segHeaderSize)
			{
				// A case in which whole user message will fit the
				// rest free space
				// in satellite message.
				hsModem_dbg(9, "segment, msg small/fit rest freeSpace\n");
				ap->f.add_segment(satMsg, userMsg, a_info);

				if(a_info->freeSpace < ap->i.fragHeaderSize + MIN_FRAGMENT_SIZE)
				{
					// If there is no free space or not enough space in 
					// satellite message, the satellite message is ready.
					msgIsReady = 1;
					if(a_info->freeSpace > 0)
					{
						// If there are some free bytes fill it with 
						// SAT_EMPTY_VAL values.
						memset(satMsg + ap->i.headerSize + a_info->wrIndx,
								SAT_EMPTY_VAL, a_info->freeSpace);
					}
				}
			}
			else
			{
				// A case in witch the free space is not enough for whole
				// user message, it means that user message will be fragmented.
				// And this segment will be first fragment of the user message.
				hsModem_dbg(9, "fragment, msg not fit rest freeSpace\n");
				ap->f.add_fragment(satMsg, userMsg, a_info, SAT_FIRST_FRAG);

				msgIsReady=1;
			}
		}
		else
		{
			// A case in which there free space in satellite message
			// but there is no user messages in queue.
			msgIsReady = 1;
			hsModem_dbg(9, "segment, empty msg\n");
			ap->f.add_segment(satMsg, NULL, a_info);
		}
	}
}

int air_msg_build(airContext_t* ctx, uint8_t modemEbN0data6bit)
{
	static uint8_t satMsgSeqNum = 0;
	static int freeSpace_orig;
	int msgLen;
	crc16_t crc16 = 0;
	
	airMsgInfo_t* a_info;
	airModeFunc_t* func;
	uint8_t* satMsg;
	sUDmsg* userMsg;
	
	if(!ctx) {
		return -EINVAL;
	}
	if(!ctx->ap) {
		return -EUNATCH;
	}
	
	func = &(ctx->ap->f);
	a_info = &(ctx->a_info);
	satMsg = ctx->satMsgTx;
	userMsg = &ctx->userMsg;
	
	a_info->wrIndx = 0;
	/* save freeSpace value */
	freeSpace_orig = a_info->freeSpace;

	func->init_header(satMsg, ctx->mtID, modemEbN0data6bit, satMsgSeqNum++);
	
	/*************************************************************************
	 * A case in which a current message is a continuation fragment          *
	 * of a previous fragment.                                               *
	 *************************************************************************/
	if(userMsg->fragFlag == 1)
	{
		hsModem_dbg(9, "userMsg->fragFlag = 1\n");
		msgLen = userMsg->len;
		/*********************************************************************
		 * A case in which the current message is fragment of previous       *
		 * message and the remained message will fill whole air message      *
		 *********************************************************************/
		if((msgLen-userMsg->reIndx > a_info->minDataToBeOneFrag) &&
			(msgLen-userMsg->reIndx <= a_info->maxDataOneFrag))
		{
			hsModem_dbg(9, "fragment, remained msg fill whole airMsg\n");
			func->add_fragment(ctx->satMsgTx, userMsg, a_info, SAT_LAST_FRAG);

			if(a_info->freeSpace)
			{
				func->empty_msg(satMsg, a_info, a_info->freeSpace);
			}
		}
		/*********************************************************************
		 * A case in which the current message is fragment of previous       *
		 * message and the remained message too long and it will             *
		 * fragmented too                                                    *
		 *********************************************************************/
		else if(msgLen-userMsg->reIndx > a_info->maxDataOneFrag)
		{
			hsModem_dbg(3, "Fragmented message with more than 2 fragments\n");
			func->add_fragment(ctx->satMsgTx, userMsg, a_info, SAT_CONT_FRAG);
		}
		/*********************************************************************
		 * A case in which the current message is fragment of previous       *
		 * message and the remained message smaller than the air message     *
		 *********************************************************************/
		else if(msgLen-userMsg->reIndx <= a_info->minDataToBeOneFrag)
		{
			hsModem_dbg(9, "fragment, remained msg smaller than airMsg\n");
			func->add_fragment(ctx->satMsgTx, userMsg, a_info, SAT_LAST_FRAG);

			fill_remain_sat_msg(ctx);
		}
	}
	/*************************************************************************
	 * A case in which a current message is not fragment of previous message *
	 *************************************************************************/
	else
	{
		hsModem_dbg(9, "userMsg->fragFlag = 0\n");
		msgLen = get_user_msg(ctx, a_info);
		hsModem_dbg(9, "msgLen = %d\n", msgLen);
		/*********************************************************************
		 * A case in which a current message is not fragment of previous     *
		 * message and user message will fill whole air message or it will   *
		 * be an empty air message                                           *
		 *********************************************************************/
		if(((msgLen > a_info->minDataToBeOneSeg) && 
			(msgLen <= a_info->maxDataOneSeg)) || !msgLen)
		{
			if(msgLen)
			{
				hsModem_dbg(9, "full segment with one msg\n");
				func->add_segment(ctx->satMsgTx, userMsg, a_info);

				// A case in which a user message not completely fills
				// the air message.
				if(a_info->freeSpace)
				{
					func->empty_msg(satMsg, a_info, a_info->freeSpace);
				}

			}
			else
			{
				// A case in which there are no user message, 
				// and whole air message is an empty message.
				hsModem_dbg(9, "add empty seg\n");
				func->add_segment(ctx->satMsgTx, NULL, a_info);
			}
		}
		/*********************************************************************
		 * A case in which a current message is not fragment of previous     *
		 * message and user message won't fill whole air message             *
		 *********************************************************************/
		else if(msgLen <= a_info->minDataToBeOneSeg)
		{// User message will occupy a part of satellite message.
			hsModem_dbg(9, "segment, msg smaller than airMsg\n");
			func->add_segment(ctx->satMsgTx, userMsg, a_info);

			// read additional user messages in order to fill the
			// free space of satellite message, msgIsReady indicates
			// that satellite message is full.
			fill_remain_sat_msg(ctx);
		}
		else
		{   // A case in which user message larger than data segment
			// in satellite message, it means that user message will be
			// fragmented and current satellite message consist of one
			// segment that includes first fragment of user message.
			hsModem_dbg(9, "fragment, fragment in middle of airMsg\n");
			func->add_fragment(ctx->satMsgTx, userMsg, a_info, SAT_FIRST_FRAG);
		}
	}
	
	crc16 = crcSlow(satMsg, a_info->satMsgLenTx - sizeof(crc16));
								
	*(satMsg + a_info->satMsgLenTx - sizeof(crc16)) = 
			(uint8_t)crc16;
	*(satMsg + a_info->satMsgLenTx - sizeof(crc16) +1) = 
			(uint8_t)(crc16 >> 8);
	

	a_info->freeSpace = freeSpace_orig;

	return func->get_num_of_segments(satMsg);
}

int get_user_msg(airContext_t* ctx, airMsgInfo_t* info)
{	
	int len = 0;
	sUDmsg* userMsg = &ctx->userMsg;
	static struct hs_user_msg_t *msg = NULL;
	
	if(msg)
	{
		hsFifo_done(&msg);
	}
	
	if(ctx->dbg.constMsg && ctx->dbg.sendConstMsg)
	{
		ctx->dbg.sendConstMsg = 0;
		/* Generate user message */
		userMsg->data = ctx->dbg.constMsg;
		userMsg->len = ctx->dbg.msgLen +1;
		userMsg->msgType = SAT_MSG_ID_DBG_STR;
		len = userMsg->len;
	}
	else if(hsFifo_get_user_msg(&msg) > 0)
	{
		hsModem_dbg(7,"hsFifo_get_user_msg: msg '%p'\n", msg);
		hsModem_dbg(7,"got from fifo type 0x%x\n", msg->type);
		hsModem_dbg(7,"got from fifo size %d\n", msg->size);
		if(msg->type == SAT_MSG_ID_DBG_STR)
		{
			hsModem_dbg(1,"got from fifo buf '%s'\n", msg->buf);
		}
		userMsg->data = msg->buf;
		userMsg->len = msg->size;
		userMsg->msgType = msg->type;
		
		
		len = userMsg->len;
	}
	
	hsModem_dbg(9,"get_user_msg return %d\n", len);
	
	return len;
}

int parse_air_msg(airContext_t* ctx, airProtStat_t* stat)
{
	int ret;	
	int i;
	airProtCtx_t* ap;
	
	uint8_t* pCurrSegment;
	int numOfSegments = 0;
	const int maxSeqNum = HS_MODEM_MAX_SEQNUM;
	
	static uint8_t prevSeqNum = 0;
	uint8_t seqNum;
	size_t h_indx;
	
	if(!ctx) {
		return -EINVAL;
	}
	if(!ctx->ap) {
		return -EUNATCH;
	}
	ap = ctx->ap;
	
	/* Check if header type valid */
	ret = ap->f.parse_header(ctx->satMsgRx, ctx->mtID, &ctx->remEbN0);
	if(ret < 0)
	{
		return ret;
	}
	seqNum = ret;
	
	/* Check if seqNum valid */
	if((prevSeqNum+1 != seqNum && prevSeqNum != maxSeqNum && seqNum != 0) ||
		(prevSeqNum == maxSeqNum && seqNum != 0))
	{
		if (prevSeqNum == maxSeqNum)
		{
			stat->rxMsgDropped += seqNum;
		}
		else if(prevSeqNum < seqNum)
		{
			stat->rxMsgDropped += (seqNum - prevSeqNum -1);
		}
		else
		{
			stat->rxMsgDropped += ((maxSeqNum-prevSeqNum) + seqNum -1);
		}
		hsModem_dbg(1, "Sat Msg Drop %u %u %u\n",
					prevSeqNum,
					seqNum,
					stat->rxMsgDropped);
	}
	hsModem_dbg(9, "current seqNum = %d\n", seqNum);
	prevSeqNum = seqNum;
	
	/* parse segments */
	pCurrSegment = ctx->satMsgRx + ap->i.headerSize;
	
	numOfSegments = ap->f.get_num_of_segments(ctx->satMsgRx);
	hsModem_dbg(9, "numOfSegments = %d\n", numOfSegments);
	
	for(i = 0; i < numOfSegments; ++i)
	{
		ret = ap->f.parse_segment(ctx->a_info.satMsgLenRx, pCurrSegment,
									&ctx->msgHandler);
		if(ret < 0)
		{
		}
		else if(!ret)
		{
			hsModem_dbg(5, "msgType = 0x%x\n", ctx->msgHandler.msgType);
			
			if(ctx->msgHandler.msgType == SAT_MSG_ID_TABLE)
			{
				h_indx = SAT_MSG_ID_TABLE;
			}
			else if(ctx->msgHandler.msgType != SAT_MSG_ID_EMPTY)
			{
				h_indx = SAT_MSG_ID_DATA;
			}
			else
			{
				pCurrSegment = ap->f.get_next_segment(pCurrSegment);
				continue;
			}
			hsModem_dbg(5, "h_indx = %u\n", h_indx);
			
			if(ctx->msgHandler.buf[h_indx].isFirst)
			{
				ctx->msg_timestamp = ctx->msgHandler.rcv_timestamp;
			}
			
			if(ctx->msgHandler.buf[h_indx].done)
			{
				ret = hsFifo_put_user_msg(ctx->msgHandler.msgType,
							ctx->msg_timestamp,
							ctx->msgHandler.buf[h_indx].pData,
							ctx->msgHandler.buf[h_indx].len);
				if(ret != ctx->msgHandler.buf[h_indx].len)
				{
					hsModem_dbg(1, "hsFifo_put_user_msg return %d\n", ret);
				}
				
				// TODO tmp only for dbg
				if(ctx->msgHandler.msgType == SAT_MSG_ID_DBG_STR)
				{
					hsModem_dbg(1, "%u:'%s':%u\n", 
						ctx->msg_timestamp,
						ctx->msgHandler.buf[h_indx].pData,
						ctx->msgHandler.rcv_timestamp);
				}
			}
		}
		pCurrSegment = ap->f.get_next_segment(pCurrSegment);
	}

	return numOfSegments;
}
