################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/INFADO_workspace/INFADO/libusbaudio/Src/main.c \
C:/INFADO_workspace/INFADO/libusbaudio/Src/stm32f4xx_hal_msp.c \
C:/INFADO_workspace/INFADO/libusbaudio/Src/stm32f4xx_it.c \
C:/INFADO_workspace/INFADO/libusbaudio/Src/usb_device.c \
C:/INFADO_workspace/INFADO/libusbaudio/Src/usbd_conf.c \
C:/INFADO_workspace/INFADO/libusbaudio/Src/usbd_desc.c 

O_SRCS += \
C:/INFADO_workspace/INFADO/libusbaudio/Src/main.o \
C:/INFADO_workspace/INFADO/libusbaudio/Src/stm32f4xx_hal_msp.o \
C:/INFADO_workspace/INFADO/libusbaudio/Src/stm32f4xx_it.o \
C:/INFADO_workspace/INFADO/libusbaudio/Src/usb_device.o \
C:/INFADO_workspace/INFADO/libusbaudio/Src/usbd_conf.o \
C:/INFADO_workspace/INFADO/libusbaudio/Src/usbd_desc.o 

OBJS += \
./libusbaudio/Src/main.o \
./libusbaudio/Src/stm32f4xx_hal_msp.o \
./libusbaudio/Src/stm32f4xx_it.o \
./libusbaudio/Src/usb_device.o \
./libusbaudio/Src/usbd_conf.o \
./libusbaudio/Src/usbd_desc.o 

C_DEPS += \
./libusbaudio/Src/main.d \
./libusbaudio/Src/stm32f4xx_hal_msp.d \
./libusbaudio/Src/stm32f4xx_it.d \
./libusbaudio/Src/usb_device.d \
./libusbaudio/Src/usbd_conf.d \
./libusbaudio/Src/usbd_desc.d 


# Each subdirectory must supply rules for building sources it contributes
libusbaudio/Src/main.o: C:/INFADO_workspace/INFADO/libusbaudio/Src/main.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

libusbaudio/Src/stm32f4xx_hal_msp.o: C:/INFADO_workspace/INFADO/libusbaudio/Src/stm32f4xx_hal_msp.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

libusbaudio/Src/stm32f4xx_it.o: C:/INFADO_workspace/INFADO/libusbaudio/Src/stm32f4xx_it.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

libusbaudio/Src/usb_device.o: C:/INFADO_workspace/INFADO/libusbaudio/Src/usb_device.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

libusbaudio/Src/usbd_conf.o: C:/INFADO_workspace/INFADO/libusbaudio/Src/usbd_conf.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

libusbaudio/Src/usbd_desc.o: C:/INFADO_workspace/INFADO/libusbaudio/Src/usbd_desc.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


