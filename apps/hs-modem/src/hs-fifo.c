/* ----------------------------
 *           includes
 * ----------------------------
 */
#include <linux/kernel.h>	// Contains types, macros, functions for the kernel
#include <linux/init.h>		// Macros used to mark up functions e.g. __init __exit
#include <linux/module.h>	// Core header for loading LKMs into the kernel
#include <linux/device.h>	// Header to support the kernel Driver Model
#include <linux/fs.h>		// Header for the Linux file system support
#include <linux/uaccess.h>	// Required for the copy to user function
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/param.h>
#include <linux/moduleparam.h>
#include <linux/string.h>
#include <linux/types.h>

#include <linux/wait.h>
#include <linux/jiffies.h>
#include <linux/poll.h>
#include <linux/mutex.h>
#include <linux/kfifo.h>

#include "hs-fifo.h"
#include "air_mode.h"
/* ----------------------------------------------
 * module command-line arguments and information
 * ----------------------------------------------
 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amit Ginat");
MODULE_DESCRIPTION("Hisky fifo driver");
MODULE_VERSION("0.1");

/* ----------------------------
 *       driver parameters
 * ----------------------------
 */
#define DEVICE_NAME "hsfifo"
#define DRIVER_NAME "hsfifo"
#define CLASS_NAME  "hsfifo"

/* ----------------------------
 *            types
 * ----------------------------
 */

#define HSFIFO_REMMSG(devt)		_IOR(MINOR(devt), 0, size_t*)

enum
{
	HSFIFO_FROM_USER_1,
	HSFIFO_FROM_USER_2,
	HSFIFO_FROM_USER_3,
	HSFIFO_FROM_USER_4,
	HSFIFO_TO_USER,
	HSFIFO_TABLE,
	__END_HSFIFO_PURPOSE
} hsFifo_propose_t;

#define HSFIFO_MAX_ELEMENTS	64
struct hsFifo_t
{
	DECLARE_KFIFO(fifo, struct hs_user_msg_t *, HSFIFO_MAX_ELEMENTS);
	struct mutex mtx;
	wait_queue_head_t wait;
};

struct hsFifoDev_t
{
	/* Char Devices */
	struct device *device; /* device associated with char_device */
	dev_t devt; /* char device number */
	struct cdev char_device[__END_HSFIFO_PURPOSE];

	struct hsFifo_t hsFifo[__END_HSFIFO_PURPOSE];
};

/* ----------------------------
 *        Declarations
 * ----------------------------
 */
static int     hsFifo_open(struct inode *, struct file *);
static int     hsFifo_release(struct inode *, struct file *);
static ssize_t hsFifo_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t hsFifo_write(struct file *, const char __user *, size_t, loff_t *);
static unsigned int hsFifo_poll(struct file *file, poll_table *wait);
static long hsFifo_ioctl(struct file *f,
							unsigned int cmd,
							unsigned long arg);

static int hsFifo_free_fifo(struct hsFifo_t *hsFifo);
/* ----------------------------
 *           globals
 * ----------------------------
 */
struct hsFifoDev_t *hsFifoDev;

static struct class* gDeviceClass  = NULL;

static const uint32_t max_minor = __END_HSFIFO_PURPOSE;

/* ----------------------------
 *    device I/O and types
 * ----------------------------
 */
static struct file_operations fops =
{
	.owner			= THIS_MODULE,
	.open			= hsFifo_open,
	.read			= hsFifo_read,
	.write			= hsFifo_write,
	.release		= hsFifo_release,
	.poll			= hsFifo_poll,
	.unlocked_ioctl	= hsFifo_ioctl,
};

/* ----------------------------------------------------
 *        implementation module base function
 * ----------------------------------------------------
 */
