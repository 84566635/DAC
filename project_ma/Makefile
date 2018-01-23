
GCCPATH ?= /buildtools/gcc-arm-none-eabi-4_8-131228/bin/
CC       = $(GCCPATH)arm-none-eabi-gcc
OBJCOPY  = $(GCCPATH)arm-none-eabi-objcopy
OBJDUMP  = $(GCCPATH)arm-none-eabi-objdump
NM       = $(GCCPATH)arm-none-eabi-nm
SIZE     = $(GCCPATH)arm-none-eabi-size
READELF  = $(GCCPATH)arm-none-eabi-readelf
RM      ?= rm -f
MKDIR   ?= mkdir -p
AWK     ?= awk
CAT     ?= cat
TEX2PDF ?= pdflatex
CP      ?= cp
ECHO    ?= echo
XXD     ?= xxd

ODIR       ?= obj

LIBRARY       = ../library
COMMON        = $(LIBRARY)/library_common
FREERTOS      = $(LIBRARY)/freeRTOS
CC85XX_EHIF_SRC      = $(LIBRARY)/ehif_lib/source
CC85XX_SRC	= $(LIBRARY)/src/cc85xx
CC85XX_INC	= $(LIBRARY)/inc/cc85xx
CC85XX_EHIF_HAL = $(CC85XX_EHIF_SRC)/hal
FREERTOS_PORT = $(FREERTOS)/portable/GCC/ARM_CM3

COMMON_CFLAGS = -I. -I./inc -I$(FREERTOS)/include -I$(FREERTOS_PORT) -I$(LIBRARY)/inc \
		-I$(CC85XX_EHIF_SRC) -I$(CC85XX_EHIF_SRC)/little_endian -I$(CC85XX_EHIF_HAL) -I$(CC85XX_INC) \
		-I$(LIBRARY)/inc/audio \
		-L .  \
		-gdwarf-2 -g3 -mthumb \
		-Werror -Wall -Wno-error=unused-value -Wno-error=unused-variable \
		-Wno-error=unused-function -Wno-pragmas -Wno-error=cpp -Wstrict-prototypes -Wreturn-type \
		-Wmissing-prototypes -Wmissing-declarations  -Wimplicit-function-declaration\
		-fno-strict-aliasing -ffunction-sections -fno-builtin-printf -lm \
		-Wno-error=array-bounds

COMMON_CFLAGS += -Wl,--gc-sections -Os
COMMON_CFLAGS  += -mcpu=cortex-m4 #-mfpu=fpv4-sp-d16 -mfloat-abi=softfp 

COMMON_CFLAGS += -DDEBUG

ifdef VERSION
COMMON_CFLAGS += -DVERSION=\"$(PROJNAME)_\"\"$(VERSION)\"
else
COMMON_CFLAGS += -DVERSION=\"$(PROJNAME)_\"\"undefined\"
endif

ifdef VT100
COMMON_CFLAGS += -DVT100
endif

ifdef WM_XTAL_FREQ
COMMON_CFLAGS += -DWM_XTAL_FREQ=$(WM_XTAL_FREQ)
endif

ifdef CONVERT_DBG
COMMON_CFLAGS += -DCONVERT_DBG
endif

ifdef CABLE_ONLY
COMMON_CFLAGS += -DCABLE_ONLY
endif

ifdef CEN_ALTER
COMMON_CFLAGS += -DCEN_ALTER
endif

COMMON_CFLAGS += -DRADIO_IMAGE=y


HWFP           = stm32f4xx
HWFP_L2        = stm32f429
HW_DRIVERS     = ../library/library_STM32F4xx
COMMON_CFLAGS += -DISR_SIZE="(92+16)" -DSTM32F4xx -DSTM32F429_439xx -T $(HWFP_L2).ld -DMADO -D_USE_XFUNC_OUT

COMMON_CFLAGS += -I$(HW_DRIVERS)/inc -I$(LIBRARY)/library_common/inc
COMMON_CFLAGS += -D"assert_param(cond)=if(!(cond)){volatile int stop=1; while(stop);};" -DconfigASSERT=assert_param
COMMON_CFLAGS += -DHSE_VALUE="((uint32_t)1000000*cfgnvs.pll_m)" -include cfg.h

OBJ  +=  src/main.o 
OBJ  +=  $(LIBRARY)/src/startup.o $(LIBRARY)/src/common.o $(LIBRARY)/src/xprintf.o 
OBJ  +=  $(LIBRARY)/src/messageProcessor.o
OBJ  +=  $(LIBRARY)/src/messageProcessorMADO.o
OBJ  +=  $(FREERTOS_PORT)/port.o $(FREERTOS)/portable/MemMang/heap_4.o
OBJ  +=  $(FREERTOS)/tasks.o $(FREERTOS)/list.o $(FREERTOS)/queue.o
OBJ  +=  $(LIBRARY)/src/moduleCommon.o 
OBJ  +=  $(LIBRARY)/src/communicator.o 
OBJ  +=  $(LIBRARY)/src/hwCommunicator.o 
OBJ  +=  $(LIBRARY)/src/hwCommunicatorUARThelper.o
OBJ  +=  $(LIBRARY)/src/hwCommunicatorSPIhelper.o
OBJ  +=  $(LIBRARY)/src/led.o 
#OBJ  +=  $(LIBRARY)/src/adc.o 
OBJ  +=  $(LIBRARY)/src/$(HWFP)_hwInit.o
OBJ  +=  $(LIBRARY)/src/debug.o 
OBJ  +=  $(LIBRARY)/src/watchdog.o 
OBJ  +=  $(LIBRARY)/src/cfg.o 
OBJ  +=  $(LIBRARY)/src/crc.o 


