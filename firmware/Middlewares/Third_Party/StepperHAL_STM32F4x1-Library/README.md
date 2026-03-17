# StepperHAL_STM32F4x1

## Description

StepperHAL is a C++ library designed as a Hardware Abstraction Layer (HAL) for controlling stepper motors on the STM32F4x1 microcontroller family.
The library was developed to simplify motion control by offering an intuitive and configurable interface that integrates with the Arduino framework and STMicroelectronics' HAL.

## Description

StepperHAL is a C++ library designed as a Hardware Abstraction Layer (HAL) for controlling stepper motors on STM32F4x1 microcontrollers.
It simplifies motion control by providing an intuitive and configurable interface that integrates with both the Arduino framework and STMicroelectronics' HAL.

---

## ✨ Main Functionalities / Key Features

- **Flexible Control**
Motion management at constant speed or with acceleration/deceleration profiles.
**Flexible Control**: Supports constant-speed motion and acceleration/deceleration profiles.

- **Motion Profiles**
Support for trapezoidal and S-curve profiles, enabled via configuration.
**Motion Profiles**: Trapezoidal and S-curve profiles for smooth and precise motion.

- **Extended Configuration**
Motor and pin parameters centrally managed (steps/rev, microsteps, mm/rev).
**Extended Configuration**: Centralized setup of motor parameters and pin mapping.

- **Debug and Diagnostics**
Detailed messages via serial link for monitoring and troubleshooting.
**Debug & Diagnostics**: Serial output for detailed runtime feedback and troubleshooting.

---

## ✨ Requirements

### 🖥️ Software

- **Arduino Framework for STM32**
Based on Arduino_Core_STM32 (stm32duino).
**Arduino Framework for STM32**: Built on Arduino_Core_STM32 (stm32duino).

- **Recommended IDE**: Arduino IDE or Visual Studio Code with PlatformIO.

**Recommended IDE**: Arduino IDE or VS Code with PlatformIO.

### 🔩 Hardware

- **STM32F4x1 Microcontroller**
Optimized for this family, compatible with other STM32s with minimal modifications.
**STM32F4x1 Microcontroller**: Optimized for this family, adaptable to other STM32 variants.

- **Stepper motor drivers**

e.g., A4988, DRV8825, connected to the pins defined in `StepperHAL_Config.h`.

**Stepper motor drivers**: e.g., A4988, DRV8825, connected via `StepperHAL_Config.h`.

- **Stepper motors**

Supports NEMA17, NEMA23, or similar.

**Stepper motors**: Supports NEMA17, NEMA23, and similar models.

- **Development board**
e.g., STM32 "Black Pill" or Nucleo.

**Development board**: e.g., STM32 "Black Pill" or Nucleo.

---