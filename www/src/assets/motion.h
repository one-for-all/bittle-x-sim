extern Servo servos[];
extern const int MIN_ANGLE;
extern const int MAX_ANGLE;
extern const int MIN_MICROSECONDS;
extern const int MAX_MICROSECONDS;

const int NUM_JOINTS = 9;
int currentAngles[NUM_JOINTS] = {
  135,
  190, 80, 190, 80,
  190, 80, 80, 190
};

// Note: need to call writeMicroseconds(), rather than write(),
//       because ESP32Servo library assume angle is 0-180
void servoSetAngle(int servo_index, int angle) {
  int microseconds = map(angle, MIN_ANGLE, MAX_ANGLE, MIN_MICROSECONDS, MAX_MICROSECONDS); // map angle to microseconds
  servos[servo_index].writeMicroseconds(microseconds);
  currentAngles[servo_index] = angle;
}

const int zeroPositionAngles[NUM_JOINTS] = {
  135,
  190, 80, 190, 80,
  190, 80, 80, 190
};

void zero() {
  for (int i = 0; i < NUM_JOINTS; i++) {
    servoSetAngle(i, zeroPositionAngles[i]);
  }
}

/// Angles that make robot stand
const int standAngles[NUM_JOINTS] = {
  0,
  30, -30, -30, 30,
  -30, 30, 30, -30
};

void stand() {
  int maxDiffAngles = 0;
  for (int i = 0; i < NUM_JOINTS; i++) {
    maxDiffAngles = max(abs(zeroPositionAngles[i] + standAngles[i] - currentAngles[i]), maxDiffAngles);
  }

  for (int step = 0; step < maxDiffAngles; step++) {
    for (int i = 0; i < NUM_JOINTS; i++) {
      if (currentAngles[i] == zeroPositionAngles[i] + standAngles[i])
        continue;
      else if (currentAngles[i] < zeroPositionAngles[i] + standAngles[i])
        servoSetAngle(i, currentAngles[i] + 1);
      else
        servoSetAngle(i, currentAngles[i] - 1);
    }
    delay(1);
  }
  Serial.println("stand");
}

const int rotationDirection[] = {
  1,
  1, -1, -1, 1, -1, 1, 1, -1
};
