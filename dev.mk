ifndef prj 
prj=$(PWD)
endif

is_dev_ok=1
devs="zedboard"

ifneq ($(dev),$(filter $(dev), zedboard x86_64))
	is_dev_ok=0
endif

ifeq ($(dev),)
	is_dev_ok=0
endif

bb_bins=$(prj)/3rdparty/$(dev)/buildroot-bins-$(dev)
staging=$(bb_bins)/staging

ifeq ($(dev),zedboard)
cross=$(prj)/3rdparty/$(dev)/toolchain/gcc-arm-linux-gnueabi/bin/arm-linux-gnueabihf-
cross_bare=$(prj)/3rdparty/$(dev)/toolchain/gcc-arm-none-eabi/bin/arm-none-eabi-
host=arm-linux
arch=arm
kern_ver=4.9.0
kern_dir=$(prj)/3rdparty/$(dev)/kernel/xlnx-4.9.0
bootloader_dir=$(prj)/3rdparty/$(dev)/bootloader
kern_defconfig=avnet-zedboard_defconfig
uboot_dir=$(prj)/3rdparty/$(dev)/bootloader/u-boot-plnx
uboot_defconfig=avnet-zedboard_uboot_defconfig
dtb=avnet-zynq-zed.dtb
#apps=ble-test
uimage_loadaddr=0x8000

all:

fsbl:
	$(MAKE) CROSS_COMPILE=$(cross_bare) INSTALL_DIR=$(bootloader_dir)/../images -C $(bootloader_dir)/fsbl all; \
	$(MAKE) CROSS_COMPILE=$(cross_bare) INSTALL_DIR=$(bootloader_dir)/../images -C $(bootloader_dir)/fsbl install; \
	$(prj)/3rdparty/$(dev)/create_img.sh $(dev) create_fsbl_image;

clean_fsbl:
	$(MAKE) CROSS_COMPILE=$(cross_bare) INSTALL_DIR=$(bootloader_dir)/../images -C $(bootloader_dir)/fsbl clean; \
	if [ -f "$(prj)/3rdparty/$(dev)/images/fsbl.elf.bin" ]; then \
		rm $(prj)/3rdparty/$(dev)/images/fsbl.elf.bin; \
	fi; \
	if [ -f "$(prj)/3rdparty/$(dev)/images/zynq_fsbl.elf" ]; then \
		rm $(prj)/3rdparty/$(dev)/images/zynq_fsbl.elf; \
	fi

install_fsbl:
	$(MAKE) CROSS_COMPILE=$(cross_bare) INSTALL_DIR=$(bootloader_dir)/../images -C $(bootloader_dir)/fsbl install

endif

ifeq ($(dev),x86_64)
cross=
host=x86_64
arch=x86_64
kern_ver=4.9.0-3-amd64
kern_dir=/usr/src/linux
kern_defconfig=
apps=hub_pp
endif

all:

test_env:
	@if [ $(is_dev_ok) -eq 0 ]; \
	then \
		echo "The device is not set! Exiting!"; \
		echo "The possible devices: $(devs)"; \
		exit 1; \
	fi

get_bins:
	$(prj)/scripts/get_bins.sh $(dev)

buildroot:	test_env
	make O=$(prj)/3rdparty/$(dev)/buildroot-output LINUX_DIR=$(kern_dir) LINUX_VERSION=$(kern_ver) -C $(prj)/3rdparty/buildroot-src defconfig BR2_DEFCONFIG=$(prj)/3rdparty/$(dev)/configs/$(dev)-buildroot-defconfig
	@if [ $$? -ne 0 ]; \
		then \
		echo "Buildroot config failed!"; \
		exit 1;\
	fi;
	make O=$(prj)/3rdparty/$(dev)/buildroot-output -C $(prj)/3rdparty/buildroot-src
	@if [ $$? -ne 0 ]; \
		then \
		echo "Buildroot build failed!"; \
		exit 1;\
	fi
	cd $(prj)/scripts/; \
	pwd; \
	./create_buildroot_tarball.sh $(dev); \
	cd -

