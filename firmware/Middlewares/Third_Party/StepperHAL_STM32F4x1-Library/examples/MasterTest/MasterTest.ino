/* Master test movimenti StepperHAL / StepperHAL master movement test
   Esegue in sequenza: / Runs in sequence:
   1. Movimenti senza accelerazione / Movements without acceleration
   2. Movimenti con accelerazione simmetrica / Movements with symmetric acceleration
   3. Movimenti con accelerazione asimmetrica / Movements with asymmetric acceleration
   4. (Opzionale) Compensazione backlash / (Optional) Backlash compensation

   Ogni test esegue 4 metodi: / Each test runs 4 methods:
   1. Assoluto in mm / Absolute in mm
   2. Assoluto in step / Absolute in steps
   3. Relativo in mm / Relative in mm
   4. Relativo in step / Relative in steps
*/

#include <StepperHAL_Config.h>
#include <StepperHAL_STM32F4x1.h>
#include <StepperHAL_Instances.h>
#include "StepperHAL_Lang.h"

// ─────────────────────────────────────────────
// PARAMETRI DI TEST (MODIFICABILI) / TEST PARAMETERS (MODIFIABLE)
// ─────────────────────────────────────────────
const float mmPerRev = 2.0f;        // 2 mm per giro / 2 mm per revolution
const uint32_t stepsPerRev = 1000;  // 1000 microstep per giro / 1000 microsteps per revolution

mms2 accel(100.0f);  // accel = decel simmetrica / symmetric accel = decel
mms2 decel(150.0f);  // decel diversa / different decel for asymmetric test

const float testSpeedNoAccel_RPM = 300.0f;     // RPM costante / constant RPM
const float testSpeedWithAccel_RPM = 600.0f;  // RPM con accelerazione / RPM with acceleration

// ─────────────────────────────────────────────
// CALCOLI DERIVATI / DERIVED CALCULATIONS
// ─────────────────────────────────────────────
float stepsPerMM = stepsPerRev / mmPerRev;
float mm_eq = 10000.0f / stepsPerMM;
float feed_eq = testSpeedNoAccel_RPM * mmPerRev;  // mm/min equivalente / equivalent
float feed_eq_acc = testSpeedWithAccel_RPM * mmPerRev;  // mm/min equivalente / equivalent

// ─────────────────────────────────────────────
// FUNZIONI DI TEST / TEST FUNCTIONS
// ─────────────────────────────────────────────
void testNoAccel() {
  Serial.println("\n=== Test senza accelerazione / No acceleration test ===");

  // 1. Assoluto in mm
  motor2.moveToPosition(mm(mm_eq), Speed(testSpeedNoAccel_RPM));
  while (motor2.isActive()) {}
  delay(2000);
  motor2.moveToPosition(mm(0), Feed(feed_eq));
  while (motor2.isActive()) {}
  delay(2000);

  // 2. Assoluto in step
  motor2.moveToPosition(step(10000), Speed(testSpeedNoAccel_RPM));
  while (motor2.isActive()) {}
  delay(2000);
  motor2.moveToPosition(step(0), Feed(feed_eq));
  while (motor2.isActive()) {}
  delay(2000);

  // 3. Relativo in mm
  motor2.moveRelative(mm(mm_eq), Speed(testSpeedNoAccel_RPM));
  while (motor2.isActive()) {}
  delay(2000);
  motor2.moveRelative(mm(-mm_eq), Feed(feed_eq));
  while (motor2.isActive()) {}
  delay(2000);

  // 4. Relativo in step
  motor2.moveRelative(step(10000), Speed(testSpeedNoAccel_RPM));
  while (motor2.isActive()) {}
  delay(2000);
  motor2.moveRelative(step(-10000), Feed(feed_eq));
  while (motor2.isActive()) {}
}

