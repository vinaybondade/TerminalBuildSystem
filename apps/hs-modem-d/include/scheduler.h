#ifndef __LINUX_SCHEDULER_H_
#define __LINUX_SCHEDULER_H_
/* ----------------------------
 *           includes
 * ----------------------------
 */
#include <linux/types.h>
/* ----------------------------
 *            types
 * ----------------------------
 */

typedef struct schedulerTdd_t
{
	int32_t tsToTx;
	uint32_t waitTimestamp;
} schedulerTdd_t;

#endif /* __LINUX_SCHEDULER_H_ */
