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
#ifdef ESP_PWM
        if (movedJoint[i])  // don't drive the servo if it's being moved by hand in the follow function.
          continue;
        if (manualHeadQ && i < HEAD_GROUP_LEN && token == T_SKILL)  // the head motion will be handled by skill.perform()
          continue;
        if (WALKING_DOF == 8 && i > 3 && i < 8)
          continue;
        if (WALKING_DOF == 12 && i < 4)
          continue;
#endif
        float dutyAng = (target[i - offset] * angleDataRatio + (steps == 0 ? 0 : (1 + cos(M_PI * s / steps)) / 2 * diff[i - offset]));
        // Serial.printf("%d: %f\t", (int8_t)i, dutyAng);
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

float adaptiveParameterArray[][NUM_ADAPT_PARAM] = {
  { -panF / 2, 0 }, { panF / 8, -tiltF / 3 }, { 0, 0 }, { -1 * panF, 0 }, { sRF, -sPF }, { -sRF, -sPF }, { -sRF, sPF }, { sRF, sPF }, { uRF, uPF }, { uRF, uPF }, { uRF, uPF }, { uRF, uPF }, { lRF, -0.5 * lPF }, { lRF, -0.5 * lPF }, { lRF, 0.5 * lPF }, { lRF, 0.5 * lPF }
};

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
    float leftRightFactor = 1;
    if ((leftQ && balanceSlope[0] * RollPitchDeviation[0] > 0)  // operator * is higher than &&
        || (!leftQ && balanceSlope[0] * RollPitchDeviation[0] < 0))
      leftRightFactor = LEFT_RIGHT_FACTOR * abs(balanceSlope[0]);
    rollAdj = (i == 1 || i > 7 ? fabs(RollPitchDeviation[0]) : RollPitchDeviation[0]) * adaptiveParameterArray[i][0] * leftRightFactor;
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

int8_t amplitude = 10;
int8_t sideRatio = 0;
int8_t stateSwitchAngle = 5;
int8_t shift[] = { -4, 4 };
int8_t loopDelay = 5;
int8_t skipStep[] = { 1, 3 };  //support, swing
int8_t phase[] = { 0, 30, 50, 80 };

class CPG {
private:
  int _nSample;
  float *precalcCos;
  int *pick;
  float *sample;  // shrinked sample after skipping points
  bool *supportStage;  // mark if each sample point is in support stage
  int sampleLen;
  int shiftIndex[5];
  int edge;
  int8_t _amplitude;
  int8_t _sideRatio;  //left/right x 10
  int8_t _stateSwitchAngle;
  int8_t _loopDelay;
  int8_t _phase[5];  //range 200. the first 100 is the support stage. The last one should always be 0.
public:
  int8_t _midShift[2];
  int8_t _skipStep[2];
  CPG(int nSample, int8_t skipStep[]) {
    _nSample = nSample;
    precalcCos = new float[_nSample + 1];
    pick = new int[_nSample + 1];
    sample = nullptr;  // will be allocated in setPar
    supportStage = nullptr;  // will be allocated in setPar
    _phase[4] = 0;  //range 200. the first 100 is the support stage. The last one should always be 0.
    sampleLen = 0;
    _skipStep[0] = skipStep[0];
    _skipStep[1] = skipStep[1];
    edge = _nSample * 0.05;
    float step = 1;
    int pickIndex = 0;
    for (int i = 0; i < _nSample; i++) {
      precalcCos[i] = -cos(i * 2 * 3.14159 / _nSample);
      pick[i] = -1;
      if (i > edge && i < _nSample / 2 - edge) {
        step = _skipStep[0];  //support stage
      } else
        step = _skipStep[1];  //swing stage
      if (i == pickIndex) {
        pick[i] = sampleLen;
        pickIndex += step;
        sampleLen++;
      }
    }
    PTHL("sampleLen", sampleLen);
  }

