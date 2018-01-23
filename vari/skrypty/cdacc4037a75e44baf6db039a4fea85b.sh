#!/bin/sh

if [ -e "/var/volatile/run/media/" ]
then
for MOUNTED in `ls  /var/volatile/run/media/`
do
#uruchom konfiguracje sieci WIFI
if [ -e "/var/volatile/run/media/"$MOUNTED"/setwifi.sh"  ]
then
. /var/volatile/run/media/$MOUNTED/setwifi.sh 
rm /var/volatile/run/media/$MOUNTED/setwifi.sh 
fi
#podmien plik satrtowy
if [ -e "/var/volatile/run/media/"$MOUNTED"/start_bb.sh"  ]
then
cp /var/volatile/run/media/$MOUNTED/start_bb.sh /home/root
chmod a+rwx /home/root/start_bb.sh
rm /var/volatile/run/media/$MOUNTED/start_bb.sh 
fi

#podmien bb_controller
if [ -e "/var/volatile/run/media/"$MOUNTED"/bb_controller"  ]
then
cp /var/volatile/run/media/$MOUNTED/bb_controller /home/root
chmod a+rwx /home/root/bb_controller
rm /var/volatile/run/media/$MOUNTED/bb_controller 
fi

#podmien bb_player
if [ -e "/var/volatile/run/media/"$MOUNTED"/bb_player"  ]
then
cp /var/volatile/run/media/$MOUNTED/bb_player /home/root
chmod a+rwx /home/root/bb_player
rm /var/volatile/run/media/$MOUNTED/bb_player 
fi


#aktualizuj FW
if [ -e "/var/volatile/run/media/"$MOUNTED"/flash.sh"  ]
then
/var/volatile/run/media/$MOUNTED/flash.sh /var/volatile/run/media/$MOUNTED
fi


#Wygeneruj plik diagnstyczny 
date >> /var/volatile/run/media/$MOUNTED/diag.txt
cat /home/root/start_bb.sh >> /var/volatile/run/media/$MOUNTED/diag.txt
ifconfig -a >> /var/volatile/run/media/$MOUNTED/diag.txt
iw wlan0 scan >> /var/volatile/run/media/$MOUNTED/diag.txt
iw wlan0 info >> /var/volatile/run/media/$MOUNTED/diag.txt
dmesg >> /var/volatile/run/media/$MOUNTED/diag.txt

. /var/volatile/run/media/$MOUNTED/delayed_diag.sh /var/volatile/run/media/$MOUNTED &
done
fi 



