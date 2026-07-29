# STM32 Bluetooth-Controlled Robot Car

Embedded firmware for an STM32 microcontroller-based robot car, controlled wirelessly via a Bluetooth-to-UART bridge and a companion mobile app. Implements a custom serial command protocol, PWM-based differential drive motor control, and a mode system for manual driving vs. autonomous obstacle avoidance.

# Overview

The robot receives text commands over UART (typically relayed from a phone via a Bluetooth module like HC-05/HC-06), parses them character-by-character using interrupt-driven reception, and translates them into motor movements. Two independently controlled wheels (via PWM duty cycle on TIM2) allow forward/backward motion and differential turning.

# Command Protocol

Commands are sent as newline-terminated ASCII strings over UART (9600 baud):

| Command | Action |
|---|---|
| `GO` | Move forward |
| `BACK` | Move backward |
| `IZQ` | Turn left |
| `DER` | Turn right |
| `STP` | Stop |
| `MINxx` | Set minimum obstacle distance threshold (cm) |
| `MAXxx` | Set maximum obstacle distance threshold (cm) |
| `MODE_MANUAL` | Switch to manual (remote-controlled) mode |
| `MODE_AUTO` | Switch to autonomous mode |

The firmware responds with a status message for each command (e.g. `"going ahead\n"`, `"turning left\n"`, `"invalid command"`), confirming execution back to the connected app.

# Hardware & Peripherals

- **MCU:** STM32 (STM32L0-series, based on HAL driver includes)
- **Motor control:** TIM2 configured for dual-channel PWM (CH3/CH4) driving left/right wheel speed independently; GPIO pins set direction (forward/reverse) per wheel
- **Communication:** USART1 (9600 baud, interrupt-driven RX) for the Bluetooth/UART link
- **Sensing:** ADC1 configured for distance sensing (obstacle detection thresholds), triggered via TIM2
- **Display:** LCD segment GPIO configuration (STM32 LCD peripheral)

# Key Implementation Details

- **Interrupt-driven UART reception** (`HAL_UART_RxCpltCallback`) buffers incoming characters until a newline, then parses and executes the full command
- **Differential drive logic:** turning is implemented by driving one wheel forward while the other reverses (`girar_izq`/`girar_der`), rather than simply stopping one side
- **PWM speed control** via `TIM2->CCR3` / `CCR4` compare registers, with direction set via GPIO `BSRR` (set/reset) registers for precise, atomic pin control
- **Mode-gated command execution:** movement commands are only accepted in manual mode; `MODE_AUTO`/`MODE_MANUAL` guard against redundant mode switches with dedicated status messages

# Current Status

- Manual (remote-controlled) driving is fully implemented and functional
- Autonomous mode (`MODE_AUTO`) sets the mode flag and threshold parameters (`MIN`/`MAX` distance), but the obstacle-avoidance control loop itself is not yet implemented in the main loop — this is the natural next step for the project

# Contents

Full STM32CubeIDE project, including CubeMX-generated peripheral configuration and HAL drivers alongside custom application logic in `main.c`.

# Tech Stack

- **Language:** C
- **Platform:** STM32 (HAL library, STM32CubeIDE / STM32CubeMX generated project)
- **Communication:** UART/Bluetooth serial protocol
- **Control:** Timer-based PWM motor control, ADC-based distance sensing

# Possible Improvements

- Implement the autonomous obstacle-avoidance loop using the ADC distance readings and `MIN`/`MAX` thresholds already parsed
- Add debouncing/error handling for malformed UART commands

# Author

Kawtharul Jannah Mohd Sukki
