#ifndef __LINUX_HS_FIFO_H_
#define __LINUX_HS_FIFO_H_
/* ----------------------------
 *           includes
 * ----------------------------
 */
#include <linux/types.h>
/* ----------------------------
 *           macros
 * ----------------------------
 */
struct hs_user_msg_t
{
	size_t type;
	size_t size;
	uint32_t timestamp;
	uint8_t buf[];
};

int hsFifo_done(struct hs_user_msg_t **msg);
int hsFifo_get_user_msg(struct hs_user_msg_t **msg);
int hsFifo_put_user_msg(size_t type,
						uint32_t timestamp,
						uint8_t *buf,
						size_t size);
	
#endif /* __LINUX_HS_FIFO_H_ */