buildroot_image: buildroot
	$(prj)/3rdparty/$(dev)/create_img.sh $(dev) create_rootfs_image;

clean_buildroot:
	make O=$(prj)/3rdparty/$(dev)/buildroot-output -C $(prj)/3rdparty/buildroot-src clean
	@if [ $$? -ne 0 ]; \
	then \
		echo "Buildroot clean failed!"; \
		exit 1; \
	fi; \
	if [ -f "$(prj)/3rdparty/$(dev)/buildroot-bins-zedboard" ]; then \
		rm -rf $(prj)/3rdparty/$(dev)/buildroot-bins-zedboard; \
	fi; \
	if [ -f "$(prj)/3rdparty/$(dev)/images/zedboard-ramdisk.img" ]; then \
		rm $(prj)/3rdparty/$(dev)/images/zedboard-ramdisk.img; \
	fi; \
	if [ -f "$(prj)/3rdparty/$(dev)/images/zedboard-ramdisk.md5" ]; then \
		rm $(prj)/3rdparty/$(dev)/images/zedboard-ramdisk.md5; \
	fi

distclean_buildroot:
	make O=$(prj)/3rdparty/$(dev)/buildroot-output -C $(prj)/3rdparty/buildroot-src distclean
	if [ $$? -ne 0 ]; \
	then \
		echo "Buildroot disclean failed!"; \
		exit 1; \
	fi; \
	if [ -f "$(prj)/3rdparty/$(dev)/buildroot-bins-zedboard" ]; then \
		rm -rf $(prj)/3rdparty/$(dev)/buildroot-bins-zedboard; \
	fi; \
	if [ -f "$(prj)/3rdparty/$(dev)/images/zedboard-ramdisk.img" ]; then \
		rm $(prj)/3rdparty/$(dev)/images/zedboard-ramdisk.img; \
	fi; \
	if [ -f "$(prj)/3rdparty/$(dev)/images/zedboard-ramdisk.md5" ]; then \
		rm $(prj)/3rdparty/$(dev)/images/zedboard-ramdisk.md5; \
	fi

buildroot_config:	test_env
	make O=$(prj)/3rdparty/$(dev)/buildroot-output LINUX_DIR=$(kern_dir) LINUX_VERSION=$(kern_ver) -C $(prj)/3rdparty/buildroot-src defconfig BR2_DEFCONFIG=$(prj)/3rdparty/$(dev)/configs/$(dev)-buildroot-defconfig
	@if [ $$? -ne 0 ]; \
		then \
		echo "Buildroot config failed!"; \
		exit 1;\
	fi;
	make O=$(prj)/3rdparty/$(dev)/buildroot-output -C $(prj)/3rdparty/buildroot-src menuconfig
	@if [ $$? -ne 0 ]; \
		then \
		echo "Buildroot build failed!"; \
		exit 1;\
	fi

buildroot_saveconfig:	test_env
	make O=$(prj)/3rdparty/$(dev)/buildroot-output -C $(prj)/3rdparty/buildroot-src savedefconfig
	@if [ $$? -ne 0 ]; \
		then \
		echo "Buildroot build failed!"; \
		exit 1;\
	fi

prepare_kernel:
	if [ ! -d $(kern_dir) ]; \
	then \
		tar -xvJf $(kern_dir).xz -C $(prj)/3rdparty/$(dev)/kernel; \
	fi
	
