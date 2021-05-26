#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/param.h>

#include "common.h"
#include "uart.h"

#define UART_DEV	"/dev/ttyPS1"

uint8_t p_get_u8(struct frame *frm)
{
	uint8_t *u8_ptr = frm->ptr;
	frm->ptr += 1;
	frm->len -= 1;
	return *u8_ptr;
}

const char *evttype2str(uint8_t type)
{
	switch (type) {
	case 0x00:
		return "ADV_IND - Connectable undirected advertising";
	case 0x01:
		return "ADV_DIRECT_IND - Connectable directed advertising";
	case 0x02:
		return "ADV_SCAN_IND - Scannable undirected advertising";
	case 0x03:
		return "ADV_NONCONN_IND - Non connectable undirected advertising";
	case 0x04:
		return "SCAN_RSP - Scan Response";
	default:
		return "Reserved";
	}
}

uint16_t get_le16(const void *ptr)
{
	return le16_to_cpu(get_unaligned((const uint16_t *) ptr));
}

void p_indent(int level, struct frame *f)
{
	int state = 0;
	int flags = DUMP_TSTAMP | DUMP_VERBOSE;
	if (level < 0) {
		state = 0;
		return;
	}

	if (!state) {
		if (flags & DUMP_TSTAMP) {
			if (flags & DUMP_VERBOSE) {
				struct tm tm;
				time_t t = f->ts.tv_sec;
				localtime_r(&t, &tm);
				printf("%04d-%02d-%02d %02d:%02d:%02d.%06lu ",
					tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
					tm.tm_hour, tm.tm_min, tm.tm_sec, f->ts.tv_usec);
			} else
				printf("%8lu.%06lu ", f->ts.tv_sec, f->ts.tv_usec);
		}
		printf("%c ", (f->in ? '>' : '<'));
		state = 1;
	} else 
		printf("  ");

	if (level)
		printf("%*c", (level*2), ' ');
}

void ext_inquiry_data_dump(int level, struct frame *frm,
						uint8_t *data)
{
	uint8_t len = data[0];
	uint8_t type;
	char *str;
	int i;

	if (len == 0)
		return;

	type = data[1];
	data += 2;
	len -= 1;

	switch (type) {
	case 0x01:
		p_indent(level, frm);
		printf("Flags:");
		for (i = 0; i < len; i++)
			printf(" 0x%2.2x", data[i]);
		printf("\n");
		break;

	case 0x02:
	case 0x03:
		p_indent(level, frm);
		printf("%s service classes:",
				type == 0x02 ? "Shortened" : "Complete");

		for (i = 0; i < len / 2; i++)
			printf(" 0x%4.4x", get_le16(data + i * 2));

		printf("\n");
		break;

	case 0x08:
	case 0x09:
		str = malloc(len + 1);
		if (str) {
			snprintf(str, len + 1, "%s", (char *) data);
			for (i = 0; i < len; i++)
				if (!isprint(str[i]))
					str[i] = '.';
			p_indent(level, frm);
			printf("%s local name: \'%s\'\n",
				type == 0x08 ? "Shortened" : "Complete", str);
			free(str);
		}
		break;

	case 0x0a:
		p_indent(level, frm);
		printf("TX power level: %d\n", *((uint8_t *) data));
		break;

	default:
		p_indent(level, frm);
		printf("Unknown type 0x%02x with %d bytes data\n",
							type, len);
		break;
	}
}

