// default: S-curve disabilitata a meno che non venga definita esternamente
#ifndef USE_S_CURVE
#define USE_S_CURVE 0
#endif

// default: trapezoidale abilitato a meno che non venga definito esternamente
#ifndef USE_TRAPEZOIDAL
#define USE_TRAPEZOIDAL 1
#endif

#pragma once

#include <cmath> // for roundf
#include <cstdint>

#include "hal_conf_extra.h"
#include "stm32f4xx_hal.h"

#include "StepperHAL_Utils.h"

struct mm
{
    float value;
    explicit mm(float v) : value(v)
    {
    }
};
struct step
{
    int32_t count;
    explicit step(int32_t s) : count(s)
    {
    }
};
struct Speed
{
    float rpm;
    explicit Speed(float r) : rpm(r)
    {
    }
};
struct Feed
{
    float mmPerMin;
    explicit Feed(float f) : mmPerMin(f)
    {
    }
};
struct mms2
{
    float value;
    explicit mms2(float a) : value(a)
    {
    }
};

struct StepperTiming
{
    uint32_t stepPulseUs = 12;
    uint32_t timerBaseFreq = 1000000;
    uint32_t pulseTicks = 0;
    uint32_t prescaler = 0;

    void compute(uint32_t sysClockHz)
    {
        if (timerBaseFreq == 0)
            timerBaseFreq = 1000000;
        uint32_t pre = sysClockHz / timerBaseFreq;
        if (pre == 0)
            pre = 1;
        prescaler = pre - 1;
        pulseTicks = (stepPulseUs * timerBaseFreq) / 1000000;
        if (pulseTicks == 0)
            pulseTicks = 1;
    }
};

enum MotionPhase
{
    PHASE_IDLE,
    PHASE_ACCEL,
    PHASE_CRUISE,
    PHASE_DECEL,
    PHASE_DONE
};

class StepperHAL
{
public:
    StepperHAL(TIM_TypeDef *timer, uint32_t channel, uint8_t dirPin, int8_t enPin, uint32_t stepsMotorRev,
               float microstep, float mmPerRev);

    StepperHAL(TIM_TypeDef *timer, uint32_t channel, uint8_t dirPin, int8_t enPin, uint32_t effectiveStepsMotorRev,
               float mmPerRev);

    void begin();
    void setEnable(bool state);
    void invertDIR(bool state);
    void setTiming(uint32_t stepPulseUs, uint32_t timerBaseFreq);
    void setPositionSteps(int32_t steps);
    void setPositionMM(float mm);
    void resetPosition();
    void stop();
    void handleInterrupt();

    void backlashCompensation(mm value);
    void backlashCompensation(step value);
    void planEffectiveTarget(long requestedTarget);

    void moveToPosition(mm distance, Speed rpm);
    void moveToPosition(mm distance, Feed feedRate);
    void moveToPosition(step steps, Speed rpm);
    void moveToPosition(step steps, Feed feedRate);

    void moveToPositionWithAccel(mm distance, Speed rpm, mms2 accel);
    void moveToPositionWithAccel(mm distance, Feed feedRate, mms2 accel);
    void moveToPositionWithAccel(step steps, Speed rpm, mms2 accel);
    void moveToPositionWithAccel(step steps, Feed feedRate, mms2 accel);

    void moveToPositionWithAccel(mm distance, Speed rpm, mms2 accel, mms2 decel);
    void moveToPositionWithAccel(mm distance, Feed feedRate, mms2 accel, mms2 decel);
    void moveToPositionWithAccel(step steps, Speed rpm, mms2 accel, mms2 decel);
    void moveToPositionWithAccel(step steps, Feed feedRate, mms2 accel, mms2 decel);

    void moveRelative(mm delta, Speed rpm);
    void moveRelative(mm delta, Feed mmPerMin);
    void moveRelative(step delta, Speed rpm);
    void moveRelative(step delta, Feed mmPerMin);

