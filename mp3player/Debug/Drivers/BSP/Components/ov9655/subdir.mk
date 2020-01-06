################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/BSP/Components/ov9655/ov9655.c 

OBJS += \
./Drivers/BSP/Components/ov9655/ov9655.o 

C_DEPS += \
./Drivers/BSP/Components/ov9655/ov9655.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/BSP/Components/ov9655/ov9655.o: ../Drivers/BSP/Components/ov9655/ov9655.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32F746xx -DDEBUG -c -I../Library/helix -I../Library/helix/pub -I../Library/helix/real -I../Drivers/BSP/Components -I../Utilities/CPU -I../Utilities/Log -I../Utilities/Fonts -I../Drivers/BSP/STM32746G-Discovery -I../Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Drivers/CMSIS/Device/ST/STM32F7xx/Include -I../Drivers/STM32F7xx_HAL_Driver/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Drivers/CMSIS/Include -I../Core/Inc -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM7/r0p1 -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../Drivers/STM32F7xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FatFs/src -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Drivers/BSP/Components/ov9655/ov9655.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