clean_kernel:	prepare_kernel
	make ARCH=$(arch) CROSS_COMPILE=$(cross) mrproper -C $(kern_dir); \
	if [ $$? -ne 0 ]; \
	then \
		echo "Kernel clean failed!"; \
		exit 1; \
	fi; \
	if [ -f "$(prj)/3rdparty/$(dev)/images/uImage" ]; then \
		rm $(prj)/3rdparty/$(dev)/images/uImage; \
	fi; \
	if [ -f "$(prj)/3rdparty/$(dev)/images/zImage" ]; then \
		rm $(prj)/3rdparty/$(dev)/images/zImage; \
	fi; \
	if [ -f "$(prj)/3rdparty/$(dev)/images/zImage.md5" ]; then \
		rm $(prj)/3rdparty/$(dev)/images/zImage.md5; \
	fi; \
	if [ -f "$(prj)/3rdparty/$(dev)/images/modules.tgz" ]; then \
		rm $(prj)/3rdparty/$(dev)/images/modules.tgz; \
	fi; \
	if [ -f "$(prj)/3rdparty/$(dev)/images/zynq-zed.dtb" ]; then \
		rm $(prj)/3rdparty/$(dev)/images/zynq-zed.dtb; \
	fi; \
	if [ -f "$(prj)/3rdparty/$(dev)/images/zynq-zed.md5" ]; then \
		rm $(prj)/3rdparty/$(dev)/images/zynq-zed.md5; \
	fi

kernel_config:	prepare_kernel
	@cd $(kern_dir); \
	cp $(prj)/3rdparty/$(dev)/configs/$(kern_defconfig) arch/$(arch)/configs; \
	make -j4 LOCALVERSION= ARCH=$(arch) CROSS_COMPILE=$(cross) $(kern_defconfig); \
	if [ $$? -ne 0 ]; \
	then \
		echo "Kernel create config failed!"; \
		exit 1; \
	fi; \
	make -j4 LOCALVERSION= ARCH=$(arch) CROSS_COMPILE=$(cross) menuconfig; \
	if [ $$? -ne 0 ]; \
	then \
		echo "Kernel create config failed!"; \
		exit 1; \
	fi

kernel_saveconfig:	prepare_kernel
	@cd $(kern_dir); \
	make -j4 LOCALVERSION= ARCH=$(arch) CROSS_COMPILE=$(cross) savedefconfig; \
	if [ $$? -ne 0 ]; \
	then \
		echo "Kernel create config failed!"; \
		exit 1; \
	fi; \
	cp defconfig $(prj)/3rdparty/$(dev)/configs/$(kern_defconfig)

kernel:		prepare_kernel
	@cd $(kern_dir); \
	cp $(prj)/3rdparty/$(dev)/configs/$(kern_defconfig) arch/$(arch)/configs; \
	make -j4 LOCALVERSION= ARCH=$(arch) CROSS_COMPILE=$(cross) $(kern_defconfig); \
	if [ $$? -ne 0 ]; \
	then \
		echo "Kernel create config failed!"; \
		exit 1; \
	fi; \
	make -j4 LOCALVERSION= LOADADDR=$(uimage_loadaddr) ARCH=$(arch) CROSS_COMPILE=$(cross); \
	if [ $$? -ne 0 ]; \
	then \
		echo "Kernel build failed!"; \
		exit 1; \
	fi; \
	make -j4 LOCALVERSION= LOADADDR=$(uimage_loadaddr) ARCH=$(arch) CROSS_COMPILE=$(cross) uImage; \
	if [ $$? -ne 0 ]; \
	then \
		echo "Kernel build failed!"; \
		exit 1; \
	fi; \
	make -j4 LOCALVERSION= ARCH=$(arch) CROSS_COMPILE=$(cross) dtbs; \
	if [ $$? -ne 0 ]; \
	then \
		echo "Kernel build failed!"; \
		exit 1; \
	fi; \
	make -j4 LOCALVERSION= ARCH=$(arch) CROSS_COMPILE=$(cross) modules; \
	if [ $$? -ne 0 ]; \
	then \
		echo "Kernel build modules failed!"; \
		exit 1; \
	fi; \
	make -j4 LOCALVERSION= ARCH=$(arch) CROSS_COMPILE=$(cross) INSTALL_MOD_PATH=$(prj)/3rdparty/$(dev)/images modules_install; \
	if [ $$? -ne 0 ]; \
	then \
		echo "Kernel install modules failed!"; \
		exit 1; \
	fi; \
	cp arch/$(arch)/boot/zImage $(prj)/3rdparty/$(dev)/images; \
	cp arch/$(arch)/boot/uImage $(prj)/3rdparty/$(dev)/images; \
	cp arch/$(arch)/boot/dts/$(dtb) $(prj)/3rdparty/$(dev)/images/zynq-zed.dtb; \
	if [ -d arch/$(arch)/boot/dts/overlays ]; \
	then \
		cd arch/$(arch)/boot/dts/overlays/ && tar -czvf $(prj)/3rdparty/$(dev)/images/overlays.tgz *.dtbo && cd -; \
	fi; \
	tar -czvf $(prj)/3rdparty/$(dev)/images/modules.tgz -C $(prj)/3rdparty/$(dev)/images/lib modules; \
	rm -rf $(prj)/3rdparty/$(dev)/images/lib; \
	$(prj)/3rdparty/$(dev)/create_img.sh $(dev) create_kernel_image; \
	$(prj)/3rdparty/$(dev)/create_img.sh $(dev) create_dtb_image;
	cd -

