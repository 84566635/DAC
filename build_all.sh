VER=$1
shift 

(cd libusbaudio; make clean; make -j6)
(cd project_smok/; make PROJNAME=SMOK clean; make -j6  PROJNAME=SMOK VT100=y RADIO_IMAGE=../PurePathProject/SLAVE.hex $@)

if [ "$VER" == "MS44" ] 
then 
(cd project_ma/;   make PROJNAME=MA clean; make -j6  PROJNAME=MA  VT100=y \
    RADIO_IMAGE_MSTM=../PurePathProject/MASTER_MULTI_SLAVE44_tam.hex \
    RADIO_IMAGE_MSTS=../PurePathProject/MASTER_MULTI_SLAVE44_tas.hex \
    RADIO_IMAGE_SSTM=../PurePathProject/MASTER_SINGLE_SLAVE_tam.hex \
    RADIO_IMAGE_SSTS=../PurePathProject/MASTER_SINGLE_SLAVE_tas.hex \
    $@ CONVERT_DBG=y) 
else
(cd project_ma/;   make PROJNAME=MA clean; make -j6  PROJNAME=MA  VT100=y \
    RADIO_IMAGE_MSTM=../PurePathProject/MASTER_MULTI_SLAVE_tam.hex \
    RADIO_IMAGE_MSTS=../PurePathProject/MASTER_MULTI_SLAVE_tas.hex \
    RADIO_IMAGE_SSTM=../PurePathProject/MASTER_SINGLE_SLAVE_tam.hex \
    RADIO_IMAGE_SSTS=../PurePathProject/MASTER_SINGLE_SLAVE_tas.hex \
    $@ CONVERT_DBG=y) 
fi
if [ "$VER" == "SS" ] 
then 
./tools/patch project_smok/params_smok.cfg project_smok/SMOK.bin ~/mnt/SMOK1.bin 8 && \
./tools/patch project_smok/params_smok.cfg project_smok/SMOK.bin ~/mnt/SMOK2.bin 9 && \
./tools/patch project_ma/params_ma.cfg project_ma/MA.bin ~/mnt/MA.bin 1
else
if [ "$VER" == "MS" ] || [ "$VER" == "MS44" ] 
then 
./tools/patch project_smok/params_smok.cfg project_smok/SMOK.bin ~/mnt/SMOK1.bin 0 && \
./tools/patch project_smok/params_smok.cfg project_smok/SMOK.bin ~/mnt/SMOK2.bin 1 && \
./tools/patch project_smok/params_smok.cfg project_smok/SMOK.bin ~/mnt/SMOK3.bin 2 && \
./tools/patch project_smok/params_smok.cfg project_smok/SMOK.bin ~/mnt/SMOK4.bin 3 && \
./tools/patch project_smok/params_smok.cfg project_smok/SMOK.bin ~/mnt/SMOK5.bin 4 && \
./tools/patch project_smok/params_smok.cfg project_smok/SMOK.bin ~/mnt/SMOK6.bin 5 && \
./tools/patch project_smok/params_smok.cfg project_smok/SMOK.bin ~/mnt/SMOK7.bin 6 && \
./tools/patch project_smok/params_smok.cfg project_smok/SMOK.bin ~/mnt/SMOK8.bin 7 && \
./tools/patch project_ma/params_ma.cfg project_ma/MA.bin ~/mnt/MA.bin 0
else
./tools/patch project_smok/params_smok.cfg project_smok/SMOK.bin ~/mnt/SMOK1.bin 10 && \
./tools/patch project_ma/params_ma.cfg project_ma/MA.bin ~/mnt/MA.bin 2
fi
fi


