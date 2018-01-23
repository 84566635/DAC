################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/INFADO_workspace/INFADO/libusbaudio/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.c \
C:/INFADO_workspace/INFADO/libusbaudio/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c \
C:/INFADO_workspace/INFADO/libusbaudio/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c 

O_SRCS += \
C:/INFADO_workspace/INFADO/libusbaudio/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.o \
C:/INFADO_workspace/INFADO/libusbaudio/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.o \
C:/INFADO_workspace/INFADO/libusbaudio/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.o 

OBJS += \
./libusbaudio/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.o \
./libusbaudio/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.o \
./libusbaudio/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.o 

C_DEPS += \
./libusbaudio/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.d \
./libusbaudio/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.d \
./libusbaudio/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.d 


# Each subdirectory must supply rules for building sources it contributes
libusbaudio/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.o: C:/INFADO_workspace/INFADO/libusbaudio/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

libusbaudio/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.o: C:/INFADO_workspace/INFADO/libusbaudio/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

libusbaudio/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.o: C:/INFADO_workspace/INFADO/libusbaudio/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


