################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/INFADO_workspace/INFADO/library/src/audio/audioQueue.c \
C:/INFADO_workspace/INFADO/library/src/audio/convert.c \
C:/INFADO_workspace/INFADO/library/src/audio/i2sStream.c \
C:/INFADO_workspace/INFADO/library/src/audio/spiStream.c 

O_SRCS += \
C:/INFADO_workspace/INFADO/library/src/audio/convert.o \
C:/INFADO_workspace/INFADO/library/src/audio/i2sStream.o \
C:/INFADO_workspace/INFADO/library/src/audio/spiStream.o 

OBJS += \
./library/src/audio/audioQueue.o \
./library/src/audio/convert.o \
./library/src/audio/i2sStream.o \
./library/src/audio/spiStream.o 

C_DEPS += \
./library/src/audio/audioQueue.d \
./library/src/audio/convert.d \
./library/src/audio/i2sStream.d \
./library/src/audio/spiStream.d 


# Each subdirectory must supply rules for building sources it contributes
library/src/audio/audioQueue.o: C:/INFADO_workspace/INFADO/library/src/audio/audioQueue.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

library/src/audio/convert.o: C:/INFADO_workspace/INFADO/library/src/audio/convert.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

library/src/audio/i2sStream.o: C:/INFADO_workspace/INFADO/library/src/audio/i2sStream.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

library/src/audio/spiStream.o: C:/INFADO_workspace/INFADO/library/src/audio/spiStream.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


