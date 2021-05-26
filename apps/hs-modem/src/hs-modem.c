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
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/param.h>
#include <linux/interrupt.h>
#include <linux/moduleparam.h>
#include <linux/proc_fs.h>

#include <linux/string.h>
#include <linux/types.h>

#include "hs-modem.h"
#include "hs_modem_hw.h"
#include "hs_modem_ioctl.h"
#include "hs_modem_dbg.h"
#include "generalDef.h"
#include "sysfs_macro.h"
#include "air_interface.h"

/* ----------------------------------------------
 * module command-line arguments and information
 * ----------------------------------------------
 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amit Ginat");
MODULE_DESCRIPTION("Hisky modem driver");
MODULE_VERSION("0.1");
/* ----------------------------
 *       driver parameters
 * ----------------------------
 */
#define DEVICE_NAME "hsModem"
#define DRIVER_NAME "hsModem"
#define CLASS_NAME  "hsmodem"

/* ----------------------------
 *        Declarations
 * ----------------------------
 */
static int     hsModem_open(struct inode *, struct file *);
static int     hsModem_release(struct inode *, struct file *);
static ssize_t hsModem_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t hsModem_write(struct file *, const char __user *, size_t, loff_t *);
static long hsModem_ioctl(struct file *, unsigned int, unsigned long);

static int hsModem_probe(struct platform_device *pdev);
static int hsModem_remove(struct platform_device *pdev);
static void hsModem_rst(struct hsModem_t* hsModem,
						modemSubPhy_t modemSubPhy,
						int isRst);
static irqreturn_t hsModem_irq_rx_handler(int irq, void *arg);
static irqreturn_t hsModem_irq_tx_handler(int irq, void *arg);

static int hsModem_handle_rpc_intr(struct hsModem_t* hsModem);
static int hsModem_read_pck(struct hsModem_t* hsModem);

static int hsModem_handle_tpr_intr(struct hsModem_t* hsModem);
static int hsModem_write_pck(struct hsModem_t* hsModem);
static int trig_tx(struct hsModem_t *hsModem);

static void hsModem_conf_rx_rate(struct hsModem_t *hsModem);
static void hsModem_conf_tx_rate(struct hsModem_t *hsModem);

static uint32_t get_rx_timestamp(struct hsModem_t *hsModem);
static void set_tx_timestamp(struct hsModem_t *hsModem, uint32_t t);
static void hsModem_last_tx(struct hsModem_t *hsModem);
static void conv_ebn0(struct ebn0_t* ebn0, uint32_t regVal);
static uint32_t ts_to_ticks(uint32_t ts);
static void select_rx_fifo(struct hsModem_t *hsModem);
static void select_tx_fifo(struct hsModem_t *hsModem);
/* ----------------------------
 *           globals
 * ----------------------------
 */
static const uint32_t HW_SUPPORT_VER = 0x1;
static const uint32_t HW_SUPPORT_REV = 0x0;

static uint32_t gNumberOpens = 0;
static uint32_t gNumOfPhys = 0;
static struct class *gDeviceClass = NULL;
static struct proc_dir_entry *procfs_ent = NULL;

airProtCtx_t* apctx[MAX_SAT_TYPE_HEADER] = {NULL};

static const uint32_t max_minor = 1;

static bool constDataFlag = false;
/* ----------------------------
 *    device I/O and types
 * ----------------------------
 */
static struct file_operations fops =
{
	.owner			= THIS_MODULE,
	.open			= hsModem_open,
	//.read			= hsModem_read,
	//.write			= hsModem_write,
	.release		= hsModem_release,
	.unlocked_ioctl	= hsModem_ioctl,
};

static const struct of_device_id hsModem_of_match[] = {
	{ .compatible = "hisky,hs-modem-1.0", },
	{},
};
MODULE_DEVICE_TABLE(of, hsModem_of_match);

static struct platform_driver hsModem_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table	= hsModem_of_match,
	},
	.probe		= hsModem_probe,
	.remove		= hsModem_remove,
};

/* ----------------------------
 *    Register sysfs entries
 * ----------------------------
 */
static ssize_t sysfs_reg_write(struct device *dev, const char *buf,
			   size_t count, uint32_t regOffset)
{
	int ret;
	uint32_t regVal;
	hsModem_t *hsModem = dev_get_drvdata(dev);

	ret = kstrtou32(buf, 0, &regVal);
	if (ret < 0) {
		return ret;
	}

	writeReg(hsModem->baseAddr, regOffset, regVal);

	return count;
}

static ssize_t sysfs_reg_read(struct device *dev, char *buf, uint32_t regOffset)
{
	uint32_t regVal;
	char tmpBuf[32] = {0};
	size_t len;
	hsModem_t *hsModem = dev_get_drvdata(dev);

	regVal = readReg(hsModem->baseAddr, regOffset);

	/* print the register value */
	len = snprintf(tmpBuf, sizeof(tmpBuf), "0x%x\n", regVal);
	memcpy(buf, tmpBuf, len);

	return len;
}

DEFINE_RW_REG_ATTR_AND_FUNC(ier, HSMDM_REG_IER_OFFSET);
DEFINE_RO_REG_ATTR_AND_FUNC(ebn0, HSMDM_REG_EBN0_OFFSET);
DEFINE_RO_REG_ATTR_AND_FUNC(freq_est, HSMDM_REG_FREQ_EST_OFFSET);
DEFINE_RW_REG_ATTR_AND_FUNC(eng_thrsh, HSMDM_REG_ENG_THRSH_OFFSET);
DEFINE_RW_REG_ATTR_AND_FUNC(rel_thrsh, HSMDM_REG_REL_THRSH_OFFSET);
DEFINE_RO_REG_ATTR_AND_FUNC(crc_ok, HSMDM_REG_RX_CRC_OK_OFFSET);
DEFINE_RW_REG_ATTR_AND_FUNC(rst, HSMDM_REG_RST_OFFSET);
DEFINE_RW_REG_ATTR_AND_FUNC(txInt_earlyOffset,
							HSMDM_REG_TXINT_EARLYOFFSET_OFFSET);
DEFINE_RW_REG_ATTR_AND_FUNC(rxInt_timeoutMax,
							HSMDM_REG_RXINT_TIMEOUTMAX_OFFSET);
DEFINE_RW_REG_ATTR_AND_FUNC(rxInt_timeoutPeriod,
							HSMDM_REG_RXINT_TIMEOUTPERIOD_OFFSET);
DEFINE_RW_REG_ATTR_AND_FUNC(rxInt_timeTolerance,
							HSMDM_REG_RXINT_TIMETOLERANCE_OFFSET);
DEFINE_WO_REG_ATTR_AND_FUNC(rx_irq_ctrl, HSMDM_REG_RX_IRQ_CTRL_OFFSET);
DEFINE_WO_REG_ATTR_AND_FUNC(early_pa_on, HSMDM_REG_EARLY_PA_ON_OFFSET);
DEFINE_WO_REG_ATTR_AND_FUNC(pa_off_delay, HSMDM_REG_PA_OFF_DELAY_OFFSET);
DEFINE_RW_REG_ATTR_AND_FUNC(tx_last_pck, HSMDM_REG_TDD_MODE_OFFSET);
DEFINE_RO_REG_ATTR_AND_FUNC(hw_version, HSMDM_REG_VERSION_OFFSET);
DEFINE_RO_REG_ATTR_AND_FUNC(hw_revision, HSMDM_REG_REVISION_OFFSET);

