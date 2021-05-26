#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>

#define FLASH_BLK_SIZE	4*65536
#define CONFIG_MTD_DEV	"/dev/mtd1"
#define CONFIG_OKAY	0xaabbccdd

uint32_t read_32_bytes()
{
	int fd;
        uint32_t value = 0;

	fd = open(CONFIG_MTD_DEV, O_RDWR);

	if (read(fd, &value, sizeof(value)) == -1) {
		printf("%s: read config failed. err:%s\n", __func__, strerror(errno));
		return -1;
	}
	
	close(fd);
	return value;
}

uint32_t write_32_bytes()
{
	int fd;
	erase_info_t ei;
	uint32_t value = 0xaabbccdd;

	unsigned char *buf;

	fd = open(CONFIG_MTD_DEV, O_RDWR);
	ei.length = FLASH_BLK_SIZE;   //set the erase block size
	ei.start = 0;

	if (ioctl(fd, MEMERASE, &ei) == -1) {
		printf("%s: err on memerase: %s\n", __func__, strerror(errno));
		close(fd);
		return -1;
	}

	if (lseek(fd, 0, SEEK_SET) == -1) {
		printf("%s: err on lseek: %s\n", __func__, strerror(errno));
		close(fd);
		return -1;
	}

	buf = (unsigned char *)malloc (FLASH_BLK_SIZE);
	memcpy(buf, &value, sizeof(value));

	if (write(fd, buf, FLASH_BLK_SIZE) == -1) {
		printf("%s: err on write: %s\n", __func__, strerror(errno));
		close(fd);
		return -1;
	}

	free(buf);
	close(fd);

	return 0;
}

int main(int argc, char** argv)
{
	uint32_t read_value = 0;

	read_value = read_32_bytes();
	printf ("read_value: 0x%x\n", read_value);

	write_32_bytes();

	/*
	if (read_config(&net)) {
		create_conf(&net);
		write_config(&net);
		memset(&net, 0, sizeof(netconf_t));
		if (read_config(&net)) {
			printf("Can't read config file!\n");
			return -1;
		}
	}
	*/

	return 0;
}
