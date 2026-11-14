void calibratedPWM(byte i, float angle, float speedRatio = 0) {
  if (PWM_NUM == 12 && WALKING_DOF == 8 && i > 3 && i < 8)  // there's no such joint in this configuration
    return;
  int actualServoIndex = (PWM_NUM == 12 && i > 3) ? i - 4 : i;
  angle = max(float(angleLimit[i][0]), min(float(angleLimit[i][1]), angle));
  int duty0 = calibratedZeroPosition[i] + currentAng[i] * rotationDirection[i];
  previousAng[i] = currentAng[i];
  currentAng[i] = angle;
  int duty = calibratedZeroPosition[i] + angle * rotationDirection[i];
  int steps = speedRatio > 0 ? int(round(abs(duty - duty0) / 1.0 /*degreeStep*/ / speedRatio)) : 0;
  // if default speed is 0, no interpolation will be used
  // otherwise the speed ratio is compared to 1 degree per second.

  for (int s = 0; s <= steps; s++) {
    int degree = duty + (steps == 0 ? 0 : (1 + cos(M_PI * s / steps)) / 2 * (duty0 - duty));
    {
      servo[actualServoIndex].write(degree);
    }
  }
}

void allCalibratedPWM(int *dutyAng, byte offset = 0) {
  for (int8_t i = DOF - 1; i >= offset; i--) {
    calibratedPWM(i, dutyAng[i]);
  }
}

template<typename T>
void transform(T *target, byte angleDataRatio = 1, float speedRatio = 1, byte offset = 0, int period = 0, int runDelay = 8) {
    int *diff = new int[DOF - offset];
    int maxDiff = 0;
    for (byte i = offset; i < DOF; i++) {
      if (manualHeadQ && i < HEAD_GROUP_LEN && token == T_SKILL)  // the head motion will be handled by skill.perform()
        continue;
      diff[i - offset] = currentAng[i] - target[i - offset] * angleDataRatio;
      maxDiff = max(maxDiff, abs(diff[i - offset]));
    }
    int steps = speedRatio > 0 ? int(round(maxDiff / 1.0 /*degreeStep*/ / speedRatio)) : 0;  // default speed is 1 degree per step
    for (int s = 0; s <= steps; s++) {
#ifdef GYRO_PIN
      if (updateGyroQ && printGyroQ) {
        print6Axis();
      }
#endif
      for (byte i = offset; i < DOF; i++) {
        if (movedJoint[i])  // don't drive the servo if it's being moved by hand in the follow function.
          continue;
        if (manualHeadQ && i < HEAD_GROUP_LEN && token == T_SKILL)  // the head motion will be handled by skill.perform()
          continue;
        if (WALKING_DOF == 8 && i > 3 && i < 8)
          continue;
        if (WALKING_DOF == 12 && i < 4)
          continue;
        float dutyAng = (target[i - offset] * angleDataRatio + (steps == 0 ? 0 : (1 + cos(M_PI * s / steps)) / 2 * diff[i - offset]));
        calibratedPWM(i, dutyAng);
      }
      delay((DOF - offset) / 2);
    }
    delete[] diff;
}

// balancing parameters
#define ROLL_LEVEL_TOLERANCE 5  // the body is still considered as level, no angle adjustment
#define PITCH_LEVEL_TOLERANCE 3
#define NUM_ADAPT_PARAM 2                                                   // number of parameters for adaption
float levelTolerance[2] = { ROLL_LEVEL_TOLERANCE, PITCH_LEVEL_TOLERANCE };  // the body is still considered as level, no angle adjustment

#define LARGE_ROLL 90
#define LARGE_PITCH 75

