#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>
#include <signal.h>

#include "common.h"

char g_filter_mac[18];
int g_handle = 0;
int g_sock = -1;

void sig_handler(int signo)
{
  if (signo == SIGINT || signo == SIGHUP) {
    printf("received SIGINT|SIGHUP\n");
	hci_close_dev(g_handle);
	close(g_sock);
	kill_init_uart();
    printf("After hci_close_dev()\n");
  }
}

int lescan(int dev)
{
	int err;
	uint8_t own_type = LE_PUBLIC_ADDRESS;
	uint8_t scan_type = 0x01;
	uint8_t filter_policy = 0x00;
	uint16_t interval = htobs(0x0010);
	uint16_t window = htobs(0x0010);
	uint8_t filter_dup = 0x00;

	err = hci_le_set_scan_parameters(dev, scan_type, interval, window,
						own_type, filter_policy, TIMEOUT);
	if (err < 0) {
		printf("%s: Set scan parameters failed. err:%s\n", __func__, strerror(errno));
		return -1;
	}

	err = hci_le_set_scan_enable(dev, 0x01, filter_dup, TIMEOUT);
	if (err < 0) {
		printf("%s: Enable scan failed. err:%s\n", __func__, strerror(errno));
		return -1;
	}

	printf("LE Scan ...\n");
	return 0;
}

int process_frames(int dev, unsigned long flags)
{
	struct cmsghdr *cmsg;
	struct msghdr msg;
	struct iovec  iv;
	struct frame frm;
	struct pollfd fds[2];
	int nfds = 0;
	char *buf;
	char ctrl[100];
	int len, hdr_size = HCIDUMP_HDR_SIZE;
	static int  snap_len = SNAP_LEN;

	printf("%s: Starting!\n", __func__);
	if (g_sock < 0) {
		printf("%s: g_sock is -1\n", __func__);
		return -1;
	}

	if (snap_len < SNAP_LEN)
		snap_len = SNAP_LEN;

	buf = malloc(snap_len + hdr_size);
	if (!buf) {
		printf("%s:Can't allocate data buffer\n", __func__);
		return -1;
	}

	frm.data = buf + hdr_size;

	if (dev == HCI_DEV_NONE)
		printf("%s:system: \n", __func__);
	else
		printf("%s: device: hci%d \n", __func__, dev);


	memset(&msg, 0, sizeof(msg));

	fds[nfds].fd = g_sock;
	fds[nfds].events = POLLIN;
	fds[nfds].revents = 0;
	nfds++;

	while (1) {
		int i, n = poll(fds, nfds, -1);
		if (n <= 0)
			continue;

		for (i = 0; i < nfds; i++) {
			if (fds[i].revents & (POLLHUP | POLLERR | POLLNVAL)) {
				if (fds[i].fd == g_sock)
					printf("%s: device: disconnected\n", __func__);
				else
					printf("%s: client: disconnect\n", __func__);
				return 0;
			}
		}

		iv.iov_base = frm.data;
		iv.iov_len  = snap_len;

		msg.msg_iov = &iv;
		msg.msg_iovlen = 1;
		msg.msg_control = ctrl;
		msg.msg_controllen = 100;

		len = recvmsg(g_sock, &msg, MSG_DONTWAIT);
		if (len < 0) {
			if (errno == EAGAIN || errno == EINTR)
				continue;
			printf("%s:Receive failed\n", __func__);
			return -1;
		}
		/* Process control message */
		frm.data_len = len;
		frm.dev_id = dev;
		frm.in = 0;
		frm.pppdump_fd = -1;//parser.pppdump_fd;
		frm.audio_fd   = -1; //parser.audio_fd;

		cmsg = CMSG_FIRSTHDR(&msg);
		while (cmsg) {
			int dir;
			switch (cmsg->cmsg_type) {
			case HCI_CMSG_DIR:
				memcpy(&dir, CMSG_DATA(cmsg), sizeof(int));
				frm.in = (uint8_t) dir;
				break;
			case HCI_CMSG_TSTAMP:
				memcpy(&frm.ts, CMSG_DATA(cmsg),
						sizeof(struct timeval));
				break;
			}
			cmsg = CMSG_NXTHDR(&msg, cmsg);
		}

		frm.ptr = frm.data;
		frm.len = frm.data_len;
		raw_dump(&frm);
		fflush(stdout);
	}
	return 0;
}

int main(int argc, char **argv)
{
	int device = 0;
	int dev_id = 0;

/*	if (argc < 2) {
		printf("Please supply MAC addr of the device!\n");
		return -1;
	} */
	
	if (signal(SIGINT, sig_handler) == SIG_ERR) {
		printf("\ncan't catch SIGINT\n");
		return -1;
	}

	if (signal(SIGHUP, sig_handler) == SIG_ERR) {
		printf("\ncan't catch SIGHUP\n");
		return -1;
	}

	//strcpy(g_filter_mac, argv[1]);
	init_hci();
	dev_id = hci_get_route(NULL);

	g_handle = hci_open_dev(dev_id);
	if (g_handle < 0) {
		printf("%s: Could not open device. err:%s\n", __func__, strerror(errno));
		return -1;
	}
	lescan(g_handle);
	g_sock = open_socket(device, DUMP_RAW);
	process_frames(device, DUMP_RAW);
	hci_close_dev(g_handle);
	close(g_sock);
	return 0;
}
