#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/printk.h>
#include <linux/errno.h>

#include "low_rate.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amit Ginat");

static airProtCtx_t* ap = NULL;

/****************************/
/***** Module functions *****/
/****************************/
static int __init register_low_rate(void)
{
	ap = kmalloc(sizeof(*ap), GFP_KERNEL);
	if(!ap) {
		return -ENOMEM;
	}
	
	ap->f.init_header = low_rate_init_header;
	ap->f.add_segment = low_rate_add_segment;
	ap->f.add_fragment = low_rate_add_fragment;
	ap->f.empty_msg = low_rate_empty_msg;
	
	ap->f.parse_header = low_rate_parse_header;
	ap->f.get_num_of_segments = low_rate_get_num_of_segments;
	ap->f.get_next_segment = low_rate_get_next_segment;
	ap->f.parse_segment = low_rate_parse_segment;
	
	ap->i.headerSize = sizeof(lrHeader_t);
	ap->i.fragHeaderSize = sizeof(lrFragHeader_t);
	ap->i.segHeaderSize = sizeof(lrSegHeader_t);
	ap->i.maxFragments = 255; /* lrFragHeader_t.fragSeqRemain */

	return register_air_protocol(ap, LOW_RATE_TYPE_HEADER);
}
module_init(register_low_rate);

static void __exit unregister_low_rate(void)
{
	unregister_air_protocol(LOW_RATE_TYPE_HEADER);
	kfree(ap);
}
module_exit(unregister_low_rate);

/*****************************/
/***** Protocol Function *****/
/*****************************/
void low_rate_init_header(uint8_t* satMsg, uint16_t mtID,
						uint8_t ebn0, uint8_t seqNum)
{
	lrAirMsg_t* pck = (lrAirMsg_t*)satMsg;
	
	pck->header.typeHeader = LOW_RATE_TYPE_HEADER;
	pck->header.mtID = mtID;
	pck->header.ebn0 = ebn0;
	pck->header.seqNum = seqNum;
	pck->header.numOfSeg = 0;
}

void low_rate_empty_msg(uint8_t* satMsg, airMsgInfo_t* info, int size)
{
	lrAirMsg_t* pck = (lrAirMsg_t*)satMsg;
	memset(pck->pData + info->wrIndx, SAT_EMPTY_VAL, size);
}

void low_rate_add_segment(uint8_t* satMsg, sUDmsg* userMsg, airMsgInfo_t* info)
{
	lrAirMsg_t* pck = (lrAirMsg_t*)satMsg;
	lrSegHeader_t segHeader = {};
	
	pck->header.numOfSeg++;

	if(!userMsg)
	{
		segHeader.segHeaderBits.msgType = SAT_MSG_ID_EMPTY;
		segHeader.segLen = info->freeSpace - sizeof(segHeader);
	}
	else
	{ /* Copy segment header */
		segHeader.segLen = userMsg->len;
		segHeader.segHeaderBits.msgType = userMsg->msgType;
	}
	memcpy(pck->pData + info->wrIndx, &segHeader, sizeof(segHeader));
	info->wrIndx += sizeof(segHeader);

	if(!userMsg)
	{
		memset(pck->pData + info->wrIndx,
				SAT_EMPTY_VAL, segHeader.segLen);
		info->freeSpace = 0;
	}
	else
	{ /* Copy UD data */
		memcpy(pck->pData + info->wrIndx,
				(uint8_t*)(userMsg->data), segHeader.segLen);
		info->wrIndx += userMsg->len;
		info->freeSpace -= (sizeof(segHeader) + userMsg->len);
	}
}