void testAccelSym() {
  Serial.println("\n=== Test accelerazione simmetrica / Symmetric acceleration test ===");

  // 1. Assoluto in mm / Absolute mm
  motor2.moveToPositionWithAccel(mm(mm_eq), Speed(testSpeedWithAccel_RPM), accel);
  while (motor2.isActive()) {}
  delay(2000);
  motor2.moveToPositionWithAccel(mm(0), Feed(feed_eq_acc), accel);
  while (motor2.isActive()) {}
  delay(2000);

  // 2. Assoluto in step / Absolute step
  motor2.moveToPositionWithAccel(step(10000), Speed(testSpeedWithAccel_RPM), accel);
  while (motor2.isActive()) {}
  delay(2000);
  motor2.moveToPositionWithAccel(step(0), Feed(feed_eq_acc), accel);
  while (motor2.isActive()) {}
  delay(2000);

  // 3. Relativo in mm / Relative mm
  motor2.moveRelativeWithAccel(mm(mm_eq), Speed(testSpeedWithAccel_RPM), accel);
  while (motor2.isActive()) {}
  delay(2000);
  motor2.moveRelativeWithAccel(mm(-mm_eq), Feed(feed_eq_acc), accel);
  while (motor2.isActive()) {}
  delay(2000);

  // 4. Relativo in step / Relative step
  motor2.moveRelativeWithAccel(step(10000), Speed(testSpeedWithAccel_RPM), accel);
  while (motor2.isActive()) {}
  delay(2000);
  motor2.moveRelativeWithAccel(step(-10000), Feed(feed_eq_acc), accel);
  while (motor2.isActive()) {}
}

void testAccelAsym() {
  Serial.println("\n=== Test accelerazione asimmetrica / Asymmetric acceleration test ===");

  // 1. Assoluto in mm / Absolute mm
  motor2.moveToPositionWithAccel(mm(mm_eq), Speed(testSpeedWithAccel_RPM), accel, decel);
  while (motor2.isActive()) {}
  delay(2000);
  motor2.moveToPositionWithAccel(mm(0), Feed(feed_eq_acc), accel, decel);
  while (motor2.isActive()) {}
  delay(2000);

  // 2. Assoluto in step / Absolute step
  motor2.moveToPositionWithAccel(step(10000), Speed(testSpeedWithAccel_RPM), accel, decel);
  while (motor2.isActive()) {}
  delay(2000);
  motor2.moveToPositionWithAccel(step(0), Feed(feed_eq_acc), accel, decel);
  while (motor2.isActive()) {}
  delay(2000);

  // 3. Relativo in mm / Relative mm
  motor2.moveRelativeWithAccel(mm(mm_eq), Speed(testSpeedWithAccel_RPM), accel, decel);
  while (motor2.isActive()) {}
  delay(2000);
  motor2.moveRelativeWithAccel(mm(-mm_eq), Feed(feed_eq_acc), accel, decel);
  while (motor2.isActive()) {}
  delay(2000);

  // 4. Relativo in step / Relative step
  motor2.moveRelativeWithAccel(step(10000), Speed(testSpeedWithAccel_RPM), accel, decel);
  while (motor2.isActive()) {}
  delay(2000);
  motor2.moveRelativeWithAccel(step(-10000), Feed(feed_eq_acc), accel, decel);
  while (motor2.isActive()) {}  
}

// ─────────────────────────────────────────────
// SETUP E LOOP
// ─────────────────────────────────────────────
void setup() {
  SystemClock_Config();

  Serial.begin(115200);
  while (!Serial) {}

  Serial.println(msg("Clock di sistema: ", "System Clock: ") + String(SystemCoreClock / 1000000) + " MHz");

  delay(2000);

  motor2.begin();
  motor2.setTiming(10, 1000000);
  motor2.invertDIR(false);
  motor2.setEnable(true);

  // motor2.backlashCompensation(step(80));  // decommenta per attivare compensazione backlash / uncomment enable backlash compensation

  testNoAccel();
  delay(2000);
  testAccelSym();
  delay(2000);
  testAccelAsym();
}

void loop() {
  // vuoto / empty
}
