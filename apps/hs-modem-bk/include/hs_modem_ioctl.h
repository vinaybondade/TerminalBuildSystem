#ifndef __LINUX_HS_MODEM_IOCTL_H_
#define __LINUX_HS_MODEM_IOCTL_H_
/* ----------------------------
 *          includes
 * ----------------------------
 */
#include <linux/types.h>
#include "hs_modem_rate.h"
#include "hs_modem_stat.h"
#include "scheduler.h"
/* ----------------------------
 *           types
 * ----------------------------
 */
typedef struct
{
	uint32_t modemSubPhy; // 0-Rx, 1-Tx
	uint32_t isRst; // 0-deRst, 1-rst
}modem_rst_req_t;
 
/* ----------------------------
 *          macros
 * ----------------------------
 */
#define HSMODEM_RESET(devt)			_IOW(MAJOR(devt), 0, modem_rst_req_t*)
#define HSMODEM_CALIB_RX(devt)		_IOW(MAJOR(devt), 1, modem_rx_param_t*)
#define HSMODEM_CALIB_TX(devt)		_IOW(MAJOR(devt), 2, modem_tx_param_t*)
#define HSMODEM_STAT(devt)			_IOR(MAJOR(devt), 3, hsModemStat_t*)
#define HSMODEM_AIR_MODE(devt)		_IOW(MAJOR(devt), 4, size_t)
#define HSMODEM_SET_SCHEDULER(devt)	_IOW(MAJOR(devt), 5, schedulerTdd_t*)
#define HSMODEM_SET_MT_ID(devt)		_IOW(MAJOR(devt), 6, uint16_t)



#endif /* __LINUX_HS_MODEM_IOCTL_H_ */
