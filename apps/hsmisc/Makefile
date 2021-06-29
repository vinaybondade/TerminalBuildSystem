obj-m := hsmisc.o
hsmisc-objs := src/hsmisc.o
kern_dir := $(CURDIR)/../../3rdparty/$(dev)/kernel/xlnx-4.9.0
cross := $(CURDIR)/../../3rdparty/$(dev)/buildroot-bins-$(dev)/host/bin/arm-linux-gnueabihf-
install_dir := ./bin
EXTRA_CFLAGS=-I$(PWD)/include

build:
	@make ARCH=arm CROSS_COMPILE=$(cross) -C $(kern_dir) M=$(PWD) modules

install:
	@mkdir -p ../$(install_dir); \
	cp ./hsmisc.ko ../$(INSTALL_DIR)/

clean:
	make ARCH=arm CROSS_COMPILE=$(cross) -C $(kern_dir) M=$(PWD) clean; \
	rm -rf $(install_dir)/hsmisc.ko
