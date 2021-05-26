obj-m := src/hs-crc.o
hs-crc-objs := src/hs-crc.o
kern_dir := $(CURDIR)/../../3rdparty/zedboard/kernel/xlnx-4.9.0
cross := $(CURDIR)/../../3rdparty/zedboard/buildroot-bins-zedboard/host/bin/arm-linux-gnueabihf-
install_dir := ./bin
EXTRA_CFLAGS=-I$(PWD)/include

build:
	@make ARCH=arm CROSS_COMPILE=$(cross) -C $(kern_dir) M=$(PWD) modules

install:
	@mkdir -p ../$(install_dir); \
	cp ./hs-crc.ko ../$(INSTALL_DIR)/

clean:
	make ARCH=arm CROSS_COMPILE=$(cross) -C $(kern_dir) M=$(PWD) clean; \
	rm -rf $(install_dir)/hs-crc.ko
