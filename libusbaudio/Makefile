
GCCPATH ?= /buildtools/gcc-arm-none-eabi-4_8-131228/bin/
AR       = $(GCCPATH)arm-none-eabi-ar
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

ODIR       ?= obj


COMMON_CFLAGS += -IMiddlewares/ST/STM32_USB_Device_Library/Class/AUDIO/Inc
COMMON_CFLAGS += -IMiddlewares/ST/STM32_USB_Device_Library/Core/Inc
COMMON_CFLAGS += -IDrivers/STM32F4xx_HAL_Driver/Inc
COMMON_CFLAGS += -IDrivers/CMSIS/Device/ST/STM32F4xx/Include
COMMON_CFLAGS += -IDrivers/CMSIS/Include
COMMON_CFLAGS += -IInc
COMMON_CFLAGS += -I.
COMMON_CFLAGS += -IDrivers/STM32F4xx_HAL_Driver/Inc/Legacy
COMMON_CFLAGS += -I../library/inc
COMMON_CFLAGS += -I../library/inc/audio

COMMON_CFLAGS +=  -L . \
		-gdwarf-2 -g3 -mthumb \
		-Werror -Wall -Wno-error=unused-value -Wno-error=unused-variable \
		-Wno-error=unused-function -Wno-error=cpp -Wstrict-prototypes -Wreturn-type \
		-Wmissing-prototypes -Wmissing-declarations  -Wimplicit-function-declaration\
		-fno-strict-aliasing -ffunction-sections -fno-builtin-printf -lm \
		-Wno-error=array-bounds

COMMON_CFLAGS +=  -Os -nostdlib
COMMON_CFLAGS  += -mcpu=cortex-m4 #-mfpu=fpv4-sp-d16 -mfloat-abi=softfp 



COMMON_CFLAGS += -DSTM32F427ZGTx
COMMON_CFLAGS += -D__weak="__attribute__((weak))"
COMMON_CFLAGS += -D__packed="__attribute__((__packed__))"
COMMON_CFLAGS += -DUSE_HAL_DRIVER
COMMON_CFLAGS += -DSTM32F427xx

OBJ  += Src/usb_device.o
OBJ  += Src/usbd_conf.o
OBJ  += Src/usbd_desc.o
OBJ  += Src/stm32f4xx_it.o
OBJ  += Src/stm32f4xx_hal_msp.o
OBJ  += Src/main.o
OBJ  += Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.o
OBJ  += Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd.o
OBJ  += Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd_ex.o
OBJ  += Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_usb.o
OBJ  += Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.o
OBJ  += Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.o
OBJ  += Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.o
OBJ  += Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.o
OBJ  += Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.o
OBJ  += Middlewares/ST/STM32_USB_Device_Library/Class/AUDIO/Src/usbd_audio.o


PROJNAME ?= libusb.a

all: $(PROJNAME) 

V=1

%.o: %.c 
ifdef V
	@echo '$@ '
endif
	@$(CC) -c -o $@ $<  $(COMMON_CFLAGS) $(CFLAGS)

%.o: %.s
ifdef V
	@echo '$@ '
endif
	@$(CC) -c -o $@ $<  $(COMMON_CFLAGS) $(CFLAGS)


$(PROJNAME): $(OBJ)
	@echo 'Building $(PROJNAME)'
#	@$(CC) -o $@ $(OBJ) $(COMMON_CFLAGS) $(CFLAGS)
	@$(AR) cr $@ $(OBJ) 
#	@$(OBJCOPY) $(OBJCOPY_FLAGS) -O ihex $@ $@.hex
#	@$(OBJCOPY) $(OBJCOPY_FLAGS) -O binary $@ $@.bin
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

