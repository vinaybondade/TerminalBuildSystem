#ifndef COMMON_H
#define COMMON_H

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <stdint.h>

#define HCIUARTGETDEVICE    _IOR('U', 202, int)
#define TIMEOUT	10000
#define SNAP_LEN	HCI_MAX_FRAME_SIZE
#define DUMP_WIDTH	20
#define DUMP_TSTAMP	0x0100
#define DUMP_VERBOSE	0x0200
#define DUMP_RAW	0x0008
#define EVENT_NUM 77
#define le16_to_cpu(val) (val)
#define get_unaligned(ptr)			\
__extension__ ({				\
	struct __attribute__((packed)) {	\
		__typeof__(*(ptr)) __v;		\
	} *__p = (__typeof__(__p)) (ptr);	\
	__p->__v;				\
})

struct hcidump_hdr {
	uint16_t	len;
	uint8_t		in;
	uint8_t		pad;
	uint32_t	ts_sec;
	uint32_t	ts_usec;
} __attribute__ ((packed));
#define HCIDUMP_HDR_SIZE (sizeof(struct hcidump_hdr))

struct frame {
	void		*data;
	uint32_t	data_len;
	void		*ptr;
	uint32_t	len;
	uint16_t	dev_id;
	uint8_t		in;
	uint8_t		master;
	uint16_t	handle;
	uint16_t	cid;
	uint16_t	num;
	uint8_t		dlci;
	uint8_t		channel;
	unsigned long	flags;
	struct timeval	ts;
	int		pppdump_fd;
	int		audio_fd;
};

extern char g_filter_mac[18];
void raw_dump(struct frame *frm);
int open_socket(int dev, unsigned long flags);
int init_hci(void);
void kill_init_uart(void);

#endif
