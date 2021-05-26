obj-m := hsmodem.o lowrate.o hsfifo.o

hsmodem-objs := src/hs-crc.o src/hs-modem.o src/air_interface.o src/hs_modem_rate.o src/hs_modem_dbg.o

lowrate-objs := src/low_rate.o

hsfifo-objs := src/hs-fifo.o

kern_dir := $(CURDIR)/../../3rdparty/zedboard/kernel/xlnx-4.9.0
cross := $(CURDIR)/../../3rdparty/zedboard/buildroot-bins-zedboard/host/bin/arm-linux-gnueabihf-
install_dir := ./bin
EXTRA_CFLAGS=-I$(PWD)/include

build:
	@make ARCH=arm CROSS_COMPILE=$(cross) -C $(kern_dir) M=$(PWD) modules

install:
	@mkdir -p ../$(install_dir); \
	cp ./hs-modem.ko ../$(INSTALL_DIR)/

clean:
	make ARCH=arm CROSS_COMPILE=$(cross) -C $(kern_dir) M=$(PWD) clean; \
	rm -rf $(install_dir)/*.ko