static struct attribute *hsModem_attrs_reg[] = {
	&DEV_ATTR(ier).attr,
	&DEV_ATTR(ebn0).attr,
	&DEV_ATTR(freq_est).attr,
	&DEV_ATTR(eng_thrsh).attr,
	&DEV_ATTR(rel_thrsh).attr,
	&DEV_ATTR(crc_ok).attr,
	&DEV_ATTR(rst).attr,
	&DEV_ATTR(txInt_earlyOffset).attr,
	&DEV_ATTR(rxInt_timeoutMax).attr,
	&DEV_ATTR(rxInt_timeoutPeriod).attr,
	&DEV_ATTR(rxInt_timeTolerance).attr,
	&DEV_ATTR(rx_irq_ctrl).attr,
	&DEV_ATTR(early_pa_on).attr,
	&DEV_ATTR(pa_off_delay).attr,
	&DEV_ATTR(tx_last_pck).attr,
	&DEV_ATTR(hw_version).attr,
	&DEV_ATTR(hw_revision).attr,
	NULL,
};

/* ----------------------------
 *   Statistics sysfs entries
 * ----------------------------
 */
static ssize_t sysfs_stat_write(struct device *dev, const char *buf,
			   size_t count, modem_sysfs_stat_t stat)
{
	hsModem_t *hsModem = dev_get_drvdata(dev);

	switch(stat)
	{
	case SYSFS_STAT_EMPTY:
		memset(&hsModem->stat, 0, sizeof(hsModem->stat));
		break;
	case SYSFS_STAT_TX_IRQ:
		hsModem->stat.txIrqCnt = 0;
		break;
	case SYSFS_STAT_RX_TOTAL_PCK:
		hsModem->stat.rxCrcErrCnt = 0;
		hsModem->stat.rxGoodPckCnt = 0;
		break;
	case SYSFS_STAT_RX_TIMESLOT:
		hsModem->stat.rxTsCnt = 0;
		break;
	default:
		return -EINVAL;
		break;
	}
	return count;
}

static ssize_t sysfs_stat_read(struct device *dev, char *buf,
				modem_sysfs_stat_t stat)
{
	hsModem_t *hsModem = dev_get_drvdata(dev);
	char tmpBuf[32] = {0};
	size_t len;
	uint32_t val;

	switch(stat)
	{
	case SYSFS_STAT_TX_IRQ:
		val = hsModem->stat.txIrqCnt;
		break;
	case SYSFS_STAT_RX_CRC_ERR:
		val = hsModem->stat.rxCrcErrCnt;
		break;
	case SYSFS_STAT_RX_GOOD_PCK:
		val = hsModem->stat.rxGoodPckCnt;
		break;
	case SYSFS_STAT_RX_TOTAL_PCK:
		val = hsModem->stat.rxCrcErrCnt + hsModem->stat.rxGoodPckCnt;
		break;
	case SYSFS_STAT_RX_TIMESLOT:
		val = hsModem->stat.rxTsCnt;
		break;
	case SYSFS_STAT_RX_PCK_DROPPED:
		val = hsModem->stat.apStat.rxMsgDropped;
		break;
	default:
		return -EINVAL;
		break;
	}

	len = snprintf(tmpBuf, sizeof(tmpBuf), "0x%x\n", val);
	memcpy(buf, tmpBuf, len);

	return len;
}

DEFINE_RW_STAT_ATTR_AND_FUNC(tx_irq, SYSFS_STAT_TX_IRQ);
DEFINE_RW_STAT_ATTR_AND_FUNC(rx_total_pck, SYSFS_STAT_RX_TOTAL_PCK);
DEFINE_RW_STAT_ATTR_AND_FUNC(rx_timeslot, SYSFS_STAT_RX_TIMESLOT);
DEFINE_WO_STAT_ATTR_AND_FUNC(empty, SYSFS_STAT_EMPTY);
DEFINE_RO_STAT_ATTR_AND_FUNC(rx_crc_err, SYSFS_STAT_RX_CRC_ERR);
DEFINE_RO_STAT_ATTR_AND_FUNC(rx_good_pck, SYSFS_STAT_RX_GOOD_PCK);
DEFINE_RO_STAT_ATTR_AND_FUNC(rx_msg_dropped, SYSFS_STAT_RX_PCK_DROPPED);

static struct attribute *hsModem_attrs_stat[] = {
	&DEV_ATTR(empty).attr,
	&DEV_ATTR(tx_irq).attr,
	&DEV_ATTR(rx_crc_err).attr,
	&DEV_ATTR(rx_good_pck).attr,
	&DEV_ATTR(rx_total_pck).attr,
	&DEV_ATTR(rx_timeslot).attr,
	&DEV_ATTR(rx_msg_dropped).attr,
	NULL,
};

/* ----------------------------
 *     Conf sysfs entries
 * ----------------------------
 */
static ssize_t air_mode_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	hsModem_t *hsModem = dev_get_drvdata(dev);
	int err = 0;
	uint32_t airMode;

	err = kstrtou32(buf, 0, &airMode);
	if (err < 0) {
		return err;
	}
	if(airMode >= __SATHEADERTYPES_END) {
		return -EINVAL;
	}
	destroy_air_protocol(&hsModem->ctx);
	err = init_air_protocol(&hsModem->ctx,
							(satHeaderTypes_t)airMode,
							hsModem->modem_tx_param.tx_frame_bytes,
							hsModem->modem_rx_param.rx_frame_bytes);
	if(err)
	{
		destroy_air_protocol(&hsModem->ctx);
		return err;
	}
	hsModem_dbg(1, "init_air_protocol with %s\n",
				airMode == LOW_RATE_TYPE_HEADER ? "low rate" :
				airMode == HIGH_RATE_TYPE_HEADER ? "high rate" :
				"error!!!");
	return count;
}
static DEVICE_ATTR_WO(air_mode);

static ssize_t calib_rx_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	hsModem_t *hsModem = dev_get_drvdata(dev);

	/* write RX rate to modem */
	hsModem_dbg(1, "Setting RX rate\n");
	hsModem_conf_rx_rate(hsModem);

	return count;
}
static DEVICE_ATTR_WO(calib_rx);

static ssize_t calib_tx_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	hsModem_t *hsModem = dev_get_drvdata(dev);

	/* write RX rate to modem */
	hsModem_dbg(1, "Setting TX rate\n");
	hsModem_conf_tx_rate(hsModem);

	return count;
}
static DEVICE_ATTR_WO(calib_tx);

static ssize_t manual_crc_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	int ret;
	uint32_t val;
	hsModem_t *hsModem = dev_get_drvdata(dev);

	ret = kstrtou32(buf, 0, &val);
	if (ret < 0) {
		return ret;
	}
	if(val) {
		hsModem->driver_rx_flag |= HSMDM_CALC_CRC;
	} else {
		hsModem->driver_rx_flag &= (~HSMDM_CALC_CRC);
	}

	hsModem_dbg(1, "driver_rx_flag = 0x%x\n", hsModem->driver_rx_flag);

	return count;
}
static DEVICE_ATTR_WO(manual_crc);

static ssize_t trig_tx_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	int err;
	//uint32_t val;
	hsModem_t *hsModem = dev_get_drvdata(dev);

	err = trig_tx(hsModem);
	if(err < 0) {
		return err;
	}

	/* call to transmit packet request to trigger TX path */
	err = hsModem_handle_tpr_intr(hsModem);
	if(err < 0) {
		return err;
	}

	return count;
}
static DEVICE_ATTR_WO(trig_tx);

static ssize_t rst_rx_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	int ret;
	uint32_t val;
	hsModem_t *hsModem = dev_get_drvdata(dev);

	ret = kstrtou32(buf, 0, &val);
	if (ret < 0) {
		return ret;
	}
	if(val) {
		hsModem_rst(hsModem, MODEM_RX, 1);
	} else {
		hsModem_rst(hsModem, MODEM_RX, 0);
	}

	return count;
}
static DEVICE_ATTR_WO(rst_rx);

static ssize_t rst_tx_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	int ret;
	uint32_t val;
	hsModem_t *hsModem = dev_get_drvdata(dev);

	ret = kstrtou32(buf, 0, &val);
	if (ret < 0) {
		return ret;
	}
	if(val) {
		hsModem_rst(hsModem, MODEM_TX, 1);
	} else {
		hsModem_rst(hsModem, MODEM_TX, 0);
	}

	return count;
}
static DEVICE_ATTR_WO(rst_tx);

