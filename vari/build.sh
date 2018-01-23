export CFLAGS="-march=armv7-a -marm -mthumb-interwork -mfloat-abi=hard -mfpu=neon -mtune=cortex-a8"
export CPP_INCLUDE_PATH="/home/infado/share/ti-sdk-am335x-evm-07.00.00.00/linux-devkit/sysroots/cortexa8hf-vfp-neon-3.8-oe-linux-gnueabi/include/c++/4.7.3"
export CPLUS_INCLUDE_PATH="/home/infado/ti-sdk-am335x-evm-07.00.00.00/targetNFS/usr/include:/home/infado/share/ti-sdk-am335x-evm-07.00.00.00/linux-devkit/sysroots/cortexa8hf-vfp-neon-3.8-oe-linux-gnueabi/include/c++/4.7.3:/home/infado/share/ti-sdk-am335x-evm-07.00.00.00/linux-devkit/sysroots/cortexa8hf-vfp-neon-3.8-oe-linux-gnueabi/include/c++/4.7.3/arm-linux-gnueabihf:/home/infado/share/ti-sdk-am335x-evm-07.00.00.00/linux-devkit/sysroots/cortexa8hf-vfp-neon-3.8-oe-linux-gnueabi/usr/include:/home/infado/share/ti-sdk-am335x-evm-07.00.00.00/linux-devkit/sysroots/i686-arago-linux/usr/lib/gcc/arm-linux-gnueabihf/4.7.3/include"
/home/infado/share/ti-sdk-am335x-evm-07.00.00.00/linux-devkit/sysroots/i686-arago-linux/usr/bin/arm-linux-gnueabihf-gcc -O3 -g -ggdb bb_flash.c -o bb_flash &&
/home/infado/share/ti-sdk-am335x-evm-07.00.00.00/linux-devkit/sysroots/i686-arago-linux/usr/bin/arm-linux-gnueabihf-g++ -O3 -ggdb3 -std=c++11 bb_player.cpp -pthread -lavcodec -lavutil -lavformat -lswscale -lavresample -o bb_player -L /home/infado/ti-sdk-am335x-evm-07.00.00.00/targetNFS/usr/lib/ &&
/home/infado/share/ti-sdk-am335x-evm-07.00.00.00/linux-devkit/sysroots/i686-arago-linux/usr/bin/arm-linux-gnueabihf-g++ -O3 -g -ggdb -std=c++11 bb_controller.cpp -ltag -o bb_controller -L /home/infado/ti-sdk-am335x-evm-07.00.00.00/targetNFS/usr/lib/

cp bb_flash      ../../targetNFS/home/root
cp bb_player     ../../targetNFS/home/root
cp bb_controller ../../targetNFS/home/root
 