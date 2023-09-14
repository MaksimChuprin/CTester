################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
D:/Projects/CAPTESTER\ STM32L152/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.c \
D:/Projects/CAPTESTER\ STM32L152/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c \
D:/Projects/CAPTESTER\ STM32L152/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c 

OBJS += \
./Middlewares/STM32_USBD_Library/Core/usbd_core.o \
./Middlewares/STM32_USBD_Library/Core/usbd_ctlreq.o \
./Middlewares/STM32_USBD_Library/Core/usbd_ioreq.o 

C_DEPS += \
./Middlewares/STM32_USBD_Library/Core/usbd_core.d \
./Middlewares/STM32_USBD_Library/Core/usbd_ctlreq.d \
./Middlewares/STM32_USBD_Library/Core/usbd_ioreq.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/STM32_USBD_Library/Core/usbd_core.o: D:/Projects/CAPTESTER\ STM32L152/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.c Middlewares/STM32_USBD_Library/Core/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32L152xC -DRTC_CLOCK_SOURCE_LSE '-DSTAT_MEM_SIZE=192*1024' -c -I../Src/Inc -I../Drivers/BSP/STM32L152DISC -I../../Drivers/CMSIS/Device/ST/STM32L1xx/Include -I../../Drivers/STM32L1xx_HAL_Driver/Inc -I../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -I../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../../Middlewares/Third_Party/FreeRTOS/Source/include -I../../Drivers/CMSIS/Include -I../../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -Os -ffunction-sections -Wall -fstack-usage -MMD -MP -MF"Middlewares/STM32_USBD_Library/Core/usbd_core.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Middlewares/STM32_USBD_Library/Core/usbd_ctlreq.o: D:/Projects/CAPTESTER\ STM32L152/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c Middlewares/STM32_USBD_Library/Core/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32L152xC -DRTC_CLOCK_SOURCE_LSE '-DSTAT_MEM_SIZE=192*1024' -c -I../Src/Inc -I../Drivers/BSP/STM32L152DISC -I../../Drivers/CMSIS/Device/ST/STM32L1xx/Include -I../../Drivers/STM32L1xx_HAL_Driver/Inc -I../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -I../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../../Middlewares/Third_Party/FreeRTOS/Source/include -I../../Drivers/CMSIS/Include -I../../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -Os -ffunction-sections -Wall -fstack-usage -MMD -MP -MF"Middlewares/STM32_USBD_Library/Core/usbd_ctlreq.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Middlewares/STM32_USBD_Library/Core/usbd_ioreq.o: D:/Projects/CAPTESTER\ STM32L152/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c Middlewares/STM32_USBD_Library/Core/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32L152xC -DRTC_CLOCK_SOURCE_LSE '-DSTAT_MEM_SIZE=192*1024' -c -I../Src/Inc -I../Drivers/BSP/STM32L152DISC -I../../Drivers/CMSIS/Device/ST/STM32L1xx/Include -I../../Drivers/STM32L1xx_HAL_Driver/Inc -I../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -I../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../../Middlewares/Third_Party/FreeRTOS/Source/include -I../../Drivers/CMSIS/Include -I../../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -Os -ffunction-sections -Wall -fstack-usage -MMD -MP -MF"Middlewares/STM32_USBD_Library/Core/usbd_ioreq.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Middlewares-2f-STM32_USBD_Library-2f-Core

clean-Middlewares-2f-STM32_USBD_Library-2f-Core:
	-$(RM) ./Middlewares/STM32_USBD_Library/Core/usbd_core.d ./Middlewares/STM32_USBD_Library/Core/usbd_core.o ./Middlewares/STM32_USBD_Library/Core/usbd_core.su ./Middlewares/STM32_USBD_Library/Core/usbd_ctlreq.d ./Middlewares/STM32_USBD_Library/Core/usbd_ctlreq.o ./Middlewares/STM32_USBD_Library/Core/usbd_ctlreq.su ./Middlewares/STM32_USBD_Library/Core/usbd_ioreq.d ./Middlewares/STM32_USBD_Library/Core/usbd_ioreq.o ./Middlewares/STM32_USBD_Library/Core/usbd_ioreq.su

.PHONY: clean-Middlewares-2f-STM32_USBD_Library-2f-Core

