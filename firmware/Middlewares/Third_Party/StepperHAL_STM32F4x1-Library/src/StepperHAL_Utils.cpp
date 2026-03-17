#include "StepperHAL_Utils.h"

const char* pinName(uint8_t pin) {
  switch (pin) {
    // GPIOA
    case PA_0:  return "PA0";
    case PA_1:  return "PA1";
    case PA_2:  return "PA2";
    case PA_3:  return "PA3";
    case PA_4:  return "PA4";
    case PA_5:  return "PA5";
    case PA_6:  return "PA6";
    case PA_7:  return "PA7";
    case PA_8:  return "PA8";
    case PA_9:  return "PA9";
    case PA_10: return "PA10";
    case PA_11: return "PA11";
    case PA_12: return "PA12";
    case PA_13: return "PA13";
    case PA_14: return "PA14";
    case PA_15: return "PA15";

    // GPIOB
    case PB_0:  return "PB0";
    case PB_1:  return "PB1";
    case PB_2:  return "PB2";
    case PB_3:  return "PB3";
    case PB_4:  return "PB4";
    case PB_5:  return "PB5";
    case PB_6:  return "PB6";
    case PB_7:  return "PB7";
    case PB_8:  return "PB8";
    case PB_9:  return "PB9";
    case PB_10: return "PB10";
    case PB_11: return "PB11";
    case PB_12: return "PB12";
    case PB_13: return "PB13";
    case PB_14: return "PB14";
    case PB_15: return "PB15";

    // GPIOC (solo quelli disponibili sulla Blackpill)
    case PC_13: return "PC13";
    case PC_14: return "PC14";
    case PC_15: return "PC15";

    default:    return "PIN?";
  }
}

const char* timerName(TIM_TypeDef* timer) {
  if (timer == TIM2) return "TIM2";
  if (timer == TIM3) return "TIM3";
  if (timer == TIM4) return "TIM4";
  if (timer == TIM5) return "TIM5";
  return "TIM?";
}
