################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../custom/fota/src/fota_ftp.c \
../custom/fota/src/fota_http.c \
../custom/fota/src/fota_http_code.c \
../custom/fota/src/fota_main.c 

OBJS += \
./custom/fota/src/fota_ftp.o \
./custom/fota/src/fota_http.o \
./custom/fota/src/fota_http_code.o \
./custom/fota/src/fota_main.o 

C_DEPS += \
./custom/fota/src/fota_ftp.d \
./custom/fota/src/fota_http.d \
./custom/fota/src/fota_http_code.d \
./custom/fota/src/fota_main.d 


# Each subdirectory must supply rules for building sources it contributes
custom/fota/src/%.o: ../custom/fota/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Windows GCC C Compiler (Sourcery Lite Bare)'
	arm-none-eabi-gcc -D__OCPU_COMPILER_GCC__ -D__EXAMPLE_MQTT__ -I"${GCC_PATH}\arm-none-eabi\include" -I"D:\Codes\Base\MC60_MQTT_OC_TEST\include" -I"D:\Codes\Base\MC60_MQTT_OC_TEST\ril\inc" -I"D:\Codes\Base\MC60_MQTT_OC_TEST\custom\config" -I"D:\Codes\Base\MC60_MQTT_OC_TEST\custom\fota\inc" -O2 -Wall -std=c99 -c -fmessage-length=0 -mlong-calls -Wstrict-prototypes -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -march=armv5te -mthumb-interwork -mfloat-abi=soft -g3 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


