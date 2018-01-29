@echo off
@cd /D %~dp0

set REL_PATH=%~dp0

set PATH=%REL_PATH%\gnuwin32\bin\;%REL_PATH%\GNU_Tools_ARM_Embedded\bin\;%PATH%
set GCCPATH=%REL_PATH%\GNU_Tools_ARM_Embedded\bin\
set ECHO=%REL_PATH%\gnuwin32\bin\echo.exe
set HEX2BIN=%REL_PATH%\gnuwin32\bin\hex2bin.exe
set XXD=%REL_PATH%\gnuwin32\bin\xxd.exe
rd /s /q project_ma/*.bin project_smok/*.bin 
set VER=%1

echo VER %VER% 
@echo %cd%


cd libusbaudio
@echo %cd%
make clean
make -j6
cd ..
cd SMOK 
make PROJNAME=SMOK clean 
make -j6  PROJNAME=SMOK RADIO_IMAGE=../PurePathProject/SLAVE.hex  VERSION=VER 
cd ..

cd MADO 
make PROJNAME=MA clean 
make -j6  PROJNAME=MA  RADIO_IMAGE_MSTM=../PurePathProject/MASTER_MULTI_SLAVE_tam.hex RADIO_IMAGE_MSTS=../PurePathProject/MASTER_MULTI_SLAVE_tas.hex RADIO_IMAGE_SSTM=../PurePathProject/MASTER_SINGLE_SLAVE_tam.hex RADIO_IMAGE_SSTS=../PurePathProject/MASTER_SINGLE_SLAVE_tas.hex VERSION=VER CONVERT_DBG=y
cd ..    
cd MADO 
make PROJNAME=MA44 clean 
make -j6  PROJNAME=MA44 RADIO_IMAGE_MSTM=../PurePathProject/MASTER_MULTI_SLAVE44_tam.hex RADIO_IMAGE_MSTS=../PurePathProject/MASTER_MULTI_SLAVE44_tas.hex RADIO_IMAGE_SSTM=../PurePathProject/MASTER_SINGLE_SLAVE_tam.hex RADIO_IMAGE_SSTS=../PurePathProject/MASTER_SINGLE_SLAVE_tas.hex VERSION=VER CONVERT_DBG=y
cd ..    
