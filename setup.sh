#!/bin/bash

HISKY_SDK_PATH=`pwd`
sed -i 's;HISKY_SDK=.*;HISKY_SDK='"$HISKY_SDK_PATH"';' $HISKY_SDK_PATH/3rdparty/zedboard/burn_flash.sh

PETALINUX_BIN_PATH=$(locate -b 'petalinux/tools/common/petalinux/bin/petalinux-build-x')
PETALINUX_SDK_PATH=$(echo ${PETALINUX_BIN_PATH%/tools/common/petalinux/bin/petalinux-build})
sed -i 's;PETALINUX_SDK=.*;PETALINUX_SDK='"${PETALINUX_BIN_PATH%/tools/common/petalinux/bin/petalinux-build}"';' $HISKY_SDK_PATH/3rdparty/zedboard/burn_flash.sh
sed -i 's;BR2_TOOLCHAIN_EXTERNAL_PATH=.*;BR2_TOOLCHAIN_EXTERNAL_PATH='"$HISKY_SDK_PATH/3rdparty/zedboard/toolchain/gcc-arm-linux-gnueabi"';' $HISKY_SDK_PATH/3rdparty/zedboard/configs/zedboard-buildroot-defconfig

PATH="${HISKY_SDK_PATH}/3rdparty/zedboard/toolchain/gcc-arm-none-eabi/bin:${HISKY_SDK_PATH}/3rdparty/zedboard/toolchain/gcc-arm-linux-gnueabi/bin:${PATH}"
export PATH

sed -i 's;/.*/3rdparty/zedboard/images/fpga.bit;'"$HISKY_SDK_PATH/3rdparty/zedboard/images/fpga.bit"';' $HISKY_SDK_PATH/3rdparty/zedboard/images/fpga.bif
sed -i 's;/.*/3rdparty/zedboard/images/zynq_fsbl.elf;'"$HISKY_SDK_PATH/3rdparty/zedboard/images/zynq_fsbl.elf"';' $HISKY_SDK_PATH/3rdparty/zedboard/images/fsbl.bif
sed -i 's;/.*/3rdparty/zedboard/images/u-boot.elf;'"$HISKY_SDK_PATH/3rdparty/zedboard/images/u-boot.elf"';' $HISKY_SDK_PATH/3rdparty/zedboard/images/u-boot.bif

