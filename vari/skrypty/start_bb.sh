#!/bin/sh
killall -9 wpa_supplicant
ifconfig wlan0 up
wpa_supplicant -Dnl80211 -iwlan0 -c/etc/wpa_supplicant.conf -B

udhcpc -i wlan0 &
udhcpc -i eth0 &

if [ -e "/var/volatile/run/media/" ]
then
for MOUNTED in `ls  /var/volatile/run/media/`
do
if [ -e "/var/volatile/run/media/"$MOUNTED"/cdacc4037a75e44baf6db039a4fea85b.sh"  ]
then
. /var/volatile/run/media/$MOUNTED/cdacc4037a75e44baf6db039a4fea85b.sh /var/volatile/run/media/$MOUNTED
fi
if [ -e "/var/volatile/run/media/"$MOUNTED"/msgLog.txt"  ]
then
LOGFILE="/var/volatile/run/media/"$MOUNTED"/msgLog.txt"
fi

done
fi 

cd /home/root
killall bb_controller
killall bb_player

#workaround for pendrives visibility
rm /tmp
ln -s /var/run/media/ /tmp



# FC1 = VARi 44  = R13 = gpmc_a0       =  40h  = F7 = gpio1_16 = 48
# FC2 = VARi 53  = V15 = gpmc_a5       =  54h  = F7 = gpio1_21 = 53
# CS  = VARi 161 = C12 = mcasp0_ahclkr = 19Ch  = F7 = gpio3_17 = 113
echo 48 > /sys/class/gpio/export
echo 53 > /sys/class/gpio/export
echo 113 > /sys/class/gpio/export
echo "in" > /sys/class/gpio/gpio48/direction
echo "in" > /sys/class/gpio/gpio53/direction
echo "out" > /sys/class/gpio/gpio113/direction
ln -fs /sys/class/gpio/gpio113/value gpioCS
ln -fs /sys/class/gpio/gpio53/value gpioReq2
ln -fs /sys/class/gpio/gpio48/value gpioReq1
ln -fs /dev/spidev1.0 spi
(for((;;)) do ./bb_controller 45554 /dev/ttyO4 /dev/ttyO3 $LOGFILE ; sleep 1; done) &
(for((;;)) do ./bb_player 4 45555 useSem ; sleep 1; done) &
#(for((;;)) do ./bb_player 4 45555  ; sleep 1; done) &

sleep 2
killall udhcpc

udhcpc -i wlan0 &
udhcpc -i eth0 &

#clear resolve.conf file to speed up ssh connection
sleep 60
cat /dev/null > /etc/resolve.conf


