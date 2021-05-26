#ifndef __LINUX_GENEREALDEF_H_
#define __LINUX_GENEREALDEF_H_
/* ----------------------------
 *           includes
 * ----------------------------
 */
#include <linux/kern_levels.h> // For using printk info level
/* ----------------------------
 *           macros
 * ----------------------------
 */
#define print_function_begin() \
	printk(KERN_DEBUG "Enter to %s\n", __FUNCTION__);
#define print_function_end() \
	printk(KERN_DEBUG "Exit from %s\n", __FUNCTION__);

#define writeReg(baseAddr, regOffset, value)	\
			iowrite32(value, (baseAddr) + ((regOffset) << 2))

#define readReg(baseAddr, regOffset)	\
		ioread32((baseAddr) + ((regOffset) << 2))
	
#endif /* __LINUX_GENEREALDEF_H_ */
