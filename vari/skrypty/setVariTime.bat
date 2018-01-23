@echo off
for /F "usebackq tokens=1,2 delims==" %%i in (`wmic os get LocalDateTime /VALUE 2^>NUL`) do if '.%%i.'=='.LocalDateTime.' set ldt=%%j
set ldt1=%ldt:~0,4%-%ldt:~4,2%-%ldt:~6,2% %ldt:~8,2%:%ldt:~10,2%:%ldt:~12,6%
echo Setting Vari time to %ldt1%
set ldt2=%ldt:~4,2%%ldt:~6,2%%ldt:~8,2%%ldt:~10,2%%ldt:~0,4%.%ldt:~12,2%
plink -P %2 -pw ""  root@%1 date %ldt2%