void advertising_report(int level, struct frame *frm)
{
	uint8_t num_reports = p_get_u8(frm);
	const uint8_t RSSI_SIZE = 1;

	while (num_reports--) {
		char addr[18];
		le_advertising_info *info = frm->ptr;
		int offset = 0;

		sprintf(addr, "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X", 
				info->bdaddr.b[5], info->bdaddr.b[4], info->bdaddr.b[3], 
				info->bdaddr.b[2], info->bdaddr.b[1], info->bdaddr.b[0]);
		/* if (strcmp(addr, g_filter_mac)) {
			printf("Not our msg: mac:%s\n", addr);
			continue;
		}*/

		printf("LE Advertising Report\n");
		p_indent(level, frm);
		printf("%s (%d)\n", evttype2str(info->evt_type), info->evt_type);

		p_indent(level, frm);
		printf("bdaddr %s\n", addr);

		while (offset < info->length) {
			int eir_data_len = info->data[offset];

			ext_inquiry_data_dump(level, frm, &info->data[offset]);

			offset += eir_data_len + 1;
		}

		frm->ptr += LE_ADVERTISING_INFO_SIZE + info->length;
		frm->len -= LE_ADVERTISING_INFO_SIZE + info->length;

		p_indent(level, frm);
		printf("RSSI: %d\n", ((int8_t *) frm->ptr)[frm->len - 1]);

		frm->ptr += RSSI_SIZE;
		frm->len -= RSSI_SIZE;
	}
}

void raw_dump(struct frame *frm)
{
	hci_event_hdr *hdr = NULL;
	uint8_t event = 0;
	evt_le_meta_event *mevt = NULL;
	uint8_t subevent;

	if (!frm->len)
		return;

	frm->ptr++; frm->len--;
	hdr = frm->ptr;
	event = hdr->evt;

	frm->ptr += HCI_EVENT_HDR_SIZE;
	frm->len -= HCI_EVENT_HDR_SIZE;
	mevt = frm->ptr;
	subevent = mevt->subevent;

	if (event > EVENT_NUM)
		return;

	if (event != EVT_LE_META_EVENT) {
		printf("event:%d\n", event);
		return;
	}

	if (subevent != EVT_LE_ADVERTISING_REPORT) {
		printf("subevent: %d\n", subevent);
		return;
	}
	p_indent(-1, NULL);
	p_indent(-1, frm);

	frm->ptr += EVT_LE_META_EVENT_SIZE;
	frm->len -= EVT_LE_META_EVENT_SIZE;

	advertising_report(0, frm); 
}

int init_hci(void)
{
	if (is_uart_ready(UART_DEV)) {
		if (init_uart(UART_DEV, 3000000, 115200, 1, 0, 0) == -1) {
				printf("%s: Can't init TI device: %s\n", __func__, UART_DEV);
				return -1;
		}
	}
	printf("%s: Succedded!\n", __func__);
	return 0;
}

int open_socket(int dev, unsigned long flags)
{
	struct sockaddr_hci addr;
	struct hci_filter flt;
	int sk, opt;

	/* Create HCI socket */
	sk = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
	if (sk < 0) {
		perror("Can't create raw socket");
		return -1;
	}

	opt = 1;
	if (setsockopt(sk, SOL_HCI, HCI_DATA_DIR, &opt, sizeof(opt)) < 0) {
		perror("Can't enable data direction info");
		goto fail;
	}

	opt = 1;
	if (setsockopt(sk, SOL_HCI, HCI_TIME_STAMP, &opt, sizeof(opt)) < 0) {
		perror("Can't enable time stamp");
		goto fail;
	}

	/* Setup filter */
	hci_filter_clear(&flt);
	hci_filter_all_ptypes(&flt);
	hci_filter_all_events(&flt);
	if (setsockopt(sk, SOL_HCI, HCI_FILTER, &flt, sizeof(flt)) < 0) {
		perror("Can't set filter");
		goto fail;
	}

	/* Bind socket to the HCI device */
	memset(&addr, 0, sizeof(addr));
	addr.hci_family = AF_BLUETOOTH;
	addr.hci_dev = dev;
	if (bind(sk, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		printf("Can't attach to device hci%d. %s(%d)\n",
					dev, strerror(errno), errno);
		goto fail;
	}

	return sk;

fail:
	close(sk);
	return -1;
}