static ssize_t mt_id_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	int ret;
	uint32_t val;
	hsModem_t *hsModem = dev_get_drvdata(dev);

	ret = kstrtou32(buf, 0, &val);
	if (ret < 0) {
		return ret;
	}

	if(val > MAX_MT_ID_VALID) {
		return -EOVERFLOW;
	}

	disable_irq(hsModem->irq_tx);
	disable_irq(hsModem->irq_rx);

	hsModem->ctx.mtID = val;

	enable_irq(hsModem->irq_tx);
	enable_irq(hsModem->irq_rx);

	return count;
}

static ssize_t mt_id_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	char tmpBuf[32] = {0};
	ssize_t len;
	hsModem_t *hsModem = dev_get_drvdata(dev);

	len = snprintf(tmpBuf, sizeof(tmpBuf), "0x%x\n", hsModem->ctx.mtID);
	memcpy(buf, tmpBuf, len);

	return len;
}
static DEVICE_ATTR_RW(mt_id);

static struct attribute *hsModem_attrs_conf[] = {
	&DEV_ATTR(air_mode).attr,
	&DEV_ATTR(calib_rx).attr,
	&DEV_ATTR(calib_tx).attr,
	&DEV_ATTR(manual_crc).attr,
	&DEV_ATTR(trig_tx).attr,
	&DEV_ATTR(rst_rx).attr,
	&DEV_ATTR(rst_tx).attr,
	&DEV_ATTR(mt_id).attr,
	NULL,
};

/* ----------------------------
 *     Debug sysfs entries
 * ----------------------------
 */
static ssize_t dbg_lvl_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	int ret;
	uint32_t val;
	//hsModem_t *hsModem = dev_get_drvdata(dev);

	ret = kstrtou32(buf, 0, &val);
	if (ret < 0) {
		return ret;
	}
	hsModemDbgLvl = val;

	return count;
}
static ssize_t dbg_lvl_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	char tmpBuf[32] = {0};
	size_t len;
	//hsModem_t *hsModem = dev_get_drvdata(dev);


	len = snprintf(tmpBuf, sizeof(tmpBuf), "%u\n", hsModemDbgLvl);
	memcpy(buf, tmpBuf, len);

	return len;
}
static DEVICE_ATTR_RW(dbg_lvl);

static ssize_t print_build_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	hsModem_t *hsModem = dev_get_drvdata(dev);
	int err;
	char* outBuf;
	int i;
	const int size = hsModem->ctx.a_info.satMsgLenTx *2 +1/*LF*/+1/*NULL*/;
	/*
	 * For using debug functions, register ier must be zero for non interrupt
	 * will occur while debugging
	 */
	if(readReg(hsModem->baseAddr, HSMDM_REG_IER_OFFSET))
	{
		return -EBUSY;
	}

	outBuf = kmalloc(size, GFP_KERNEL);
	if(!outBuf)
	{
		return -ENOMEM;
	}

	err = air_msg_build(&hsModem->ctx, 0x2f); // const EbN0 for testing
	if(err < 0)
	{ // handle error
		printk(KERN_WARNING "ERROR: air_msg_build return %d\n", err);
		kfree(outBuf);
		return err;
	}

	for(i = 0; i < hsModem->ctx.a_info.satMsgLenTx-1; ++i)
	{
		sprintf(outBuf +i*2, "%02x", hsModem->ctx.satMsgTx[i]);
	}
	sprintf(outBuf +i*2, "%02x\n", hsModem->ctx.satMsgTx[i]);

	memcpy(buf, outBuf, size);
	kfree(outBuf);
	return size;
}
static DEVICE_ATTR_RO(print_build);

static ssize_t const_msg_build_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	hsModem_t *hsModem = dev_get_drvdata(dev);
	airContext_t* ctx = &hsModem->ctx;
	//int ret;
	int allocSize;

	ctx->dbg.constMsgLen = strlen(buf)-1;
	if(ctx->dbg.constMsgLen < 1)
	{
		return -EINVAL;
	}


	hsModem_dbg(8, "count = %d\n", count);
	hsModem_dbg(8, "constMsgLen = %d\n", ctx->dbg.constMsgLen);
	hsModem_dbg(8, "str = '%.*s'\n", ctx->dbg.constMsgLen, buf);

	if(ctx->dbg.constMsg)
	{
		kfree(ctx->dbg.constMsg);
		ctx->dbg.constMsg = NULL;
	}

	ctx->dbg.msgPreSize = 3; // 2 prefix digits, ':'
	hsModem_dbg(8, "msgPreSize = %d\n", ctx->dbg.msgPreSize);

	allocSize = ctx->dbg.msgPreSize + ctx->dbg.constMsgLen +1;
	ctx->dbg.constMsg = kmalloc(allocSize, GFP_KERNEL);
	if(!ctx->dbg.constMsg)
	{
		return -ENOMEM;
	}
	snprintf(ctx->dbg.constMsg, allocSize, "%u:%.*s",
			ctx->dbg.msgNum%100,
			ctx->dbg.constMsgLen, buf);
	hsModem_dbg(8, "msg = '%s'\n", ctx->dbg.constMsg);
	ctx->dbg.msgNum++;
	ctx->dbg.msgLen = strlen(ctx->dbg.constMsg);

	ctx->dbg.sendConstMsg = 1;
	return count;
}
static DEVICE_ATTR_WO(const_msg_build);

static ssize_t const_data_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	int ret;
	uint32_t val;

	ret = kstrtou32(buf, 0, &val);

	if (ret < 0) {
		return ret;
	}

	if(val) {
		constDataFlag = true;
	} else {
		constDataFlag = false;
	}

	return count;
}
static DEVICE_ATTR_WO(const_data);

static ssize_t print_parse_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	hsModem_t *hsModem = dev_get_drvdata(dev);
	int ret = 0;
	int err;

	if(hsModem->ctx.a_info.satMsgLenTx == hsModem->ctx.a_info.satMsgLenRx)
	{
		hsModem_dbg(9, "copy Tx to Rx\n");
		memcpy(hsModem->ctx.satMsgRx,
				hsModem->ctx.satMsgTx,
				hsModem->ctx.a_info.satMsgLenTx);

		err = parse_air_msg(&hsModem->ctx, &hsModem->stat.apStat);
		if(err < 0)
		{
			hsModem_dbg(1, "parse_air_msg return %d\n", err);
		}
	}

	return ret;
}
static DEVICE_ATTR_RO(print_parse);

static ssize_t iot_mss_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	int ret;
	uint32_t val;
	hsModem_t *hsModem = dev_get_drvdata(dev);

	ret = kstrtou32(buf, 0, &val);

	if (ret < 0) {
		return ret;
	}

	if(val == 1) // mss
	{
		hsModem->tdd.waitTimestamp = get_rx_timestamp(hsModem) +
										ts_to_ticks(40);
		hsModem->tdd.tsToTx = 0x7FFFFFFF;
		hsModem->ctx.dbg.tdd_self_trig = 0;
		set_tx_timestamp(hsModem, hsModem->tdd.waitTimestamp);
	}
	else if(val == 2) // iot 5 ts every 40 ts
	{
		hsModem->tdd.waitTimestamp = get_rx_timestamp(hsModem) +
										ts_to_ticks(40);
		hsModem->tdd.tsToTx = 5;
		hsModem->ctx.dbg.tdd_self_trig = hsModem->tdd.tsToTx;
		set_tx_timestamp(hsModem, hsModem->tdd.waitTimestamp);
	}
	else
	{
		hsModem->ctx.dbg.tdd_self_trig = 0;
		hsModem->tdd.tsToTx = 1;
	}

	return count;
}
static DEVICE_ATTR_WO(iot_mss);

