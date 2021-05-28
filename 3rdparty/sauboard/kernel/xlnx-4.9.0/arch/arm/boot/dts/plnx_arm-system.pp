# 1 "/home/yan/source/hiSky/avnet-digilent-zedboard-2017.4/build/../components/plnx_workspace/device-tree/device-tree-generation/system-top.dts"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "/home/yan/source/hiSky/avnet-digilent-zedboard-2017.4/build/../components/plnx_workspace/device-tree/device-tree-generation/system-top.dts"







/dts-v1/;
/include/ "zynq-7000.dtsi"
/include/ "zedboard.dtsi"
/include/ "pl.dtsi"
/include/ "pcw.dtsi"
/ {
 chosen {
  bootargs = "earlycon";
  stdout-path = "serial0:115200n8";
 };
 aliases {
  ethernet0 = &gem0;
  serial0 = &uart1;
  spi0 = &qspi;
 };
 memory {
  device_type = "memory";
  reg = <0x0 0x20000000>;
 };
};
# 1 "/home/yan/source/hiSky/avnet-digilent-zedboard-2017.4/build/tmp/work/plnx_arm-xilinx-linux-gnueabi/device-tree-generation/xilinx+gitAUTOINC+3c7407f6f8-r0/system-user.dtsi" 1
/include/ "system-conf.dtsi"
/ {
};
# 28 "/home/yan/source/hiSky/avnet-digilent-zedboard-2017.4/build/../components/plnx_workspace/device-tree/device-tree-generation/system-top.dts" 2