prepare_kernel_headers: clean_kernel kernel
	@patch -f -d $(prj)/3rdparty/$(dev)/kernel/linux* -p1 < $(prj)/scripts/modules_headers_install.patch; \
	cd $(prj)/3rdparty/$(dev)/kernel/linux*; \
	make ARCH=$(arch) CROSS_COMPILE=$(cross) INSTALL_MODULES_HDR_PATH=$(prj)/3rdparty/$(dev)/kernel/kernel_headers modules_headers_install; \
	if [ $$? -ne 0 ]; \
	then \
		echo "Kernel create_headers failed!"; \
		exit 1; \
	fi; \
	cd -

clean_u-boot:
	make ARCH=${arch} CROSS_COMPILE=$(cross) mrproper -C $(uboot_dir); \
	if [ $$? -ne 0 ]; \
	then \
		echo "U-boot clean failed!"; \
		exit 1; \
	fi; \
	if [ -f "$(prj)/3rdparty/$(dev)/images/uboot.bin" ]; then \
		rm $(prj)/3rdparty/$(dev)/images/uboot.bin; \
	fi; \
	if [ -f "$(prj)/3rdparty/$(dev)/images/u-boot.bin" ]; then \
		rm $(prj)/3rdparty/$(dev)/images/u-boot.bin; \
	fi; \
	if [ -f "$(prj)/3rdparty/$(dev)/images/u-boot.elf" ]; then \
		rm $(prj)/3rdparty/$(dev)/images/u-boot.elf; \
	fi

u-boot_config:
	@cd $(uboot_dir); \
	cp $(prj)/3rdparty/$(dev)/configs/$(uboot_defconfig) configs; \
	make ARCH=${arch} CROSS_COMPILE=$(cross) $(uboot_defconfig) menuconfig; \
	if [ $$? -ne 0 ]; \
	then \
        	echo "U-boot config failed!"; \
        	exit 1; \
	fi; \
	cd -

u-boot_saveconfig:
	@cd $(uboot_dir); \
	make ARCH=${arch} CROSS_COMPILE=$(cross) savedefconfig; \
	if [ $$? -ne 0 ]; \
	then \
        	echo "U-boot create config failed!"; \
        	exit 1; \
	fi; \
	cp defconfig $(prj)/3rdparty/$(dev)/configs/$(uboot_defconfig); \
	cd -

