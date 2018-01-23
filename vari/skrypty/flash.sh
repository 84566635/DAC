#!/bin/sh

# przykladowe wywolania 
#stty  -F /dev/ttyO3 0:0:18b2:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0
# zaprogramuj SMOKa pierwszego (maska urzadzen jest odana jako 8 binarnych wartosci . Tutaj 00000001. Drugi SMOK bylby 00000010 itd. Mozna programowac wiele na raz. 
# ponizsza linie nalezy odkomentowac i uzpelnic o dpowiedni plik  (SMOK.bin)
# $1/bb_flash STM 00000001 $1/SMOK.bin /dev/ttyO3 >> $1/flash_log.txt

# zaprogramuj MADO (maska SMOKow = 00000000)
# $1/bb_flash STM 00000000 $1/MA.bin /dev/ttyO3 >> $1/flash_log.txt

# zaprogramuj sharc'a na pierwszym SMOKu
# $1/bb_flash SHARC 00000001 $1/sharc.bin /dev/ttyO3 >> $1/flash_log.txt

