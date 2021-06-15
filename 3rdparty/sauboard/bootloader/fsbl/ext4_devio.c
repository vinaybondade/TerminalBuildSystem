#include <ext4_config.h>
#include <ext4_blockdev.h>
#include <ext4_errno.h>
#include "diskio.h"
#include "xsdps_hw.h"
#include "xil_printf.h"

static BYTE driveNo = 0;

/**@brief   Image block size.*/
#define EXT4_FILEDEV_BSIZE 512

#define DROP_LINUXCACHE_BUFFERS 0

/**********************BLOCKDEV INTERFACE**************************************/
static int file_dev_open(struct ext4_blockdev *bdev);
static int file_dev_bread(struct ext4_blockdev *bdev, void *buf, uint64_t blk_id,
			 uint32_t blk_cnt);
static int file_dev_bwrite(struct ext4_blockdev *bdev, const void *buf,
			  uint64_t blk_id, uint32_t blk_cnt);
static int file_dev_close(struct ext4_blockdev *bdev);

/******************************************************************************/
EXT4_BLOCKDEV_STATIC_INSTANCE(file_dev, EXT4_FILEDEV_BSIZE, 0, file_dev_open,
		file_dev_bread, file_dev_bwrite, file_dev_close, 0, 0);
static uint8_t *ioBuf;

/******************************************************************************/
static int file_dev_open(struct ext4_blockdev *bdev)
{
    // Init the SD card
    if((disk_initialize(driveNo) & STA_NOINIT) != 0U)
        return ENOENT;
    
    file_dev.part_offset = 16*XSDPS_BLK_SIZE_512_MASK;

    // get block count.
    if(disk_ioctl(driveNo, GET_SECTOR_COUNT, &file_dev.bdif->ph_bcnt) != RES_OK)
        return EIO;

	file_dev.part_size = file_dev.bdif->ph_bcnt * file_dev.bdif->ph_bsize;

    return EOK;
}
/******************************************************************************/

static int file_dev_bread(struct ext4_blockdev *bdev, void *buf, uint64_t blk_id,
			 uint32_t blk_cnt)
{
    if(disk_read(driveNo, buf, blk_id, blk_cnt) != RES_OK)
        return EIO;


    return EOK;
}
/******************************************************************************/

static void drop_cache(void)
{
#if defined(__linux__) && DROP_LINUXCACHE_BUFFERS
	int fd;
	char *data = "3";

	sync();
	fd = open("/proc/sys/vm/drop_caches", O_WRONLY);
	write(fd, data, sizeof(char));
	close(fd);
#endif
}

/******************************************************************************/
static int file_dev_bwrite(struct ext4_blockdev *bdev, const void *buf,
			  uint64_t blk_id, uint32_t blk_cnt)
{
    if(disk_write(driveNo, buf, blk_id, blk_cnt) != RES_OK)
        return EIO;

    return EOK;
}
/******************************************************************************/
static int file_dev_close(struct ext4_blockdev *bdev)
{
    // Dummy close
	return EOK;
}

/******************************************************************************/
struct ext4_blockdev *file_dev_get(void)
{
	return &file_dev;
}