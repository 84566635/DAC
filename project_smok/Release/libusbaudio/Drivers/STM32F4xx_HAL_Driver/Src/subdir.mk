################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/INFADO_workspace/INFADO/libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c \
C:/INFADO_workspace/INFADO/libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c \
C:/INFADO_workspace/INFADO/libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c \
C:/INFADO_workspace/INFADO/libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd.c \
C:/INFADO_workspace/INFADO/libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd_ex.c \
C:/INFADO_workspace/INFADO/libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c \
C:/INFADO_workspace/INFADO/libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_usb.c 

O_SRCS += \
C:/INFADO_workspace/INFADO/libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.o \
C:/INFADO_workspace/INFADO/libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.o \
C:/INFADO_workspace/INFADO/libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd.o \
C:/INFADO_workspace/INFADO/libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd_ex.o \
C:/INFADO_workspace/INFADO/libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.o \
C:/INFADO_workspace/INFADO/libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_usb.o 

OBJS += \
./libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.o \
./libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.o \
./libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.o \
./libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd.o \
./libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd_ex.o \
./libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.o \
./libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_usb.o 

C_DEPS += \
./libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.d \
./libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.d \
./libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.d \
./libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd.d \
./libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd_ex.d \
./libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.d \
./libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_usb.d 


# Each subdirectory must supply rules for building sources it contributes
libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.o: C:/INFADO_workspace/INFADO/libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.o: C:/INFADO_workspace/INFADO/libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.o: C:/INFADO_workspace/INFADO/libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd.o: C:/INFADO_workspace/INFADO/libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd_ex.o: C:/INFADO_workspace/INFADO/libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd_ex.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.o: C:/INFADO_workspace/INFADO/libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_usb.o: C:/INFADO_workspace/INFADO/libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_usb.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


