################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/BSP/STM32L152DISC/stm32l152c_discovery_cts.c 

OBJS += \
./Drivers/BSP/STM32L152DISC/stm32l152c_discovery_cts.o 

C_DEPS += \
./Drivers/BSP/STM32L152DISC/stm32l152c_discovery_cts.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/BSP/STM32L152DISC/%.o Drivers/BSP/STM32L152DISC/%.su: ../Drivers/BSP/STM32L152DISC/%.c Drivers/BSP/STM32L152DISC/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32L152xC -DRTC_CLOCK_SOURCE_LSE '-DSTAT_MEM_SIZE=192*1024' -c -I../Src/Inc -I../Drivers/BSP/STM32L152DISC -I../../Drivers/CMSIS/Device/ST/STM32L1xx/Include -I../../Drivers/STM32L1xx_HAL_Driver/Inc -I../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -I../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../../Middlewares/Third_Party/FreeRTOS/Source/include -I../../Drivers/CMSIS/Include -I../../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -Os -ffunction-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Drivers-2f-BSP-2f-STM32L152DISC

clean-Drivers-2f-BSP-2f-STM32L152DISC:
	-$(RM) ./Drivers/BSP/STM32L152DISC/stm32l152c_discovery_cts.d ./Drivers/BSP/STM32L152DISC/stm32l152c_discovery_cts.o ./Drivers/BSP/STM32L152DISC/stm32l152c_discovery_cts.su

.PHONY: clean-Drivers-2f-BSP-2f-STM32L152DISC

