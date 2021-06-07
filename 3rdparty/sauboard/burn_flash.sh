#!/bin/bash
HISKY_SDK=/home/vinay/Workspace/TerminalBuildSystem
PETALINUX_SDK=

FLASHUTIL=$PETALINUX_SDK/tools/hsm/bin/program_flash
IMGS_DIR=$HISKY_SDK/3rdparty/zedboard/images
FSBL_JTAG=$IMGS_DIR/zynq_fsbl.elf.jtag
FLASHOPT="-flash_type qspi_single -verify -cable type xilinx_tcf url TCP:localhost:3121"


