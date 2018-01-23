
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

COMMON_CFLAGS = -I. -I./inc -I$(FREERTOS)/include -I$(FREERTOS_PORT) -I$(LIBRARY)/inc  -I$(LIBRARY)/inc/cc85xx  -I$(LIBRARY)/inc/audio \
		-I$(CC85XX_EHIF_SRC) -I$(CC85XX_EHIF_SRC)/little_endian -I$(CC85XX_EHIF_HAL) \
		-L . \
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
COMMON_CFLAGS += -DVERSION=\"$(VERSION)\"
else
COMMON_CFLAGS += -DVERSION=\"undefined\"
endif

ifdef VT100
COMMON_CFLAGS += -DVT100
endif

ifdef CABLE_ONLY
COMMON_CFLAGS += -DCABLE_ONLY
endif

ifdef KONDPOM
COMMON_CFLAGS += -DKONDYGNACJA_POMIESZCZENIE=$(KONDPOM)
endif

ifdef NO_RST_AMP
COMMON_CFLAGS += -DNO_RST_AMP
endif

COMMON_CFLAGS += -DRADIO_IMAGE=y 


HWFP           = stm32f4xx
HW_DRIVERS     = ../library/library_STM32F4xx
COMMON_CFLAGS += -DISR_SIZE="(82+16)" -DSTM32F4xx -DSTM32F40_41xxx -T stm32f407.ld -D_USE_XFUNC_OUT -DSMOK

COMMON_CFLAGS += -I$(HW_DRIVERS)/inc -I$(COMMON)/inc 
COMMON_CFLAGS += -D"assert_param(cond)=if(!(cond)){volatile int stop=1; while(stop);};" -DconfigASSERT=assert_param

OBJ  +=  src/main.o 
OBJ  +=  $(LIBRARY)/src/startup.o $(LIBRARY)/src/common.o $(LIBRARY)/src/xprintf.o 
OBJ  +=  $(LIBRARY)/src/messageProcessor.o
OBJ  +=  $(LIBRARY)/src/messageProcessorSMOK.o
OBJ  +=  $(LIBRARY)/src/uit_monitor.o
#OBJ  +=  $(LIBRARY)/src/power_proc.o
OBJ  +=  $(FREERTOS_PORT)/port.o $(FREERTOS)/portable/MemMang/heap_4.o
OBJ  +=  $(FREERTOS)/tasks.o $(FREERTOS)/list.o $(FREERTOS)/queue.o
OBJ  +=  $(LIBRARY)/src/moduleCommon.o 
OBJ  +=  $(LIBRARY)/src/communicator.o 
OBJ  +=  $(LIBRARY)/src/hwCommunicator.o 
OBJ  +=  $(LIBRARY)/src/hwCommunicatorUARThelper.o
OBJ  +=  $(LIBRARY)/src/hwCommunicatorSPIhelper.o
OBJ  +=  $(LIBRARY)/src/led.o 
OBJ  +=  $(LIBRARY)/src/adc.o 
OBJ  +=  $(LIBRARY)/src/bqueue.o 
OBJ  +=  $(LIBRARY)/src/cfg.o 
OBJ  +=  $(LIBRARY)/src/crc.o 

OBJ  +=  $(LIBRARY)/src/$(HWFP)_hwInit.o
OBJ  +=  $(LIBRARY)/src/debug.o 
OBJ  +=  $(LIBRARY)/src/watchdog.o 
OBJ  +=  $(LIBRARY)/src/i2c.o 
OBJ  +=  $(LIBRARY)/src/m25p20.o 

OBJ  += src/hwInit.o

OBJ  += src/system_$(HWFP).o
OBJ  += $(HW_DRIVERS)/src/misc.o
OBJ  += $(HW_DRIVERS)/src/$(HWFP)_gpio.o
OBJ  += $(HW_DRIVERS)/src/$(HWFP)_rcc.o
OBJ  += $(HW_DRIVERS)/src/$(HWFP)_usart.o
OBJ  += $(HW_DRIVERS)/src/$(HWFP)_spi.o
OBJ  += $(HW_DRIVERS)/src/$(HWFP)_exti.o
OBJ  += $(HW_DRIVERS)/src/$(HWFP)_syscfg.o
OBJ  += $(HW_DRIVERS)/src/$(HWFP)_i2c.o
OBJ  += $(HW_DRIVERS)/src/$(HWFP)_dma.o
OBJ  += $(HW_DRIVERS)/src/$(HWFP)_adc.o
OBJ  += $(HW_DRIVERS)/src/$(HWFP)_flash.o
OBJ  += $(HW_DRIVERS)/src/$(HWFP)_iwdg.o

COMMON_CFLAGS += -DCC85XX_USE_SLAVE
OBJ += $(CC85XX_SRC)/cc85xx_common.o
OBJ += $(CC85XX_SRC)/cc85xx_control_slv.o
OBJ += $(CC85XX_EHIF_SRC)/cc85xx_ehif_cmd_exec.o
OBJ += $(CC85XX_EHIF_SRC)/cc85xx_ehif_bootloader.o
OBJ += $(CC85XX_EHIF_SRC)/cc85xx_ehif_basic_op.o
OBJ += $(CC85XX_EHIF_SRC)/little_endian/cc85xx_ehif_field_op.o

V=1

PROJNAME = SMOK

all: $(PROJNAME)

radio_image.h: $(RADIO_IMAGE)
	@cp $(RADIO_IMAGE) radio.hex
	@$(ECHO) "bylem"
#	@hex2bin radio.hex
	@$(HEX2BIN) radio.hex
	@$(ECHO) "static const char radioImage[1][1][0x8000] = {{{" >> $@
	@$(XXD) -i < radio.bin >> $@
	@$(ECHO) "}}};" >>  $@

%.o: %.c radio_image.h
ifdef V
	@$(ECHO) '$@ \n'
endif
	@$(CC)  -c -o $@ $<  $(COMMON_CFLAGS) $(CFLAGS)


$(PROJNAME): $(OBJ)
	@$(ECHO) 'Building $(PROJNAME)'
	@$(CC) -o $@ $(OBJ) $(COMMON_CFLAGS) $(CFLAGS)
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
	@-$(RM) $(PROJNAME).elf
	@-$(RM) radio*.*

cleanall: clean

