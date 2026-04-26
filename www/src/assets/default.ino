#include <ESP32Servo.h>

// Petoi servo parameters
const int MIN_ANGLE = 0;
const int MAX_ANGLE = 270;
const int MIN_MICROSECONDS = 500;
const int MAX_MICROSECONDS = 2500;

const int NUM_JOINTS = 9;
const int servoPins[NUM_JOINTS] = {
  18,             // head
  23, 4, 12, 33,  // shoulders; left-front, right-front, right-back, left-back
  19, 15, 13, 32  // knee;
};

const int zeroPositionAngles[NUM_JOINTS] = {
  135,
  190, 80, 190, 80,
  190, 80, 80, 190
};

Servo servos[NUM_JOINTS];

void setup() {
  // put your setup code here, to run once:
  for (int i = 0; i < NUM_JOINTS; i++) {
    servos[i].setPeriodHertz(50);
    servos[i].attach(servoPins[i], MIN_MICROSECONDS, MAX_MICROSECONDS);
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  // Use servoSetAngle() to set servo angles
}

// Note: need to call writeMicroseconds(), rather than write(),
//       because ESP32Servo library assume angle is 0-180
void servoSetAngle(int servo_index, int angle) {
  int microseconds = map(angle, MIN_ANGLE, MAX_ANGLE, MIN_MICROSECONDS, MAX_MICROSECONDS); // map angle to microseconds
  servos[servo_index].writeMicroseconds(microseconds);
}
