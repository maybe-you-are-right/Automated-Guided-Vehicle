# CLAUDE.md

This file provides guidance for Claude Code (claude.ai/code) when working in this repository.

## 1. Purpose

The repository implements an infrared line-following robot on Renesas RA4M2. The main handwritten firmware lives in `src/`; `ra_gen/`, `ra/`, and `Debug/ra_gen/` contain FSP-generated support code.

## 2. Where to make changes

Primary source code locations:
- `src/hal_entry.c` - main entry and periodic 10ms loop
- `src/app/APP_MOTOR.c` / `APP_MOTOR.h` - line-follow logic and motor control with dual-loop architecture
- `src/algorithm/pid.c` / `pid.h` - PID controller implementation
- `src/algorithm/filter.c` / `filter.h` - moving average and 1st-order low-pass IIR filters
- `src/bsp/bsp_motor.c` / `bsp_motor.h` - motor abstraction and PWM control
- `src/bsp/bsp_track.c` / `bsp_track.h` - line sensor interface
- `src/bsp/bsp_encoder.c` / `bsp_encoder.h` - incremental encoder interface with velocity sampling
- `src/bsp/bsp_uart.c` / `bsp_uart.h` - UART BSP layer with ring buffer
- `src/app/app_uart.c` / `app_uart.h` - application-level UART interface

Generated or auto-generated files to avoid unless necessary:
- `ra_gen/`
- `Debug/ra_gen/`
- `ra/`
- `ra_cfg/`
- `configuration.xml`

## 3. Build commands

Use the `Debug` directory and the existing Makefile:

```bash
cd Debug
make
make clean
make all
```

Generated outputs:
- `game_project1.elf` - main ELF binary
- `game_project1.hex` - Intel HEX format for flashing
- `game_project1.srec` - Motorola S-record format
- `game_project1.map` - linker map file
- `game_project1.siz` - size analysis

## 4. Key architecture

### 4.1 Main control loop (dual-loop architecture)

- `src/hal_entry.c`
  - main firmware entry point
  - calls `app_motor_line_follow_update()` every 10ms
  - initializes UART, encoder, and motor subsystems
- `src/app/APP_MOTOR.c`
  - **Outer loop**: 8-channel line sensor weighted centroid → normalized error [-1,1] → PID → differential duty cycle
  - **Inner loop** (optional): target duty cycle → target velocity → velocity PID (encoder feedback) → duty cycle correction
  - provides both line-following and manual motor control modes

### 4.2 Algorithm modules

- `src/algorithm/pid.c`
  - discrete PID implementation with clamped integral and output limits
- `src/algorithm/filter.c`
  - moving average filter (float32, ring buffer based)
  - 1st-order low-pass IIR filter: y = α·x + (1-α)·y_prev
  - used for encoder velocity smoothing

### 4.3 BSP layer (hardware abstraction)

- `src/bsp/bsp_motor.c`
  - dual PWM per wheel (GPT2/GPT4, GTIOCA/GTIOCB pairs)
  - left motor: P103/P102 (GPT2), right motor: P302/P301 (GPT4)
  - direction control pins for each motor
- `src/bsp/bsp_track.c`
  - 8-channel multiplexed infrared line sensor
  - address lines: P000-P002, data input: P013
  - channel switching with configurable delay
- `src/bsp/bsp_encoder.c`
  - incremental encoder interface with A/B phase detection
  - left encoder: A=P402 (IRQ), B=P409 (GPIO)
  - right encoder: A=P104 (IRQ), B=P200 (GPIO)
  - 13 pulses/rev × 30:1 gear ratio = 390 counts/wheel revolution
  - provides pulse count, distance (m), and velocity (m/s)
- `src/bsp/bsp_uart.c`
  - SCI9 UART BSP layer with interrupt-driven ring buffer
  - TX: P109, RX: P110
  - blocking write and buffered read interfaces

## 5. Project conventions

- Chinese comments and documentation
- Function naming: `layer_module_action()`
- Use `_t` suffix for type names
- Target language: C99
- Compiler flags: `-O2`, `-Wall -Wextra`
- Floating-point ABI: `hard`

## 6. Practical guidance

- Tune line-following via PID constants in `APP_MOTOR.c`
- Use `HAL_LINE_FOLLOW_BASE_SPEED` and `HAL_LINE_FOLLOW_PERIOD_MS` to adjust behavior
- Use `app_motor_set_differential_manual()` for hardware validation
- For UART communication, use `bsp_uart` layer functions; initialize with `uart_bsp_init()`
- For encoder feedback: call `encoder_bsp_velocity_sample(dt_sec)` each control period before `app_motor_line_follow_update()`
- Use filter module (`src/algorithm/filter.h`) for signal smoothing; encoder velocity uses 1st-order low-pass IIR
- Prefer small, local changes over broad refactors in firmware code

## 7. Hardware configuration

Key pin mappings (see `docs/引脚配置说明.md` for details):
- **Motor control**: Left PWM (P103/P102 on GPT2), Right PWM (P302/P301 on GPT4), direction pins per motor
- **Encoders**: Left encoder A=P402 (IRQ), B=P409; Right encoder A=P104 (IRQ), B=P200
- **Line sensor**: Address lines (P000-P002), sensor output (P013)
- **UART**: SCI9 TXD (P109), RXD (P110) for debug/communication
- **Debug**: SWCLK (P300), SWDIO (P108)

Target MCU: Renesas RA4M2 (R7FA4M2AD3CFL)

## 8. Useful references

- `Debug/compile_commands.json` - compilation database for IDE integration
- `Debug/fsp.ld` - linker script
- `ra_cfg/` - FSP configuration headers
- `ra_gen/` - generated FSP initialization code
- `docs/引脚配置说明.md` - detailed hardware pin configuration and mappings, including encoder IRQ setup