  ~CPG() {
    delete[] precalcCos;
    delete[] pick;
    if (sample != nullptr) {
      delete[] sample;
    }
    if (supportStage != nullptr) {
      delete[] supportStage;
    }
  }
  void setPar(int8_t amplitude, int8_t sideRatio, int8_t stateSwitchAngle, int8_t loopDelay, int8_t midShift[], int8_t phase[]) {
    _amplitude = amplitude;
    _sideRatio = sideRatio;
    _stateSwitchAngle = stateSwitchAngle;
    _loopDelay = loopDelay;
    _midShift[0] = midShift[0];
    _midShift[1] = midShift[1];
    for (byte i = 0; i < 4; i++)
      _phase[i] = phase[i];

    // Release old arrays if they exist
    if (sample != nullptr) {
      delete[] sample;
    }
    if (supportStage != nullptr) {
      delete[] supportStage;
    }

    sample = new float[sampleLen];
    supportStage = new bool[sampleLen];
    sampleLen = 0;

    for (int8_t l = 0; l < 5; l++) {
      shiftIndex[l] = _phase[l] / 100.0 * _nSample;
      for (int i = 0; i < _nSample; i++) {
        int j = (shiftIndex[l] + i) % _nSample;
        if (pick[j] >= 0) {
          if (l == 4) {  //calculate the 0 shift sample, no stateSwitchAngle here
            sample[sampleLen] = precalcCos[j];
            supportStage[sampleLen] = (i > edge && i < _nSample / 2 - edge);  // mark support stage
            sampleLen++;
          } else {
            shiftIndex[l] = pick[j];
            Serial.print(shiftIndex[l]);
            break;
          }
        }
      }
      Serial.println();
    }
  }
  void sendSignal() {
    int prevAngle[4];
    float leftRatio = _sideRatio > 0 ? 1 : (10 + _sideRatio) / 10.0;
    float rightRatio = _sideRatio > 0 ? (10 - _sideRatio) / 10.0 : 1;
    PTHL(leftRatio, rightRatio);
    for (int m = 0; m < sampleLen; m++) {
      for (int8_t l = 0; l < 4; l++) {
        int sampleIndex = (m + shiftIndex[l]) % sampleLen;
        float ratio = (l % 3 ? rightRatio : leftRatio);

        // Calculate base angle with ratio
        float angle = _amplitude * ratio * sample[sampleIndex];

        // Add stateSwitchAngle for support stage (not affected by ratio)
        if (supportStage[sampleIndex]) {
          angle += _stateSwitchAngle;
        }

        calibratedPWM(8 + l, angle + _midShift[l < 2 ? 0 : 1]);
      }
      delay(_loopDelay);
    }
  }
  void printCPG() {
    printToAllPorts("Amp\tside\tswitch\tShiftF\tShiftB\tdelay\tSupport\tSwing\tPhase");
    char message[50];
    sprintf(message, "%d\t%0.1f\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d",
            _amplitude, _sideRatio / 10.0, _stateSwitchAngle, _midShift[0], _midShift[1], _loopDelay, _skipStep[0], _skipStep[1], _phase[0], _phase[1], _phase[2], _phase[3]);
    printToAllPorts(message);
  }
};
CPG *cpg = NULL;
void updateCPG() {
  char *pch;
  char subToken = newCmd[0];
  int8_t parsingShift = 1;
  if (isdigit(subToken) || subToken == '-')  //followed by subtoken
    parsingShift = 0;

  int pars[12] = {};
  int p = 0;

  // Text format: parse with strtok
  pch = strtok(newCmd + parsingShift, " ,");
  while (pch != NULL) {
    pars[p++] = atoi(pch);  //@@@ cast
    pch = strtok(NULL, " ,\t");
  }

  if (subToken == 'g') {
    static int8_t gaits[][13] = {
      { 20, 0, 5, -8, 4, 3, 1, 2, 35, 1, 35, 1, '~' },     //small
      { 35, 0, 5, -8, 4, 3, 1, 2, 35, 1, 35, 1, '~' },     //large
      { 15, 0, 5, -6, -4, 3, 1, 3, 21, 51, 31, 1, '~' },   //back
      { 15, 0, 5, -14, -10, 1, 2, 1, 70, 70, 1, 1, '~' },  //bound
      { 18, 0, 5, -6, -10, 3, 3, 1, 80, 80, 1, 1, '~' },   //bound2
      { 17, 0, 5, 0, 0, 3, 1, 3, 1, 75, 50, 25, '~' },     //heng
      { 18, 0, 5, -4, -4, 3, 2, 3, 36, 49, 49, 36, '~' },  //heng2
      { 18, 0, 5, 0, 0, 3, 1, 2, 35, 46, 35, 46, '~' }     //turnR
    };
  } else {
    if (parsingShift == 0)  //shift
    {
      amplitude = pars[0];
      sideRatio = pars[1];
      stateSwitchAngle = pars[2],
      shift[0] = pars[3];
      shift[1] = pars[4];
      loopDelay = pars[5];
      skipStep[0] = pars[6];
      skipStep[1] = pars[7];
      phase[0] = pars[8];
      phase[1] = pars[9];
      phase[2] = pars[10];
      phase[3] = pars[11];
    } else if (subToken == 'k') {
      skipStep[0] = pars[0];
      skipStep[1] = pars[1];
      if (cpg != NULL)
        delete cpg;
      cpg = new CPG(600, skipStep);
    } else if (subToken == 'a')  //amplitude
      amplitude = pars[0];
    else if (subToken == 'd')
      loopDelay = pars[0];
    else if (subToken == 'p')
      for (byte l = 0; l < 4; l++)
        phase[l] = pars[l];
    else if (subToken == 's')  //shift
    {
      shift[0] = pars[0];
      shift[1] = pars[1];
    }
    if (cpg == NULL)
      cpg = new CPG(300, skipStep);
    else if (skipStep[0] != cpg->_skipStep[0] || skipStep[1] != cpg->_skipStep[1]) {
      delete cpg;
      cpg = new CPG(300, skipStep);
    }
    cpg->setPar(amplitude, sideRatio, stateSwitchAngle, loopDelay, shift, phase);
    if (subToken == 'q') {
      printToAllPorts('r');
      token = T_REST;
      gyroBalanceQ = true;
    }
    cpg->printCPG();
    gyroBalanceQ = false;
  }
}