// the following coefficients will be divided by radPerDeg in the adjust() function. so (float) 0.1 can be saved as (int8_t) 1
// this trick allows using int8_t array insead of float array, saving 96 bytes and allows storage on EEPROM
#define panF 60
#define tiltF 60
#define sRF 50            // shoulder roll factor
#define sPF 12            // shoulder pitch factor
#define uRF 50            // upper leg roll factor
#define uPF 50            // upper leg pitch factor
#define lRF (-1.5 * uRF)  // lower leg roll factor
#define lPF (-1.5 * uPF)  // lower leg pitch factor
#define LEFT_RIGHT_FACTOR 2
#define FRONT_BACK_FACTOR 1.2
#define POSTURE_WALKING_FACTOR 0.5
#define ADJUSTMENT_DAMPER 5
// #ifdef POSTURE_WALKING_FACTOR
// float postureOrWalkingFactor = 1;
// #endif

#ifdef X_LEG  // >< leg
float adaptiveParameterArray[][NUM_ADAPT_PARAM] = {
  { -panF / 2, 0 }, { -panF / 2, -tiltF }, { -2 * panF, 0 }, { 0, -1 * tiltF }, { sRF, -sPF }, { -sRF, -sPF }, { -sRF, sPF }, { sRF, sPF }, { uRF, uPF }, { uRF, uPF }, { -uRF, uPF }, { -uRF, uPF }, { lRF, lPF }, { lRF, lPF }, { -lRF, lPF }, { -lRF, lPF }
};
#else  // >> leg
float adaptiveParameterArray[][NUM_ADAPT_PARAM] = {
  { -panF / 2, 0 }, { panF / 8, -tiltF / 3 }, { 0, 0 }, { -1 * panF, 0 }, { sRF, -sPF }, { -sRF, -sPF }, { -sRF, sPF }, { sRF, sPF }, { uRF, uPF }, { uRF, uPF }, { uRF, uPF }, { uRF, uPF }, { lRF, -0.5 * lPF }, { lRF, -0.5 * lPF }, { lRF, 0.5 * lPF }, { lRF, 0.5 * lPF }
};
#endif

void resetAdjust() {
  for (int i = 0; i < DOF; i++)
    currentAdjust[i] = 0;
}

float adjust(byte i, bool postureQ = false) {
  float rollAdj, pitchAdj;
  float cutOff = postureQ ? 45 : 15;  // reduce angle deviation for non-posture skills to filter noise
  pitchAdj = adaptiveParameterArray[i][1] * max(float(-cutOff), min(float(cutOff), RollPitchDeviation[1]));
  if (i == 1 || i > 3) {  // check idx = 1
    bool leftQ = (i - 1) % 4 > 1 ? true : false;
    // bool frontQ = i % 4 < 2 ? true : false;
    // bool upperQ = i / 4 < 3 ? true : false;
    float leftRightFactor = 1;
    if ((leftQ && balanceSlope[0] * RollPitchDeviation[0] > 0)  // operator * is higher than &&
        || (!leftQ && balanceSlope[0] * RollPitchDeviation[0] < 0))
      leftRightFactor = LEFT_RIGHT_FACTOR * abs(balanceSlope[0]);
    rollAdj = (i == 1 || i > 7 ? fabs(RollPitchDeviation[0]) : RollPitchDeviation[0]) * adaptiveParameterArray[i][0] * leftRightFactor;
    //    rollAdj = fabs(RollPitchDeviation[0]) * adaptiveParameterArray[i][0] * leftRightFactor;
  } else
    rollAdj = RollPitchDeviation[0] * adaptiveParameterArray[i][0];
  float idealAdjust = radPerDeg * (
#ifdef POSTURE_WALKING_FACTOR
                        (i > 3 ? POSTURE_WALKING_FACTOR : 1) *
#endif
                          balanceSlope[0] * rollAdj
                        - balanceSlope[1] * pitchAdj);
#ifdef ADJUSTMENT_DAMPER
  currentAdjust[i] += max(min(idealAdjust - currentAdjust[i], float(ADJUSTMENT_DAMPER)), -float(ADJUSTMENT_DAMPER));
#else
  currentAdjust[i] = idealAdjust;
#endif
  int thres = (i > 3 && i % 4 < 2) ? 15 : 45;
  currentAdjust[i] = max(float(-45), min(float(thres), currentAdjust[i]));
  return currentAdjust[i];
}
