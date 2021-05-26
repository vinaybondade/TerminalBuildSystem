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

typedef struct airProtStat_t
{
	uint32_t rxMsgDropped;
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