static ssize_t mdm_reg_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	int ret;
	uint32_t val;
	uint32_t offset;
	hsModem_t *hsModem = dev_get_drvdata(dev);

	switch(buf[0])
	{
		case 'w':
		case 'W':
			ret = sscanf(buf+2, "%x %x", &offset, &val);
			if (ret != 2) {
				return -EINVAL;
			}
			writeReg(hsModem->baseAddr, offset, val);
			break;

		case 'r':
		case 'R':
			ret = sscanf(buf+2, "%x", &offset);
			if (ret != 1) {
				return -EINVAL;
			}
			val = readReg(hsModem->baseAddr, offset);
			printk(KERN_INFO "0x%x\n", val);
			break;

		default:
			return -EINVAL;
			break;

	}
	return count;
}
static DEVICE_ATTR_WO(mdm_reg);

static struct attribute *hsModem_attrs_dbg[] = {
	&DEV_ATTR(dbg_lvl).attr,
	&DEV_ATTR(print_build).attr,
	&DEV_ATTR(const_msg_build).attr,
	&DEV_ATTR(const_data).attr,
	&DEV_ATTR(print_parse).attr,
	&DEV_ATTR(iot_mss).attr,
	&DEV_ATTR(mdm_reg).attr,
	NULL,
};

/* ----------------------------
 *         sysfs groups
 * ----------------------------
 */
static const struct attribute_group hsModem_attr_group_reg = {
	.name = "ip_registers",
	.attrs = hsModem_attrs_reg,
};
static const struct attribute_group hsModem_attr_group_stat = {
	.name = "stat",
	.attrs = hsModem_attrs_stat,
};
static const struct attribute_group hsModem_attr_group_conf = {
	.name = "conf",
	.attrs = hsModem_attrs_conf,
};
static const struct attribute_group hsModem_attr_group_dbg = {
	.name = "dbg",
	.attrs = hsModem_attrs_dbg,
};
static const struct attribute_group *hsModem_attrs_groups[] = {
	&hsModem_attr_group_reg,
	&hsModem_attr_group_stat,
	&hsModem_attr_group_conf,
	&hsModem_attr_group_dbg,
	NULL,
};

/* ----------------------------
 *         procfs groups
 * ----------------------------
 */
static int statistics_show(struct seq_file *sf, void *data)
{
	struct hsModem_t *hsModem = (struct hsModem_t *)sf->private;
	struct ebn0_t ebn0;
	uint32_t val = readReg(hsModem->baseAddr, HSMDM_REG_EBN0_OFFSET);
	conv_ebn0(&ebn0, val);

	seq_printf(sf,
				"ebn0:%c%u.%02u\n"
				"txIrq:%x\n"
				"rxCrcErr:%x\n"
				"rxGoodPckCnt:%x\n"
				"rxTotalPck:%x\n"
				"rxTs:%x\n"
				"rxMsgDropped:%x\n",
				ebn0.neg ? '-':'+', ebn0.intVal, ebn0.fracVal,
				hsModem->stat.txIrqCnt,
				hsModem->stat.rxCrcErrCnt,
				hsModem->stat.rxGoodPckCnt,
				hsModem->stat.rxCrcErrCnt + hsModem->stat.rxGoodPckCnt,
				hsModem->stat.rxTsCnt,
				hsModem->stat.apStat.rxMsgDropped);

	return 0;
}

static int statistics_open(struct inode *inodep, struct file *filep)
{
	struct hsModem_t *hsModem = PDE_DATA(inodep);
	if(hsModem == NULL) {
		return -EINVAL;
	}
	filep->private_data = hsModem;


	return single_open(filep, statistics_show, PDE_DATA(inodep));

	return 0;
}

static struct file_operations procfs_stat_ops =
{
	.owner		= THIS_MODULE,
	.open		= statistics_open,
	.read		= seq_read,
	.release	= single_release,

};
/* ----------------------------------------------------
 *        implementation module base function
 * ----------------------------------------------------
 */
static int __init hsModem_init(void)
{
	print_function_begin();

	/* Register the device class */
	gDeviceClass = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(gDeviceClass)) // Check for error and clean up if there is
	{
		printk(KERN_ERR "Failed to register device class\n");
		return PTR_ERR(gDeviceClass);
	}

	print_function_end();
	return platform_driver_register(&hsModem_driver);
}

static void __exit hsModem_exit(void)
{
	platform_driver_unregister(&hsModem_driver);
	class_destroy(gDeviceClass);
}

module_init(hsModem_init);
module_exit(hsModem_exit);

static int hsModem_open(struct inode *inodep, struct file *filep)
{
	struct hsModem_t *hsModem = (struct hsModem_t *)container_of(inodep->i_cdev,
					struct hsModem_t, char_device);

	filep->private_data = hsModem;

	++gNumberOpens;
	return 0;
}
static int hsModem_release(struct inode *inodep, struct file *filep)
{
	filep->private_data = NULL;

	--gNumberOpens;
	return 0;
}
static ssize_t hsModem_read(struct file *filep,
								char *buffer,
								size_t len,
								loff_t *offset)
{
	//TODO
	return 0;
}
static ssize_t hsModem_write(struct file *filep,
								const char *buffer,
								size_t len,
								loff_t *offset)
{
	//struct hsModem_t *hsModem = (struct hsModem_t *)filep->private_data;

	//TODO
	return 0;
}

