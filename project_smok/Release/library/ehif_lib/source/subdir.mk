################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/INFADO_workspace/INFADO/library/ehif_lib/source/cc85xx_ehif_basic_op.c \
C:/INFADO_workspace/INFADO/library/ehif_lib/source/cc85xx_ehif_bootloader.c \
C:/INFADO_workspace/INFADO/library/ehif_lib/source/cc85xx_ehif_cmd_exec.c 

O_SRCS += \
C:/INFADO_workspace/INFADO/library/ehif_lib/source/cc85xx_ehif_basic_op.o \
C:/INFADO_workspace/INFADO/library/ehif_lib/source/cc85xx_ehif_bootloader.o \
C:/INFADO_workspace/INFADO/library/ehif_lib/source/cc85xx_ehif_cmd_exec.o 

OBJS += \
./library/ehif_lib/source/cc85xx_ehif_basic_op.o \
./library/ehif_lib/source/cc85xx_ehif_bootloader.o \
./library/ehif_lib/source/cc85xx_ehif_cmd_exec.o 

C_DEPS += \
./library/ehif_lib/source/cc85xx_ehif_basic_op.d \
./library/ehif_lib/source/cc85xx_ehif_bootloader.d \
./library/ehif_lib/source/cc85xx_ehif_cmd_exec.d 


# Each subdirectory must supply rules for building sources it contributes
library/ehif_lib/source/cc85xx_ehif_basic_op.o: C:/INFADO_workspace/INFADO/library/ehif_lib/source/cc85xx_ehif_basic_op.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

library/ehif_lib/source/cc85xx_ehif_bootloader.o: C:/INFADO_workspace/INFADO/library/ehif_lib/source/cc85xx_ehif_bootloader.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

library/ehif_lib/source/cc85xx_ehif_cmd_exec.o: C:/INFADO_workspace/INFADO/library/ehif_lib/source/cc85xx_ehif_cmd_exec.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


