// StepperHAL_STM32F4x1.cpp

#include "StepperHAL_STM32F4x1.h"
#include "StepperHAL_Config.h" // MUST come prima per definire StepperHAL_DEBUG_ENABLE e feature
#include "StepperHAL_Debug.h"
#include "StepperHAL_Lang.h"
#include "StepperHAL_Utils.h"
#include "stm32f4xx_hal_gpio.h"
#include <math.h>
#include <string.h>


// -----------------------------
// Constructors
// -----------------------------
StepperHAL::StepperHAL(TIM_TypeDef *timer, uint32_t channel, uint8_t dirPin, int8_t enPin, uint32_t stepsMotorRev,
                       float microstep, float mmPerRev)
    : _timer(timer), _htim{} // default init
      ,
      _timerClockHz(0), _dirPin(dirPin), _enPin(enPin), _stepPin(PA_0) // placeholder, overwritten in the body
      ,
      _channel(channel), _stepsMotorRev(stepsMotorRev), _microstep(microstep), _mmPerRev(mmPerRev),
      _invertDirection(false), _active(false), _useDMA(false), _pendingStop(false), _position(0), _logicalPosition(0),
      _target(0), _effectiveTarget(0), _direction(true), _moveDir(0), _lastDir(0), _pendingLogical(0),
      _backlashMM(mm(0.0f)), _backlashStep(step(0)), _useBacklashMM(false), _useBacklashStep(false),
      _directionInitialized(false), _timing(), _currentPhase(PHASE_IDLE), _accelTable(nullptr), _decelTable(nullptr),
      _accelSize(0), _decelSize(0), _accelIndex(0), _decelIndex(0), _cruiseARR(0), _cruiseSize(0), _cruiseRemaining(0),
      _hdma{}, _dmaIndex(0), _targetARR(0), _holdPhase(false), _decelPhase(false), _diag_accelIndex(0),
      _diag_cruiseRemaining(0), _diag_decelIndex(0), _diag_phase(0), _useSCurve(false), _jerkSymmetric(true),
      _jerkAccMMs3(2500.0f), _jerkDecMMs3(2000.0f), _useManualJerk(false), _jerkAccelUp(0.0f), _jerkAccelFlat(0.0f),
      _jerkAccelDown(0.0f), _jerkDecelUp(0.0f), _jerkDecelFlat(0.0f), _jerkDecelDown(0.0f), _jerkRatio(1.0f),
      _minFreqHz(0.0f), _label(nullptr)
{
    // Non leggere clock qui: lo facciamo in begin() per sicurezza
    if (_timer == TIM2)
        _stepPin = PA_15;
    else if (_timer == TIM3)
        _stepPin = PB_5;
    else if (_timer == TIM4)
        _stepPin = PB_6;
    else if (_timer == TIM5)
        _stepPin = PA_2;
    else
        _stepPin = PA_0;
}
// costruttore semplificato che passa effectiveStepsMotorRev al posto
// di (stepsMotorRev, microstep=1.0f)
StepperHAL::StepperHAL(TIM_TypeDef *timer, uint32_t channel, uint8_t dirPin, int8_t enPin,
                       uint32_t effectiveStepsMotorRev, float mmPerRev)
    : StepperHAL(timer, channel, dirPin, enPin,
                 /* stepsMotorRev = */ effectiveStepsMotorRev,
                 /* microstep    = */ 1.0f,
                 /* mmPerRev     = */ mmPerRev)
{
}

// -----------------------------
// Setup
// -----------------------------
void StepperHAL::begin()
{
    _timerClockHz = HAL_RCC_GetSysClockFreq();
    configureStepPin();
    configureDirPin();
    if (_enPin >= 0)
    {
        configureEnPin();
    }

    SerialDB.print("[StepperHAL] (");
    SerialDB.print(getLabel());
    SerialDB.print(") ");
    SerialDB.print(msg("pin configurati: | STEP: ", "pin configured: | STEP: "));
    SerialDB.print(pinName(_stepPin));
    SerialDB.print(" | ");
    SerialDB.print(msg("DIR: ", "DIR: "));
    SerialDB.print(pinName(_dirPin));
    SerialDB.print(" | ");
    SerialDB.print(msg("EN: ", "EN: "));
    SerialDB.println(_enPin >= 0 ? pinName(_enPin) : msg("nessuno", "none"));

    float stepsPerRev = getStepsPerRev();
    float mmPerRev = _mmPerRev;
    float mmPerStep = mmPerRev / stepsPerRev;
    float stepsPerMm = stepsPerRev / mmPerRev;

    SerialDB.print("[StepperHAL] (");
    SerialDB.print(getLabel());
    SerialDB.print(") ");
    SerialDB.print(msg("Passi effettivi/giro: ", "Effective steps/rev: "));
    SerialDB.print(stepsPerRev);
    SerialDB.print(" | ");
    SerialDB.print(msg("mm/giro: ", "mm/rev: "));
    SerialDB.print(mmPerRev, 2);
    SerialDB.print(" | ");
    SerialDB.print(msg("1 passo = ", "1 step = "));
    SerialDB.print(mmPerStep, 6);
    SerialDB.print(" mm | ");
    SerialDB.print(msg("1 mm = ", "1 mm = "));
    SerialDB.print(stepsPerMm);
    SerialDB.println(msg(" passi", " steps"));
}

void StepperHAL::configureStepPin()
{
    GPIO_InitTypeDef GPIO_InitStruct;
    memset(&GPIO_InitStruct, 0, sizeof(GPIO_InitStruct));

    if (_stepPin >= PA_0 && _stepPin <= PA_15)
        __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (_stepPin >= PB_0 && _stepPin <= PB_15)
        __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_TypeDef *port = (_stepPin >= PB_0 ? GPIOB : GPIOA);
    uint16_t pin = (uint16_t)(1u << (_stepPin - (_stepPin >= PB_0 ? PB_0 : PA_0)));

    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = 0;

    if (_timer == TIM2)
        GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    else if (_timer == TIM3)
        GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
    else if (_timer == TIM4)
        GPIO_InitStruct.Alternate = GPIO_AF2_TIM4;
    else if (_timer == TIM5)
        GPIO_InitStruct.Alternate = GPIO_AF2_TIM5;

    HAL_GPIO_Init(port, &GPIO_InitStruct);
}

void StepperHAL::configureDirPin()
{
    GPIO_InitTypeDef GPIO_InitStruct;
    memset(&GPIO_InitStruct, 0, sizeof(GPIO_InitStruct));

    if (_dirPin >= PA_0 && _dirPin <= PA_15)
        __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (_dirPin >= PB_0 && _dirPin <= PB_15)
        __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_TypeDef *port = (_dirPin >= PB_0 ? GPIOB : GPIOA);
    uint16_t pin = (uint16_t)(1u << (_dirPin - (_dirPin >= PB_0 ? PB_0 : PA_0)));

    HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);

    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = 0;

    HAL_GPIO_Init(port, &GPIO_InitStruct);
}

void StepperHAL::configureEnPin()
{
    GPIO_InitTypeDef GPIO_InitStruct;
    memset(&GPIO_InitStruct, 0, sizeof(GPIO_InitStruct));

    if (_enPin >= PA_0 && _enPin <= PA_15)
        __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (_enPin >= PB_0 && _enPin <= PB_15)
        __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_TypeDef *port = (_enPin >= PB_0 ? GPIOB : GPIOA);
    uint16_t pin = (uint16_t)(1u << (_enPin - (_enPin >= PB_0 ? PB_0 : PA_0)));

    HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = 0;

    HAL_GPIO_Init(port, &GPIO_InitStruct);
}

