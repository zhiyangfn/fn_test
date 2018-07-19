################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../vendor/light_8267/libdiylight.c \
../vendor/light_8267/libscenes.c \
../vendor/light_8267/libworkthread.c \
../vendor/light_8267/main.c \
../vendor/light_8267/main_light.c \
../vendor/light_8267/tl8267_uart.c \
../vendor/light_8267/vendor_att_light.c \
../vendor/light_8267/vendor_light.c 

OBJS += \
./vendor/light_8267/libdiylight.o \
./vendor/light_8267/libscenes.o \
./vendor/light_8267/libworkthread.o \
./vendor/light_8267/main.o \
./vendor/light_8267/main_light.o \
./vendor/light_8267/tl8267_uart.o \
./vendor/light_8267/vendor_att_light.o \
./vendor/light_8267/vendor_light.o 


# Each subdirectory must supply rules for building sources it contributes
vendor/light_8267/%.o: ../vendor/light_8267/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: TC32 Compiler'
	tc32-elf-gcc -ffunction-sections -fdata-sections -I"E:\TelinkSDK\opt\tc32\lib\gcc\tc32-elf\4.5.1-tc32-1.3\include" -D__PROJECT_LIGHT_8267__=1 -Wall -O2 -fpack-struct -fshort-enums -finline-small-functions -std=gnu99 -fshort-wchar -fms-extensions -c -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


