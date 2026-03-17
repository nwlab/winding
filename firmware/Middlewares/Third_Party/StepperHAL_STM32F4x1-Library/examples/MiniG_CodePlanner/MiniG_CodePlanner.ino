//#define LANGUAGE_IT  // Commenta per passare all'inglese/Comment for english

#include <StepperHAL_STM32F4x1.h>
#include <StepperHAL_Instances.h>
#include "StepperHAL_Lang.h"

void executeGCode(const String& line);

float lastFeedrate = 0.0; 

void setup() {
  SystemClock_Config();

  Serial.begin(115200);
  while (!Serial); 

  Serial.println(msg("Clock di sistema: ", "System Clock: ") + String(SystemCoreClock / 1000000) + " MHz");

  delay(2000);

  motor2.begin();
  motor2.setTiming(10, 1000000);
  motor2.invertDIR(false);
  motor2.setEnable(true);

  Serial.print(msg("Step per giro: ", "Steps per revolution: "));
  Serial.println(motor2.getStepsPerRev());

  Serial.println(msg("\nInvia comandi G-code come: G1 X40 F1200 oppure g1 x20 f600",
                     "\nSend G-code commands like: G1 X40 F1200 or g1 x20 f600"));
}

void loop() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) {
      executeGCode(line);
    }
  }
}

void executeGCode(const String& line) {
  String cmd = line;
  cmd.toUpperCase(); 

  if (!cmd.startsWith("G1")) {
    Serial.println(msg("[G-code] Comando non supportato.", "[G-code] Unsupported command."));
    return;
  }

  float x = NAN;
  float f = NAN;

  int idxX = cmd.indexOf('X');
  int idxF = cmd.indexOf('F');

  if (idxX >= 0) {
    int endX = cmd.indexOf(' ', idxX);
    x = cmd.substring(idxX + 1, endX > 0 ? endX : cmd.length()).toFloat();
  }

  if (idxF >= 0) {
    int endF = cmd.indexOf(' ', idxF);
    f = cmd.substring(idxF + 1, endF > 0 ? endF : cmd.length()).toFloat();
    if (!isnan(f)) lastFeedrate = f;
  } else {
    f = lastFeedrate;
  }

  if (isnan(x)) {
    Serial.println(msg("[G-code] Parametro X mancante o non valido.",
                       "[G-code] Missing or invalid X parameter."));
    return;
  }

  if (f <= 0.0) {
    Serial.println(msg("[G-code] Feedrate mancante o non valido. Usa Fxxxx.",
                       "[G-code] Missing or invalid feedrate. Use Fxxxx."));
    return;
  }

  Serial.print(msg("[G-code] G1 X", "[G-code] G1 X"));
  Serial.print(x);
  Serial.print(" F");
  Serial.println(f);

  motor2.moveToPosition(mm(x), Feed(f));

  while (motor2.isActive()) {
    Serial.print(msg("Posizione: ", "Position: "));
    Serial.print(motor2.getPositionMM());
    Serial.print(" mm (");
    Serial.print(motor2.getPositionSteps());
    Serial.println(" steps)");
    delay(200);
  }

  Serial.println(msg("[G-code] Movimento completato.", "[G-code] Movement completed."));
  Serial.print(msg("Posizione: ", "Position: "));
  Serial.print(motor2.getPositionMM());
  Serial.print(" mm (");
  Serial.print(motor2.getPositionSteps());
  Serial.println(" steps)");
}

