################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/INFADO_workspace/INFADO/library/src/cc85xx/cc85xx_common.c \
C:/INFADO_workspace/INFADO/library/src/cc85xx/cc85xx_control_mst.c \
C:/INFADO_workspace/INFADO/library/src/cc85xx/cc85xx_control_slv.c 

O_SRCS += \
C:/INFADO_workspace/INFADO/library/src/cc85xx/cc85xx_common.o \
C:/INFADO_workspace/INFADO/library/src/cc85xx/cc85xx_control_mst.o \
C:/INFADO_workspace/INFADO/library/src/cc85xx/cc85xx_control_slv.o 

OBJS += \
./library/src/cc85xx/cc85xx_common.o \
./library/src/cc85xx/cc85xx_control_mst.o \
./library/src/cc85xx/cc85xx_control_slv.o 

C_DEPS += \
./library/src/cc85xx/cc85xx_common.d \
./library/src/cc85xx/cc85xx_control_mst.d \
./library/src/cc85xx/cc85xx_control_slv.d 


# Each subdirectory must supply rules for building sources it contributes
library/src/cc85xx/cc85xx_common.o: C:/INFADO_workspace/INFADO/library/src/cc85xx/cc85xx_common.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

library/src/cc85xx/cc85xx_control_mst.o: C:/INFADO_workspace/INFADO/library/src/cc85xx/cc85xx_control_mst.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

library/src/cc85xx/cc85xx_control_slv.o: C:/INFADO_workspace/INFADO/library/src/cc85xx/cc85xx_control_slv.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


