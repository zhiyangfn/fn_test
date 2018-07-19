################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../proj/ext_drv/mpu6050.c 

OBJS += \
./proj/ext_drv/mpu6050.o 


# Each subdirectory must supply rules for building sources it contributes
proj/ext_drv/%.o: ../proj/ext_drv/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: TC32 Compiler'
	tc32-elf-gcc -ffunction-sections -fdata-sections -I"E:\TelinkSDK\opt\tc32\lib\gcc\tc32-elf\4.5.1-tc32-1.3\include" -D__PROJECT_LIGHT_8267__=1 -Wall -O2 -fpack-struct -fshort-enums -finline-small-functions -std=gnu99 -fshort-wchar -fms-extensions -c -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


