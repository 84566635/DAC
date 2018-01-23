################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include libusbaudio/Src/subdir.mk
-include libusbaudio/Middlewares/ST/STM32_USB_Device_Library/Core/Src/subdir.mk
-include libusbaudio/Middlewares/ST/STM32_USB_Device_Library/Class/AUDIO/Src/subdir.mk
-include libusbaudio/Drivers/STM32F4xx_HAL_Driver/Src/subdir.mk
-include library/src/cc85xx/subdir.mk
-include library/src/audio/subdir.mk
-include library/src/subdir.mk
-include library/library_STM32F4xx/src/subdir.mk
-include library/freeRTOS/portable/RVDS/ARM_CM3/subdir.mk
-include library/freeRTOS/portable/MemMang/subdir.mk
-include library/freeRTOS/portable/GCC/ARM_CM3_MPU/subdir.mk
-include library/freeRTOS/portable/GCC/ARM_CM3/subdir.mk
-include library/freeRTOS/subdir.mk
-include library/ehif_lib/source/little_endian/subdir.mk
-include library/ehif_lib/source/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
project_smok.hex \

SECONDARY_SIZE += \
project_smok.siz \


# All Target
all: project_smok.elf secondary-outputs

# Tool invocations
project_smok.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Cross ARM GNU C++ Linker'
	arm-none-eabi-g++ -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g -T mem.ld -T libs.ld -T sections.ld -nostartfiles -Xlinker --gc-sections -L"../ldscripts2" -Wl,-Map,"project_smok.map" --specs=nano.specs -o "project_smok.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

project_smok.hex: project_smok.elf
	@echo 'Invoking: Cross ARM GNU Create Flash Image'
	arm-none-eabi-objcopy -O ihex "project_smok.elf"  "project_smok.hex"
	@echo 'Finished building: $@'
	@echo ' '

project_smok.siz: project_smok.elf
	@echo 'Invoking: Cross ARM GNU Print Size'
	arm-none-eabi-size --format=berkeley "project_smok.elf"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(CC_DEPS)$(C++_DEPS)$(OBJS)$(C_UPPER_DEPS)$(CXX_DEPS)$(SECONDARY_FLASH)$(SECONDARY_SIZE)$(ASM_DEPS)$(S_UPPER_DEPS)$(C_DEPS)$(CPP_DEPS) project_smok.elf
	-@echo ' '

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_SIZE)

.PHONY: all clean dependents

-include ../makefile.targets