static long hsModem_ioctl(struct file *filep,
							unsigned int cmd,
							unsigned long arg)
{
	struct hsModem_t *hsModem = (struct hsModem_t *)filep->private_data;
	long err = 0;
	modem_rst_req_t req;
	//uint32_t value = 0;

	if( cmd == HSMODEM_RESET(hsModem->devt))
	{
		hsModem_dbg(1, "cmd = HSMODEM_RESET\n");

		if(copy_from_user(&req, (modem_rst_req_t *)arg, sizeof(req)))
		{/* Bad address */
			return -EFAULT;
		}

		if((req.modemSubPhy != 1 && req.modemSubPhy != 0) ||
			(req.isRst != 1 && req.isRst != 0))
		{/* Invalid argument */
			return -EINVAL;
		}

		if(req.modemSubPhy) {
			disable_irq(hsModem->irq_tx);
		} else {
			disable_irq(hsModem->irq_rx);
		}

		hsModem_dbg(1, "%s %s\n", req.isRst ? "RESET": "DE-RESET",
			req.modemSubPhy ? "TX" : "RX");

		hsModem_rst(hsModem, (modemSubPhy_t)req.modemSubPhy, req.isRst);

		if(req.modemSubPhy) {
			enable_irq(hsModem->irq_tx);
		} else {
			enable_irq(hsModem->irq_rx);
		}
	}
	else if( cmd == HSMODEM_CALIB_RX(hsModem->devt))
	{
		hsModem_dbg(1, "cmd = HSMODEM_CALIB_RX\n");
		if(copy_from_user(&hsModem->modem_rx_param, (modem_rx_param_t *)arg,
			sizeof(modem_rx_param_t)))
		{/* Bad address */
			return -EFAULT;
		}

		disable_irq(hsModem->irq_rx);

#ifdef DEBUG_PRINT_EX
		printk ("IOCTL: HSMODEM_CALIB_RX - rx_frame_bytes: %d\n", hsModem->modem_rx_param.rx_frame_bytes);
#endif
		/* write RX rate to modem */
		hsModem_dbg(1, "Setting Rx rate\n");
		hsModem_conf_rx_rate(hsModem);

		/* reinitialize buffer */

		destroy_air_protocol_rx(&hsModem->ctx);

		err = init_air_protocol_rx(&hsModem->ctx,
					  (satHeaderTypes_t) LOW_RATE_TYPE_HEADER,
					  hsModem->modem_rx_param.rx_frame_bytes);

		if(err) {
			destroy_air_protocol_rx(&hsModem->ctx);
		}

		enable_irq(hsModem->irq_rx);
	}
	else if( cmd == HSMODEM_CALIB_TX(hsModem->devt))
	{
		hsModem_dbg(1, "cmd = HSMODEM_CALIB_TX\n");
		if(copy_from_user(&hsModem->modem_tx_param, (modem_tx_param_t *)arg,
			sizeof(modem_tx_param_t)))
		{/* Bad address */
			return -EFAULT;
		}

		disable_irq(hsModem->irq_tx);

#ifdef DEBUG_PRINT_EX
		printk ("IOCTL: HSMODEM_CALIB_TX - tx_frame_bytes: %d\n", hsModem->modem_tx_param.tx_frame_bytes);
#endif

		/* write TX rate to modem */
		hsModem_dbg(1, "Setting Tx rate\n");
		hsModem_conf_tx_rate(hsModem);

		/* reinitialize buffer */

		destroy_air_protocol_tx(&hsModem->ctx);

		err = init_air_protocol_tx(&hsModem->ctx,
					  (satHeaderTypes_t)LOW_RATE_TYPE_HEADER,
					  hsModem->modem_tx_param.tx_frame_bytes);

		if(err) {
			destroy_air_protocol_tx(&hsModem->ctx);
		}

		enable_irq(hsModem->irq_tx);
	}
	else if( cmd == HSMODEM_AIR_MODE(hsModem->devt))
	{
		hsModem_dbg(1, "cmd = HSMODEM_AIR_MODE\n");
		if((size_t)arg >= __SATHEADERTYPES_END) {
			return -EINVAL;
		}

		disable_irq(hsModem->irq_tx);
		disable_irq(hsModem->irq_rx);

		destroy_air_protocol(&hsModem->ctx);
		err = init_air_protocol(&hsModem->ctx,
								(satHeaderTypes_t)arg,
								hsModem->modem_tx_param.tx_frame_bytes,
								hsModem->modem_rx_param.rx_frame_bytes);
		if(err) {
			destroy_air_protocol(&hsModem->ctx);
		}
		enable_irq(hsModem->irq_tx);
		enable_irq(hsModem->irq_rx);
	}
	else if( cmd == HSMODEM_STAT(hsModem->devt))
	{
		hsModem_dbg(1, "cmd = HSMODEM_STAT\n");
		if(copy_to_user((hsModemStat_t *)arg, &hsModem->stat,
			sizeof(hsModem->stat)))
		{/* Bad address */
			return -EFAULT;
		}
	}
	else if( cmd == HSMODEM_SET_SCHEDULER(hsModem->devt))
	{
		hsModem_dbg(1, "cmd = HSMODEM_SET_SCHEDULER\n");
		if(copy_from_user(&hsModem->tdd, (schedulerTdd_t *)arg,
			sizeof(schedulerTdd_t)))
		{/* Bad address */
			return -EFAULT;
		}
		set_tx_timestamp(hsModem, hsModem->tdd.waitTimestamp);
	}
	else if( cmd == HSMODEM_SET_MT_ID(hsModem->devt))
	{
		hsModem_dbg(1, "cmd = HSMODEM_SET_MT_ID\n");
		if((uint16_t)arg > MAX_MT_ID_VALID) {
			return -EINVAL;
		}

		disable_irq(hsModem->irq_tx);
		disable_irq(hsModem->irq_rx);

		hsModem->ctx.mtID = (uint16_t)arg;

		enable_irq(hsModem->irq_tx);
		enable_irq(hsModem->irq_rx);

	}
	else
	{/* Invalid argument */
		hsModem_dbg(1, "cmd = none\n");
		return -EINVAL;
	}

	return err;
}


/* ----------------------------
 *         driver function
 * ----------------------------
 */
static int get_dts_property(struct hsModem_t *hsModem,
							char *propName,
							void *dest,
							ePropType_t ePropType)
{
	int err;
	const char* strVal;
	uint32_t u32Val;
	uint8_t u8Val;
	/* Get the data from device tree base on "ePropType" */
	switch(ePropType)
	{
	case PROPTYPE_U32:
		err = of_property_read_u32(hsModem->dt_device->of_node, propName, &u32Val);
		break;
	case PROPTYPE_U8:
		err = of_property_read_u8(hsModem->dt_device->of_node, propName, &u8Val);
		break;
	case PROPTYPE_STRING:
		err = of_property_read_string(hsModem->dt_device->of_node, propName, &strVal);
		break;
	default:
		dev_err(hsModem->dt_device, "ERROR: unknown type in get_dts_property\n");
		return -EINVAL;
	}

	/* Error checking */
	if (err)
	{
		dev_err(hsModem->dt_device, "couldn't read IP dts property '%s'\n", propName);
		return err;
	}

	/* Transfer the data into "dest" */
	switch(ePropType)
	{
	case PROPTYPE_U32:
		*((uint32_t*)dest) = u32Val;
		dev_dbg(hsModem->dt_device, "dts property '%s' = '%u'\n", propName, u32Val);
		break;
	case PROPTYPE_U8:
		*((uint8_t*)dest) = u8Val;
		dev_dbg(hsModem->dt_device, "dts property '%s' = '%u'\n", propName, u8Val);
		break;
	case PROPTYPE_STRING:
		dev_dbg(hsModem->dt_device, "dts property '%s' = '%s'\n", propName, strVal);
		strcpy((char*)dest, strVal);
		break;
	}

	return 0;
}

