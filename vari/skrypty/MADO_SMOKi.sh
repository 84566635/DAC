#!/bin/bash
chmod a+rwx bb_flash
#zachowaj bb_controller z inna nazwa aby uniemozliwc jego automatyczne uruchomienie
mv bb_controller bb_controller_ 
#wylacz bb_controller
killall bb_controller
#wyslij obraz SMOK'a/MADO do MADO->SMOK(i) 
./bb_flash STM $1 $2 /dev/ttyO3 
#przwroc bb_controller. Po tym w ciagu 1s sam sie uruchomi
mv bb_controller_ bb_controller 
