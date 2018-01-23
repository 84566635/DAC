@echo off
echo y | plink -P %2 -pw ""  root@%1 "cat /dev/null > /etc/resolve.conf"  
echo .
echo Wysylanie plikow do VARI
pscp -P %2 -pw "" start_bb.sh root@%1:.
pscp -P %2 -pw "" mount.sh root@%1:/etc/udev/scripts/
pscp -P %2 -pw "" bb_controller root@%1:bb_controller_new
pscp -P %2 -pw "" bb_player root@%1:bb_player_new
plink -P %2 -pw ""  root@%1 "rm bb_player; rm bb_controller; chmod a+x *.sh bb_player* bb_controller* "  
plink -P %2 -pw ""  root@%1 "killall bb_player; killall bb_controller; mv bb_controller_new bb_controller; mv bb_player_new bb_player"  