static int hsModem_probe(struct platform_device *pdev)
{
	struct resource *r_irq; /* interrupt resources */
	struct resource *r_mem; /* IO mem resources */
	struct device *dev = &pdev->dev; /* OS device (from device tree) */
	hsModem_t *hsModem = NULL;

	uint32_t hwVer;
	uint32_t hwRev;
	int err = 0;

	print_function_begin();
	/* ----------------------------
	 *     init wrapper device
	 * ----------------------------
	 */
	hsModem = devm_kzalloc(dev, sizeof(*hsModem), GFP_KERNEL);
	if (!hsModem) {
		return -ENOMEM;
	}
	dev_set_drvdata(dev, hsModem);
	hsModem->dt_device = dev;

	/* ----------------------------
	 *   init device memory space
	 * ----------------------------
	 */
	r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!r_mem)
	{
		dev_err(hsModem->dt_device, "invalid address\n");
		err = -ENODEV;
		goto err_initial;
	}

	hsModem->mem = r_mem;

	/* request physical memory */
	if (!request_mem_region(hsModem->mem->start, resource_size(hsModem->mem),
				DRIVER_NAME)) {
		dev_err(hsModem->dt_device,
			"couldn't lock memory region at 0x%pa\n",
			&hsModem->mem->start);
		err = -EBUSY;
		goto err_initial;
	}

	dev_dbg(hsModem->dt_device, "got memory location [0x%pa - 0x%pa]\n",
		&hsModem->mem->start, &hsModem->mem->end);

	/* map physical memory to kernel virtual address space */
	hsModem->baseAddr = ioremap(hsModem->mem->start, resource_size(hsModem->mem));
	if (!hsModem->baseAddr) {
		dev_err(hsModem->dt_device, "couldn't map physical memory\n");
		err = -ENOMEM;
		goto err_mem;
	}
	dev_info(hsModem->dt_device, "remapped memory to 0x%p\n", hsModem->baseAddr);

	/* ----------------------------
	 *          init IP
	 * ----------------------------
	 */
	err = get_dts_property(hsModem, "hisky,tx-fifo-depth",
							&(hsModem->txFifoDepth), PROPTYPE_U32);
	if (err) {
		goto err_unmap;
	}

	err = get_dts_property(hsModem, "hisky,rx-fifo-depth",
							&(hsModem->rxFifoDepth), PROPTYPE_U32);
	if (err) {
		goto err_unmap;
	}

	/* Validate hardware match the driver */
	hwVer = readReg(hsModem->baseAddr, HSMDM_REG_VERSION_OFFSET);
	hwRev = readReg(hsModem->baseAddr, HSMDM_REG_REVISION_OFFSET);

	if(hwVer != HW_SUPPORT_VER || hwRev != HW_SUPPORT_REV)
	{
		dev_err(hsModem->dt_device,
			"HW error: Driver support ver:%u rev:%u, but ver=%u rev=%u\n",
			HW_SUPPORT_VER, HW_SUPPORT_REV,
			hwVer, hwRev);
		err = -EIO;
		goto err_unmap;
	}
	dev_info(hsModem->dt_device, "HW version:%u revision:%u\n", hwVer, hwRev);

	/* ----------------------------
	 *    init device interrupts
	 * ----------------------------
	 */
	/* Get Rx IRQ resource */
	r_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!r_irq)
	{
		dev_err(hsModem->dt_device, "no Rx IRQ found for 0x%pa\n",
			&hsModem->mem->start);
		err = -EIO;
		goto err_unmap;
	}

	dev_info(hsModem->dt_device,"Rx IRQ flag = 0x%lx\n",
				r_irq->flags & IORESOURCE_BITS);

	/* request Rx IRQ */
	hsModem->irq_rx = r_irq->start;
	err = request_irq(hsModem->irq_rx, &hsModem_irq_rx_handler, 0, DRIVER_NAME, hsModem);
	if (err)
	{
		dev_err(hsModem->dt_device,
			"couldn't allocate Rx interrupt %i\n", hsModem->irq_rx);
		goto err_unmap;
	}

	/* Get Tx IRQ resource */
	r_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 1);
	if (!r_irq)
	{
		dev_err(hsModem->dt_device, "no Tx IRQ found for 0x%pa\n",
			&hsModem->mem->start);
		err = -EIO;
		goto err_irq_rx;
	}

	dev_info(hsModem->dt_device,"Tx IRQ flag = 0x%lx\n",
				r_irq->flags & IORESOURCE_BITS);

	/* request Tx IRQ */
	hsModem->irq_tx = r_irq->start;
	err = request_irq(hsModem->irq_tx, &hsModem_irq_tx_handler, 0, DRIVER_NAME, hsModem);
	if (err)
	{
		dev_err(hsModem->dt_device,
			"couldn't allocate Tx interrupt %i\n", hsModem->irq_tx);
		goto err_irq_rx;
	}

	/* ----------------------------
	 *      init char device
	 * ----------------------------
	 */

	/* allocate device number */
	err = alloc_chrdev_region(&hsModem->devt, 0, max_minor, DRIVER_NAME);
	if (err < 0) {
		goto err_irq_tx;
	}
	dev_dbg(hsModem->dt_device, "allocated device number major %i minor %i\n",
			MAJOR(hsModem->devt), MINOR(hsModem->devt));

	hsModem->device = device_create(gDeviceClass, NULL, hsModem->devt, NULL,
							"%s%u", DRIVER_NAME, gNumOfPhys);
	if (!hsModem->device)
	{
		dev_err(hsModem->dt_device, "couldn't create driver file\n");
		err = -ENOMEM;
		goto err_chrdev_region;
	}
	dev_info(hsModem->dt_device, "%s%u", DRIVER_NAME, gNumOfPhys);
	dev_set_drvdata(hsModem->device, hsModem);

	/* create character device */
	cdev_init(&hsModem->char_device, &fops);
	err = cdev_add(&hsModem->char_device, hsModem->devt, 1);
	if (err < 0)
	{
		dev_err(hsModem->dt_device, "couldn't create character device\n");
		goto err_dev;
	}

	/* create sysfs entries */
	err = sysfs_create_groups(&hsModem->device->kobj, hsModem_attrs_groups);
	if (err < 0)
	{
		dev_err(hsModem->dt_device, "couldn't register sysfs group\n");
		goto err_cdev;
	}

	/* create procfs entries */
	procfs_ent = proc_create_data(hsModem->device->kobj.name, 0440,
									NULL, &procfs_stat_ops, hsModem);
	if(!procfs_ent)
	{
		dev_err(hsModem->dt_device, "couldn't register procfs\n");
		err = -ENOMEM;
		goto err_sysfs;
	}

	/* ----------------------------
	 *         init driver
	 * ----------------------------
	 */
	hsModem->driver_rx_flag = HSMDM_CALC_CRC;
	hsModem_rx_rate_default(&hsModem->modem_rx_param);

	hsModem->isTxTrig = false;
	writeReg(hsModem->baseAddr, HSMDM_REG_IRQ_SEL_OFFSET, 1);
	hsModem_tx_rate_default(&hsModem->modem_tx_param);

	hsModem->tdd.tsToTx = 0;
	hsModem->isRxDummyOn = false;
	hsModem->ctx.remEbN0 = 0x40; // -16.0 in 6 bits
	hsModem->ctx.mtID = 0;

	++gNumOfPhys;
	print_function_end();
	return 0;

err_sysfs:
	sysfs_remove_groups(&hsModem->device->kobj, hsModem_attrs_groups);
err_cdev:
	cdev_del(&hsModem->char_device);
err_dev:
	device_destroy(gDeviceClass, hsModem->devt);
err_chrdev_region:
	unregister_chrdev_region(hsModem->devt, 1);
err_irq_tx:
	free_irq(hsModem->irq_tx, hsModem);
err_irq_rx:
	free_irq(hsModem->irq_rx, hsModem);
err_unmap:
	iounmap(hsModem->baseAddr);
err_mem:
	release_mem_region(hsModem->mem->start, resource_size(hsModem->mem));
err_initial:
	dev_set_drvdata(dev, NULL);

	return err;
}

static int hsModem_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	hsModem_t *hsModem = dev_get_drvdata(dev);

	print_function_begin();

	if(hsModem)
	{
		destroy_air_protocol(&hsModem->ctx);
		proc_remove(procfs_ent);
		sysfs_remove_groups(&hsModem->device->kobj, hsModem_attrs_groups);
		cdev_del(&hsModem->char_device);
		device_destroy(gDeviceClass, hsModem->devt);
		dev_set_drvdata(hsModem->device, NULL);
		unregister_chrdev_region(hsModem->devt, max_minor);
		free_irq(hsModem->irq_tx, hsModem);
		free_irq(hsModem->irq_rx, hsModem);
		iounmap(hsModem->baseAddr);
		release_mem_region(hsModem->mem->start, resource_size(hsModem->mem));
		dev_set_drvdata(dev, NULL);
	}

	print_function_end();

	return 0;
}

static irqreturn_t hsModem_irq_rx_handler(int irq, void *arg)
{
	int err;
	struct hsModem_t* hsModem = (struct hsModem_t*)arg;

	disable_irq(hsModem->irq_tx);

	hsModem->stat.rxTsCnt++;
	hsModem->ctx.msgHandler.rcv_timestamp = get_rx_timestamp(hsModem);
	err = hsModem_handle_rpc_intr(hsModem);
	if(err < 0)
	{
		printk(KERN_WARNING "hsModem_handle_rpc_intr return %d\n", err);
	}

	enable_irq(hsModem->irq_tx);
	return IRQ_HANDLED;
}

static irqreturn_t hsModem_irq_tx_handler(int irq, void *arg)
{
	int err;
	struct hsModem_t* hsModem = (struct hsModem_t*)arg;

	disable_irq(hsModem->irq_rx);

	err = hsModem_handle_tpr_intr(hsModem);
	if(err < 0)
	{
		printk(KERN_WARNING "hsModem_handle_tpr_intr return %d\n", err);
	}

	enable_irq(hsModem->irq_rx);
	return IRQ_HANDLED;
}
static void hsModem_rst(struct hsModem_t* hsModem,
						modemSubPhy_t modemSubPhy,
						int isRst)
{
	void __iomem *baseAddr = hsModem->baseAddr;
	const uint32_t curState = readReg(baseAddr, HSMDM_REG_RST_OFFSET) & 0x03;

	switch (modemSubPhy)
	{
	case MODEM_RX:
		if(isRst) {
			writeReg(baseAddr, HSMDM_REG_RST_OFFSET, curState | 0x2);
		} else {
			writeReg(baseAddr, HSMDM_REG_RST_OFFSET, curState & 0x1);
		}
		break;
	case MODEM_TX:
		if(isRst) {
			hsModem->isTxTrig = false;
			writeReg(baseAddr, HSMDM_REG_RST_OFFSET, curState | 0x1);
		} else {
			writeReg(baseAddr, HSMDM_REG_RST_OFFSET, curState & 0x2);
		}
		break;
	}
}

