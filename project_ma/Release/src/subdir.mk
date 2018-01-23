################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/hwInit.c \
../src/i2sConfig.c \
../src/main.c \
../src/saihwInit.c \
../src/spiAudioStreamConfig.c \
../src/system_stm32f4xx.c 

O_SRCS += \
../src/hwInit.o \
../src/i2sConfig.o \
../src/main.o \
../src/saihwInit.o \
../src/spiAudioStreamConfig.o \
../src/system_stm32f4xx.o 

OBJS += \
./src/hwInit.o \
./src/i2sConfig.o \
./src/main.o \
./src/saihwInit.o \
./src/spiAudioStreamConfig.o \
./src/system_stm32f4xx.o 

C_DEPS += \
./src/hwInit.d \
./src/i2sConfig.d \
./src/main.d \
./src/saihwInit.d \
./src/spiAudioStreamConfig.d \
./src/system_stm32f4xx.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F429xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include" -I"../system/include" -I"../system/include/cmsis" -I"../system/include/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


