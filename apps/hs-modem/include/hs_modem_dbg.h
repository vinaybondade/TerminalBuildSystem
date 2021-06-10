#ifndef __LINUX_HS_MODEM_DBG_H
#define __LINUX_HS_MODEM_DBG_H

/* ----------------------------
 *           includes
 * ----------------------------
 */
#include <linux/types.h>
#include <linux/printk.h>

extern int hsModemDbgLvl;

#define hsModem_dbg(lvl, __f, ...) printk(KERN_WARNING __f, ##__VA_ARGS__)//\
		// if(hsModemDbgLvl >= lvl) \
			printk(KERN_WARNING __f, ##__VA_ARGS__)

#endif /* __LINUX_HS_MODEM_DBG_H */