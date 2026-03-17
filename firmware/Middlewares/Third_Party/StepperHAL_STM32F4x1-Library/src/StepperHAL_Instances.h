#ifndef STEPPERHAL_INSTANCES_H
#define STEPPERHAL_INSTANCES_H

#include "StepperHAL_STM32F4x1.h"

// Istanze dei motori
extern StepperHAL motor1; // TIM2 → PIN_STEP → PA_15
extern StepperHAL motor2; // TIM3 → PIN_STEP → PB_5
extern StepperHAL motor3; // TIM4 → PIN_STEP → PB_6
extern StepperHAL motor4; // TIM5 → PIN_STEP → PA_2

#endif