static int __init hsFifo_init(void)
{
	int err;
	int i;
	dev_t curr_dev;

	/* Allocate driver structure */
	hsFifoDev = kzalloc(sizeof(*hsFifoDev), GFP_KERNEL);
	if(!hsFifoDev)
	{/* Out of memory */
		return -ENOMEM;
	}

	/* Register the device class */
	gDeviceClass = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(gDeviceClass)) // Check for error and clean up if there is
	{
		printk(KERN_ERR "Failed to register device class\n");
		err = PTR_ERR(gDeviceClass);
		goto err_alloc;
	}

	/* Initialize hsFifo */
	for (i = 0; i < max_minor; ++i)
	{
		INIT_KFIFO(hsFifoDev->hsFifo[i].fifo);
		mutex_init(&hsFifoDev->hsFifo[i].mtx);
		init_waitqueue_head(&hsFifoDev->hsFifo[i].wait);
	}

	/* ----------------------------
	 *      init char device
	 * ----------------------------
	 */
	/* allocate device number */
	err = alloc_chrdev_region(&hsFifoDev->devt, 0, max_minor, DRIVER_NAME);
	if (err < 0) {
		goto err_class;
	}
	printk(KERN_INFO "allocated device number major %i minor %i\n",
			MAJOR(hsFifoDev->devt), MINOR(hsFifoDev->devt));

	for (i = 0; i < max_minor; ++i)
	{
		curr_dev = MKDEV(MAJOR(hsFifoDev->devt), MINOR(hsFifoDev->devt) + i);

		/* create driver file */
		hsFifoDev->device = device_create(gDeviceClass, NULL, curr_dev, NULL,
							"%s_%d", DRIVER_NAME, i);
		if (!hsFifoDev->device)
		{
			while((--i) >= 0)
			{
				//printk(KERN_ERR "Remove minor %d\n",
					//MINOR(hsFifoDev->devt) +i);

				curr_dev = MKDEV(MAJOR(hsFifoDev->devt),
								MINOR(hsFifoDev->devt) +i);
				cdev_del(&hsFifoDev->char_device[i]);
				device_destroy(gDeviceClass, curr_dev);
				goto err_chrdev_region;
			}
		}
		//printk(KERN_INFO "Try add minor %d\n", MINOR(hsFifoDev->devt) +i);
		//dev_set_drvdata(hsFifoDev->device, hsFifoDev);
		/* create character device */
		cdev_init(&(hsFifoDev->char_device[i]), &fops);
		err = cdev_add(&hsFifoDev->char_device[i], curr_dev, 1);
		if (err < 0)
		{
			//printk(KERN_ERR "Remove dev minor %d\n", MINOR(hsFifoDev->devt) +i);
			device_destroy(gDeviceClass, curr_dev);
			printk(KERN_ERR "couldn't create character device\n");
			while((--i) >= 0)
			{
				//printk(KERN_ERR "Remove minor %d\n",
					//MINOR(hsFifoDev->devt) +i);
				curr_dev = MKDEV(MAJOR(hsFifoDev->devt),
								MINOR(hsFifoDev->devt) +i);
				cdev_del(&hsFifoDev->char_device[i]);
				device_destroy(gDeviceClass, curr_dev);
				goto err_chrdev_region;
			}
		}
		//printk(KERN_INFO "Added minor %d\n", MINOR(hsFifoDev->devt) +i);
	}

	return 0;

err_chrdev_region:
	unregister_chrdev_region(hsFifoDev->devt, max_minor);
err_class:
	class_destroy(gDeviceClass);
err_alloc:
	kfree(hsFifoDev);
	hsFifoDev = NULL;
	return err;
}

static void __exit hsFifo_exit(void)
{
	int i;
	dev_t curr_dev;

	for (i = 0; i < max_minor; ++i)
	{
		//printk(KERN_INFO "Remove minor %d\n", MINOR(hsFifoDev->devt) +i);
		curr_dev = MKDEV(MAJOR(hsFifoDev->devt), MINOR(hsFifoDev->devt) +i);
		cdev_del(&hsFifoDev->char_device[i]);
		device_destroy(gDeviceClass, curr_dev);
	}
	unregister_chrdev_region(hsFifoDev->devt, max_minor);
	for (i = 0; i < max_minor; ++i)
	{
		//printk(KERN_INFO "Remove kfifo %d\n", i);
		hsFifo_free_fifo(&hsFifoDev->hsFifo[i]);
	}
	class_destroy(gDeviceClass);
	kfree(hsFifoDev);
	hsFifoDev = NULL;
}

module_init(hsFifo_init);
module_exit(hsFifo_exit);

static int hsFifo_open(struct inode *inod, struct file *f)
{
	//struct hsFifoDev_t *hsFifoDev = (struct hsFifoDev_t *)container_of(inod->i_cdev,
	//				struct hsFifoDev_t, char_device);

	unsigned i = iminor(inod);

	//printk(KERN_INFO "Open minor %u\n", i);

	if((i < HSFIFO_TO_USER) && ((f->f_flags & O_ACCMODE) != O_WRONLY))
	{
		return -EPERM;
	}
	else if ((i >= HSFIFO_TO_USER) && ((f->f_flags & O_ACCMODE) != O_RDONLY))
	{
		return -EPERM;
	}

	f->private_data = &hsFifoDev->hsFifo[i];

	return 0;
}