int calibrationReference[] = {
// the angle difference between P1L and P1S is significant for auto calibration.
#ifdef NYBBLE  // with plastic servo P1L
  0, 42, 0, 0, 0, 0, 0, 0,
  72, 72, -68, -68, -63, -63, 63, 63
#elif defined ROBOT_ARM  //  Bittle R with metal servo P1S
  0, 55, 0, 0, 0, 0, 0, 0,
  65, 65, 77, 77, -63, -63, -63, -63
#else                    // Bittle with plastic servo P1L
  0, 0, 0, 0, 0, 0, 0, 0,
  73, 73, 76, 76, -66, -66, -66, -66
#endif
};

void signalGenerator(int8_t resolution, int8_t speed, int8_t *pars, int8_t len, bool move = 1, char curveMethod = 't') {  // trigonometric
  // circular
  int targetFrame[DOF + 1];
  for (int i = 0; i < DOF; i++)
    targetFrame[i] = currentAng[i];
  for (int t = 0; t < 360; t += resolution) {
    for (int8_t i = 0; i < len / 5; i++) {
      int8_t jointIdx = pars[i * 5];
      int8_t midpoint = pars[i * 5 + 1];
      int8_t amp = pars[i * 5 + 2];
      int8_t freq = pars[i * 5 + 3];
      int8_t phase = pars[i * 5 + 4];
      int angle = 0;
      if (curveMethod == 't')  // currently we set sin function as the default and only option
        angle = midpoint + round(amp * sin(2.0 * M_PI * ((t + phase * 3 / freq) / (360.0 / freq))));
      PTT(angle, ',');
      targetFrame[jointIdx] = angle;
    }
    if (move)
      transform(targetFrame, 1, speed);
    PTL();
  }
}
#define MAX_FRAME 125
#define IDLE_LEARN 2000
#define SMALL_DIFF 7
#define READY_COUNTDOWN 2
int totalFrame = 0;
int8_t learnData[11 * MAX_FRAME];
int8_t learnDataPrev[11];

// Function to print comparison arrays
void printComparisonArrays(int8_t* originalData, int originalTotalFrame, int8_t* optimizedData, int newTotalFrame) {
  // Print original data
  printToAllPorts("=== Original Data ===");
  printToAllPorts("{");
  for (int i = 0; i < originalTotalFrame; i++) {
    for (int j = 0; j < 11; j++) {
      printToAllPorts(String(originalData[i * 11 + j]) + ",", false);  // No newline after each value
    }
    printToAllPorts("");  // Print newline to end the frame
  }
  printToAllPorts("}");

  // Print optimized data
  printToAllPorts("=== Optimized Data ===");
  printToAllPorts("{");
  for (int i = 0; i < newTotalFrame; i++) {
    for (int j = 0; j < 11; j++) {
      printToAllPorts(String(optimizedData[i * 11 + j]) + ",", false);  // No newline after each value
    }
    printToAllPorts("");  // Print newline to end the frame
  }
  printToAllPorts("}");
}

