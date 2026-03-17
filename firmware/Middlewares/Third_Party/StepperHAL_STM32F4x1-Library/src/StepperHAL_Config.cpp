// StepperHAL_Config.cpp
// Definizione centralizzata delle istanze motore usando i parametri in StepperHAL_Config.h

#include "StepperHAL_Config.h"
#include "StepperHAL_Instances.h"
#include "StepperHAL_STM32F4x1.h"

// Le istanze globali (una sola fonte di verità)
StepperHAL motor1(MOTOR1_PARAMS);
StepperHAL motor2(MOTOR2_PARAMS);
StepperHAL motor3(MOTOR3_PARAMS);
StepperHAL motor4(MOTOR4_PARAMS);