static int hsFifo_release(struct inode *inod, struct file *f)
{
	//struct hsFifo_t *hsFifo = (struct hsFifo_t *)container_of(inod->i_cdev,
	//				struct hsFifo_t, char_device);

	unsigned i = iminor(inod);
	struct hsFifo_t *hsFifo;

	//printk(KERN_INFO "Release minor %u\n", i);

	hsFifo = &hsFifoDev->hsFifo[i];

	if(mutex_is_locked(&hsFifo->mtx))
	{
		mutex_unlock(&hsFifo->mtx);
	}

	f->private_data = NULL;

	return 0;
}

static unsigned int hsFifo_poll(struct file *f, poll_table *wait)
{
	unsigned int mask = 0;
	struct hsFifo_t *hsFifo = (struct hsFifo_t *)f->private_data;

	poll_wait(f, &hsFifo->wait, wait);

	if ((f->f_flags & O_ACCMODE) == O_RDONLY)
	{
		if(!kfifo_is_empty(&hsFifo->fifo)) {
			mask |= POLLIN | POLLRDNORM;
		}
	}
	else if ((f->f_flags & O_ACCMODE) == O_WRONLY)
	{
		if(!kfifo_is_full(&hsFifo->fifo)) {
			mask |= POLLOUT;
		}
	}

	return mask;
}

static ssize_t hsFifo_read(struct file *f,
							char __user *buf,
							size_t count,
							loff_t *offset)
{
	int ret;
	struct hsFifo_t *hsFifo = (struct hsFifo_t *)f->private_data;
	struct hs_user_msg_t *msg;
	const uint32_t headerSize = sizeof(msg->type) +
								sizeof(msg->size) +
								sizeof(msg->timestamp);

	if (f->f_flags & O_NONBLOCK)
	{
		if(!mutex_trylock(&hsFifo->mtx)) {
			return -EAGAIN;
		}
	}
	else
	{
		ret = mutex_lock_interruptible(&hsFifo->mtx);
		if(ret) {
			return ret;
		}

		ret = wait_event_interruptible(hsFifo->wait,
				!kfifo_is_empty(&hsFifo->fifo));
		if (ret) {
			return ret;
		}

	}

	/* Check if msg too long or fifo empty */
	if(!kfifo_peek(&hsFifo->fifo, &msg))
	{
		mutex_unlock(&hsFifo->mtx);
		return 0;
	}

	if(count < (msg->size + headerSize))
	{
		mutex_unlock(&hsFifo->mtx);
		return -EMSGSIZE;
	}

	/* Copy the msg to the user */
	if(!kfifo_get(&hsFifo->fifo, &msg))
	{
		ret = -EFAULT;
	}
	else if(copy_to_user(buf, msg, headerSize))
	{
		ret = -EFAULT;
	}
	else if(copy_to_user(buf+headerSize, msg->buf, msg->size))
	{
		ret = -EFAULT;
	}
	else
	{
		ret = msg->size + headerSize;
	}

	kfree(msg);
	mutex_unlock(&hsFifo->mtx);
	return ret;
}

static ssize_t hsFifo_write(struct file *f,
							const char __user *buf,
							size_t count,
							loff_t *offset)
{
	int ret;
	struct hsFifo_t *hsFifo = (struct hsFifo_t *)f->private_data;
	struct hs_user_msg_t userMsg;
	const uint32_t headerSize = sizeof(userMsg.type) +
								sizeof(userMsg.size) +
								sizeof(userMsg.timestamp);
	struct hs_user_msg_t *allocMsg = NULL;

	if(count < headerSize) {
		return -EINVAL;
	}

	if (f->f_flags & O_NONBLOCK)
	{
		if(!mutex_trylock(&hsFifo->mtx)) {
			return -EAGAIN;
		}
	}
	else
	{
		ret = mutex_lock_interruptible(&hsFifo->mtx);
		if(ret) {
			return ret;
		}

		ret = wait_event_interruptible(hsFifo->wait,
				!kfifo_is_empty(&hsFifo->fifo));
		if (ret) {
			return ret;
		}
	}

	if(!kfifo_avail(&hsFifo->fifo))
	{
		ret = -EAGAIN;
		goto err_wr_hsFifo;
	}

	if(copy_from_user(&userMsg, buf, headerSize))
	{
		ret = -EFAULT;
		goto err_wr_hsFifo;
	}

	if(userMsg.size + headerSize != count)
	{
		ret = -EPROTO;
		goto err_wr_hsFifo;
	}

	allocMsg = kmalloc(count, GFP_KERNEL);
	if(!allocMsg)
	{
		ret = -ENOMEM;
		goto err_wr_hsFifo;
	}
	//printk(KERN_DEBUG "write: allocMsg '%p'\n", allocMsg);
	if(copy_from_user(allocMsg, buf, count))
	{
		kfree(allocMsg);
		ret = -EFAULT;
		goto err_wr_hsFifo;
	}

	if(!kfifo_put(&hsFifo->fifo, allocMsg))
	{
		ret = -EFAULT;
		goto err_wr_hsFifo;
	}

	ret = count;

err_wr_hsFifo:
	mutex_unlock(&hsFifo->mtx);
	return ret;
}