    void moveRelativeWithAccel(mm delta, Speed rpm, mms2 accel);
    void moveRelativeWithAccel(mm delta, Feed feedRate, mms2 accel);
    void moveRelativeWithAccel(step delta, Speed rpm, mms2 accel);
    void moveRelativeWithAccel(step delta, Feed feedRate, mms2 accel);

    void moveRelativeWithAccel(mm delta, Speed rpm, mms2 accel, mms2 decel);
    void moveRelativeWithAccel(mm delta, Feed feedRate, mms2 accel, mms2 decel);
    void moveRelativeWithAccel(step delta, Speed rpm, mms2 accel, mms2 decel);
    void moveRelativeWithAccel(step delta, Feed feedRate, mms2 accel, mms2 decel);

#if USE_S_CURVE
    void useSCurve(bool enable);
    void setJerk(float jerkAcc);
    void setJerk(float jerkAcc, float jerkDec);
    void setJerkRatio(float ratio);
    void setManualJerk(float jerkAccUp, float jerkAccFlat, float jerkAccDown, float jerkDecUp, float jerkDecFlat,
                       float jerkDecDown);
    void useDefaultJerk(bool enable);
#else
    // Stubs inline diagnostici: compilano ancora, ma loggano un avviso per evitare confusione.
    // Parametri non usati sono resi anonimi o marcati per evitare warning.
    inline void useSCurve(bool enable)
    {
        (void)enable;
        Serial.print("[StepperHAL] (");
        Serial.print(getLabel());
        Serial.print(") ");
        Serial.print("useSCurve called but S-curve disabled. arg=");
        Serial.println(enable ? "true" : "false");
    }
    inline void setJerk(float /*jerkAcc*/)
    {
        Serial.print("[StepperHAL] (");
        Serial.print(getLabel());
        Serial.print(") ");
        Serial.println("setJerk called but S-curve disabled");
    }
    inline void setJerk(float /*jerkAcc*/, float /*jerkDec*/)
    {
        Serial.print("[StepperHAL] (");
        Serial.print(getLabel());
        Serial.print(") ");
        Serial.println("setJerk(acc,dec) called but S-curve disabled");
    }
    inline void setJerkRatio(float /*ratio*/)
    {
        Serial.print("[StepperHAL] (");
        Serial.print(getLabel());
        Serial.print(") ");
        Serial.println("setJerkRatio called but S-curve disabled");
    }
    inline void setManualJerk(float, float, float, float, float, float)
    {
        Serial.print("[StepperHAL] (");
        Serial.print(getLabel());
        Serial.print(") ");
        Serial.println("setManualJerk called but S-curve disabled");
    }
    inline void useDefaultJerk(bool /*enable*/)
    {
        Serial.print("[StepperHAL] (");
        Serial.print(getLabel());
        Serial.print(") ");
        Serial.println("useDefaultJerk called but S-curve disabled");
    }
#endif

    // Min-frequency API always visible (implementations in .cpp)
    void setMinFreqHz(float freqHz);
    void setMinRPM(float rpm);
    float getMinFreqHz() const;

    bool targetReached() const;
    bool isActive() const;
    bool hasBacklash() const;
    int32_t getPositionSteps() const;
    float getPositionMM() const;
    uint32_t getStepsPerRev() const;

    TIM_HandleTypeDef &getHandle();
    const char *getLabel() const;

    int32_t getDiagAccelIndex() const;
    int32_t getDiagCruiseRemaining() const;
    int32_t getDiagDecelIndex() const;
    int getDiagPhase() const;
    void dumpMotionTablesSerialOnly(int window = 16);

protected:
    void moveToTarget(int32_t targetSteps, float freqHz);
    void moveToTargetWithAccelDMA(int32_t targetSteps, float maxFreqHz, float accelMMs2, float decelMMs2);
    void generateMotionTables(int32_t totalSteps, float maxFreqHz, float accelMMs2, float decelMMs2);
    void generateMotionTablesSCurve(int32_t totalSteps, float maxFreqHz, float accelMMs2, float decelMMs2);
    void computeDefaultJerk(float accelMMs2, float decelMMs2, int jerkSteps, float minFreqHz, float maxFreqHz,
                            float jerkRatio);

