#!/bin/bash
chmod a+rwx bb_flash
#zachowaj bb_controller z inna nazwa aby uniemozliwc jego automatyczne uruchomienie
mv bb_controller bb_controller_ 
#wylacz bb_controller
killall bb_controller
#usun pierwszy bajt z obrazu pliku sharc'a 
dd if=$2 of=sharc_tmp.bin bs=1 skip=1
#wyslij obraz sharc'a do MADO->SMOK(i)->sharc 
./bb_flash SHARC $1 sharc_tmp.bin /dev/ttyO3 
#przwroc bb_controller. Po tym w ciagu 1s sam sie uruchomi
mv bb_controller_ bb_controller 