static long hsFifo_ioctl(struct file *f,
							unsigned int cmd,
							unsigned long arg)
{
	long err = 0;
	unsigned i = iminor(f->f_inode);
	struct hsFifo_t *hsFifo = (struct hsFifo_t *)f->private_data;
	dev_t curr_dev = MKDEV(MAJOR(hsFifoDev->devt), MINOR(hsFifoDev->devt) +i);
	size_t rem_elements = 0;

	if( cmd == HSFIFO_REMMSG(curr_dev)) {
		rem_elements = kfifo_avail(&hsFifo->fifo);
		if(copy_to_user((size_t *)arg, &rem_elements,
			sizeof(rem_elements)))
		{/* Bad address */
			return -EFAULT;
		}
	} else {/* Invalid argument */
		return -EINVAL;
	}

	return err;
}

static int hsFifo_free_fifo(struct hsFifo_t *hsFifo)
{
	int i;
	struct hs_user_msg_t *msg;
	const int numOfElem = kfifo_len(&hsFifo->fifo);

	for (i = 0; i < numOfElem; ++i)
	{
		if(!kfifo_get(&hsFifo->fifo, &msg))
		{
			return -EFAULT;
		}
		kfree(msg);
	}

	return 0;
}

int hsFifo_done(struct hs_user_msg_t **msg)
{
	if(!msg)
	{
		return -EINVAL;
	}
	if(!*msg)
	{
		return -EINVAL;
	}
	kfree(*msg);
	*msg = NULL;
	return 0;
}
EXPORT_SYMBOL_GPL(hsFifo_done);

int hsFifo_get_user_msg(struct hs_user_msg_t **msg)
{
	int i;
	struct hsFifo_t *hsFifo;

	for (i = 0; i < HSFIFO_TO_USER; ++i)
	{
		hsFifo = &hsFifoDev->hsFifo[i];

		if(kfifo_len(&hsFifo->fifo))
		{
			if(!kfifo_get(&hsFifo->fifo, msg))
			{
				return -EFAULT;
			}

			wake_up_interruptible(&hsFifo->wait);
			return 1;
		}
	}
	return 0;
}
EXPORT_SYMBOL_GPL(hsFifo_get_user_msg);

int hsFifo_put_user_msg(size_t type,
						uint32_t timestamp,
						uint8_t *buf,
						size_t size)
{
	struct hs_user_msg_t *allocMsg;
	const uint32_t headerSize = sizeof(allocMsg->type) +
								sizeof(allocMsg->size) +
								sizeof(allocMsg->timestamp);

	struct hsFifo_t *hsFifo;

	if(type == SAT_MSG_ID_TABLE) {
		hsFifo = &hsFifoDev->hsFifo[HSFIFO_TABLE];
	}
	else {
		hsFifo = &hsFifoDev->hsFifo[HSFIFO_TO_USER];
	}
	//printk(KERN_DEBUG "type 0x%x\n", type);
	//printk(KERN_DEBUG "size %d\n", size);
	//if(type == SAT_MSG_ID_DBG_STR)
	//{
	//	printk(KERN_DEBUG "buf '%s'\n", buf);
	//}

	if(!kfifo_avail(&hsFifo->fifo))
	{
		return -EAGAIN;
	}

	allocMsg = kmalloc(headerSize + size, GFP_KERNEL);
	if(!allocMsg)
	{
		return -ENOMEM;
	}
	allocMsg->type = type;
	allocMsg->size = size;
	allocMsg->timestamp = timestamp;
	memcpy(allocMsg->buf, buf, size);

	if(!kfifo_put(&hsFifo->fifo, allocMsg))
	{
		kfree(allocMsg);
		return -EFAULT;
	}

	wake_up_interruptible(&hsFifo->wait);
	return size;
}
EXPORT_SYMBOL_GPL(hsFifo_put_user_msg);
