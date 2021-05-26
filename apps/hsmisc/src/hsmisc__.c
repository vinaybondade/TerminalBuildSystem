

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>	/* printk() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/ioctl.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/reboot.h>
#include "power-interface.h"

#define MODNAME "hsmisc"

//////////////////////////////////
/* Local function declarations. */
//////////////////////////////////

static int power_interface_open(struct inode *inode, struct file *file);
static int power_interface_release(struct inode *inode, struct file *filp);
static long power_interface_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param);

static struct file_operations power_interface_file_ops = {
	.owner   			= THIS_MODULE,
	.open    			= power_interface_open,
	.unlocked_ioctl	 		= power_interface_ioctl,
	.release 			= power_interface_release
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
	switch(ioctl_num)
	{
		case POWER_INTERFACE_REBOOT:

            		// Received Reboot command.
            		//machine_restart(NULL);//LINUX_REBOOT_CMD_RESTART);
			//kernel_restart(NULL);//LINUX_REBOOT_CMD_RESTART);
			emergency_restart();//LINUX_REBOOT_CMD_RESTART);

			printk("In Reboot IOCTL.\n");
			break;

		case POWER_INTERFACE_SHUTDOWN:
           	 	// Shutdown command received. Call apropriate function here.
           	 	break;

		case POWER_INTERFACE_SHUTDOWN:
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

    printk(KERN_INFO "Power Interface driver loaded successfully.\n");
    return 0;
}
module_init(power_interface_init);

static void __exit power_interface_exit(void)
{
    misc_deregister(&power_interface_misc_dev);
}
module_exit(power_interface_exit);

MODULE_LICENSE("GPL");
