rm -fr project_ma/*.bin project_smok/*.bin 
VER=$1
shift 

(cd libusbaudio; make clean; make -j6)
(cd project_smok/; make PROJNAME=SMOK clean; make -j6  PROJNAME=SMOK RADIO_IMAGE=../PurePathProject/SLAVE.hex  VERSION=$VER $@) && \
(cd project_ma/; make PROJNAME=MA clean; make -j6  PROJNAME=MA  \
    RADIO_IMAGE_MSTM=../PurePathProject/MASTER_MULTI_SLAVE_tam.hex \
    RADIO_IMAGE_MSTS=../PurePathProject/MASTER_MULTI_SLAVE_tas.hex \
    RADIO_IMAGE_SSTM=../PurePathProject/MASTER_SINGLE_SLAVE_tam.hex \
    RADIO_IMAGE_SSTS=../PurePathProject/MASTER_SINGLE_SLAVE_tas.hex \
    VERSION=$VER $@ CONVERT_DBG=y) && \
(cd project_ma/; make PROJNAME=MA44 clean; make -j6  PROJNAME=MA44  \
    RADIO_IMAGE_MSTM=../PurePathProject/MASTER_MULTI_SLAVE44_tam.hex \
    RADIO_IMAGE_MSTS=../PurePathProject/MASTER_MULTI_SLAVE44_tas.hex \
    RADIO_IMAGE_SSTM=../PurePathProject/MASTER_SINGLE_SLAVE_tam.hex \
    RADIO_IMAGE_SSTS=../PurePathProject/MASTER_SINGLE_SLAVE_tas.hex \
    VERSION=$VER $@ CONVERT_DBG=y) && \
7zr a -mx=9 -ms=on project_$VER.7z \
    ./tools/patch.bat \
    ./tools/cygwin1.dll \
    ./tools/patch.exe \
    ./tools/pscp.exe \
    ./tools/plink.exe \
    ./tools/instrukcje.txt \
    ./project_ma/MA.bin \
    ./project_ma/MA44.bin \
    ./project_ma/params_ma.cfg \
    ./project_smok/SMOK.bin \
    ./project_smok/params_smok.cfg \
    ./vari/bb_player \
    ./vari/bb_controller \
    ./vari/bb_player.cpp \
    ./vari/bb_controller.cpp \
    ./vari/bb_player.cpp.diff \
    ./vari/bb_controller.cpp.diff \
    ./vari/bb_flash \
    ./vari/skrypty/cdacc4037a75e44baf6db039a4fea85b.sh \
    ./vari/skrypty/sharc.sh \
    ./vari/skrypty/sharc.bat \
    ./vari/skrypty/MADO_SMOKi.sh \
    ./vari/skrypty/MADO_SMOKi.bat \
    ./vari/skrypty/send_all.bat \
    ./vari/skrypty/delayed_diag.sh \
    ./vari/skrypty/flash.sh \
    ./vari/skrypty/setwifi.sh \
    ./vari/skrypty/mount.sh \
    ./vari/skrypty/start_bb.sh