void StepperHAL::digitalWrite(uint8_t _pin, uint8_t _val)
{
    GPIO_TypeDef *port = (_pin >= PB_0 ? GPIOB : GPIOA);
    uint16_t pin = (uint16_t)(1u << (_pin - (_pin >= PB_0 ? PB_0 : PA_0)));

    HAL_GPIO_WritePin(port, pin, _val ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void StepperHAL::configureTimer(uint32_t arr, uint32_t pulse)
{
    if (_timer == TIM2)
        __HAL_RCC_TIM2_CLK_ENABLE();
    else if (_timer == TIM3)
        __HAL_RCC_TIM3_CLK_ENABLE();
    else if (_timer == TIM4)
        __HAL_RCC_TIM4_CLK_ENABLE();
    else if (_timer == TIM5)
        __HAL_RCC_TIM5_CLK_ENABLE();

    _htim.Instance = _timer;
    _htim.Init.Prescaler = _timing.prescaler;
    _htim.Init.CounterMode = TIM_COUNTERMODE_UP;
    _htim.Init.Period = (arr == 0 ? 1 : arr);
    _htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    _htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    HAL_TIM_Base_Init(&_htim);
    HAL_TIM_PWM_Init(&_htim);

    __HAL_TIM_SET_COUNTER(&_htim, 0);
    __HAL_TIM_SET_COMPARE(&_htim, _channel, 0);

    TIM_OC_InitTypeDef sConfigOC;
    memset(&sConfigOC, 0, sizeof(sConfigOC));

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = (pulse == 0 ? 1 : pulse);
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;

#if defined(TIM_OCNPOLARITY_HIGH)
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
#else
    sConfigOC.OCNPolarity = 0;
#endif

#if defined(TIM_OCFAST_ENABLE)
    sConfigOC.OCFastMode = TIM_OCFAST_ENABLE;
#else
    sConfigOC.OCFastMode = 0;
#endif

#if defined(TIM_OCIDLESTATE_RESET)
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
#else
    sConfigOC.OCIdleState = 0;
#endif

#if defined(TIM_OCNIDLESTATE_RESET)
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
#else
    sConfigOC.OCNIdleState = 0;
#endif

    HAL_TIM_PWM_ConfigChannel(&_htim, &sConfigOC, _channel);

    __HAL_TIM_CLEAR_FLAG(&_htim, TIM_FLAG_UPDATE);
    __HAL_TIM_ENABLE_IT(&_htim, TIM_IT_UPDATE);

    if (_timer == TIM2)
    {
        HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(TIM2_IRQn);
    }
    else if (_timer == TIM3)
    {
        HAL_NVIC_SetPriority(TIM3_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(TIM3_IRQn);
    }
    else if (_timer == TIM4)
    {
        HAL_NVIC_SetPriority(TIM4_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(TIM4_IRQn);
    }
    else if (_timer == TIM5)
    {
        HAL_NVIC_SetPriority(TIM5_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(TIM5_IRQn);
    }
}

void StepperHAL::delay(unsigned long ms)
{
    HAL_Delay(ms);
}

void StepperHAL::delayMicroseconds(unsigned int usec)
{
    if (usec == 0)
    {
        return;
    }

    /*
     *  The following loop:
     *
     *    for (; ul; ul--) {
     *      __asm__ volatile("");
     *    }
     *
     *  produce the following assembly code:
     *
     *    loop:
     *      subs r3, #1        // 1 Core cycle
     *      bne.n loop         // 1 Core cycle + 1 if branch is taken
     */

    // VARIANT_MCK / 1000000 == cycles needed to delay 1uS
    //                     3 == cycles used in a loop
    uint32_t n = usec * (SystemCoreClock / 1000000) / 3;
    __asm__ __volatile__("1:              \n"
                         "   sub %0, #1   \n" // substract 1 from %0 (n)
                         "   bne 1b       \n" // if result is not 0 jump to 1
                         : "+r"(n)            // '%0' is n variable with RW constraints
                         :                    // no input
                         :                    // no clobber
    );
    // https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html
    // https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html#Volatile
}

// -----------------------------
// Helpers
// -----------------------------
uint32_t StepperHAL::clampARRFromFreq(uint32_t timerBaseFreq, float freq)
{
    if (freq <= 0.0f)
        freq = 1.0f;
    float arrf = (float)timerBaseFreq / freq;
    if (arrf < 2.0f)
        arrf = 2.0f;
    if (arrf > 65535.0f)
        arrf = 65535.0f;
    return (uint32_t)arrf;
}

// -----------------------------
// Backlash + planning
// -----------------------------
void StepperHAL::planEffectiveTarget(long requestedTarget)
{
    _target = requestedTarget;
    _pendingLogical = requestedTarget;

    long delta = requestedTarget - _logicalPosition;
    int8_t newDir = (delta > 0) ? 1 : (delta < 0 ? -1 : 0);

    long backlashSteps = resolveBacklashSteps();
    long stepsToDo = llabs(delta);

    if (_lastDir == 0)
    {
        _effectiveTarget = _position + (newDir * stepsToDo);
    }
    else
    {
        if (newDir != _lastDir)
        {
            long currentPreload = _position - _logicalPosition;
            if (currentPreload == 0)
            {
                _effectiveTarget = _position + (newDir * (stepsToDo + backlashSteps));
                stepsToDo += backlashSteps;
            }
            else if ((currentPreload > 0 && newDir > 0) || (currentPreload < 0 && newDir < 0))
            {
                _effectiveTarget = _position + (newDir * stepsToDo);
            }
            else
            {
                _effectiveTarget = _position + (newDir * (stepsToDo + backlashSteps));
                stepsToDo += backlashSteps;
            }
        }
        else
        {
            _effectiveTarget = _position + (newDir * stepsToDo);
        }
    }

    _direction = (newDir < 0);
    _moveDir = newDir;
    _lastDir = newDir;

    int32_t backlashEffettivo = 0;
    if (backlashSteps > 0 && stepsToDo > llabs(delta))
    {
        backlashEffettivo = (newDir > 0) ? backlashSteps : -backlashSteps;
    }

    const char *dirLabel = (newDir > 0) ? "CW" : (newDir < 0 ? "CCW" : "STOP");

    SerialDB.print("[StepperHAL] (");
    SerialDB.print(getLabel());
    SerialDB.print(") ");
    SerialDB.print(msg("Movimento iniziato da posizione logica=", "Movement started from position logical="));
    SerialDB.print(_logicalPosition);
    SerialDB.print(msg(" : reale=", " : real="));
    SerialDB.print(_position);
    SerialDB.print(msg(" / target=", " / target="));
    SerialDB.print(_target);
    SerialDB.print(msg(" / step necessari=", " / steps needed="));
    SerialDB.print(stepsToDo);
    if (backlashEffettivo != 0)
    {
        SerialDB.print(msg(", correzione backlash applicata (", ", backlash correction applied ("));
        SerialDB.print(backlashEffettivo);
        SerialDB.print(")");
    }
    SerialDB.print(msg(" / Dir=", " / Dir="));
    SerialDB.println(dirLabel);
}

// -----------------------------
// Constant-step movement
// -----------------------------
void StepperHAL::moveToTarget(int32_t targetSteps, float freqHz)
{
    int32_t stepsNeeded = targetSteps - _position;

    if (stepsNeeded == 0)
    {
        _logicalPosition = targetSteps;
        _position = targetSteps;
        SerialDB.print("[StepperHAL] (");
        SerialDB.print(getLabel());
        SerialDB.println(msg(") Nessun movimento: già in posizione", ") No movement: already in position"));
        return;
    }

    digitalWrite(_dirPin, _invertDirection ? !_direction : _direction);

    _active = true;
    _currentPhase = PHASE_CRUISE;

    uint32_t arr = clampARRFromFreq(_timing.timerBaseFreq, freqHz);
    uint32_t pulse = _timing.pulseTicks;

    configureTimer(arr, pulse);

    HAL_TIM_PWM_Start(&_htim, _channel);
    HAL_TIM_Base_Start_IT(&_htim);
}

// -----------------------------
// Movement API (no accel)
// -----------------------------
void StepperHAL::moveToPosition(mm distance, Speed rpm)
{
    int32_t targetSteps = static_cast<int32_t>(roundf(distance.value * stepsPerMM()));
    planEffectiveTarget(targetSteps);
    float revsPerSec = rpm.rpm / 60.0f;
    float stepsPerSec = revsPerSec * _stepsMotorRev * _microstep;
    moveToTarget(_effectiveTarget, stepsPerSec);
}

void StepperHAL::moveToPosition(mm distance, Feed feedRate)
{
    int32_t targetSteps = static_cast<int32_t>(roundf(distance.value * stepsPerMM()));
    planEffectiveTarget(targetSteps);
    float stepsPerSec = (feedRate.mmPerMin / 60.0f) * stepsPerMM();
    moveToTarget(_effectiveTarget, stepsPerSec);
}

void StepperHAL::moveToPosition(step steps, Speed rpm)
{
    planEffectiveTarget(steps.count);
    float revsPerSec = rpm.rpm / 60.0f;
    float stepsPerSec = revsPerSec * _stepsMotorRev * _microstep;
    moveToTarget(_effectiveTarget, stepsPerSec);
}

void StepperHAL::moveToPosition(step steps, Feed feedRate)
{
    planEffectiveTarget(steps.count);
    float stepsPerSec = (feedRate.mmPerMin / 60.0f) * stepsPerMM();
    moveToTarget(_effectiveTarget, stepsPerSec);
}

void StepperHAL::moveRelative(mm delta, Speed rpm)
{
    int32_t targetSteps = _logicalPosition + static_cast<int32_t>(roundf(delta.value * stepsPerMM()));
    planEffectiveTarget(targetSteps);
    float revsPerSec = rpm.rpm / 60.0f;
    float stepsPerSec = revsPerSec * _stepsMotorRev * _microstep;
    moveToTarget(_effectiveTarget, stepsPerSec);
}

void StepperHAL::moveRelative(mm delta, Feed feedRate)
{
    int32_t targetSteps = _logicalPosition + static_cast<int32_t>(roundf(delta.value * stepsPerMM()));
    planEffectiveTarget(targetSteps);
    float stepsPerSec = (feedRate.mmPerMin / 60.0f) * stepsPerMM();
    moveToTarget(_effectiveTarget, stepsPerSec);
}

void StepperHAL::moveRelative(step delta, Speed rpm)
{
    int32_t targetSteps = _logicalPosition + delta.count;
    planEffectiveTarget(targetSteps);
    float revsPerSec = rpm.rpm / 60.0f;
    float stepsPerSec = revsPerSec * _stepsMotorRev * _microstep;
    moveToTarget(_effectiveTarget, stepsPerSec);
}

void StepperHAL::moveRelative(step delta, Feed feedRate)
{
    int32_t targetSteps = _logicalPosition + delta.count;
    planEffectiveTarget(targetSteps);
    float stepsPerSec = (feedRate.mmPerMin / 60.0f) * stepsPerMM();
    moveToTarget(_effectiveTarget, stepsPerSec);
}

// -----------------------------
// Trapezoidal table generator
// -----------------------------
// Compiled only se USE_TRAPEZOIDAL è definito
#if USE_TRAPEZOIDAL

void StepperHAL::generateMotionTables(int32_t totalSteps, float maxFreqHz, float accelMMs2, float decelMMs2)
{
    if (totalSteps <= 0)
    {
        _accelSize = 0;
        _cruiseSize = 0;
        _decelSize = 0;
        return;
    }

    const float mmPerStep = _mmPerRev / (_stepsMotorRev * _microstep);

    // minimo frequenza applicata anche al profilo trapezoidale
    float minFreqHz = fmaxf(_minFreqHz, DEFAULT_MIN_FREQ_HZ);

    float vMax_mm_s = maxFreqHz * mmPerStep;
    float sAccel_mm = (vMax_mm_s * vMax_mm_s) / (2.0f * accelMMs2);
    float sDecel_mm = (vMax_mm_s * vMax_mm_s) / (2.0f * decelMMs2);

    int32_t sAccel_steps = (int32_t)roundf(sAccel_mm / mmPerStep);
    int32_t sDecel_steps = (int32_t)roundf(sDecel_mm / mmPerStep);

    if (sAccel_steps + sDecel_steps > totalSteps)
    {
        float ratio = (float)sAccel_steps / (float)(sAccel_steps + sDecel_steps);
        sAccel_steps = (int32_t)(totalSteps * ratio);
        sDecel_steps = totalSteps - sAccel_steps;
    }

    int32_t sConst_steps = totalSteps - sAccel_steps - sDecel_steps;

    _accelSize = sAccel_steps;
    _cruiseSize = sConst_steps;
    _decelSize = sDecel_steps;

    if (_accelTable)
    {
        delete[] _accelTable;
        _accelTable = nullptr;
    }
    if (_decelTable)
    {
        delete[] _decelTable;
        _decelTable = nullptr;
    }

    if (_accelSize > 0)
        _accelTable = new uint32_t[_accelSize];
    if (_decelSize > 0)
        _decelTable = new uint32_t[_decelSize];

    for (int32_t i = 0; i < _accelSize; ++i)
    {
        float s = (i + 1) * mmPerStep;
        float v = sqrtf(2.0f * accelMMs2 * s);
        float freq = v / mmPerStep;
        if (freq < 1.0f)
            freq = 1.0f;
        freq = fmaxf(freq, minFreqHz);
        _accelTable[i] = clampARRFromFreq(_timing.timerBaseFreq, freq);
    }

    // cruise ARR (use maxFreqHz but ensure consistency with minFreqHz if needed)
    _cruiseARR = clampARRFromFreq(_timing.timerBaseFreq, maxFreqHz);

    for (int32_t i = 0; i < _decelSize; ++i)
    {
        float s = (_decelSize - i) * mmPerStep;
        float v = sqrtf(2.0f * decelMMs2 * s);
        float freq = v / mmPerStep;
        if (freq < 1.0f)
            freq = 1.0f;
        freq = fmaxf(freq, minFreqHz);
        _decelTable[i] = clampARRFromFreq(_timing.timerBaseFreq, freq);
    }

    if (_accelSize > 0)
    {
        _accelTable[0] = clampARRFromFreq(_timing.timerBaseFreq, minFreqHz);
        _accelTable[_accelSize - 1] = _cruiseARR;
    }
    if (_decelSize > 0)
    {
        _decelTable[0] = _cruiseARR;
        _decelTable[_decelSize - 1] = clampARRFromFreq(_timing.timerBaseFreq, minFreqHz);
    }

    SerialDB.print("[StepperHAL] (");
    SerialDB.print(getLabel());
    SerialDB.print(") ");
    SerialDB.print(msg("Profilo DMA → ", "DMA profile → "));
    SerialDB.print(totalSteps);
    SerialDB.print(msg(" steps, accel = ", " steps, accel = "));
    SerialDB.print(accelMMs2, 2);
    SerialDB.print(msg(" mm/s², decel = ", " mm/s², decel = "));
    SerialDB.print(decelMMs2, 2);
    SerialDB.println(" mm/s²");

    SerialDB.print("  → ");
    SerialDB.print(msg("Accelerazione: ", "Acceleration: "));
    SerialDB.print(_accelSize);
    SerialDB.println(msg(" steps", " steps"));

    SerialDB.print("  → ");
    SerialDB.print(msg("Regime: ", "Cruise: "));
    SerialDB.print(_cruiseSize);
    SerialDB.println(msg(" steps", " steps"));

    SerialDB.print("  → ");
    SerialDB.print(msg("Decelerazione: ", "Deceleration: "));
    SerialDB.print(_decelSize);
    SerialDB.println(msg(" steps", " steps"));

    SerialDB.print("  → ");
    SerialDB.println(_cruiseSize > 0 ? msg("Profilo: trapezoidale", "Profile: trapezoidal")
                                     : msg("Profilo: triangolare", "Profile: triangular"));
}

#endif // USE_TRAPEZOIDAL

// -----------------------------
// S-curve (optional, compiled only if USE_S_CURVE)
// -----------------------------
#if USE_S_CURVE

void StepperHAL::useSCurve(bool enable)
{
    _useSCurve = enable;
    SerialDB.print("[StepperHAL] (");
    SerialDB.print(getLabel());
    SerialDB.print(") ");
    SerialDB.print("S‑curve mode: ");
    SerialDB.println(enable ? "ON" : "OFF");
}

void StepperHAL::setJerk(float jerkAcc)
{
    _jerkAccMMs3 = jerkAcc;
    _jerkDecMMs3 = jerkAcc;
    _jerkSymmetric = true;
}
void StepperHAL::setJerk(float jerkAcc, float jerkDec)
{
    _jerkAccMMs3 = jerkAcc;
    _jerkDecMMs3 = jerkDec;
    _jerkSymmetric = false;
}

void StepperHAL::setJerkRatio(float ratio)
{
    // clamp del parametro ratio (jerkRatio)
    if (ratio < JERK_RATIO_MIN)
    {
        SerialDB.print(msg("Attenzione: jerkRatio sotto il minimo, limitato a ",
                           "Warning: jerkRatio below minimum, clamped to "));
        SerialDB.println(JERK_RATIO_MIN, 3);
        ratio = JERK_RATIO_MIN;
    }
    else if (ratio > JERK_RATIO_MAX)
    {
        SerialDB.print(msg("Attenzione: jerkRatio sopra il massimo, limitato a ",
                           "Warning: jerkRatio above maximum, clamped to "));
        SerialDB.println(JERK_RATIO_MAX, 3);
        ratio = JERK_RATIO_MAX;
    }

    // salva il valore già corretto e disabilita il manual jerk
    _jerkRatio = ratio;
    _useManualJerk = false;

    // log di conferma
    SerialDB.print("[StepperHAL] (");
    SerialDB.print(getLabel());
    SerialDB.print(") JerkRatio set — ratio=");
    SerialDB.println(_jerkRatio, 3);
}

void StepperHAL::setManualJerk(float jerkAccUp, float jerkAccFlat, float jerkAccDown, float jerkDecUp,
                               float jerkDecFlat, float jerkDecDown)
{
    _jerkAccelUp = jerkAccUp;
    _jerkAccelFlat = jerkAccFlat;
    _jerkAccelDown = jerkAccDown;
    _jerkDecelUp = jerkDecUp;
    _jerkDecelFlat = jerkDecFlat;
    _jerkDecelDown = jerkDecDown;
    _useManualJerk = true;

    SerialDB.print("[StepperHAL] (");
    SerialDB.print(getLabel());
    SerialDB.print(") ");
    SerialDB.print("Manual jerk set — aUp=");
    SerialDB.print(_jerkAccelUp, 1);
    SerialDB.print(" aFlat=");
    SerialDB.print(_jerkAccelFlat, 1);
    SerialDB.print(" aDown=");
    SerialDB.print(_jerkAccelDown, 1);
    SerialDB.print(" dUp=");
    SerialDB.print(_jerkDecelUp, 1);
    SerialDB.print(" dFlat=");
    SerialDB.print(_jerkDecelFlat, 1);
    SerialDB.print(" dDown=");
    SerialDB.println(_jerkDecelDown, 1);
}

void StepperHAL::useDefaultJerk(bool enable)
{
    if (enable)
        _useManualJerk = false;
    SerialDB.print("[StepperHAL] (");
    SerialDB.print(getLabel());
    SerialDB.print(") ");
    SerialDB.print("Jerk mode: ");
    SerialDB.println(_useManualJerk ? "manual" : "default");
}

void StepperHAL::computeDefaultJerk(float accelMMs2, float decelMMs2, int jerkSteps, float minFreqHz, float maxFreqHz,
                                    float jerkRatio)
{

    if (jerkSteps < 1)
        jerkSteps = 1;

    float jerkFactor = 0.08f;
    float timeFactor = 4.0f;

    float fMean = (minFreqHz + maxFreqHz) * 0.5f;
    if (fMean <= 0.0f)
        fMean = fmaxf(minFreqHz, 1.0f);

    float dt = timeFactor / fMean;
    float tJerk = jerkSteps * dt;
    if (tJerk <= 0.0f)
        tJerk = dt;

    _jerkAccelUp = accelMMs2 * jerkFactor / tJerk;
    _jerkAccelDown = accelMMs2 * jerkFactor / tJerk;
    _jerkAccelFlat = 0.0f;

    _jerkDecelUp = decelMMs2 * jerkFactor / tJerk * jerkRatio;
    _jerkDecelDown = decelMMs2 * jerkFactor / tJerk * jerkRatio;
    _jerkDecelFlat = 0.0f;

    SerialDB.print("[StepperHAL] (");
    SerialDB.print(getLabel());
    SerialDB.print(") ");
    SerialDB.print("Jerk default — aUp=");
    SerialDB.print(_jerkAccelUp, 1);
    SerialDB.print(" aFlat=");
    SerialDB.print(_jerkAccelFlat, 1);
    SerialDB.print(" aDown=");
    SerialDB.print(_jerkAccelDown, 1);
    SerialDB.print(" dUp=");
    SerialDB.print(_jerkDecelUp, 1);
    SerialDB.print(" dFlat=");
    SerialDB.print(_jerkDecelFlat, 1);
    SerialDB.print(" dDown=");
    SerialDB.print(_jerkDecelDown, 1);
    SerialDB.print(" JerkRatio=");
    SerialDB.println(_jerkRatio);
}

void StepperHAL::generateMotionTablesSCurve(int32_t totalSteps, float maxFreqHz, float accelMMs2, float decelMMs2)
{
    // niente da fare se non ci sono passi
    if (totalSteps <= 0)
    {
        _accelSize = 0;
        _cruiseSize = 0;
        _decelSize = 0;
        return;
    }

    // frequenza minima e conversione mm→passo
    float minFreqHz = fmaxf(_minFreqHz, DEFAULT_MIN_FREQ_HZ);
    const float mmPerStep = _mmPerRev / (_stepsMotorRev * _microstep);

    // distanza percorsa in accel/decel [mm]
    float vMax_mm_s = maxFreqHz * mmPerStep;
    float sAccel_mm = (vMax_mm_s * vMax_mm_s) / (2.0f * accelMMs2);
    float sDecel_mm = (vMax_mm_s * vMax_mm_s) / (2.0f * decelMMs2);

    // passi in accel/decel
    int32_t accelSteps = (int32_t)roundf(sAccel_mm / mmPerStep);
    int32_t decelSteps = (int32_t)roundf(sDecel_mm / mmPerStep);

    // se accel+decel superano totalSteps, ridistribuisci
    if (accelSteps + decelSteps > totalSteps)
    {
        accelSteps = (int32_t)(totalSteps * 0.4f);
        decelSteps = (int32_t)(totalSteps * 0.4f);
        if (accelSteps + decelSteps > totalSteps)
        {
            accelSteps = totalSteps / 2;
            decelSteps = totalSteps - accelSteps;
        }
    }

    // assegna dimensioni sezioni
    _accelSize = (accelSteps < 0 ? 0 : accelSteps);
    _decelSize = (decelSteps < 0 ? 0 : decelSteps);
    _cruiseSize = totalSteps - _accelSize - _decelSize;
    if (_cruiseSize < 0)
        _cruiseSize = 0;

    // pausa IRQ per ricreare tabelle
    IRQn_Type irq = (TIM2 == _timer   ? TIM2_IRQn
                     : TIM3 == _timer ? TIM3_IRQn
                     : TIM4 == _timer ? TIM4_IRQn
                     : TIM5 == _timer ? TIM5_IRQn
                                      : (IRQn_Type)0);
    if (irq)
        NVIC_DisableIRQ(irq);

    // (ri)crea buffer
    if (_accelTable)
        delete[] _accelTable;
    if (_decelTable)
        delete[] _decelTable;
    _accelTable = (_accelSize > 0 ? new uint32_t[_accelSize] : nullptr);
    _decelTable = (_decelSize > 0 ? new uint32_t[_decelSize] : nullptr);

    if (irq)
        NVIC_EnableIRQ(irq);

    // valore ARR di crociera
    _cruiseARR = clampARRFromFreq(_timing.timerBaseFreq, maxFreqHz);

    // suddivisione aUp/aFlat/aDown e dUp/dFlat/dDown
    int baseJerkAccel = _accelSize / 3;
    if (baseJerkAccel < 1)
        baseJerkAccel = 1;
    _jerkAccelUpSteps = baseJerkAccel;
    _jerkAccelDownSteps = baseJerkAccel;
    {
        int flatAcc = _accelSize - _jerkAccelUpSteps - _jerkAccelDownSteps;
        _jerkAccelFlatSteps = (flatAcc > 0 ? flatAcc : 0);
    }

    int baseJerkDecel = _decelSize / 3;
    if (baseJerkDecel < 1)
        baseJerkDecel = 1;
    _jerkDecelUpSteps = baseJerkDecel;
    _jerkDecelDownSteps = baseJerkDecel;
    {
        int flatDec = _decelSize - _jerkDecelUpSteps - _jerkDecelDownSteps;
        _jerkDecelFlatSteps = (flatDec > 0 ? flatDec : 0);
    }

    // (ri)calcolo parametri jerk se non manuale
    if (!_useManualJerk)
    {
        computeDefaultJerk(accelMMs2, decelMMs2, _jerkAccelUpSteps, minFreqHz, maxFreqHz, _jerkRatio);
    }

    // tabella ARR S-curve accel
    for (int i = 0; i < _accelSize; ++i)
    {
        float p = (_accelSize > 1 ? (float)i / (_accelSize - 1) : 1.0f);
        float e = (p < 0.5f) ? 4.0f * p * p * p : 1.0f - powf(-2.0f * p + 2.0f, 3.0f) / 2.0f;
        float freq = minFreqHz + (maxFreqHz - minFreqHz) * e;
        _accelTable[i] = clampARRFromFreq(_timing.timerBaseFreq, freq);
    }

    // tabella ARR S-curve decel
    for (int i = 0; i < _decelSize; ++i)
    {
        float p = (_decelSize > 1 ? (float)i / (_decelSize - 1) : 1.0f);
        float e = (p < 0.5f) ? 4.0f * p * p * p : 1.0f - powf(-2.0f * p + 2.0f, 3.0f) / 2.0f;
        float freq = maxFreqHz - (maxFreqHz - minFreqHz) * e;
        _decelTable[i] = clampARRFromFreq(_timing.timerBaseFreq, freq);
    }

    // forzature estremi
    if (_accelSize > 0)
    {
        _accelTable[0] = clampARRFromFreq(_timing.timerBaseFreq, minFreqHz);
        _accelTable[_accelSize - 1] = _cruiseARR;
    }
    if (_decelSize > 0)
    {
        _decelTable[0] = _cruiseARR;
        _decelTable[_decelSize - 1] = clampARRFromFreq(_timing.timerBaseFreq, minFreqHz);
    }

    // log suddivisione jerk
    SerialDB.print("[StepperHAL] (");
    SerialDB.print(getLabel());
    SerialDB.print(")   ");
    SerialDB.print(msg("S-curve fasi accellerazione (Up/Flat/Down)= ", "S-curve accel phases steps (Up/Flat/Down)= "));
    SerialDB.print(_jerkAccelUpSteps);
    SerialDB.print('/');
    SerialDB.print(_jerkAccelFlatSteps);
    SerialDB.print('/');
    SerialDB.println(_jerkAccelDownSteps);

    SerialDB.print("[StepperHAL] (");
    SerialDB.print(getLabel());
    SerialDB.print(")   ");
    SerialDB.print(msg("S-curve fasi decelerazione (Up/Flat/Down)= ", "S-curve decel phases steps (Up/Flat/Down)= "));
    SerialDB.print(_jerkDecelUpSteps);
    SerialDB.print('/');
    SerialDB.print(_jerkDecelFlatSteps);
    SerialDB.print('/');
    SerialDB.println(_jerkDecelDownSteps);

    // log dimensioni profilo
    uint32_t total = _accelSize + _cruiseSize + _decelSize;

    SerialDB.print("[StepperHAL] (");
    SerialDB.print(getLabel());
    SerialDB.print(") ");
    SerialDB.print(msg("Profilo DMA -> ", "DMA profile -> "));
    SerialDB.print(total);
    SerialDB.print(" steps, accel = ");
    SerialDB.print(accelMMs2, 2);
    SerialDB.print(" mm/s², decel = ");
    SerialDB.print(decelMMs2, 2);
    SerialDB.println(" mm/s²");

    // log suddivisione jerk
    SerialDB.print("[StepperHAL] (");
    SerialDB.print(getLabel());
    SerialDB.print(")   ");
    SerialDB.print(msg("-> Accelerazione: ", "-> Acceleration: "));
    SerialDB.println(_accelSize);
    SerialDB.print("[StepperHAL] (");
    SerialDB.print(getLabel());
    SerialDB.print(")   ");
    SerialDB.print(msg("-> Regime:        ", "-> Cruise:        "));
    SerialDB.println(_cruiseSize);
    SerialDB.print("[StepperHAL] (");
    SerialDB.print(getLabel());
    SerialDB.print(")   ");
    SerialDB.print(msg("-> Decelerazione: ", "-> Deceleration: "));
    SerialDB.println(_decelSize);
    SerialDB.print("[StepperHAL] (");
    SerialDB.print(getLabel());
    SerialDB.println(msg(")   -> Profilo: S-curve", ")   -> Profile: S-curve"));
}

#endif // USE_S_CURVE

// -----------------------------
// Movement with accel (DMA-style tables)
// -----------------------------
void StepperHAL::moveToTargetWithAccelDMA(int32_t targetSteps, float maxFreqHz, float accelMMs2, float decelMMs2)
{
    int32_t stepsNeeded = targetSteps - _position;

    if (stepsNeeded == 0)
    {
        _logicalPosition = targetSteps;
        _position = targetSteps;
        SerialDB.print("[StepperHAL] (");
        SerialDB.print(getLabel());
        SerialDB.println(msg(") Nessun movimento: già in posizione", ") No movement: already in position"));
        return;
    }

    bool effectiveDirection = _direction ^ _invertDirection;
    digitalWrite(_dirPin, effectiveDirection ? 1 : 0);
    delayMicroseconds(2);

    _target = targetSteps;
    _active = true;
    _useDMA = true;
    _dmaIndex = 0;
    _holdPhase = false;
    _decelPhase = false;

    int32_t totalSteps = abs(_target - _position);
    if (totalSteps == 0)
    {
        _active = false;
        return;
    }

#if USE_S_CURVE
    if (_useSCurve)
        generateMotionTablesSCurve(totalSteps, maxFreqHz, accelMMs2, decelMMs2);
#if USE_TRAPEZOIDAL
    else
        generateMotionTables(totalSteps, maxFreqHz, accelMMs2, decelMMs2);
#endif
#elif defined(USE_TRAPEZOIDAL)
    // Build without S-curve: force trapezoid
    generateMotionTables(totalSteps, maxFreqHz, accelMMs2, decelMMs2);
#else
    // Neither profile selected at compile time -> fallback to trapezoid for safety
    generateMotionTables(totalSteps, maxFreqHz, accelMMs2, decelMMs2);
#endif

    _accelIndex = 0;
    _decelIndex = 0;
    _cruiseRemaining = _cruiseSize;
    _currentPhase = PHASE_ACCEL;

    uint32_t firstARR = (_accelSize > 0)
                            ? _accelTable[0]
                            : (_cruiseARR > 0 ? _cruiseARR : clampARRFromFreq(_timing.timerBaseFreq, maxFreqHz));

    configureTimer(firstARR, _timing.pulseTicks);

    HAL_TIM_PWM_Start(&_htim, _channel);
    HAL_TIM_Base_Start_IT(&_htim);

    float vMaxRPM = 0.0f;
    float stepsPerRevF = static_cast<float>(_stepsMotorRev) * static_cast<float>(_microstep);
    if (stepsPerRevF > 0.0f)
        vMaxRPM = maxFreqHz * 60.0f / stepsPerRevF;

    SerialDB.print("[StepperHAL] (");
    SerialDB.print(getLabel());
    SerialDB.print(") ");
    SerialDB.print(msg("Profilo DMA → ", "DMA profile → "));
    SerialDB.print(msg("Target richiesto=", "Requested target="));
    SerialDB.print(_pendingLogical);
    SerialDB.print(" ");
    SerialDB.print(msg("Vmax ", "Vmax "));
    SerialDB.print(vMaxRPM, 2);
    SerialDB.print(" RPM @ ");
    SerialDB.print(maxFreqHz, 2);
    SerialDB.println(" Hz");

    // dumpMotionTablesSerialOnly(0);
}

// -----------------------------
// Dump Motion Tables (advance internal debug)
// -----------------------------
void StepperHAL::dumpMotionTablesSerialOnly(int window /*=16*/)
{
    int32_t aN = _accelSize;
    int32_t cN = _cruiseSize;
    int32_t dN = _decelSize;
    uint32_t cARR = _cruiseARR;

    SerialDB.print("[DUMP] sizes acc/cruise/dec = ");
    SerialDB.print(aN);
    SerialDB.print("/");
    SerialDB.print(cN);
    SerialDB.print("/");
    SerialDB.println(dN);
    SerialDB.print("[DUMP] cruiseARR=");
    SerialDB.println(cARR);

    auto throttle = [&]() { delay(0); };

    auto printArrayBlocks = [&](const uint32_t *arr, int32_t n, const char *label)
    {
        if (n <= 0)
        {
            SerialDB.print("[DUMP] ");
            SerialDB.print(label);
            SerialDB.println(": <empty>");
            return;
        }

        SerialDB.print("[DUMP] ");
        SerialDB.print(label);
        SerialDB.print(" (CSV blocks of 30, total=");
        SerialDB.print(n);
        SerialDB.println(")");

        const int BLOCK = 30;
        int32_t i = 0;
        while (i < n)
        {
            int32_t end = (i + BLOCK < n) ? (i + BLOCK) : n;
            for (int32_t j = i; j < end; ++j)
            {
                SerialDB.print(arr[j]);
                if (j + 1 < end)
                    SerialDB.print(',');
            }
            SerialDB.println();
            i = end;
            if ((i & 0x3FF) == 0)
                throttle();
        }
        SerialDB.println();
    };

    if (window > 0)
    {
        if (aN == 0)
        {
            SerialDB.println("[DUMP] accel: <empty>");
        }
        else
        {
            int start = (aN > window) ? (aN - window) : 0;
            SerialDB.print("[DUMP] accel tail from ");
            SerialDB.print(start);
            SerialDB.print(" to ");
            SerialDB.println(aN - 1);
            for (int i = start; i < aN; ++i)
            {
                SerialDB.print("A[");
                SerialDB.print(i);
                SerialDB.print("] = ");
                SerialDB.println(_accelTable[i]);
            }
            SerialDB.println();
        }

        if (cN == 0)
        {
            SerialDB.println("[DUMP] cruise: <none>");
        }
        else
        {
            SerialDB.print("[DUMP] cruise ARR = ");
            SerialDB.print(cARR);
            SerialDB.print(" repeated ");
            SerialDB.print(cN);
            SerialDB.println(" times");
            SerialDB.println();
        }

        if (dN == 0)
        {
            SerialDB.println("[DUMP] decel: <empty>");
        }
        else
        {
            int end = (dN > window) ? window : dN;
            SerialDB.print("[DUMP] decel head 0..");
            SerialDB.println(end - 1);
            for (int i = 0; i < end; ++i)
            {
                SerialDB.print("D[");
                SerialDB.print(i);
                SerialDB.print("] = ");
                SerialDB.println(_decelTable[i]);
            }
            SerialDB.println();
        }

        SerialDB.print("[DUMP.CSV] accelSize,cruiseARR,cruiseSize,decelSize = ");
        SerialDB.print(aN);
        SerialDB.print(',');
        SerialDB.print(cARR);
        SerialDB.print(',');
        SerialDB.print(cN);
        SerialDB.print(',');
        SerialDB.println(dN);
        if (_accelTable)
            printArrayBlocks(_accelTable, aN, "accel (blocks)");
        if (_decelTable)
            printArrayBlocks(_decelTable, dN, "decel (blocks)");

        SerialDB.println("[DUMP] END");
        return;
    }

    SerialDB.println("[DUMP.FULL] BEGIN");
    SerialDB.print("[DUMP.FULL] sizes acc/cruise/dec = ");
    SerialDB.print(aN);
    SerialDB.print("/");
    SerialDB.print(cN);
    SerialDB.print("/");
    SerialDB.println(dN);
    SerialDB.print("[DUMP.FULL] cruiseARR=");
    SerialDB.println(cARR);

    if (_accelTable)
        printArrayBlocks(_accelTable, aN, "accel (full blocks)");

    SerialDB.print("[DUMP.FULL] cruise ARR = ");
    SerialDB.print(cARR);
    SerialDB.print(" repeated ");
    SerialDB.print(cN);
    SerialDB.println(" times");
    SerialDB.println();

    if (_decelTable)
        printArrayBlocks(_decelTable, dN, "decel (full blocks)");

    SerialDB.println("[DUMP.FULL] END");
}

// -----------------------------
// Wrappers for various overloads (accel/no-accel) - use roundf as needed
// -----------------------------
void StepperHAL::moveRelativeWithAccel(mm delta, Speed rpm, mms2 accel)
{
    mms2 decel(accel.value);
    moveRelativeWithAccel(delta, rpm, accel, decel);
}
void StepperHAL::moveRelativeWithAccel(mm delta, Feed feedRate, mms2 accel)
{
    mms2 decel(accel.value);
    moveRelativeWithAccel(delta, feedRate, accel, decel);
}
void StepperHAL::moveRelativeWithAccel(step delta, Speed rpm, mms2 accel)
{
    mms2 decel(accel.value);
    moveRelativeWithAccel(delta, rpm, accel, decel);
}
void StepperHAL::moveRelativeWithAccel(step delta, Feed feedRate, mms2 accel)
{
    mms2 decel(accel.value);
    moveRelativeWithAccel(delta, feedRate, accel, decel);
}

void StepperHAL::moveRelativeWithAccel(mm delta, Speed rpm, mms2 accel, mms2 decel)
{
    int32_t deltaSteps = static_cast<int32_t>(roundf(delta.value * stepsPerMM()));
    int32_t targetSteps = _logicalPosition + deltaSteps;
    planEffectiveTarget(targetSteps);

    float revsPerSec = rpm.rpm / 60.0f;
    float stepsPerSec = revsPerSec * _stepsMotorRev * _microstep;

    moveToTargetWithAccelDMA(_effectiveTarget, stepsPerSec, accel.value, decel.value);
}

void StepperHAL::moveRelativeWithAccel(mm delta, Feed feedRate, mms2 accel, mms2 decel)
{
    int32_t deltaSteps = static_cast<int32_t>(roundf(delta.value * stepsPerMM()));
    int32_t targetSteps = _logicalPosition + deltaSteps;
    planEffectiveTarget(targetSteps);

    float stepsPerSec = (feedRate.mmPerMin / 60.0f) * stepsPerMM();

    moveToTargetWithAccelDMA(_effectiveTarget, stepsPerSec, accel.value, decel.value);
}

void StepperHAL::moveRelativeWithAccel(step delta, Speed rpm, mms2 accel, mms2 decel)
{
    int32_t targetSteps = _logicalPosition + delta.count;
    planEffectiveTarget(targetSteps);

    float revsPerSec = rpm.rpm / 60.0f;
    float stepsPerSec = revsPerSec * _stepsMotorRev * _microstep;

    moveToTargetWithAccelDMA(_effectiveTarget, stepsPerSec, accel.value, decel.value);
}

void StepperHAL::moveRelativeWithAccel(step delta, Feed feedRate, mms2 accel, mms2 decel)
{
    int32_t targetSteps = _logicalPosition + delta.count;
    planEffectiveTarget(targetSteps);

    float stepsPerSec = (feedRate.mmPerMin / 60.0f) * stepsPerMM();

    moveToTargetWithAccelDMA(_effectiveTarget, stepsPerSec, accel.value, decel.value);
}

void StepperHAL::moveToPositionWithAccel(mm distance, Speed rpm, mms2 accel)
{
    mms2 decel(accel.value);
    moveToPositionWithAccel(distance, rpm, accel, decel);
}
void StepperHAL::moveToPositionWithAccel(mm distance, Feed feedRate, mms2 accel)
{
    mms2 decel(accel.value);
    moveToPositionWithAccel(distance, feedRate, accel, decel);
}
void StepperHAL::moveToPositionWithAccel(step steps, Speed rpm, mms2 accel)
{
    mms2 decel(accel.value);
    moveToPositionWithAccel(steps, rpm, accel, decel);
}
void StepperHAL::moveToPositionWithAccel(step steps, Feed feedRate, mms2 accel)
{
    mms2 decel(accel.value);
    moveToPositionWithAccel(steps, feedRate, accel, decel);
}

void StepperHAL::moveToPositionWithAccel(mm distance, Speed rpm, mms2 accel, mms2 decel)
{
    int32_t targetSteps = static_cast<int32_t>(roundf(distance.value * stepsPerMM()));
    planEffectiveTarget(targetSteps);
    float revsPerSec = rpm.rpm / 60.0f;
    float stepsPerSec = revsPerSec * _stepsMotorRev * _microstep;
    moveToTargetWithAccelDMA(_effectiveTarget, stepsPerSec, accel.value, decel.value);
}

void StepperHAL::moveToPositionWithAccel(mm distance, Feed feedRate, mms2 accel, mms2 decel)
{
    int32_t targetSteps = static_cast<int32_t>(roundf(distance.value * stepsPerMM()));
    planEffectiveTarget(targetSteps);
    float stepsPerSec = (feedRate.mmPerMin / 60.0f) * stepsPerMM();
    moveToTargetWithAccelDMA(_effectiveTarget, stepsPerSec, accel.value, decel.value);
}

void StepperHAL::moveToPositionWithAccel(step steps, Speed rpm, mms2 accel, mms2 decel)
{
    planEffectiveTarget(steps.count);
    float revsPerSec = rpm.rpm / 60.0f;
    float stepsPerSec = revsPerSec * _stepsMotorRev * _microstep;
    moveToTargetWithAccelDMA(_effectiveTarget, stepsPerSec, accel.value, decel.value);
}

void StepperHAL::moveToPositionWithAccel(step steps, Feed feedRate, mms2 accel, mms2 decel)
{
    planEffectiveTarget(steps.count);
    float stepsPerSec = (feedRate.mmPerMin / 60.0f) * stepsPerMM();
    moveToTargetWithAccelDMA(_effectiveTarget, stepsPerSec, accel.value, decel.value);
}

// -----------------------------
// State & control (resto del file unchanged)
// -----------------------------
const char *StepperHAL::getLabel() const
{
    if (_timer == TIM2)
        return "motor1";
    if (_timer == TIM3)
        return "motor2";
    if (_timer == TIM4)
        return "motor3";
    if (_timer == TIM5)
        return "motor4";
    return "motor?";
}

void StepperHAL::setEnable(bool state)
{
    if (_enPin >= 0)
    {
        digitalWrite(_enPin, state ? 0 : 1); // LOW = enable attivo
        SerialDB.print("[StepperHAL] (");
        SerialDB.print(getLabel());
        SerialDB.print(") Driver ");
        SerialDB.println(state ? msg("abilitato", "enabled") : msg("disabilitato", "disabled"));
    }
    // I generate a step and reset the position otherwise an extra real step will be added to the first movement
    moveToPosition(step(1), Speed(100.0f));
    delay(100);
    _lastDir = 0;
    _position = 0;
    _logicalPosition = 0;
    SerialDB.print("[StepperHAL] (");
    SerialDB.print(getLabel());
    SerialDB.print(") ");
    SerialDB.println(msg("Passo di compensazione completato", "Compensation step completed"));
}

void StepperHAL::invertDIR(bool state)
{
    _invertDirection = state;
    SerialDB.print("[StepperHAL] (");
    SerialDB.print(getLabel());
    SerialDB.print(") ");
    SerialDB.print(msg("Direzione invertita via software: ", "Direction inverted via software: "));
    SerialDB.println(state ? "ON" : "OFF");
}

void StepperHAL::setTiming(uint32_t stepPulseUs, uint32_t timerBaseFreq)
{
    _timing.stepPulseUs = stepPulseUs;
    _timing.timerBaseFreq = timerBaseFreq;
    _timing.compute(_timerClockHz);

    SerialDB.print("[StepperHAL] (");
    SerialDB.print(getLabel());
    SerialDB.print(") ");
    SerialDB.print("[");
    SerialDB.print(timerName(_timer));
    SerialDB.print("] ");
    SerialDB.print(msg("Timing configurato → impulso = ", "Timing configured → pulse = "));
    SerialDB.print(stepPulseUs);
    SerialDB.print(" µs, ");
    SerialDB.print(msg("base timer = ", "base timer = "));
    SerialDB.print(timerBaseFreq);
    SerialDB.print(" Hz, ");
    SerialDB.print(msg("prescaler = ", "prescaler = "));
    SerialDB.print(_timing.prescaler);
    SerialDB.print(", ");
    SerialDB.print(msg("pulseTicks = ", "pulseTicks = "));
    SerialDB.print(_timing.pulseTicks);
    SerialDB.print(" | ");
    SerialDB.print(msg("System Clock = ", "System Clock = "));
    SerialDB.print(SystemCoreClock / 1000000);
    SerialDB.println(" MHz");
}

void StepperHAL::setMinFreqHz(float freqHz)
{
    if (freqHz < 1.0f)
        freqHz = 1.0f;
    _minFreqHz = freqHz;

    // RPM calculation for log
    float rpm = 0.0f;
    if (_stepsMotorRev > 0 && _microstep > 0)
    {
        rpm = _minFreqHz * 60.0f / (static_cast<float>(_stepsMotorRev) * static_cast<float>(_microstep));
    }

    SerialDB.print("[StepperHAL] (");
    SerialDB.print(getLabel());
    SerialDB.print(") ");
    SerialDB.print("Min freq set — fMin=");
    SerialDB.print(_minFreqHz, 3);
    SerialDB.print(msg(" ; RPMmin=", " ; RPMmin="));
    SerialDB.print(rpm, 0);
    SerialDB.print(" ; ARR=");
    SerialDB.println(clampARRFromFreq(_timing.timerBaseFreq, _minFreqHz));
}

void StepperHAL::setMinRPM(float rpm)
{
    float f = (rpm * _stepsMotorRev * _microstep) / 60.0f;
    setMinFreqHz(f);
}

float StepperHAL::getMinFreqHz() const
{
    return _minFreqHz;
}

void StepperHAL::stop()
{
    _active = false;
    HAL_TIM_PWM_Stop(&_htim, _channel);
    HAL_TIM_Base_Stop_IT(&_htim);
}

void StepperHAL::resetPosition()
{
    _lastDir = 0;
    _position = 0;
    _logicalPosition = 0;
    SerialDB.print("[StepperHAL] (");
    SerialDB.print(getLabel());
    SerialDB.println(msg(") Posizione logica e reale azzerata", ") Logic and real position reset"));
}

void StepperHAL::setPositionSteps(int32_t steps)
{
    int32_t offset = _position - _logicalPosition; // può essere positivo o negativo
    int32_t oldLogical = _logicalPosition;
    int32_t oldReal = _position;

    _logicalPosition = steps;
    _position = steps + offset;

    SerialDB.print("[StepperHAL] (");
    SerialDB.print(getLabel());
    SerialDB.print(") ");
    SerialDB.print(msg("Posizione logica: ", "Logical pos: "));
    SerialDB.print(oldLogical);
    SerialDB.print(" -> ");
    SerialDB.print(_logicalPosition);
    SerialDB.print(msg(" ; reale: ", " ; real: "));
    SerialDB.print(oldReal);
    SerialDB.print(" -> ");
    SerialDB.println(_position);
}

void StepperHAL::setPositionMM(float mm)
{
    int32_t steps = static_cast<int32_t>(roundf(mm * stepsPerMM()));
    int32_t offset = _position - _logicalPosition; // preserve offset for logging
    int32_t oldLogical = _logicalPosition;
    int32_t oldReal = _position;

    _logicalPosition = steps;
    _position = steps + offset;

    SerialDB.print("[StepperHAL] (");
    SerialDB.print(getLabel());
    SerialDB.print(") ");
    SerialDB.print(msg("Posizione logica (mm->steps): ", "Logical pos (mm->steps): "));
    SerialDB.print(mm, 6);
    SerialDB.print(" mm -> ");
    SerialDB.print(steps);
    SerialDB.print(msg(" passi ; precedente logica: ", " steps ; prev logical: "));
    SerialDB.print(oldLogical);
    SerialDB.print(" -> ");
    SerialDB.print(_logicalPosition);
    SerialDB.print(msg(" ; reale: ", " ; real: "));
    SerialDB.print(oldReal);
    SerialDB.print(" -> ");
    SerialDB.println(_position);
}

void StepperHAL::backlashCompensation(mm value)
{
    _backlashMM = value;
    _useBacklashMM = (value.value != 0.0f);
    _useBacklashStep = false;
}

void StepperHAL::backlashCompensation(step value)
{
    _backlashStep = value;
    _useBacklashStep = (value.count != 0);
    _useBacklashMM = false;
}

bool StepperHAL::targetReached() const
{
    return _position == _effectiveTarget;
}

bool StepperHAL::isActive() const
{
    return _active;
}

int32_t StepperHAL::getPositionSteps() const
{
    return _logicalPosition;
}

float StepperHAL::getPositionMM() const
{
    return static_cast<float>(getPositionSteps()) /
           (static_cast<float>(_stepsMotorRev) * static_cast<float>(_microstep)) * _mmPerRev;
}

uint32_t StepperHAL::getStepsPerRev() const
{
    return (uint32_t)(_stepsMotorRev * _microstep);
}

int32_t StepperHAL::getDiagAccelIndex() const
{
    return _diag_accelIndex;
}
int32_t StepperHAL::getDiagCruiseRemaining() const
{
    return _diag_cruiseRemaining;
}
int32_t StepperHAL::getDiagDecelIndex() const
{
    return _diag_decelIndex;
}
int StepperHAL::getDiagPhase() const
{
    return _diag_phase;
}

// -----------------------------
// ISR core
// -----------------------------
void StepperHAL::handleInterrupt()
{
    if (!_active)
        return;

    if (_useDMA)
    {
        switch (_currentPhase)
        {
        case PHASE_ACCEL:
            if (_accelIndex < _accelSize)
            {
                int32_t idx = _accelIndex;
                _accelIndex = idx + 1;
                _timer->ARR = _accelTable[idx];
            }
            else
            {
                _currentPhase = PHASE_CRUISE;
                _cruiseRemaining = _cruiseSize;
                _timer->ARR = _cruiseARR;
            }
            break;

        case PHASE_CRUISE:
        {
            int32_t remaining = _cruiseRemaining;
            if (remaining > 0)
            {
                _cruiseRemaining = remaining - 1;
                _timer->ARR = _cruiseARR;
            }
            else
            {
                _currentPhase = PHASE_DECEL;
                _decelIndex = 0;
            }
            break;
        }
        case PHASE_DECEL:
            if ((_decelIndex >= _decelSize) || (_position == _effectiveTarget))
            {
                _currentPhase = PHASE_DONE;
            }
            else
            {
                int32_t idx = _decelIndex;      // store current value
                _decelIndex = idx + 1;          // increment volatile safely
                _timer->ARR = _decelTable[idx]; // use stored value
            }
            break;
        case PHASE_IDLE:
            break;
        case PHASE_DONE:
            break;
        default:
            break;
        }

        if (_currentPhase != PHASE_DONE)
        {
            _position += (_direction ? -1 : 1);
        }

        if (_currentPhase == PHASE_DONE)
        {
            goto movement_completed;
        }

        _diag_accelIndex = _accelIndex;
        _diag_cruiseRemaining = _cruiseRemaining;
        _diag_decelIndex = _decelIndex;
        _diag_phase = (int)_currentPhase;
    }
    else
    {
        _position += (_direction ? -1 : 1);
        bool reached = (_position == _effectiveTarget) || _pendingStop;
        if (reached)
        {
            goto movement_completed;
        }
    }

    return;

movement_completed:
    __HAL_TIM_DISABLE_IT(&_htim, TIM_IT_UPDATE);
    HAL_TIM_PWM_Stop(&_htim, _channel);
    __HAL_TIM_DISABLE(&_htim);
    __HAL_TIM_SET_COMPARE(&_htim, _channel, 0);
    __HAL_TIM_SET_COUNTER(&_htim, 0);
    __HAL_TIM_CLEAR_FLAG(&_htim, TIM_FLAG_UPDATE);

    _logicalPosition = _pendingLogical;
    _active = false;
    _pendingStop = false;

    const char *dirLabel = (_moveDir > 0) ? "CW" : (_moveDir < 0 ? "CCW" : "STOP");

    SerialDB.print("[StepperHAL] (");
    SerialDB.print(getLabel());
    SerialDB.print(") ");
    SerialDB.print(
        msg("Movimento completato: fermo in posizione logica=", "Movement complete: stopped in position logical="));
    SerialDB.print(_logicalPosition);
    SerialDB.print(msg(": reale=", ": real="));
    SerialDB.print(_position);
    SerialDB.print(msg(" / target=", " / target="));
    SerialDB.print(_pendingLogical);
    SerialDB.print(msg(" / Dir=", " / Dir="));
    SerialDB.println(dirLabel);
}

TIM_HandleTypeDef &StepperHAL::getHandle()
{
    return _htim;
}