// Optimize learned data by removing intermediate frames while preserving local extrema
void smoothMerge() {
  if (totalFrame <= 3) return;  // No optimization needed for less than 4 frames

  bool keepFrame[MAX_FRAME];
  int totalChange[MAX_FRAME];

  // Initialize arrays
  for (int i = 0; i < totalFrame; i++) {
    keepFrame[i] = false;
    totalChange[i] = 0;
  }

  // Always keep the first and last frames
  keepFrame[0] = true;
  keepFrame[totalFrame - 1] = true;

  // Calculate total change amount for each frame
  for (int frame = 1; frame < totalFrame - 1; frame++) {
    int frameChange = 0;
    for (int joint = 0; joint < 11; joint++) {
      int8_t prev = learnData[(frame - 1) * 11 + joint];
      int8_t curr = learnData[frame * 11 + joint];
      int8_t next = learnData[(frame + 1) * 11 + joint];

      // Calculate the change amount for this joint in this frame
      int jointChange = abs(curr - prev) + abs(next - curr);
      frameChange += jointChange;
    }
    totalChange[frame] = frameChange;
  }

  // Find statistics of change amounts
  int maxChange = 0;
  int minChange = 10000;
  int avgChange = 0;

  for (int i = 1; i < totalFrame - 1; i++) {
    maxChange = max(maxChange, totalChange[i]);
    minChange = min(minChange, totalChange[i]);
    avgChange += totalChange[i];
  }
  avgChange /= (totalFrame - 2);

  // Balanced threshold: keep frames with significant changes
  int threshold = avgChange * 1.2;  // More selective threshold

  PTHL("Average Change:", avgChange);
  PTHL("Max Change:", maxChange);
  PTHL("Threshold:", threshold);

  // Keep frames based on threshold
  for (int frame = 1; frame < totalFrame - 1; frame++) {
    if (totalChange[frame] > threshold) {
      keepFrame[frame] = true;
    }
  }

  // Check local extrema for all joints with unified threshold
  for (int joint = 0; joint < 11; joint++) {
    for (int frame = 1; frame < totalFrame - 1; frame++) {
      int8_t prev = learnData[(frame - 1) * 11 + joint];
      int8_t curr = learnData[frame * 11 + joint];
      int8_t next = learnData[(frame + 1) * 11 + joint];

      // Check if it's a significant local extrema
      if (((curr > prev && curr > next) || (curr < prev && curr < next))
          && (abs(curr - prev) > 15 || abs(next - curr) > 15)) {
        keepFrame[frame] = true;
      }
    }
  }

  // Check for significant direction changes in important joints (0,1,2,8-15)
  for (int joint = 0; joint < 11; joint++) {
    // Skip joints 3-7 as they have small movements
    if (joint >= 3 && joint <= 7) continue;

    for (int frame = 2; frame < totalFrame - 2; frame++) {
      int8_t prev2 = learnData[(frame - 2) * 11 + joint];
      int8_t prev1 = learnData[(frame - 1) * 11 + joint];
      int8_t curr = learnData[frame * 11 + joint];
      int8_t next1 = learnData[(frame + 1) * 11 + joint];
      int8_t next2 = learnData[(frame + 2) * 11 + joint];

      // Check for direction change with meaningful amplitude
      int prevDir = (prev1 > prev2) ? 1 : -1;
      int currDir = (curr > prev1) ? 1 : -1;
      int nextDir = (next1 > curr) ? 1 : -1;

      if (prevDir != currDir || currDir != nextDir) {
        if (abs(curr - prev1) > 15 || abs(next1 - curr) > 15) {
          keepFrame[frame] = true;
        }
      }
    }
  }

  // Ensure the interval between adjacent kept frames is not too large
  for (int i = 0; i < totalFrame - 1; i++) {
    if (keepFrame[i]) {
      int nextKeep = -1;
      for (int j = i + 1; j < totalFrame; j++) {
        if (keepFrame[j]) {
          nextKeep = j;
          break;
        }
      }

      // If interval exceeds 12 frames, insert a frame in the middle
      if (nextKeep > 0 && nextKeep - i > 12) {
        int midFrame = (i + nextKeep) / 2;
        keepFrame[midFrame] = true;
      }
    }
  }

  // Save original data for printing first
  int8_t originalData[11 * MAX_FRAME];
  for (int i = 0; i < totalFrame * 11; i++) {
    originalData[i] = learnData[i];
  }
  int originalTotalFrame = totalFrame;

  // Reorganize data, keeping only the needed frames
  int8_t optimizedData[11 * MAX_FRAME];
  int newTotalFrame = 0;

  for (int i = 0; i < totalFrame; i++) {
    if (keepFrame[i]) {
      for (int j = 0; j < 11; j++) {
        optimizedData[newTotalFrame * 11 + j] = learnData[i * 11 + j];
      }
      newTotalFrame++;
    }
  }

  // Copy optimized data back to original array
  for (int i = 0; i < newTotalFrame * 11; i++) {
    learnData[i] = optimizedData[i];
  }

  PTHL("Original Frames:", originalTotalFrame);
  PTHL("Optimized Frames:", newTotalFrame);

  // Call function to print comparison arrays (can be toggled on/off)
  printComparisonArrays(originalData, originalTotalFrame, optimizedData, newTotalFrame);

  // Update total frame count
  totalFrame = newTotalFrame;
}
