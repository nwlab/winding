/*
  Master example: S-curve motion with jerk settings
  Master example: S-curve motion with different jerk settings

  1. DEFAULT JERK
  2. JERK RATIO
  3. MANUAL JERK

  4. A full test of all motion methods is performed: – absolute in mm and in steps – relative in mm and in steps in both cases using the Speed ​​and Feed modes.
  4. A full test of all motion methods is run: – absolute in mm and in steps – relative in mm and in steps in both cases using the Speed ​​and Feed modes.
*/
#include <StepperHAL_Config.h>
#include <StepperHAL_STM32F4x1.h>
#include <StepperHAL_Instances.h>
#include "StepperHAL_Lang.h"

void setup() {
  SystemClock_Config();

  Serial.begin(115200);
  while (!Serial) {}

  // stampa clock di sistema / print system clock
  Serial.print(msg("Clock di sistema: ", "System Clock: "));
  Serial.print(SystemCoreClock / 1000000);
  Serial.println(" MHz");
  delay(200);

  // ---------- Inizializzazione motor2 / motor2 setup ----------
  motor2.begin();
  motor2.setTiming(10, 1000000);  // impulso 10 µs, base timer 1 MHz / 10 µs pulse, 1 MHz timer
  motor2.invertDIR(true);         // direzione invertita / direction inverted
  motor2.setEnable(true);         // abilita driver / enable driver
  //motor2.backlashCompensation(step(80));  // compensazione gioco / backlash compensation
  motor2.setMinFreqHz(400.0f);  // frequenza minima per Start e Stop / minimum Start and Stop frequency
  motor2.useSCurve(true);       // abilita profilo S-curve / enable S-curve profile
  delay(500);

  // ---------- Esempio 1: JERK DEFAULT / Example 1: DEFAULT JERK ----------
  Serial.println(msg(
    ">>> 1: S-curve con JERK DEFAULT (accel=decel) <<<",
    ">>> 1: S-curve with DEFAULT JERK (accel=decel) <<<"));
  motor2.useDefaultJerk(true);
  motor2.moveRelativeWithAccel(
    step(10000),    // passi / steps
    Speed(800.0f),  // 800 RPM
    mms2(500.0f)    // accel 500 mm/s²
  );
  while (motor2.isActive()) {}  //aspetta che il motore sia fermo / wait motor is stopped
  delay(1000);

  // ---------- Esempio 2: JERK RATIO / Example 2: JERK RATIO ----------
  Serial.println(msg(
    ">>> 2: S-curve con JERK RATIO (decel=50% accel) <<<",
    ">>> 2: S-curve with JERK RATIO (decel=50% accel) <<<"));
  motor2.setJerkRatio(0.5f);  // rapporto decel/accel = 0.5 / decel/accel ratio = 0.5
  motor2.moveRelativeWithAccel(
    step(10000),
    Speed(800.0f),
    mms2(500.0f));
  while (motor2.isActive()) {}
  delay(1000);

  // ---------- Esempio 3: JERK MANUALE / Example 3: MANUAL JERK ----------
  Serial.println(msg(
    ">>> 3: S-curve con JERK MANUALE <<<",
    ">>> 3: S-curve with MANUAL JERK <<<"));
  // definisci parametri jerk: aUp, aFlat, aDown, dUp, dFlat, dDown / define jerk params
  motor2.setManualJerk(
    2000.0f, 1000.0f, 2000.0f,  // accel phases mm/s³
    1500.0f, 800.0f, 1500.0f    // decel phases mm/s³
  );
  motor2.moveRelativeWithAccel(
    step(10000),
    Speed(800.0f),
    mms2(500.0f),
    mms2(400.0f));
  while (motor2.isActive()) {}
  delay(1000);

  // ─────────────────────────────────────────────
  // Esempio 4: test MOVIMENTI CON ACCEL/DECEL DEFAULT – test vari metodi
  // Example 4: movements with DEFAULT ACCEL/DECEL – test various methods
  // ─────────────────────────────────────────────
  Serial.println(msg(
    ">>> 4: Movimenti con ACCEL/DECEL DEFAULT – test vari metodi <<<",
    ">>> 4: Movements with DEFAULT ACCEL/DECEL – test various methods <<<"));
  motor2.useDefaultJerk(true);
  motor2.setJerkRatio(1.0f);  // rapporto decel/accel = 1.0 / decel/accel ratio  = 1.0

  // 4.1 Assoluto in mm, Speed
  motor2.moveToPositionWithAccel(
    mm(20.0f),      // mm_eq = 10000 / (1000/2) = 20
    Speed(600.0f),  // testSpeedWithAccel_RPM
    mms2(500.0f),
    mms2(400.0f));
  while (motor2.isActive()) {}
  delay(500);

  // 4.2 Assoluto in mm, Feed
  motor2.moveToPositionWithAccel(
    mm(40.0f),
    Feed(1200.0f),  // feed_eq_acc = 600 RPM × 2 mm/rev = 1200 mm/min
    mms2(500.0f),
    mms2(400.0f));
  while (motor2.isActive()) {}
  delay(500);

  // 4.3 Assoluto in step, Speed
  motor2.moveToPositionWithAccel(
    step(30000),
    Speed(600.0f),
    mms2(500.0f),
    mms2(400.0f));
  while (motor2.isActive()) {}
  delay(500);

  // 4.4 Assoluto in step, Feed
  motor2.moveToPositionWithAccel(
    step(40000),
    Feed(1200.0f),
    mms2(500.0f),
    mms2(400.0f));
  while (motor2.isActive()) {}
  delay(500);

  // 4.5 Relativo in mm, Speed
  motor2.moveRelativeWithAccel(
    mm(20.0f),
    Speed(600.0f),
    mms2(500.0f),
    mms2(400.0f));
  while (motor2.isActive()) {}
  delay(500);

  // 4.6 Relativo in mm, Feed
  motor2.moveRelativeWithAccel(
    mm(20.0f),
    Feed(1200.0f),
    mms2(500.0f),
    mms2(400.0f));
  while (motor2.isActive()) {}
  delay(500);

  // 4.7 Relativo in step, Speed
  motor2.moveRelativeWithAccel(
    step(10000),
    Speed(600.0f),
    mms2(500.0f),
    mms2(400.0f));
  while (motor2.isActive()) {}
  delay(500);

  // 4.8 Relativo in step, Feed
  motor2.moveRelativeWithAccel(
    step(10000),
    Feed(1200.0f),
    mms2(500.0f),
    mms2(400.0f));
  while (motor2.isActive()) {}
  delay(500);
}

void loop() {
  // vuoto / empty
}

