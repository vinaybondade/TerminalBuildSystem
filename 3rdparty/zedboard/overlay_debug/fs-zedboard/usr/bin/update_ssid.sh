#!/bin/sh

passwd=$(strings /dev/mtd1 | awk '/p>/ {gsub("<.>",""); gsub("</p>.*",""); print $1}')
if [  $passwd ]
then
	sed "s/^wpa_passphrase=.*/wpa_passphrase=$passwd/g" -i /etc/hostapd.conf
fi

ssid=$(strings /dev/mtd1 | awk '/s>/ {gsub(".*<s>",""); gsub("</s>.*",""); print $1}')
if [ $ssid ]
then
	sed "s/^ssid=.*/ssid=$ssid/g" -i /etc/hostapd.conf
fi


