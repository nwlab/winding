#include "StepperHAL_Instances.h"
#include "StepperHAL_Config.h"

// Questo file ora contiene esclusivamente gli IRQ handlers.
// Le istanze motor1..4 sono definite in StepperHAL_Config.cpp

extern "C" void TIM2_IRQHandler(void) {
    if (__HAL_TIM_GET_FLAG(&motor1.getHandle(), TIM_FLAG_UPDATE) != RESET &&
        __HAL_TIM_GET_IT_SOURCE(&motor1.getHandle(), TIM_IT_UPDATE) != RESET) {
        __HAL_TIM_CLEAR_IT(&motor1.getHandle(), TIM_IT_UPDATE);
        motor1.handleInterrupt();
    }
}

extern "C" void TIM3_IRQHandler(void) {
    if (__HAL_TIM_GET_FLAG(&motor2.getHandle(), TIM_FLAG_UPDATE) != RESET &&
        __HAL_TIM_GET_IT_SOURCE(&motor2.getHandle(), TIM_IT_UPDATE) != RESET) {
        __HAL_TIM_CLEAR_IT(&motor2.getHandle(), TIM_IT_UPDATE);
        motor2.handleInterrupt();
    }
}

extern "C" void TIM4_IRQHandler(void) {
    if (__HAL_TIM_GET_FLAG(&motor3.getHandle(), TIM_FLAG_UPDATE) != RESET &&
        __HAL_TIM_GET_IT_SOURCE(&motor3.getHandle(), TIM_IT_UPDATE) != RESET) {
        __HAL_TIM_CLEAR_IT(&motor3.getHandle(), TIM_IT_UPDATE);
        motor3.handleInterrupt();
    }
}

extern "C" void TIM5_IRQHandler(void) {
    if (__HAL_TIM_GET_FLAG(&motor4.getHandle(), TIM_FLAG_UPDATE) != RESET &&
        __HAL_TIM_GET_IT_SOURCE(&motor4.getHandle(), TIM_IT_UPDATE) != RESET) {
        __HAL_TIM_CLEAR_IT(&motor4.getHandle(), TIM_IT_UPDATE);
        motor4.handleInterrupt();
    }
}
