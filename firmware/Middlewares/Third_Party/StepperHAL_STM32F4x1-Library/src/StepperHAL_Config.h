#pragma once

// ---------- Feature selection (0 = off, 1 = on) ----------
// Modifica qui per cambiare il comportamento di build.
// Puoi sovrascrivere da riga di comando: -DUSE_S_CURVE=1 oppure -DUSE_S_CURVE=0

#ifndef USE_S_CURVE
  #define USE_S_CURVE 1
#endif

#ifndef USE_TRAPEZOIDAL
  #define USE_TRAPEZOIDAL 1
#endif

// Compile-time informative messages (visibili in fase di build)
#if !defined(USE_S_CURVE) || (USE_S_CURVE == 0)
  #pragma message("Note: USE_S_CURVE disabled (0) or not defined: S-curve APIs will NOT be available in this build.")
#endif

#if !defined(USE_TRAPEZOIDAL) || (USE_TRAPEZOIDAL == 0)
  #pragma message("Note: USE_TRAPEZOIDAL disabled (0) or not defined: Trapezoidal profile APIs will NOT be available in this build.")
#endif

#if ( (!defined(USE_S_CURVE) || (USE_S_CURVE == 0)) && (!defined(USE_TRAPEZOIDAL) || (USE_TRAPEZOIDAL == 0)) )
  #pragma message("Note: No motion profile selected: forcing USE_TRAPEZOIDAL as fallback.")
  #undef USE_TRAPEZOIDAL
  #define USE_TRAPEZOIDAL 1
#endif

// ---------- Motor pin mapping (board specific) ----------
#define PIN_DIR_MOTOR1  PA_9
#define PIN_EN_MOTOR1   PA_8

#define PIN_DIR_MOTOR2  PB_9
#define PIN_EN_MOTOR2   PB_3

#define PIN_DIR_MOTOR3  PB_10
#define PIN_EN_MOTOR3   PB_8

#define PIN_DIR_MOTOR4  PA_1
#define PIN_EN_MOTOR4   -1

// ---------- Motor constructor parameters ----------
// Formato: (timer, channel, dirPin, enPin, stepsPerRev, microstep, mmPerRev)
#define MOTOR1_PARAMS  TIM2, TIM_CHANNEL_1, PIN_DIR_MOTOR1, PIN_EN_MOTOR1, 200, 256, 2.0f
//#define MOTOR2_PARAMS  TIM3, TIM_CHANNEL_2, PIN_DIR_MOTOR2, PIN_EN_MOTOR2, 1000, 1.0f, 2.0f
#define MOTOR2_PARAMS  TIM3, TIM_CHANNEL_2, PIN_DIR_MOTOR2, PIN_EN_MOTOR2, 1000, 2.0f
#define MOTOR3_PARAMS  TIM4, TIM_CHANNEL_1, PIN_DIR_MOTOR3, PIN_EN_MOTOR3, 200, 256, 2.0f
#define MOTOR4_PARAMS  TIM5, TIM_CHANNEL_3, PIN_DIR_MOTOR4, PIN_EN_MOTOR4, 1000, 1.0f, 2.0f

#ifndef DEFAULT_MIN_FREQ_HZ
  #define DEFAULT_MIN_FREQ_HZ  200.0f
#endif

#ifndef DEFAULT_JERK_RATIO
  #define DEFAULT_JERK_RATIO   1.0f
#endif

// Limiti per il rapporto tra jerk di decelerazione e accelerazione
#ifndef JERK_RATIO_MIN
#define JERK_RATIO_MIN   0.2f    // minimo 20% di decel vs accel
#endif

#ifndef JERK_RATIO_MAX
#define JERK_RATIO_MAX  5.0f    // massimo 5× decel vs accel
#endif

