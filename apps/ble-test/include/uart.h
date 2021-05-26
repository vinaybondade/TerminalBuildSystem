#ifndef UART_H
#define UART_H

#include <termios.h>

int set_speed(int fd, struct termios *ti, int speed);
int texas_init(int fd, int *speed, struct termios *ti);
int texas_post(int fd, struct termios *ti);
int is_uart_ready(const char* dev);
int init_uart(const char *dev, int speed, int init_speed, int flow, int send_break, int raw);
int revert_uart(const char *dev, int speed);
int read_hci_event(int fd, unsigned char* buf, int size);

#endif
