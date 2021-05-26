#!/bin/sh

killall wpa_supplicant > /dev/null 2>&1
killall udhcpc > /dev/null 2>&1
ssid=
psk=
echo "Please enter network SSID"
read ssid
echo "Please enter password to net:$ssid"
read psk
/usr/sbin/wpa_passphrase $ssid $psk >> /etc/wpa_supplicant.conf

/usr/sbin/wpa_supplicant -B -i wlan0 -c /etc/wpa_supplicant.conf
sleep 2 
udhcpc -i wlan0
