@echo off
devcon find "USB\VID_0483&PID_3748*"
pause
set ROOT=q:\projects\INFADO__0001branch
set COMPORT=COM5
set STLINK_SMOK1="@USB\VID_0483&PID_3748\*3"
set STLINK_SMOK2="@USB\VID_0483&PID_3748\*4"
set STLINK_MADO="@USB\VID_0483&PID_3748\*1"
set STLINK_ALL="@USB\VID_0483&PID_3748\*"
set STLINK="ST-LINK_CLI.exe"
set GDBSERVER="st-util.exe"



title Device FW update and sending commands
devcon disable %STLINK_ALL%
:home
rem set MADO_FILE=old_96_protoMS_MA.hex
set MADO_FILE=SS_MA.hex        
set SMOK_FILE=SMOK.hex
mode %COMPORT% baud=115200 parity=n data=8 >nul
cls
echo.
echo Select a device:
echo =============
echo.
echo 0) exit
echo 1) FW-SMOK1
echo 2) FW-SMOK2
echo 3) FW-MADO
echo 4) FW-ALL
echo 5) FW-SMOKI
echo 6) RESET-ALL
echo 7) GDB-SMOK1
echo 8) GDB-SMOK2
echo 9) GDB-MADO
echo.
echo or select a command:
echo =============
echo q) CMD On  0x50
echo w) CMD Off 0x52
echo.
set /p ch=Type option:
if "%ch%"=="0" goto ex
if "%ch%"=="1" goto program
if "%ch%"=="2" goto program
if "%ch%"=="3" goto program
if "%ch%"=="4" goto program
if "%ch%"=="5" goto program
if "%ch%"=="6" goto reset
if "%ch%"=="7" goto gdb_smok1
if "%ch%"=="8" goto gdb_smok2
if "%ch%"=="9" goto gdb_mado
if "%ch%"=="q" type CMD_on > %COMPORT%
if "%ch%"=="w" type CMD_off > %COMPORT%
goto home

:program
copy %ROOT%\project_smok\%SMOK_FILE% .
copy %ROOT%\project_ma\%MADO_FILE% .
if "%ch%"=="1" goto smok1
if "%ch%"=="2" goto smok2
if "%ch%"=="3" goto mado
if "%ch%"=="4" goto all
if "%ch%"=="5" goto smoki

:ex
devcon enable %STLINK_ALL%
exit

rem =====================================================================
:gdb_smok1
echo SMOK1
devcon.exe enable %STLINK_SMOK1%
%GDBSERVER%
devcon.exe disable %STLINK_SMOK1%
goto home

rem =====================================================================
:gdb_smok2
echo SMOK2
devcon.exe enable %STLINK_SMOK2%
%GDBSERVER%
devcon.exe disable %STLINK_SMOK2%
goto home

rem =====================================================================
:gdb_mado
echo MADO
devcon.exe enable %STLINK_MADO%
%GDBSERVER%
devcon.exe disable %STLINK_MADO%
goto home

rem =====================================================================
:smok1
echo SMOK1 %SMOK_FILE%
devcon.exe enable %STLINK_SMOK1%
%STLINK%  -c SWD UR -P %SMOK_FILE% -V -Rst
devcon.exe disable %STLINK_SMOK1%
goto home

rem =====================================================================
:smok2
echo SMOK2 %SMOK_FILE%
devcon.exe enable %STLINK_SMOK2%
%STLINK% -c SWD UR -P %SMOK_FILE% -V -Rst
devcon.exe disable %STLINK_SMOK2%
goto home

rem =====================================================================
:mado
echo MADO %MADO_FILE%
devcon.exe enable %STLINK_MADO%
%STLINK% -c SWD UR -P %MADO_FILE% -V -Rst
devcon.exe disable %STLINK_MADO%
goto home

rem =====================================================================
:all
devcon.exe enable %STLINK_MADO%
%STLINK% -c SWD UR -P %MADO_FILE% -V -Rst
devcon.exe disable %STLINK_MADO%
:smoki
devcon.exe enable %STLINK_SMOK1%
%STLINK% -c SWD UR -P %SMOK_FILE% -V -Rst
devcon.exe disable %STLINK_SMOK1%

devcon.exe enable %STLINK_SMOK2%
%STLINK% -c SWD UR -P %SMOK_FILE% -V -Rst
devcon.exe disable %STLINK_SMOK2%

goto home
:reset
devcon.exe enable %STLINK_SMOK1%
%STLINK%  -Rst
devcon.exe disable %STLINK_SMOK1%
:smoki
devcon.exe enable %STLINK_SMOK2%
%STLINK% -Rst
devcon.exe disable %STLINK_SMOK2%

devcon.exe enable %STLINK_MADO%
%STLINK% -Rst
devcon.exe disable %STLINK_MADO%
goto home
