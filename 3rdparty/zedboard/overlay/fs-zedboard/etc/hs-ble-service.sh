#!/bin/sh

EXE="/opt/zedboard/bin/ble-service"
IP="127.0.0.1"

daemonize()
{
	while [ 1 ]; do
		echo 0 > /sys/class/gpio/gpio959/value
		sleep 1
		echo 1 > /sys/class/gpio/gpio959/value
		${EXE} -i ${IP} -t 60 -r
	done
}

daemonize &
