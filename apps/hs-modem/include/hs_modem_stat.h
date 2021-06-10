#ifndef __LINUX_HS_MODEM_STAT_H_
#define __LINUX_HS_MODEM_STAT_H_
/* ----------------------------
 *           includes
 * ----------------------------
 */
#include <linux/types.h>
/* ----------------------------
 *            types
 * ----------------------------
 */

typedef struct serviceStat_t
{
	uint32_t numOfBytes;
	uint32_t numOfFullSeg;
	uint32_t numOfPartialSeg;
} serviceStat_t;

typedef enum
{
	LINK_DIR_TX = 0,
	LINK_DIR_RX = 1,
	LINK_DIR_LAST
} linkDir_t;

typedef enum term_service_type {
    TERM_SERVICE_CORE_ADMIN       	= 0,
    TERM_SERVICE_IOT              	= 1,
    TERM_SERVICE_VOIP             	= 2,
    TERM_SERVICE_USER_MOBILE      	= 3,
    TERM_SERVICE_USER_UDP         	= 4,
    TERM_SERVICE_USER_RELIABLE_UDP	= 5,
    TERM_SERVICE_USER_TCP	  		= 6,
    TERM_SERVICE_TYPE_MAX  
} term_service_type_t; //copied from hubpp terminal.h

typedef struct airProtStat_t
{
	uint32_t rxMsgDropped;
	serviceStat_t srvStat[LINK_DIR_LAST][TERM_SERVICE_TYPE_MAX];
} airProtStat_t;
 
typedef struct hsModemStat_t
{
	uint32_t      txIrqCnt;
	uint32_t      rxCrcErrCnt;
	uint32_t      rxGoodPckCnt;
	uint32_t      rxTsCnt;
	airProtStat_t apStat;
} hsModemStat_t;

#endif /* __LINUX_HS_MODEM_STAT_H_ */
