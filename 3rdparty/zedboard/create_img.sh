#!/bin/bash

dev=$1
top=$(dirname -- "$0")/../../
currdir=$(dirname -- "$0")
builddir=$currdir/buildimg
mkimage=/usr/bin/mkimage
images=$currdir/images
image=$builddir/$dev-$(date +%F)-initramfs.cpio.gz
#ubootImage=$builddir/$dev-$(date +%F)-ramdisk.img
ubootImage=$builddir/$dev-ramdisk.img
rootfs="${builddir}/rootfs"

prepare_bin_images()
{
#	rm -rf $currdir/images/fsbl.bin
#	$currdir/toolchain/bin/bootgen -arch zynq -image $currdir/images/fsbl.bif -split bin
#	mv u-boot.elf.bin $currdir/images
#	mv fpga.bit.bin $currdir/images
	rm -rf $currdir/images/fsbl.elf.bin
	rm -rf $currdir/images/fpga.bin
	rm -rf $currdir/images/uboot.bin
	rm -rf $currdir/images/avnet-zynq-zed.md5
	rm -rf $currdir/images/zImage.md5
	rm -rf $currdir/images/zedboard-ramdisk.md5
        $currdir/toolchain/bin/bootgen -arch zynq -image $currdir/images/fsbl.bif -o $currdir/images/fsbl.elf.bin -w on
	$currdir/toolchain/bin/bootgen -arch zynq -image $currdir/images/fpga.bif -o $currdir/images/fpga.bin -w on
	$currdir/toolchain/bin/bootgen -arch zynq -image $currdir/images/u-boot.bif -o $currdir/images/uboot.bin -w on
	#mv fsbl.elf.bin $currdir/images
	#mv u-boot.elf.bin $currdir/images
}

prepare_rootfs()
{
	mkdir -p $rootfs
	tar -zxvf $currdir/buildroot-bins-$dev/rootfs.tar.gz -C $rootfs
	tar -zxvf $images/modules.tgz -C $rootfs/lib/
	cp -a $top/overlay/fs-$dev/* $rootfs
}

create_image()
{
	cd $rootfs
	pwd
	ls
	echo "image:$image"
	find . -print0 | cpio --null --create --verbose --format=newc > $image
	$mkimage -n 'Ramdisk Image' -A arm -O linux -T ramdisk -C gzip -d $image $ubootImage
	cd -
	mv $ubootImage $images/
	rm -rf $builddir
}

create_md5()
{
	md5_dtb=($(md5sum $currdir/images/zynq-zed.dtb))
	echo $md5_dtb > $currdir/images/zynq-zed.md5
	md5_kernel=($(md5sum $currdir/images/zImage))
	echo $md5_kernel > $currdir/images/zImage.md5
	md5_ramdsk=($(md5sum $currdir/images/zedboard-ramdisk.img))
	echo $md5_ramdsk > $currdir/images/zedboard-ramdisk.md5
}

create_fsbl_md5()
{
	echo "creating FSBL"
	rm -rf $currdir/images/fsbl.elf.bin
        $currdir/toolchain/bin/bootgen -arch zynq -image $currdir/images/fsbl.bif -o $currdir/images/fsbl.elf.bin -w on
}

create_uboot_md5()
{
	echo "creating uboot"
	rm -rf $currdir/images/uboot.bin
	$currdir/toolchain/bin/bootgen -arch zynq -image $currdir/images/u-boot.bif -o $currdir/images/uboot.bin -w on
}

create_kernel_md5()
{	
	echo "creating kernel"
	rm -rf $currdir/images/zImage.md5
	md5_kernel=($(md5sum $currdir/images/zImage))
	echo $md5_kernel > $currdir/images/zImage.md5
}

create_dtb_md5()
{
	echo "creating dtb"
	rm -rf $currdir/images/zynq-zed.md5
	md5_dtb=($(md5sum $currdir/images/zynq-zed.dtb))
	echo $md5_dtb > $currdir/images/zynq-zed.md5
}

create_rootfs_md5()
{
	echo "creating rootfs"
	prepare_rootfs
	create_image
	md5_ramdsk=($(md5sum $currdir/images/zedboard-ramdisk.img))
	echo $md5_ramdsk > $currdir/images/zedboard-ramdisk.md5
}

if [ "$2" ]; then
	if [ "$2" == "create_fsbl_image" ];then
		create_fsbl_md5
	elif [ "$2" == "create_uboot_image" ];then
		create_uboot_md5
	elif [ "$2" == "create_kernel_image" ];then
		create_kernel_md5
	elif [ "$2" == "create_dtb_image" ];then
		create_dtb_md5
	elif [ "$2" == "create_rootfs_image" ];then
		create_rootfs_md5
	fi
else
	prepare_bin_images
	prepare_rootfs
	create_image
	create_md5
fi