void low_rate_add_fragment(uint8_t* satMsg, sUDmsg* userMsg,
					airMsgInfo_t* info, int firstLast)
{
	lrAirMsg_t* pck = (lrAirMsg_t*)satMsg;
	lrFragHeader_t f_header;

	pck->header.numOfSeg++;

	f_header.segHeader.fragHeaderBits.fragEn = 1;
	f_header.segHeader.fragHeaderBits.msgType = userMsg->msgType;
	f_header.segHeader.fragHeaderBits.firstLast =  firstLast;

	if(firstLast == SAT_FIRST_FRAG || firstLast == SAT_CONT_FRAG)
	{
		f_header.segHeader.segLen = 
				info->freeSpace - sizeof(f_header);
	}
	else
	{
		f_header.segHeader.segLen = userMsg->len - userMsg->reIndx;
	}

	if(firstLast == SAT_FIRST_FRAG)
	{
		userMsg->fragSec = calc_num_of_frags(info->maxDataOneFrag,
										f_header.segHeader.segLen,
										userMsg->len);
	}
	else
	{
		userMsg->fragSec--;
	}

	f_header.fragSeqRemain = userMsg->fragSec;
	if(firstLast == SAT_LAST_FRAG && f_header.fragSeqRemain != 0)
	{
		printk(KERN_INFO "ERROR: fragSeqRemain should be zero(%d)",
					f_header.fragSeqRemain);
	}

	/* Copy fragment header */
	memcpy(pck->pData + info->wrIndx,
			&f_header, sizeof(f_header));
	/* Copy part of UD data */
	info->wrIndx += sizeof(f_header);
	memcpy(pck->pData + info->wrIndx,
			(uint8_t*)(userMsg->data) + userMsg->reIndx,
			f_header.segHeader.segLen);

	info->wrIndx += f_header.segHeader.segLen;

	if(firstLast == SAT_FIRST_FRAG || firstLast == SAT_CONT_FRAG)
	{
		userMsg->fragFlag = 1;
		userMsg->reIndx += f_header.segHeader.segLen;
	}
	else
	{
		userMsg->fragFlag = 0;
		userMsg->reIndx = 0;
	}
	// freeSpace indicates a amount of bytes free for additional segments
	// in the current satellite message.
	info->freeSpace -= (sizeof(f_header) + f_header.segHeader.segLen);
}

int low_rate_parse_header(uint8_t* satMsg, uint16_t mtID, uint8_t* remEbn0)
{
	lrAirMsg_t* pck = (lrAirMsg_t*)satMsg;
	
	if(pck->header.typeHeader != LOW_RATE_TYPE_HEADER)
	{
		return -EPROTOTYPE;
	}
	if(pck->header.mtID != mtID && pck->header.mtID != 0)
	{
		return -ENODEV;
	}
	*remEbn0 = pck->header.ebn0;
	
	return pck->header.seqNum;
}

int low_rate_get_num_of_segments(uint8_t* satMsg)
{
	lrAirMsg_t* pck = (lrAirMsg_t*)satMsg;
	return ((int)pck->header.numOfSeg);
}

uint8_t* low_rate_get_next_segment(uint8_t* pCurrSegment)
{
	lrSegMsg_t* pSeg = (lrSegMsg_t*)pCurrSegment;
	lrFragMsg_t* pFrag = (lrFragMsg_t*)pCurrSegment;

	if(pSeg->header.fragHeaderBits.fragEn == 0)
	{
		return (pSeg->pData + pSeg->header.segLen);
	}
	else
	{
		return (pFrag->pData + pFrag->header.segHeader.segLen);
	}
}

