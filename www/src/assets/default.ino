#include <ESP32Servo.h>
#include "motion.h"
#include "walk.h"
#include "turn.h"

// Petoi servo parameters
const int MIN_ANGLE = 0;
const int MAX_ANGLE = 270;
const int MIN_MICROSECONDS = 500;
const int MAX_MICROSECONDS = 2500;

const int servoPins[NUM_JOINTS] = {
  18,             // head
  23, 4, 12, 33,  // shoulders; left-front, right-front, right-back, left-back
  19, 15, 13, 32  // knee;
};

Servo servos[NUM_JOINTS];

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