OBJ  += src/hwInit.o
OBJ  += src/spiAudioStreamConfig.o
OBJ  += $(LIBRARY)/src/wm8805.o

OBJ  += src/system_$(HWFP).o

OBJ  += $(HW_DRIVERS)/src/misc.o
OBJ  += $(HW_DRIVERS)/src/$(HWFP)_gpio.o
OBJ  += $(HW_DRIVERS)/src/$(HWFP)_rcc.o
OBJ  += $(HW_DRIVERS)/src/$(HWFP)_usart.o
OBJ  += $(HW_DRIVERS)/src/$(HWFP)_spi.o
OBJ  += $(HW_DRIVERS)/src/$(HWFP)_dma.o
OBJ  += $(HW_DRIVERS)/src/$(HWFP)_exti.o
OBJ  += $(HW_DRIVERS)/src/$(HWFP)_syscfg.o
OBJ  += $(HW_DRIVERS)/src/$(HWFP)_sai.o
OBJ  += $(HW_DRIVERS)/src/$(HWFP)_tim.o
OBJ  += $(HW_DRIVERS)/src/$(HWFP)_flash.o
OBJ  += $(HW_DRIVERS)/src/$(HWFP)_iwdg.o

#ifdef CC85XX

OBJ += $(CC85XX_SRC)/cc85xx_common.o
OBJ += $(CC85XX_SRC)/cc85xx_control_mst.o
OBJ += $(CC85XX_EHIF_SRC)/cc85xx_ehif_cmd_exec.o
OBJ += $(CC85XX_EHIF_SRC)/cc85xx_ehif_bootloader.o
OBJ += $(CC85XX_EHIF_SRC)/cc85xx_ehif_basic_op.o
OBJ += $(CC85XX_EHIF_SRC)/little_endian/cc85xx_ehif_field_op.o
#endif



OBJ += src/saihwInit.o
OBJ += src/i2sConfig.o
OBJ += $(LIBRARY)/src/audio/spiStream.o
OBJ += $(LIBRARY)/src/audio/i2sStream.o
OBJ  +=  $(LIBRARY)/src/bqueue.o 
OBJ  +=  $(LIBRARY)/src/audio/convert.o 


PROJNAME ?= MA

all: $(PROJNAME) 


radio_image.h: $(RADIO_IMAGE_MSTM)  $(RADIO_IMAGE_MSTS)  $(RADIO_IMAGE_SSTM) $(RADIO_IMAGE_SSTS)
	@cp $(RADIO_IMAGE_MSTM) radioMSTM.hex 
	@cp $(RADIO_IMAGE_MSTS) radioMSTS.hex 
	@cp $(RADIO_IMAGE_SSTM) radioSSTM.hex 
	@cp $(RADIO_IMAGE_SSTS) radioSSTS.hex 
	@hex2bin radioMSTM.hex
	@hex2bin radioMSTS.hex
	@hex2bin radioSSTM.hex
	@hex2bin radioSSTS.hex
	@$(ECHO) "static const char radioImage[2][2][0x8000] = {{{" >> $@
	@$(XXD) -i < radioMSTS.bin >> $@
	@$(ECHO) "},{" >> $@
	@$(XXD) -i < radioMSTM.bin >> $@
	@$(ECHO) "}},{{" >>  $@
	@$(XXD) -i < radioSSTS.bin >> $@
	@$(ECHO) "},{" >> $@
	@$(XXD) -i < radioSSTM.bin >> $@
	@$(ECHO) "}}};" >>  $@

%.o: %.c radio_image.h
ifdef V
	@$(ECHO) '$@ \n'
endif
	@$(CC)  -c -o $@ $<  $(COMMON_CFLAGS) $(CFLAGS)


$(PROJNAME): $(OBJ)
	@$(ECHO) 'Building $(PROJNAME)'
	@$(CC) -o $@ $(OBJ) $(COMMON_CFLAGS) $(CFLAGS) ../libusbaudio/libusb.a
	@$(OBJCOPY) $(OBJCOPY_FLAGS) -O ihex $@ $@.hex
	@$(OBJCOPY) $(OBJCOPY_FLAGS) -O binary $@ $@.bin
	@$(OBJDUMP) -d -S $@ > $@.disasm
	@$(NM) -n -S $@ > $@.sym
	@$(READELF) -a $@ > $@.readelf

.PHONY: clean cleanall dirs

clean:
	@-$(RM)  $(OBJ)
	@-$(RM) $(PROJNAME)
	@-$(RM) $(PROJNAME).hex
	@-$(RM) $(PROJNAME).bin
	@-$(RM) $(PROJNAME).sym
	@-$(RM) $(PROJNAME).disasm
	@-$(RM) $(PROJNAME).readelf
	@-$(RM) radio*.*

cleanall: clean

