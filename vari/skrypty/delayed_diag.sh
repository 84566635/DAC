#!/bin/sh
echo delayed diag >> $1/delayed_diag.txt

sleep 60

date >> $1/delayed_diag.txt
ifconfig -a >> $1/delayed_diag.txt
iw wlan0 scan >> $1/delayed_diag.txt
iw wlan0 info >> $1/delayed_diag.txt

sleep 60

date >> $1/delayed_diag.txt
ifconfig -a >> $1/delayed_diag.txt
iw wlan0 scan >> $1/delayed_diag.txt
iw wlan0 info >> $1/delayed_diag.txt

sleep 60

date >> $1/delayed_diag.txt
ifconfig -a >> $1/delayed_diag.txt
iw wlan0 scan >> $1/delayed_diag.txt
iw wlan0 info >> $1/delayed_diag.txt

dmesg >> $1/delayed_diag.txt