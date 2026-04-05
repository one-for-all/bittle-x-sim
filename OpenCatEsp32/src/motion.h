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
        // Serial.printf("%d: %f\t", (int8_t)i, dutyAng);
        calibratedPWM(i, dutyAng);
      }
      delay((DOF - offset) / 2);
    }
    delete[] diff;
}
