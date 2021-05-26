#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <poll.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/ioctl.h>

#include "common.h"
#include "uart.h"

#define HCIUARTSETPROTO		_IOW('U', 200, int)
#define HCIUARTSETFLAGS		_IOW('U', 203, int)
#define HCI_UART_RAW_DEVICE	0
#define HCI_UART_CREATE_AMP	2
#define HCI_UART_LL	4
#define MAKEWORD(a, b)  ((uint16_t)(((uint8_t)(a)) | ((uint16_t)((uint8_t)(b))) << 8))
#define TI_MANUFACTURER_ID	13
#define FIRMWARE_DIRECTORY	"/lib/firmware/ti-connectivity/"
#define FILE_HEADER_MAGIC	0x42535442

#define FLOW_CTL	0x0001
#define AMP_DEV		0x0002
#define ENABLE_PM	1
#define DISABLE_PM	0

#ifndef N_HCI
#define N_HCI	15
#endif

/*
 * BRF Firmware header
 */
struct bts_header {
	uint32_t	magic;
	uint32_t	version;
	uint8_t	future[24];
	uint8_t	actions[0];
}__attribute__ ((packed));

struct uart_t {
	char *type;
	int  m_id;
	int  p_id;
	int  proto;
	int  init_speed;
	int  speed;
	int  flags;
	int  pm;
	char *bdaddr;
};

int g_inituart_fd = -1;

struct uart_t g_uart = 	{ "texas", 0x0000, 0x0000, HCI_UART_LL, 115200, 115200,
				FLOW_CTL, DISABLE_PM, NULL };

static inline unsigned int tty_get_speed(int speed)
{
	switch (speed) {
	case 9600:
		return B9600;
	case 19200:
		return B19200;
	case 38400:
		return B38400;
	case 57600:
		return B57600;
	case 115200:
		return B115200;
	case 230400:
		return B230400;
	case 460800:
		return B460800;
	case 500000:
		return B500000;
	case 576000:
		return B576000;
	case 921600:
		return B921600;
	case 1000000:
		return B1000000;
	case 1152000:
		return B1152000;
	case 1500000:
		return B1500000;
	case 2000000:
		return B2000000;
#ifdef B2500000
	case 2500000:
		return B2500000;
#endif
#ifdef B3000000
	case 3000000:
		return B3000000;
#endif
#ifdef B3500000
	case 3500000:
		return B3500000;
#endif
#ifdef B3710000
	case 3710000:
		return B3710000;
#endif
#ifdef B4000000
	case 4000000:
		return B4000000;
#endif
	}
	return 0;
}

int set_speed(int fd, struct termios *ti, int speed)
{
	if (cfsetospeed(ti, tty_get_speed(speed)) < 0)
		return -errno;

	if (cfsetispeed(ti, tty_get_speed(speed)) < 0)
		return -errno;

	if (tcsetattr(fd, TCSANOW, ti) < 0)
		return -errno;

	return 0;
}

int is_uart_ready(const char* dev)
{
    int dev_id, dd, fd;
    
    fd = open(dev, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        perror("Can't open serial port");
        return -1;
    }
        
    dev_id = ioctl(fd, HCIUARTGETDEVICE, 0);
    if (dev_id < 0) {
        printf("%s:cannot get device id\n", __func__);
		close(fd);
        return -1;
    }
    
    dd = hci_open_dev(dev_id);
    if (dd < 0) {
        printf("%s: HCI device open failed\n", __func__);
		close(fd);
        return -1;
    }

    if (ioctl(dd, HCIDEVUP, dev_id) < 0 && errno != EALREADY) {
        printf("%s: Can't init device hci%d: %s (%d)", __func__, dev_id,
                            strerror(errno), errno);
        hci_close_dev(dd);
		close(fd);
        return -1;
    }
    hci_close_dev(dd);
	close(fd);
    return 0;
}

void kill_init_uart(void)
{
	close(g_inituart_fd);
}

