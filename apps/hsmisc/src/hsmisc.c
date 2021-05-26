

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>	/* printk() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/ioctl.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/ioport.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/reboot.h>
#include "power-interface.h"

#define MODNAME "hsmisc"

//////////////////////////////////
/* Local function declarations. */
//////////////////////////////////

#define PHYSICAL_MEMORY_ADDR	0x18000000
#define MEMORY_SIZE 		0x20000
#define MEMORY_SIZE_HALF 	0x10000
#define MEMORY_SIZE_OFFSET_1        0x4

static int power_interface_open(struct inode *inode, struct file *file);
static int power_interface_release(struct inode *inode, struct file *filp);
static long power_interface_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param);

//////////////////////////////////
/* Module Variables */
//////////////////////////////////

void *physical_mem_p = 0;

//////////////////////////////////

static struct file_operations power_interface_file_ops = {
	.owner   		= THIS_MODULE,
	.open    		= power_interface_open,
	.unlocked_ioctl	 	= power_interface_ioctl,
	.release 		= power_interface_release
};

static struct miscdevice power_interface_misc_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = MODNAME,
	.fops = &power_interface_file_ops,
};

static int power_interface_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int power_interface_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long power_interface_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
{
	unsigned int local_value = 0;
	unsigned int *p_mem_addr = (physical_mem_p + MEMORY_SIZE_HALF);
	unsigned int *p_mem_addr_r1 = (physical_mem_p + MEMORY_SIZE_HALF + MEMORY_SIZE_OFFSET_1);
	int ret = 0;

	switch(ioctl_num)
	{
		case POWER_INTERFACE_REBOOT:

            		// Received Reboot command.
            		//machine_restart(NULL);//LINUX_REBOOT_CMD_RESTART);
			//kernel_restart(NULL);//LINUX_REBOOT_CMD_RESTART);
			emergency_restart();//LINUX_REBOOT_CMD_RESTART);
			//printk("In Reboot IOCTL.\n");
			break;

		case POWER_INTERFACE_SHUTDOWN:
           	 	// Shutdown command received. Call apropriate function here.
           	 	break;

		case POWER_INTERFACE_STARTUP_WRITE:

			ret = copy_from_user(&local_value,  (unsigned int *)ioctl_param, sizeof(local_value));

			if (ret < 0) {
				printk ("copy_from_uer Error (%d)\n", ret);
				return -1; 
			}

			//printk ("phsysical_mem_p: %x|%x\n", *p_mem_addr, local_value);
			*p_mem_addr = local_value;
			break;
			
		case POWER_INTERFACE_STARTUP_READ:

			local_value = *p_mem_addr;
			
			ret = copy_to_user((unsigned int*)ioctl_param, &local_value, sizeof(local_value));

			if (ret != 0) {
				printk ("copy_to_user Error (%d)\n", local_value);
				return -1; 
			}

			break;

		case POWER_INTERFACE_STARTUP_READ_R1:

			local_value = *p_mem_addr_r1;
			
			ret = copy_to_user((unsigned int*)ioctl_param, &local_value, sizeof(local_value));

			if (ret != 0) {
				printk ("copy_to_user Error (%d)\n", local_value);
				return -1; 
			}

			break;

		default:
			break;
	}

	return 0;
}

static int __init power_interface_init(void)
{
	if(misc_register(&power_interface_misc_dev)) {
		printk(KERN_ERR MODNAME ": can't register miscdev\n");
		return -1;
	}

	if (! physical_mem_p) {
		
		if (! request_mem_region (PHYSICAL_MEMORY_ADDR, MEMORY_SIZE, "hsmisc")) {
			printk ("request_mem_region failed\n");
			return -1;
		}	
		
		physical_mem_p = ioremap_nocache(PHYSICAL_MEMORY_ADDR, MEMORY_SIZE);
		
		if (! physical_mem_p) {
			printk(KERN_INFO "ioremap_nocache Error\n");
			return -1;
		}
	}

    	printk(KERN_INFO "hsmisc driver loaded successfully\n");
    	return 0;
}
module_init(power_interface_init);

static void __exit power_interface_exit(void)
{
	if (physical_mem_p != NULL) {

		release_mem_region (PHYSICAL_MEMORY_ADDR, MEMORY_SIZE);
		iounmap(physical_mem_p);
	}
	
    misc_deregister(&power_interface_misc_dev);
}
module_exit(power_interface_exit);

MODULE_LICENSE("GPL");
