################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/app_bluenrg.c \
../Core/Src/connection_handler.c \
../Core/Src/flash_manager.c \
../Core/Src/gpio.c \
../Core/Src/main.c \
../Core/Src/service.c \
../Core/Src/stm32g0xx_hal_msp.c \
../Core/Src/stm32g0xx_it.c \
../Core/Src/stm32g0xx_nucleo_bus.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32g0xx.c \
../Core/Src/usart.c 

OBJS += \
./Core/Src/app_bluenrg.o \
./Core/Src/connection_handler.o \
./Core/Src/flash_manager.o \
./Core/Src/gpio.o \
./Core/Src/main.o \
./Core/Src/service.o \
./Core/Src/stm32g0xx_hal_msp.o \
./Core/Src/stm32g0xx_it.o \
./Core/Src/stm32g0xx_nucleo_bus.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32g0xx.o \
./Core/Src/usart.o 

C_DEPS += \
./Core/Src/app_bluenrg.d \
./Core/Src/connection_handler.d \
./Core/Src/flash_manager.d \
./Core/Src/gpio.d \
./Core/Src/main.d \
./Core/Src/service.d \
./Core/Src/stm32g0xx_hal_msp.d \
./Core/Src/stm32g0xx_it.d \
./Core/Src/stm32g0xx_nucleo_bus.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32g0xx.d \
./Core/Src/usart.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32G0B1xx -c -I../BlueNRG-MS/Target -I../Core/Inc -I../Drivers/STM32G0xx_HAL_Driver/Inc -I../Drivers/STM32G0xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32G0xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/ST/BlueNRG-MS/utils -I../Middlewares/ST/BlueNRG-MS/includes -I../Middlewares/ST/BlueNRG-MS/hci/hci_tl_patterns/Basic -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/app_bluenrg.d ./Core/Src/app_bluenrg.o ./Core/Src/app_bluenrg.su ./Core/Src/connection_handler.d ./Core/Src/connection_handler.o ./Core/Src/connection_handler.su ./Core/Src/flash_manager.d ./Core/Src/flash_manager.o ./Core/Src/flash_manager.su ./Core/Src/gpio.d ./Core/Src/gpio.o ./Core/Src/gpio.su ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/service.d ./Core/Src/service.o ./Core/Src/service.su ./Core/Src/stm32g0xx_hal_msp.d ./Core/Src/stm32g0xx_hal_msp.o ./Core/Src/stm32g0xx_hal_msp.su ./Core/Src/stm32g0xx_it.d ./Core/Src/stm32g0xx_it.o ./Core/Src/stm32g0xx_it.su ./Core/Src/stm32g0xx_nucleo_bus.d ./Core/Src/stm32g0xx_nucleo_bus.o ./Core/Src/stm32g0xx_nucleo_bus.su ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32g0xx.d ./Core/Src/system_stm32g0xx.o ./Core/Src/system_stm32g0xx.su ./Core/Src/usart.d ./Core/Src/usart.o ./Core/Src/usart.su

.PHONY: clean-Core-2f-Src

