################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../vendor/bqb/phytest.c \
../vendor/bqb/rf_test.c 

OBJS += \
./vendor/bqb/phytest.o \
./vendor/bqb/rf_test.o 


# Each subdirectory must supply rules for building sources it contributes
vendor/bqb/%.o: ../vendor/bqb/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: TC32 Compiler'
	tc32-elf-gcc -ffunction-sections -fdata-sections -D__PROJECT_LIGHT_8267__=1 -Wall -O2 -fpack-struct -fshort-enums -finline-small-functions -std=gnu99 -fshort-wchar -fms-extensions -c -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