u-boot:
	@cd $(uboot_dir); \
	cp $(prj)/3rdparty/$(dev)/configs/$(uboot_defconfig) configs; \
	make ARCH=${arch} CROSS_COMPILE=$(cross) $(uboot_defconfig); \
	if [ $$? -ne 0 ]; \
    then \
        echo "U-boot config failed!"; \
        exit 1; \
    fi; \
    make CROSS_COMPILE=$(cross) all; \
    if [ $$? -ne 0 ]; \
    then \
        echo "U-boot build failed!"; \
        exit 1; \
    fi; \
	cp u-boot.bin $(prj)/3rdparty/$(dev)/images; \
	cp u-boot $(prj)/3rdparty/$(dev)/images/u-boot.elf; \
	$(cross)strip -g $(prj)/3rdparty/$(dev)/images/u-boot.elf; \
	$(prj)/3rdparty/$(dev)/create_img.sh $(dev) create_uboot_image; \
	cd -

rootfs:
	$(prj)/3rdparty/$(dev)/create_img.sh $(dev) create_rootfs_image;

clean_rootfs:	
	if [ -f "$(prj)/3rdparty/$(dev)/images/zedboard-ramdisk.img" ]; then \
		rm $(prj)/3rdparty/$(dev)/images/zedboard-ramdisk.img; \
	fi; \
	if [ -f "$(prj)/3rdparty/$(dev)/images/zedboard-ramdisk.md5" ]; then \
		rm $(prj)/3rdparty/$(dev)/images/zedboard-ramdisk.md5; \
	fi

clean_all: clean_fsbl clean_u-boot clean_kernel clean_buildroot distclean_buildroot

clean_images:
	if [ -f "$(prj)/3rdparty/$(dev)/images/fsbl.elf.bin" ]; then \
		rm $(prj)/3rdparty/$(dev)/images/fsbl.elf.bin; \
	fi; \
	if [ -f "$(prj)/3rdparty/$(dev)/images/zynq_fsbl.elf" ]; then \
		rm $(prj)/3rdparty/$(dev)/images/zynq_fsbl.elf; \
	fi; \
	if [ -f "$(prj)/3rdparty/$(dev)/images/uboot.bin" ]; then \
		rm $(prj)/3rdparty/$(dev)/images/uboot.bin; \
	fi; \
	if [ -f "$(prj)/3rdparty/$(dev)/images/u-boot.bin" ]; then \
		rm $(prj)/3rdparty/$(dev)/images/u-boot.bin; \
	fi; \
	if [ -f "$(prj)/3rdparty/$(dev)/images/u-boot.elf" ]; then \
		rm $(prj)/3rdparty/$(dev)/images/u-boot.elf; \
	fi; \
	if [ -f "$(prj)/3rdparty/$(dev)/images/uImage" ]; then \
		rm $(prj)/3rdparty/$(dev)/images/uImage; \
	fi; \
	if [ -f "$(prj)/3rdparty/$(dev)/images/zImage" ]; then \
		rm $(prj)/3rdparty/$(dev)/images/zImage; \
	fi; \
	if [ -f "$(prj)/3rdparty/$(dev)/images/zImage.md5" ]; then \
		rm $(prj)/3rdparty/$(dev)/images/zImage.md5; \
	fi; \
	if [ -f "$(prj)/3rdparty/$(dev)/images/modules.tgz" ]; then \
		rm $(prj)/3rdparty/$(dev)/images/modules.tgz; \
	fi; \
	if [ -f "$(prj)/3rdparty/$(dev)/images/zynq-zed.dtb" ]; then \
		rm $(prj)/3rdparty/$(dev)/images/zynq-zed.dtb; \
	fi; \
	if [ -f "$(prj)/3rdparty/$(dev)/images/zynq-zed.md5" ]; then \
		rm $(prj)/3rdparty/$(dev)/images/zynq-zed.md5; \
	fi; \
	if [ -f "$(prj)/3rdparty/$(dev)/images/zedboard-ramdisk.img" ]; then \
		rm $(prj)/3rdparty/$(dev)/images/zedboard-ramdisk.img; \
	fi; \
	if [ -f "$(prj)/3rdparty/$(dev)/images/zedboard-ramdisk.md5" ]; then \
		rm $(prj)/3rdparty/$(dev)/images/zedboard-ramdisk.md5; \
	fi; \