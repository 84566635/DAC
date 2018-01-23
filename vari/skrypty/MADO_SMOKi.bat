@echo off
echo y | plink -P %2 -pw ""  root@%1 "cat /dev/null > /etc/resolv.conf ; exit"  
echo .
echo Wysylanie plikow do VARI
pscp -P %2 -pw "" MADO_SMOKi.sh root@%1:.
pscp -P %2 -pw "" bb_flash root@%1:.
pscp -P %2 -pw "" %4 root@%1:.
echo Programowanie MADO/SMOK
plink -P %2 -pw ""  root@%1 ". ./MADO_SMOKi.sh %3 %4 &> MADO_SMOKi.log" 
echo Odbieranie logu
pscp -P %2 -pw ""  root@%1:MADO_SMOKi.log .
