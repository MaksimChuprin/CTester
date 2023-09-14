################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
D:/Projects/CAPTESTER\ STM32L152/Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c \
D:/Projects/CAPTESTER\ STM32L152/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3/port.c 

OBJS += \
./Middlewares/FreeRTOS/portable/heap_4.o \
./Middlewares/FreeRTOS/portable/port.o 

C_DEPS += \
./Middlewares/FreeRTOS/portable/heap_4.d \
./Middlewares/FreeRTOS/portable/port.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/FreeRTOS/portable/heap_4.o: D:/Projects/CAPTESTER\ STM32L152/Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c Middlewares/FreeRTOS/portable/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32L152xC -DRTC_CLOCK_SOURCE_LSE '-DSTAT_MEM_SIZE=192*1024' -c -I../Src/Inc -I../Drivers/BSP/STM32L152DISC -I../../Drivers/CMSIS/Device/ST/STM32L1xx/Include -I../../Drivers/STM32L1xx_HAL_Driver/Inc -I../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -I../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../../Middlewares/Third_Party/FreeRTOS/Source/include -I../../Drivers/CMSIS/Include -I../../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -Os -ffunction-sections -Wall -fstack-usage -MMD -MP -MF"Middlewares/FreeRTOS/portable/heap_4.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Middlewares/FreeRTOS/portable/port.o: D:/Projects/CAPTESTER\ STM32L152/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3/port.c Middlewares/FreeRTOS/portable/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32L152xC -DRTC_CLOCK_SOURCE_LSE '-DSTAT_MEM_SIZE=192*1024' -c -I../Src/Inc -I../Drivers/BSP/STM32L152DISC -I../../Drivers/CMSIS/Device/ST/STM32L1xx/Include -I../../Drivers/STM32L1xx_HAL_Driver/Inc -I../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -I../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../../Middlewares/Third_Party/FreeRTOS/Source/include -I../../Drivers/CMSIS/Include -I../../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -Os -ffunction-sections -Wall -fstack-usage -MMD -MP -MF"Middlewares/FreeRTOS/portable/port.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Middlewares-2f-FreeRTOS-2f-portable

clean-Middlewares-2f-FreeRTOS-2f-portable:
	-$(RM) ./Middlewares/FreeRTOS/portable/heap_4.d ./Middlewares/FreeRTOS/portable/heap_4.o ./Middlewares/FreeRTOS/portable/heap_4.su ./Middlewares/FreeRTOS/portable/port.d ./Middlewares/FreeRTOS/portable/port.o ./Middlewares/FreeRTOS/portable/port.su

.PHONY: clean-Middlewares-2f-FreeRTOS-2f-portable