    void configureStepPin();
    void configureDirPin();
    void configureEnPin();
    void digitalWrite(uint8_t pin, uint8_t val);
    void configureTimer(uint32_t arr, uint32_t pulse);

    void delay(unsigned long ms);
    void delayMicroseconds(unsigned int usec);

    static inline int8_t sgn(long x)
    {
        return (x > 0) - (x < 0);
    }
    inline float stepsPerMM() const
    {
        const float stepsPerRevEff = static_cast<float>(_stepsMotorRev) * _microstep;
        return stepsPerRevEff / _mmPerRev;
    }
    inline int32_t resolveBacklashSteps() const
    {
        if (_useBacklashStep && _backlashStep.count > 0)
        {
            return _backlashStep.count;
        }
        else if (_useBacklashMM && _backlashMM.value > 0.0f)
        {
            return static_cast<int32_t>(roundf(_backlashMM.value * stepsPerMM()));
        }
        return 0;
    }

    TIM_TypeDef *_timer;
    TIM_HandleTypeDef _htim;
    uint32_t _timerClockHz = 0;
    uint8_t _dirPin;
    int8_t _enPin;
    PinName _stepPin;
    uint32_t _channel;

    uint32_t _stepsMotorRev;
    float _microstep;
    float _mmPerRev;
    bool _invertDirection = false;

    volatile bool _active = false;
    volatile bool _useDMA = false;
    volatile bool _pendingStop = false;
    volatile int32_t _position = 0;
    volatile int32_t _logicalPosition = 0;
    volatile int32_t _target = 0;
    volatile int32_t _effectiveTarget = 0;
    volatile bool _direction = true;
    int8_t _moveDir = 0;
    int8_t _lastDir = 0;
    volatile int32_t _pendingLogical = 0;

    mm _backlashMM = mm(0.0f);
    step _backlashStep = step(0);
    bool _useBacklashMM = false;
    bool _useBacklashStep = false;
    bool _directionInitialized = false;

    StepperTiming _timing;
    volatile MotionPhase _currentPhase = PHASE_IDLE;
    uint32_t *_accelTable = nullptr;
    uint32_t *_decelTable = nullptr;
    volatile int32_t _accelSize = 0;
    volatile int32_t _decelSize = 0;
    volatile int32_t _accelIndex = 0;
    volatile int32_t _decelIndex = 0;
    volatile uint32_t _cruiseARR = 0;
    volatile int32_t _cruiseSize = 0;
    volatile int32_t _cruiseRemaining = 0;

    DMA_HandleTypeDef _hdma;
    int32_t _dmaIndex = 0;
    uint32_t _targetARR = 0;
    bool _holdPhase = false;
    bool _decelPhase = false;

    volatile int32_t _diag_accelIndex = 0;
    volatile int32_t _diag_cruiseRemaining = 0;
    volatile int32_t _diag_decelIndex = 0;
    volatile int _diag_phase = 0;

    bool _useSCurve = false;
    bool _jerkSymmetric = true;
    float _jerkAccMMs3 = 2500.0f;
    float _jerkDecMMs3 = 2000.0f;

    bool _useManualJerk = false;
    float _jerkAccelUp = 0.0f;
    float _jerkAccelFlat = 0.0f;
    float _jerkAccelDown = 0.0f;
    float _jerkDecelUp = 0.0f;
    float _jerkDecelFlat = 0.0f;
    float _jerkDecelDown = 0.0f;

    float _jerkRatio = 1.0f;

    int _jerkAccelUpSteps = 0;
    int _jerkAccelFlatSteps = 0;
    int _jerkAccelDownSteps = 0;
    int _jerkDecelUpSteps = 0;
    int _jerkDecelFlatSteps = 0;
    int _jerkDecelDownSteps = 0;

    uint32_t clampARRFromFreq(uint32_t timerBaseFreq, float freq);
    float _minFreqHz = 0.0f;
    const char *_label;
};

inline bool StepperHAL::hasBacklash() const
{
    return (_useBacklashStep && _backlashStep.count > 0) || (_useBacklashMM && _backlashMM.value > 0.0f);
}
