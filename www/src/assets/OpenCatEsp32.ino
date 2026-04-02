// modify the model and board definitions
//***********************
#define BITTLE  // Petoi 9 DOF robot dog: 1 on head + 8 on leg

#define BiBoard_V1_0
//***********************

#include "src/OpenCat.h"

void setup() {
  Serial.begin(115200);  // USB serial
  Serial.setTimeout(SERIAL_TIMEOUT);

  while (Serial.available() && Serial.read())
    ;  // empty buffer

  Serial.println("init robot");
  initRobot();
}

void loop() {
  readSignal();
  reaction();
}