static int hsModem_handle_rpc_intr(struct hsModem_t* hsModem)
{
	int err;
	uint32_t rxCrcOk;

	select_rx_fifo(hsModem);
	if(!readReg(hsModem->baseAddr, hsModem->rx_fifo.reg_rdfo))
	{ /* Rx Ts signal detected */
		return 0;
	}

	err = hsModem_read_pck(hsModem);
	if(err < 0)
	{ // handle error
		printk(KERN_WARNING "ERROR: hsModem_read_pck return %d\n", err);
		return err;
	}

	if((hsModem->driver_rx_flag & HSMDM_FLAG_CRCOK_MASK) == HSMDM_PHY_CRC)
	{
		rxCrcOk = readReg(hsModem->baseAddr, HSMDM_REG_RX_CRC_OK_OFFSET);
	}
	else
	{

#ifdef DEBUG_PRINT_EX
		unsigned char *p_data;
		p_data = (&hsModem->ctx)->satMsgRx;
		unsigned char msg_length = (&hsModem->ctx)->a_info.satMsgLenRx;

		printk(KERN_WARNING, "hsModem_read_pck with size of: %d\n", msg_length);
		print_hex_dump (KERN_DEBUG, "MESSAGE_FROM_PP", DUMP_PREFIX_NONE, 16, 1, p_data, msg_length, false);
#endif
		rxCrcOk = is_air_msg_valid(&hsModem->ctx);
	}

	if(!rxCrcOk)
	{
		hsModem->stat.rxCrcErrCnt++;
	}
	else
	{
		hsModem->stat.rxGoodPckCnt++;
		err = parse_air_msg(&hsModem->ctx, &hsModem->stat.apStat);
		if(err < 0 && err != ENODEV)
		{
			printk(KERN_WARNING "parse_air_msg ret %d\n", err);
			return err;
		}

		if(!hsModem->isRxDummyOn)
		{
			writeReg(hsModem->baseAddr, HSMDM_REG_RX_IRQ_CTRL_OFFSET,
				HSMDM_RX_IRQ_EN_DUMMY_MASK);
			hsModem->isRxDummyOn = true;
		}
	}

	return 0;
}

static int hsModem_read_pck(struct hsModem_t* hsModem)
{
	int ret;
	int i;
	uint8_t* dst = hsModem->ctx.satMsgRx;

	const uint32_t rdfo = readReg(hsModem->baseAddr, hsModem->rx_fifo.reg_rdfo);
	const uint32_t satMsgLen = hsModem->ctx.a_info.satMsgLenRx;

	const uint32_t dataToRead = min(rdfo, satMsgLen);


	if(!dst) {
		return -EINVAL;
	}

	if(rdfo < satMsgLen)
	{
		hsModem_dbg(7, "rdfo=%u, satMsgLen=%u\n", rdfo, satMsgLen);
		ret = -EPROTO;
	}

	if(dst)
	{
		for(i = 0; i < dataToRead; ++i)
		{
			dst[i] = readReg(hsModem->baseAddr, hsModem->rx_fifo.reg_data);
		}
		ret = satMsgLen;
	}
	else
	{ /* Protocol driver not attached */
		ret = -EUNATCH;
		hsModem_dbg(1, "dest is NULL\n");
	}

	return ret;
}

static int hsModem_handle_tpr_intr(struct hsModem_t* hsModem)
{
	int ret;

	const uint32_t ebn0 = readReg(hsModem->baseAddr, HSMDM_REG_EBN0_OFFSET);
	// ebn0 is 9 bits: ([8]neg/pos | [7:0] data)
	const uint8_t ebn0_6bit = (uint8_t)(ebn0 >> 3);

	hsModem_dbg(4, "tsToTx:%d\n", hsModem->tdd.tsToTx);
	if(hsModem->tdd.tsToTx < 1)
	{
		printk(KERN_WARNING "ERROR: false TX IRQ happened\n");
		return 0;
	}

	if(!hsModem->isTxTrig)
	{
		trig_tx(hsModem);
	}

	if(hsModem->tdd.tsToTx == 1)
	{
		hsModem_last_tx(hsModem);
	}
	hsModem->tdd.tsToTx--;

	ret = air_msg_build(&hsModem->ctx, ebn0_6bit);
	if(ret < 0 && ret != -EUNATCH)
	{ // handle error
		printk(KERN_WARNING "ERROR: air_msg_build return %d\n", ret);
	}
	if(ret > 0)
	{

#ifdef DEBUG_PRINT_EX
		printk ("Write air pakcet with size: %d to modem fifo\n", hsModem->ctx.a_info.satMsgLenTx);
#endif
		hsModem_write_pck(hsModem);
		hsModem->stat.txIrqCnt++;
	}

	return ret;
}

static int hsModem_write_pck(struct hsModem_t* hsModem)
{
	int i;
	uint32_t data;
	const int len = hsModem->ctx.a_info.satMsgLenTx;

	select_tx_fifo(hsModem);

	if(constDataFlag == false)
	{
		for (i = 0; i < len; ++i)
		{
			data = *(hsModem->ctx.satMsgTx + i);
			writeReg(hsModem->baseAddr, hsModem->tx_fifo.reg_data, data);
		}
	}
	else
	{
		for (i = 0; i < len; ++i)
		{
			writeReg(hsModem->baseAddr, hsModem->tx_fifo.reg_data, (uint32_t)i);
		}
	}

	writeReg(hsModem->baseAddr, HSMDM_REG_TX_PCK_END_OFFSET, 1);

	return 1;
}

static int trig_tx(struct hsModem_t *hsModem)
{
	if(hsModem->isTxTrig)
	{
		return -EALREADY;
	}

	hsModem_dbg(1, "Triggering TX\n");
	hsModem_rst(hsModem, MODEM_TX, 1);
	writeReg(hsModem->baseAddr, HSMDM_REG_TDD_MODE_OFFSET, 0);
	hsModem_rst(hsModem, MODEM_TX, 0);

	hsModem->isTxTrig = true;

	//if(hsModem->tdd.tsToTx == 1) {
	//	hsModem_last_tx(hsModem);
	//}

	return 0;
}