int low_rate_parse_segment(int satMsgLength,
						uint8_t* pSeg,
						airMsgHandler_t* handler)
{
	lrSegMsg_t* pCurrSegment = (lrSegMsg_t*)pSeg;
	lrSegHeader_t* pSegHeader = &pCurrSegment->header;

	lrFragMsg_t* pFragSegment = NULL;
	int fragSeqRemain;
	
	int h_indx = SAT_MSG_ID_DATA;
	if(pSegHeader->segHeaderBits.msgType == SAT_MSG_ID_TABLE)
	{
		h_indx = SAT_MSG_ID_TABLE;
	}

	handler->buf[h_indx].isFirst = 0;
	
	if(handler->buf[h_indx].done && handler->buf[h_indx].need_free)
	{
		kfree(handler->buf[h_indx].pData);
		handler->buf[h_indx].pData = NULL;
		handler->buf[h_indx].need_free = 0;
		handler->buf[h_indx].done = 0;
		handler->buf[h_indx].fragByteCnt = 0;
		handler->buf[h_indx].fragSeqNum = 0;
	}

	/* Check if this segment was not fragmented */
	if(pSegHeader->fragHeaderBits.fragEn == 0)
	{
		//if(handler->buf[h_indx].need_free)
		//{
		//	kfree(handler->buf[h_indx].pData);
		//	handler->buf[h_indx].pData = NULL;
		//	handler->buf[h_indx].need_free = 0;
		//	handler->buf[h_indx].done = 0;
		//	handler->buf[h_indx].fragByteCnt = 0;
		//	handler->buf[h_indx].fragSeqNum = 0;
		//}
		
		handler->buf[h_indx].isFirst = 1;
		handler->buf[h_indx].pData = pCurrSegment->pData;
		handler->buf[h_indx].len = pSegHeader->segLen;
		handler->msgType = pSegHeader->segHeaderBits.msgType;
		handler->buf[h_indx].done = 1;
		handler->buf[h_indx].need_free = 0;
	}
	else /* This segment was fragmented */
	{
		pFragSegment = (lrFragMsg_t*)pSeg;
		pSegHeader = &pFragSegment->header.segHeader;
		fragSeqRemain = pFragSegment->header.fragSeqRemain;

		if(pSegHeader->fragHeaderBits.firstLast == SAT_FIRST_FRAG)
		{
			handler->buf[h_indx].isFirst = 1;
			handler->buf[h_indx].done = 0;
			if(handler->buf[h_indx].need_free)
			{
				kfree(handler->buf[h_indx].pData);
				handler->buf[h_indx].pData = NULL;
				handler->buf[h_indx].need_free = 0;
				printk(KERN_WARNING
					"ERROR: Received fragment from different message."
					"Last fragment will be drop\n");
			}
			/* first fragment */
			handler->msgType = pSegHeader->fragHeaderBits.msgType;

			/* Allocate memory for the fragmented message */
			handler->buf[h_indx].pDataSize = satMsgLength * (fragSeqRemain+1);

			handler->buf[h_indx].pData = kmalloc(handler->buf[h_indx].pDataSize, GFP_KERNEL);
			if(handler->buf[h_indx].pData == NULL)
			{
				//printk(KERN_WARNING "handler->buf[h_indx].pData is NULL\n");
				return -ENOMEM; /* Out of memory */
			}
			handler->buf[h_indx].need_free = 1;

			handler->buf[h_indx].fragSeqNum = fragSeqRemain;
			handler->buf[h_indx].fragByteCnt = pSegHeader->segLen;
			memcpy(handler->buf[h_indx].pData,
					pFragSegment->pData,
					pSegHeader->segLen);
			/*
			 * There is no need to increment the segment's pointer
			 *('pCurrSegment') to point on the segment.
			 * Because if this is the first fragment,
			 * it must be last segment in 'pSatRecBuf'.
			 */
		}
		else if(pSegHeader->fragHeaderBits.firstLast == SAT_CONT_FRAG ||
				pSegHeader->fragHeaderBits.firstLast == SAT_LAST_FRAG)
		{
			/* ERROR checking */
			if(handler->msgType != pSegHeader->fragHeaderBits.msgType)
			{
				if(handler->buf[h_indx].need_free)
				{
					kfree(handler->buf[h_indx].pData);
					handler->buf[h_indx].pData = NULL;
					handler->buf[h_indx].need_free = 0;
				}
				printk(KERN_WARNING
					"ERROR: Received fragment from different message."
					"Last fragment will be drop\n");
			}
			else if(fragSeqRemain != handler->buf[h_indx].fragSeqNum -1)
			{
				if(handler->buf[h_indx].need_free)
				{
					kfree(handler->buf[h_indx].pData);
					handler->buf[h_indx].pData = NULL;
					handler->buf[h_indx].need_free = 0;
				}
				printk(KERN_WARNING 
					"Fragment number %d didn't received\n",
					handler->buf[h_indx].fragSeqNum -1);

			}
			else if (handler->buf[h_indx].pData && handler->buf[h_indx].need_free)
			{
				memcpy((handler->buf[h_indx].pData + handler->buf[h_indx].fragByteCnt),
						pFragSegment->pData,
						pSegHeader->segLen);

				handler->buf[h_indx].fragSeqNum = fragSeqRemain;
				handler->buf[h_indx].fragByteCnt += pSegHeader->segLen;

				/* Check if received the last fragment */
				if(pSegHeader->fragHeaderBits.firstLast == SAT_LAST_FRAG)
				{
					/* Finally received the whole fragmented message */
					handler->buf[h_indx].len = handler->buf[h_indx].fragByteCnt;
					handler->buf[h_indx].done = 1;
				}
			}
		}
	}
	return 0;
}