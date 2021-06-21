#ifndef __LINUX_HSMODEDM_H
#define __LINUX_HSMODEDM_H

/* ----------------------------
 *           includes
 * ----------------------------
 */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/interrupt.h>

#include "air_interface.h"
#include "hs_modem_rate.h"
#include "hs_modem_stat.h"
#include "scheduler.h"

/* ----------------------------
 *            types
 * ----------------------------
 */
struct ebn0_t
{
    uint32_t intVal;
    uint32_t fracVal;
    uint8_t neg;
} ebn0_t;

typedef struct hsModem_t
{
	/* physical and basic kernel things */
	struct resource *mem; /* physical space memory */
	int irq_rx; /* RX interrupt */
	int irq_tx; /* TX interrupt */
	void __iomem *baseAddr; /* kernel space memory */
	struct device *dt_device; /* device created from the device tree */
	
	/* Device-tree properties */
	uint32_t txFifoDepth;
	uint32_t rxFifoDepth;
	
	/* Char Devices */
	struct device *device; /* device associated with char_device */
	dev_t devt; /* char device number */
	struct cdev char_device;
	unsigned int dev_flags; /* read/write file flags */
	
	/* Driver flags */
	uint32_t driver_rx_flag;
	bool isTxTrig;
	bool isRxDummyOn;
	/* Context Variables */
	airContext_t ctx;
	
	/* TDD/Scheduler Variables */
	schedulerTdd_t tdd;
	/* Statistics */
	hsModemStat_t stat;
	
	/* FIFOs */
	struct rx_fifo
	{
		uint32_t reg_rdfo;
		uint32_t reg_data;
	} rx_fifo;
	struct tx_fifo
	{
		uint32_t reg_data;
	} tx_fifo;
	
	modem_rx_param_t modem_rx_param;
	modem_tx_param_t modem_tx_param;
}hsModem_t;

typedef enum
{
	PROPTYPE_U8,
	PROPTYPE_U32,
	PROPTYPE_STRING,
}ePropType_t;

typedef enum
{
	MODEM_RX = 0,
	MODEM_TX = 1
}modemSubPhy_t;

typedef enum
{
	SYSFS_STAT_EMPTY,
	SYSFS_STAT_TX_IRQ,
	SYSFS_STAT_RX_CRC_ERR,
	SYSFS_STAT_RX_GOOD_PCK,
	SYSFS_STAT_RX_TOTAL_PCK,
	SYSFS_STAT_RX_TIMESLOT,
	SYSFS_STAT_RX_PCK_DROPPED,
	SYSFS_STAT_ADMIN_TX_BYTES,
	SYSFS_STAT_ADMIN_TX_SEG,
	SYSFS_STAT_ADMIN_TX_PART_SEG,
	SYSFS_STAT_ADMIN_TX_FREG,
	SYSFS_STAT_ADMIN_RX_BYTES,
	SYSFS_STAT_ADMIN_RX_SEG,
	SYSFS_STAT_ADMIN_RX_PART_SEG,
	SYSFS_STAT_ADMIN_RX_FREG,
	SYSFS_STAT_UDP_TX_BYTES,
	SYSFS_STAT_UDP_TX_SEG,
	SYSFS_STAT_UDP_TX_PART_SEG,
	SYSFS_STAT_UDP_TX_FREG,
	SYSFS_STAT_UDP_RX_BYTES,
	SYSFS_STAT_UDP_RX_SEG,
	SYSFS_STAT_UDP_RX_PART_SEG,
	SYSFS_STAT_UDP_RX_FREG
}modem_sysfs_stat_t;

/* ----------------------------
 *     driver_rx_flag Masks
 * ----------------------------
 */
#define HSMDM_FLAG_CRCOK_MASK	0x1
/* ----------------------------
 *    HSMDM_FLAG_CRCOK values
 * ----------------------------
 */
#define HSMDM_PHY_CRC	0x0
#define HSMDM_CALC_CRC	0x1

/* ----------------------------
 *        Declarations
 * ----------------------------
 */

#endif /* __LINUX_HSMODEDM_H */