static void hsModem_conf_rx_rate(struct hsModem_t *hsModem)
{
	static uint32_t t_flag = 1;
	modem_rx_param_t* param = &hsModem->modem_rx_param;

	hsModem_rst(hsModem, MODEM_RX, 1);
	hsModem_rst(hsModem, MODEM_RX, 0);

#ifdef DEBUG_PRINT_EX
	printk ("hsModem_conf_rx_rate rx_frame_bytes: %d\n", param->rx_frame_bytes);
#endif

	writeReg(hsModem->baseAddr,
			HSMDM_REG_RX_PACKET_SIZE_OFFSET,
			param->rx_frame_bytes);

	writeReg(hsModem->baseAddr,
			HSMDM_REG_RX_sSPREAD_OFFSET,
			param->rxs_spread);

	writeReg(hsModem->baseAddr,
			HSMDM_REG_RX_CORE_DELAY_OFFSET,
			param->rxs_cor_delay);

	writeReg(hsModem->baseAddr,
			HSMDM_REG_RX_sDYN_SCITO_OFFSET,
			param->rxs_dyn_scl_to);

	writeReg(hsModem->baseAddr,
			HSMDM_REG_RX_sWIN_DELAY_OFFSET,
			param->rxs_win_delay);

	writeReg(hsModem->baseAddr,
			HSMDM_REG_RX_sDET_WIN_OFFSET,
			param->rxs_det_win);

	writeReg(hsModem->baseAddr,
			HSMDM_REG_RX_dDET_OFFSET_OFFSET,
			param->rxd_det_offset);

	writeReg(hsModem->baseAddr,
			HSMDM_REG_RX_dDSS_FACTOR_OFFSET,
			param->rxd_dss_factor);

	writeReg(hsModem->baseAddr,
			HSMDM_REG_RX_dDATA_LEN_OFFSET,
			param->rxd_dat_sym);

	writeReg(hsModem->baseAddr,
			HSMDM_REG_RX_dTCC_BLOCK_OFFSET,
			param->rxd_tcc_block);

	writeReg(hsModem->baseAddr,
			HSMDM_REG_RX_dCOD_BLOCK_OFFSET,
			param->rxd_cod_blk_len);

	writeReg(hsModem->baseAddr,
			HSMDM_REG_RX_CIC_RATIO_OFFSET,
			param->rx_cic_ratio);

	writeReg(hsModem->baseAddr,
			HSMDM_REG_RX_CIC_SCALE_OFFSET,
			param->rx_cic_scale);

	writeReg(hsModem->baseAddr,
			HSMDM_REG_RX_RESAMP_FRAC_L_OFFSET,
			param->rx_resamp_frac);

	writeReg(hsModem->baseAddr,
			HSMDM_REG_RX_RESAMP_FRAC_H_OFFSET,
			param->rx_resamp_int);

	writeReg(hsModem->baseAddr,
			HSMDM_REG_RX_NUM_TAPS_OFFSET,
			param->rxs_num_taps);

	writeReg(hsModem->baseAddr, HSMDM_REG_CHANGE_RATE_FLAG_OFFSET, t_flag);

	if(t_flag) {
		t_flag = 0;
	} else {
		t_flag = 1;
	}

	hsModem_rst(hsModem, MODEM_RX, 1);
	hsModem_rst(hsModem, MODEM_RX, 0);
}

static void hsModem_conf_tx_rate(struct hsModem_t *hsModem)
{
	modem_tx_param_t* param = &hsModem->modem_tx_param;

	hsModem_rst(hsModem, MODEM_TX, 1);

#ifdef DEBUG_PRINT_EX
	printk ("hsModem_conf_tx_rate tx_frame_bytes: %d\n", param->tx_frame_bytes);
#endif
	writeReg(hsModem->baseAddr,
			HSMDM_REG_TX_PACKET_SIZE_OFFSET,
			param->tx_frame_bytes);

	writeReg(hsModem->baseAddr,
			HSMDM_REG_TX_FRAME_LEN_OFFSET,
			param->tx_frame_len);

	writeReg(hsModem->baseAddr,
			HSMDM_REG_TX_ENC_BLK_SIZE_OFFSET,
			param->tx_enc_blk_size);

	writeReg(hsModem->baseAddr,
			HSMDM_REG_TX_DSS_FACTOR_OFFSET,
			param->tx_dss_factor);

	writeReg(hsModem->baseAddr,
			HSMDM_REG_TX_SHAP_INT_CLK_OFFSET,
			param->tx_shap_int_clk);

	writeReg(hsModem->baseAddr,
			HSMDM_REG_TX_RESAMP_FRAC_L_OFFSET,
			param->tx_resamp_frac);

	writeReg(hsModem->baseAddr,
			HSMDM_REG_TX_RESAMP_FRAC_H_OFFSET,
			param->tx_resamp_int);

	writeReg(hsModem->baseAddr,
			HSMDM_REG_TX_CIC_INT_RATE_OFFSET,
			param->tx_cic_int_rate);

	writeReg(hsModem->baseAddr,
			HSMDM_REG_TX_CIC_SCALE_OFFSET,
			param->tx_cic_scale);

	hsModem_rst(hsModem, MODEM_TX, 0);
}

static uint32_t get_rx_timestamp(struct hsModem_t *hsModem)
{
	return readReg(hsModem->baseAddr, HSMDM_REG_RX_TIMESTAMP_OFFSET);
}

static void set_tx_timestamp(struct hsModem_t *hsModem, uint32_t t)
{
	writeReg(hsModem->baseAddr, HSMDM_REG_TX_TIMESTAMP_OFFSET, t);
}

static void hsModem_last_tx(struct hsModem_t *hsModem)
{
	hsModem_dbg(1, "TX last\n");
	writeReg(hsModem->baseAddr, HSMDM_REG_TDD_MODE_OFFSET, 1);
	hsModem->isTxTrig = false;

	if(hsModem->ctx.dbg.tdd_self_trig)
	{
		hsModem->tdd.tsToTx = hsModem->ctx.dbg.tdd_self_trig +1;
		hsModem->tdd.waitTimestamp = get_rx_timestamp(hsModem) +
										ts_to_ticks(40);
		set_tx_timestamp(hsModem, hsModem->tdd.waitTimestamp);
	}
}
static void conv_ebn0(struct ebn0_t* ebn0, uint32_t regVal)
{
	const uint32_t acc = 100;

	if((regVal&0x100)==0x100)
	{
		regVal |=0xFFFFFE00;
		ebn0->fracVal = (((regVal%16)*acc)/16);
		if(ebn0->fracVal) {
			ebn0->fracVal = acc - ebn0->fracVal;
		}
		ebn0->neg = 1;
	}
	else
    {
		ebn0->neg = 0;
		ebn0->fracVal = ((regVal%16)*acc)/16;
	}
	ebn0->intVal = (int)regVal/16;
	if(ebn0->neg) {
		ebn0->intVal = -ebn0->intVal;
	}
}

static void conv_ebn0_ex(struct ebn0_t* ebn0, uint32_t regVal,
						uint8_t bits, uint8_t delta, uint32_t acc)
{
    const int32_t bit_mask = 1<<bits;

	if((regVal&bit_mask)==bit_mask)
	{
		regVal |= (-bit_mask);
		ebn0->fracVal = (((regVal%delta)*acc)/delta);
		if(ebn0->fracVal) {
			ebn0->fracVal = acc - ebn0->fracVal;
		}
		ebn0->neg = 1;
	}
	else
    {
		ebn0->neg = 0;
		ebn0->fracVal = ((regVal%delta)*acc)/delta;
	}
	ebn0->intVal = (int)regVal/delta;
	if(ebn0->neg) {
		ebn0->intVal = -ebn0->intVal;
	}
}

static uint32_t ts_to_ticks(uint32_t ts)
{
	return (ts * 5110000); //ticks in ts
}

static void select_rx_fifo(struct hsModem_t *hsModem)
{
	if(readReg(hsModem->baseAddr, HSMDM_REG_RX_FIFO_A_B_VALID_OFFSET) == 1)
	{
		hsModem->rx_fifo.reg_rdfo = HSMDM_REG_RX_FIFO_A_COUNT_OFFSET;
		hsModem->rx_fifo.reg_data = HSMDM_REG_RX_FIFO_A_DATA_OFFSET;
	}
	else
	{
		hsModem->rx_fifo.reg_rdfo = HSMDM_REG_RX_FIFO_B_COUNT_OFFSET;
		hsModem->rx_fifo.reg_data = HSMDM_REG_RX_FIFO_B_DATA_OFFSET;
	}
}

static void select_tx_fifo(struct hsModem_t *hsModem)
{
	if(readReg(hsModem->baseAddr, HSMDM_REG_TX_FIFO_A_B_VALID_OFFSET) == 1)
	{
		hsModem->tx_fifo.reg_data = HSMDM_REG_TX_FIFO_A_DATA_OFFSET;
	}
	else
	{
		hsModem->tx_fifo.reg_data = HSMDM_REG_TX_FIFO_B_DATA_OFFSET;
	}
}
