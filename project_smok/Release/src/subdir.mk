################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/hwInit.c \
../src/main.c \
../src/system_stm32f4xx.c 

O_SRCS += \
../src/hwInit.o \
../src/main.o \
../src/system_stm32f4xx.o 

OBJS += \
./src/hwInit.o \
./src/main.o \
./src/system_stm32f4xx.o 

C_DEPS += \
./src/hwInit.d \
./src/main.d \
./src/system_stm32f4xx.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


