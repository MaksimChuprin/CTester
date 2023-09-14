################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/date_time.c \
../Src/main.c \
../Src/measure_main.c \
../Src/stm32l1xx_hal_msp.c \
../Src/stm32l1xx_it.c \
../Src/system_stm32l1xx.c \
../Src/usb_main.c \
../Src/usbd_cdc_interface.c \
../Src/usbd_conf.c \
../Src/usbd_desc.c 

OBJS += \
./Src/date_time.o \
./Src/main.o \
./Src/measure_main.o \
./Src/stm32l1xx_hal_msp.o \
./Src/stm32l1xx_it.o \
./Src/system_stm32l1xx.o \
./Src/usb_main.o \
./Src/usbd_cdc_interface.o \
./Src/usbd_conf.o \
./Src/usbd_desc.o 

C_DEPS += \
./Src/date_time.d \
./Src/main.d \
./Src/measure_main.d \
./Src/stm32l1xx_hal_msp.d \
./Src/stm32l1xx_it.d \
./Src/system_stm32l1xx.d \
./Src/usb_main.d \
./Src/usbd_cdc_interface.d \
./Src/usbd_conf.d \
./Src/usbd_desc.d 


# Each subdirectory must supply rules for building sources it contributes
Src/%.o Src/%.su: ../Src/%.c Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32L152xC -DRTC_CLOCK_SOURCE_LSE '-DSTAT_MEM_SIZE=192*1024' -c -I../Src/Inc -I../Drivers/BSP/STM32L152DISC -I../../Drivers/CMSIS/Device/ST/STM32L1xx/Include -I../../Drivers/STM32L1xx_HAL_Driver/Inc -I../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -I../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../../Middlewares/Third_Party/FreeRTOS/Source/include -I../../Drivers/CMSIS/Include -I../../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -Os -ffunction-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Src/usbd_cdc_interface.o: ../Src/usbd_cdc_interface.c Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32L152xC -DRTC_CLOCK_SOURCE_LSE '-DSTAT_MEM_SIZE=192*1024' -c -I../Src/Inc -I../Drivers/BSP/STM32L152DISC -I../../Drivers/CMSIS/Device/ST/STM32L1xx/Include -I../../Drivers/STM32L1xx_HAL_Driver/Inc -I../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -I../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../../Middlewares/Third_Party/FreeRTOS/Source/include -I../../Drivers/CMSIS/Include -I../../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -Os -ffunction-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Src/usbd_conf.o: ../Src/usbd_conf.c Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32L152xC -DRTC_CLOCK_SOURCE_LSE '-DSTAT_MEM_SIZE=192*1024' -c -I../Src/Inc -I../Drivers/BSP/STM32L152DISC -I../../Drivers/CMSIS/Device/ST/STM32L1xx/Include -I../../Drivers/STM32L1xx_HAL_Driver/Inc -I../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -I../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../../Middlewares/Third_Party/FreeRTOS/Source/include -I../../Drivers/CMSIS/Include -I../../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -Os -ffunction-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Src/usbd_desc.o: ../Src/usbd_desc.c Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32L152xC -DRTC_CLOCK_SOURCE_LSE '-DSTAT_MEM_SIZE=192*1024' -c -I../Src/Inc -I../Drivers/BSP/STM32L152DISC -I../../Drivers/CMSIS/Device/ST/STM32L1xx/Include -I../../Drivers/STM32L1xx_HAL_Driver/Inc -I../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -I../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../../Middlewares/Third_Party/FreeRTOS/Source/include -I../../Drivers/CMSIS/Include -I../../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -Os -ffunction-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Src

clean-Src:
	-$(RM) ./Src/date_time.d ./Src/date_time.o ./Src/date_time.su ./Src/main.d ./Src/main.o ./Src/main.su ./Src/measure_main.d ./Src/measure_main.o ./Src/measure_main.su ./Src/stm32l1xx_hal_msp.d ./Src/stm32l1xx_hal_msp.o ./Src/stm32l1xx_hal_msp.su ./Src/stm32l1xx_it.d ./Src/stm32l1xx_it.o ./Src/stm32l1xx_it.su ./Src/system_stm32l1xx.d ./Src/system_stm32l1xx.o ./Src/system_stm32l1xx.su ./Src/usb_main.d ./Src/usb_main.o ./Src/usb_main.su ./Src/usbd_cdc_interface.d ./Src/usbd_cdc_interface.o ./Src/usbd_cdc_interface.su ./Src/usbd_conf.d ./Src/usbd_conf.o ./Src/usbd_conf.su ./Src/usbd_desc.d ./Src/usbd_desc.o ./Src/usbd_desc.su

.PHONY: clean-Src

