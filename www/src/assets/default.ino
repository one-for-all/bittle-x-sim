#include <ESP32Servo.h>
#include "config.h"
#include "motion.h"
#include "walk.h"
#include "turn.h"

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  for (int i = 0; i < NUM_JOINTS; i++) {
    servos[i].setPeriodHertz(50);
    servos[i].attach(servoPins[i], MIN_MICROSECONDS, MAX_MICROSECONDS);
  }
  zero();
  stand();
}

void loop() {
  // put your main code here, to run repeatedly:
  // turnLeft();
  // stand();
  // walk();
  // stand();
}