int init_uart(const char *dev, int speed, int init_speed, int flow, int send_break, int raw)
{
	struct termios ti;
	int i;
	unsigned long flags = 0;

	if (raw)
		flags |= 1 << HCI_UART_RAW_DEVICE;

	if (flow)
		g_uart.flags |= FLOW_CTL;

	g_uart.init_speed = init_speed;
	g_uart.speed = speed;
	if (g_uart.flags & AMP_DEV)
		flags |= 1 << HCI_UART_CREATE_AMP;
	g_inituart_fd = open(dev, O_RDWR | O_NOCTTY);
	if (g_inituart_fd < 0) {
		printf("%s:Can't open serial port\n", __func__);
		return -1;
	}
	tcflush(g_inituart_fd, TCIOFLUSH);
	if (tcgetattr(g_inituart_fd, &ti) < 0) {
		printf("%s:Can't get port settings\n", __func__);
		goto fail;
	}
	cfmakeraw(&ti);
	ti.c_cflag |= CLOCAL;
	if (g_uart.flags & FLOW_CTL)
		ti.c_cflag |= CRTSCTS;
	else
		ti.c_cflag &= ~CRTSCTS;
	if (tcsetattr(g_inituart_fd, TCSANOW, &ti) < 0) {
		printf("%s:Can't set port settings\n", __func__);
		goto fail;
	}
	/* Set initial baudrate */
	if (set_speed(g_inituart_fd, &ti, g_uart.init_speed) < 0) {
		printf("%s:Can't set initial baud rate\n", __func__);
		goto fail;
	}
	tcflush(g_inituart_fd, TCIOFLUSH);
	if (send_break) {
		tcsendbreak(g_inituart_fd, 0);
		usleep(500000);
	}
	if (texas_init(g_inituart_fd, &g_uart.speed, &ti) < 0) {
		printf("%s: texas_init() failed!\n", __func__);
		goto fail;
	}
	tcflush(g_inituart_fd, TCIOFLUSH);
	/* Set actual baudrate */
	if (set_speed(g_inituart_fd, &ti, g_uart.speed) < 0) {
		printf("%s:Can't set baud rate\n", __func__);
		goto fail;
	}
	/* Set TTY to N_HCI line discipline */
	i = N_HCI;
	if (ioctl(g_inituart_fd, TIOCSETD, &i) < 0) {
		printf("%s:Can't set line discipline\n", __func__);
		goto fail;
	}
	if (flags && ioctl(g_inituart_fd, HCIUARTSETFLAGS, flags) < 0) {
		printf("%s:Can't set UART flags\n", __func__);
		goto fail;
	}
	if (ioctl(g_inituart_fd, HCIUARTSETPROTO, g_uart.proto) < 0) {
		printf("%s:Can't set device\n", __func__);
		goto fail;
	}
	if (texas_post(g_inituart_fd, &ti) < 0) {
		printf("%s: texas_post() failed!\n", __func__);
		goto fail;
	}
	printf("%s: Succeedded!\n", __func__);
	return g_inituart_fd;

fail:
	printf("%s: Failed!\n", __func__);
	close(g_inituart_fd);
	return -1;
}

int read_hci_event(int fd, unsigned char* buf, int size)
{
	int remain, r;
	int count = 0;

	if (size <= 0)
		return -1;
	/* The first byte identifies the packet type. For HCI event packets, it
	 * should be 0x04, so we read until we get to the 0x04. */
	while (1) {
		r = read(fd, buf, 1);
		if (r <= 0)
			return -1;
		if (buf[0] == 0x04)
			break;
	}
	count++;
	/* The next two bytes are the event code and parameter total length. */
	while (count < 3) {
		r = read(fd, buf + count, 3 - count);
		if (r <= 0)
			return -1;
		count += r;
	}
	/* Now we read the parameters. */
	if (buf[2] < (size - 3))
		remain = buf[2];
	else
		remain = size - 3;
	while ((count - 3) < remain) {
		r = read(fd, buf + count, remain - (count - 3));
		if (r <= 0)
			return -1;
		count += r;
	}
	return count;
}
