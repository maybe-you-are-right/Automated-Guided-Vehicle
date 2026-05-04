################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/bsp/bsp_encoder.c \
../src/bsp/bsp_motor.c \
../src/bsp/bsp_track.c \
../src/bsp/bsp_uart.c 

C_DEPS += \
./src/bsp/bsp_encoder.d \
./src/bsp/bsp_motor.d \
./src/bsp/bsp_track.d \
./src/bsp/bsp_uart.d 

OBJS += \
./src/bsp/bsp_encoder.o \
./src/bsp/bsp_motor.o \
./src/bsp/bsp_track.o \
./src/bsp/bsp_uart.o 

SREC += \
game_project1.srec 

MAP += \
game_project1.map 


# Each subdirectory must supply rules for building sources it contributes
src/bsp/%.o: ../src/bsp/%.c
	$(file > $@.in,-mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O2 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-strict-aliasing -Wunused -Wuninitialized -Wall -Wextra -Wmissing-declarations -Wconversion -Wpointer-arith -Wshadow -Wlogical-op -Waggregate-return -Wfloat-equal -g -D_RENESAS_RA_ -D_RA_CORE=CM33 -D_RA_ORDINAL=1 -I"D:/e2_studio/workspace/game_project1/ra_gen" -I"." -I"D:/e2_studio/workspace/game_project1/ra_cfg/fsp_cfg/bsp" -I"D:/e2_studio/workspace/game_project1/ra_cfg/fsp_cfg" -I"D:/e2_studio/workspace/game_project1/src" -I"D:/e2_studio/workspace/game_project1/ra/fsp/inc" -I"D:/e2_studio/workspace/game_project1/ra/fsp/inc/api" -I"D:/e2_studio/workspace/game_project1/ra/fsp/inc/instances" -I"D:/e2_studio/workspace/game_project1/ra/arm/CMSIS_6/CMSIS/Core/Include" -std=c99 -Wno-stringop-overflow -Wno-format-truncation --param=min-pagesize=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" -x c "$<")
	@echo Building file: $< && arm-none-eabi-gcc @"$@.in"

