################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Startup/subdir.mk
-include Src/subdir.mk
-include Middlewares/STM32_USBD_Library/Core/subdir.mk
-include Middlewares/STM32_USBD_Library/Class/CDC/subdir.mk
-include Middlewares/FreeRTOS/portable/subdir.mk
-include Middlewares/FreeRTOS/CMSIS_RTOS/subdir.mk
-include Middlewares/FreeRTOS/subdir.mk
-include Drivers/STM32L1xx_HAL_Driver/subdir.mk
-include Drivers/BSP/STM32L152DISC/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := CAP_TESTER_STM32L152RCT6
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
CAP_TESTER_STM32L152RCT6.elf \

MAP_FILES += \
CAP_TESTER_STM32L152RCT6.map \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
CAP_TESTER_STM32L152RCT6.list \

OBJCOPY_HEX += \
CAP_TESTER_STM32L152RCT6.hex \


# All Target
all: main-build

# Main-build Target
main-build: CAP_TESTER_STM32L152RCT6.elf secondary-outputs

# Tool invocations
CAP_TESTER_STM32L152RCT6.elf CAP_TESTER_STM32L152RCT6.map: $(OBJS) $(USER_OBJS) ../Ld/STM32L152RCTx_FLASH.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-gcc -o "CAP_TESTER_STM32L152RCT6.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m3 -T"../Ld/STM32L152RCTx_FLASH.ld" --specs=nosys.specs -Wl,-Map="CAP_TESTER_STM32L152RCT6.map" -Wl,--gc-sections -static -L../../Drivers/CMSIS/Lib/GCC --specs=nano.specs -mfloat-abi=soft -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

CAP_TESTER_STM32L152RCT6.list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "CAP_TESTER_STM32L152RCT6.list"
	@echo 'Finished building: $@'
	@echo ' '

CAP_TESTER_STM32L152RCT6.hex: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objcopy  -O ihex $(EXECUTABLES) "CAP_TESTER_STM32L152RCT6.hex"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) CAP_TESTER_STM32L152RCT6.elf CAP_TESTER_STM32L152RCT6.hex CAP_TESTER_STM32L152RCT6.list CAP_TESTER_STM32L152RCT6.map default.size.stdout
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST) $(OBJCOPY_HEX)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents main-build fail-specified-linker-script-missing warn-no-linker-script-specified

-include ../makefile.targets
