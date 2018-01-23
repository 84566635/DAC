################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/INFADO_workspace/INFADO/library/freeRTOS/portable/MemMang/heap_1.c \
C:/INFADO_workspace/INFADO/library/freeRTOS/portable/MemMang/heap_2.c \
C:/INFADO_workspace/INFADO/library/freeRTOS/portable/MemMang/heap_3.c \
C:/INFADO_workspace/INFADO/library/freeRTOS/portable/MemMang/heap_4.c 

O_SRCS += \
C:/INFADO_workspace/INFADO/library/freeRTOS/portable/MemMang/heap_4.o 

OBJS += \
./library/freeRTOS/portable/MemMang/heap_1.o \
./library/freeRTOS/portable/MemMang/heap_2.o \
./library/freeRTOS/portable/MemMang/heap_3.o \
./library/freeRTOS/portable/MemMang/heap_4.o 

C_DEPS += \
./library/freeRTOS/portable/MemMang/heap_1.d \
./library/freeRTOS/portable/MemMang/heap_2.d \
./library/freeRTOS/portable/MemMang/heap_3.d \
./library/freeRTOS/portable/MemMang/heap_4.d 


# Each subdirectory must supply rules for building sources it contributes
library/freeRTOS/portable/MemMang/heap_1.o: C:/INFADO_workspace/INFADO/library/freeRTOS/portable/MemMang/heap_1.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

library/freeRTOS/portable/MemMang/heap_2.o: C:/INFADO_workspace/INFADO/library/freeRTOS/portable/MemMang/heap_2.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

library/freeRTOS/portable/MemMang/heap_3.o: C:/INFADO_workspace/INFADO/library/freeRTOS/portable/MemMang/heap_3.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

library/freeRTOS/portable/MemMang/heap_4.o: C:/INFADO_workspace/INFADO/library/freeRTOS/portable/MemMang/heap_4.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -DNDEBUG -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include2" -I"../system2/include2" -I"../system2/include2/cmsis2" -I"../system2/include2/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


