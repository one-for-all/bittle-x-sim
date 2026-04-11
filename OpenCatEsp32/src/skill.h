#define MEMORY_ADDRESS_SIZE 4
class SkillPreview {
public:
  char *skillName;  // use char array instead of String to save memory
  int period;       // the period of a skill. 1 for posture, >1 for gait, <-1 for behavior
  int index;
  SkillPreview(int s) {
    skillName = new char[strlen(skillNameWithType[s])];
    strcpy(skillName, skillNameWithType[s]);
    skillName[strlen(skillNameWithType[s]) - 1] = '\0';  // drop the last charactor of skill type
    unsigned int pgmAddress = (unsigned int)progmemPointer[s];
    period = (int8_t)pgm_read_byte(pgmAddress);  // automatically cast to char*
    index = s;
  }
};

class SkillList : public QList<SkillPreview *> {
public:
  SkillList() {
    PT("Build skill list...");
    PTL(sizeof(skillNameWithType) / MEMORY_ADDRESS_SIZE);
    for (int s = 0; s < sizeof(progmemPointer) / MEMORY_ADDRESS_SIZE; s++) {
      SkillPreview *tempAddress = new SkillPreview(s);
      this->push_back(tempAddress);
    }
  }
  int lookUp(const char *key) {
    byte nSkills = sizeof(progmemPointer) / MEMORY_ADDRESS_SIZE;
    byte randSkillIdx = strcmp(key, "x") ? nSkills : random(nSkills);
    byte keyLen = strlen(key);
    char lr = key[keyLen - 1];
    for (int s = 0; s < nSkills; s++) {
      char readName[CMD_LEN + 1];
      strcpy(readName, this->get(s)->skillName);
      char readNameLR = readName[strlen(readName) - 1];
      byte nameLen = strlen(readName);
      if (s == randSkillIdx          // random skill
          || !strcmp(readName, key)  // exact match: gait type + F or L, behavior
          || (readName[nameLen - 1] != 'F' && strcmp(readName, "bk") && !strncmp(readName, key, keyLen - 1) && (lr == 'L' || lr == 'R' || lr == 'X'))  // L, R or X
      ) {
        printToAllPorts(readName);
        return s;
      }
    }
    PT('?');  // key not found
    PT(key);
    PTL('?');  // it will print ?? in random mode. Why?
    return -1;
  }
};
SkillList *skillList;

class Skill {
public:
  char skillName[20];  // use char array instead of String to save memory
  int8_t offsetLR;
  int period;  // the period of a skill. 1 for posture, >1 for gait, <-1 for behavior
  float transformSpeed;
  byte skillHeader;
  byte frameSize;
  int expectedRollPitch[2];  // expected body orientation (roll, pitch)
  byte angleDataRatio;       // divide large angles by 1 or 2. if the max angle of a skill is >128, all the angls will be divided by 2
  int8_t loopCycle[3];       // the looping section of a behavior (starting row, ending row, repeating cycles)
  byte firstMotionJoint;
  int8_t *dutyAngles;  // the data array for skill angles and parameters

  Skill() {
    skillName[0] = '\0';  // use char array instead of String to save memory
    offsetLR = 0;
    period = 0;
    transformSpeed = 1;
    frameSize = 0;
    expectedRollPitch[0] = expectedRollPitch[1] = 0;
    angleDataRatio = 1;
    loopCycle[0] = loopCycle[1] = loopCycle[2] = 0;
    firstMotionJoint = 0;
    dutyAngles = NULL;
  }
  void buildSkill() {  // K token
    strcpy(skillName, "tmp");
    offsetLR = 0;
    period = (int8_t)newCmd[0];  // automatically cast to char*
    dataLen(period);
    formatSkill();
  }

  void buildSkill(int s) {
    strcpy(skillName, newCmd);
    unsigned int pgmAddress = (unsigned int)progmemPointer[s];
    period = (int8_t)pgm_read_byte(pgmAddress);  // automatically cast to char*
    for (int i = 0; i < dataLen(period); i++) {
      newCmd[i] = pgm_read_byte(pgmAddress++);
    }
    newCmd[dataLen(period)] = '~';
    formatSkill();
  }
  ~Skill() { }
  int dataLen(int8_t p) {
    skillHeader = p > 0 ? 4 : 7;
    frameSize = p > 1 ? WALKING_DOF :  // gait
                  p == 1 ?
                  DOF :  // posture
                  DOF + 4;  // behavior
    int len = skillHeader + abs(p) * frameSize;
    return len;
  }

  void inplaceShift() {
    int angleLen = abs(period) * frameSize;  // need one extra byte for terminator '~'
    // int shiftRequiredByNewCmd = CMD_LEN - skillHeader + 1;
    // required shift to store CMD_LEN + 1 chars. it can hold a command with CMD_LEN chars. the additioanl byte is required by '\0'.
    spaceAfterStoringData = BUFF_LEN - angleLen - 1;  // the bytes before the dutyAngles. The allowed command's bytes needs to -1
    if (CMD_LEN > spaceAfterStoringData) {
      PTF("LMT ");
      PTL(spaceAfterStoringData);
    }
    for (int i = 0; i <= angleLen; i++)
      newCmd[BUFF_LEN - i] = newCmd[skillHeader + angleLen - i];
    dutyAngles = (int8_t *)newCmd + BUFF_LEN - angleLen;
  }

  void formatSkill() {
    transformSpeed = 1;  // period > 1 ? 1 : 0.5;
    firstMotionJoint = (period <= 1) ? 0 : DOF - WALKING_DOF;

    for (int i = 0; i < 2; i++) {
      expectedRollPitch[i] = (int8_t)newCmd[1 + i];
    }
    angleDataRatio = (int8_t)newCmd[3];
    byte baseHeader = 4;
    if (period < 0) {
      for (byte i = 0; i < 3; i++)
        loopCycle[i] = (int8_t)newCmd[baseHeader++];
    }
    inplaceShift();
    periodGlobal = period;
  }
#define PRINT_SKILL_DATA
  void info() {
    PT("Skill Name: ");
    PTL(skillName);
    PTF("period: ");
    PT(period);
    PT(",\texpected(pitch,roll): (");
    PT(expectedRollPitch[0]);
    PT(",");
    PT(expectedRollPitch[1]);
    PT(")\t");
    PTF("angleRatio: ");
    PTL(angleDataRatio);
    if (period < 0) {
      PT("loop frame: ");
      for (byte i = 0; i < 3; i++)
        PT(String((byte)loopCycle[i]) + ", ");
      PTL();
    }
#ifdef PRINT_SKILL_DATA
    int showRows = 1;
    for (int k = 0; k < abs(period); k++) {
      if (abs(period) <= showRows + 2 || k < showRows || k == abs(period) - 1) {
        for (int col = 0; col < frameSize; col++) {
          PT((int8_t)dutyAngles[k * frameSize + col]);
          PT(",\t");
        }
        PTL();
      } else {
        if (k == showRows) {
          PTF(" skipping ");
          PT(abs(period) - 1 - showRows);
          PTF(" frames");
        }
        PT('.');
        if (k == abs(period) - 2)
          PTL();
      }
    }
#endif
    PTL();
  }
  void mirror() {  // Create a mirror function to allow the robot to pick random directions of behaviors.
    // It makes the robot more unpredictable and helps it get rid of an infinite loop,
    // such as failed fall-recovering against a wall.
    expectedRollPitch[0] = -expectedRollPitch[0];
    for (int k = 0; k < abs(period); k++) {
      if (period <= 1) {                                         // behavior
        dutyAngles[k * frameSize] = -dutyAngles[k * frameSize];  // head and tail panning angles
        dutyAngles[k * frameSize + 2] = -dutyAngles[k * frameSize + 2];
      }
      for (byte col = (period > 1) ? 0 : 2; col < ((period > 1) ? WALKING_DOF : DOF) / 2; col++) {
        int8_t temp = dutyAngles[k * frameSize + 2 * col];
        dutyAngles[k * frameSize + 2 * col] = dutyAngles[k * frameSize + 2 * col + 1];
        dutyAngles[k * frameSize + 2 * col + 1] = temp;
      }
    }
  }
  void shiftCenterOfMass(int angle) {
    int offset = 8;
    if (period > 1)
      offset = 0;
    for (int k = 0; k < abs(period); k++) {
      float rate = 1.2;
      if (angle < 0)
        rate = 0.6;
      for (byte col = 0; col < 2; col++) {
        dutyAngles[k * frameSize + offset + col] = dutyAngles[k * frameSize + offset + col] + angle;
      }
      for (byte col = 4; col < 6; col++) {
        dutyAngles[k * frameSize + offset + col] = dutyAngles[k * frameSize + offset + col] - angle * rate;
      }
    }
  }
  int nearestFrame() {
    if (period == 1)
      frame = 0;
    else  // find the nearest frame using certain algorithm
      frame = 0;
    return frame;
  }
  void transformToSkill(int frame = 0) {
    transform(dutyAngles + frame * frameSize, angleDataRatio, transformSpeed, firstMotionJoint, period, runDelay);
  }
  void convertTargetToPosture(int *targetFrame) {
    int extreme[2];
    getExtreme(targetFrame, extreme);
    if (extreme[0] < -125 || extreme[1] > 125) {
      angleDataRatio = 2;
      for (int i = 0; i < DOF; i++)
        targetFrame[i] /= 2;
    } else
      angleDataRatio = 1;
    arrayNCPY(dutyAngles, targetFrame, DOF);
    period = 1;
    firstMotionJoint = 0;
    frameSize = DOF;
    frame = 0;
  }
  void perform() {
    if (period < 0) {  // behaviors
      interruptedDuringBehavior = false;
      int8_t repeat = loopCycle[2] >= 0 && loopCycle[2] < 2 ? 0 : loopCycle[2] - 1;
      gyroBalanceQlag = gyroBalanceQ;
      gyroBalanceQ = strcmp(skillName, "bf") && strcmp(skillName, "ff") && strcmp(skillName, "flipF") && strcmp(skillName, "flipD") && strcmp(skillName, "flipL") && strcmp(skillName, "flipR") && strcmp(skillName, "pd") && strcmp(skillName, "hds") && strcmp(skillName, "bx") && strstr(skillName, "rl") == NULL;  // won't read gyro for fast motion
      for (byte c = 0; c < abs(period); c++) {
        // the last two in the row are transition speed and delay
        Stream *serialPort = NULL;
        String source;
          // the BT_BLE is unhandled here
          if (moduleActivatedQ[0] && Serial2.available()) {
            serialPort = &Serial2;
            source = "Serial2";
          } else if (Serial.available()) {
            serialPort = &Serial;
            source = "Serial";
          }
        if (serialPort // user input
            || (gyroBalanceQ                                                               // the IMU should be used for balancing
                && ((imuException != IMU_EXCEPTION_FLIPPED && !strcmp(skillName, "rc"))    // recovered during recover
                    || (imuException == IMU_EXCEPTION_FLIPPED && strcmp(skillName, "rc"))  // flipped during other skills
                    ))) {
          PTHL("imuException: ", imuException);
          PTLF("Behavior interrupted");
          interruptedDuringBehavior = true;
          gyroBalanceQ = gyroBalanceQlag;  // Restore gyroBalanceQ before returning
          return;
        }
        transform(dutyAngles + c * frameSize, angleDataRatio, dutyAngles[DOF + c * frameSize] / 8.0);
        delay(abs(dutyAngles[DOF + 1 + c * frameSize] * 50));

        if (repeat != 0 && c != 0 && c == loopCycle[1]) {
          c = loopCycle[0] - 1;
          if (repeat > 0)  // if repeat <0, infinite loop. only reset button will break the loop
            repeat--;
        }
      }
      gyroBalanceQ = gyroBalanceQlag;
    } else  {  // postures and gaits
      for (int jointIndex = 0; jointIndex < DOF; jointIndex++) {
#ifndef HEAD
        if (jointIndex == 0)
          jointIndex = 2;
#endif
#ifndef TAIL
        if (jointIndex == 2)
          jointIndex = DOF - WALKING_DOF;
#endif
#if WALKING_DOF == 8
        if (jointIndex == 4)
          jointIndex = 8;
#endif
        float duty;
        if ((abs(period) > 1 && jointIndex < firstMotionJoint)      // gait and non-walking joints
            || (abs(period) == 1 && jointIndex < 4 && manualHeadQ)  // posture and head group and manually controlled head
        ) {
          if (!manualHeadQ && jointIndex < 4) {
            duty =
              (jointIndex != 1 ? offsetLR : 0)  // look left or right
              + 10 * sin(frame * (jointIndex + 2) * M_PI / abs(period));
          } else
            duty = currentAng[jointIndex] + max(-20, min(20, (targetHead[jointIndex] - currentAng[jointIndex])));
        } else {
          duty = dutyAngles[frame * frameSize + jointIndex - firstMotionJoint] * angleDataRatio;
        }
        calibratedPWM(jointIndex, duty);
      }
      frame += tStep;
      if (frame >= abs(period)) {
        frame = 0;

        // Check if in cycle counting mode and count completed cycles
        if (cycleCountingMode && period > 1) {
          completedCycles++;
          PTH("Completed cycle: ", completedCycles);
          PTHL(" / ", targetCycles);

          if (completedCycles >= targetCycles) {
            // Target cycles reached, stop the gait
            cycleCountingMode = false;
            completedCycles = 0;
            targetCycles = 0;
            PTLF("Cycle target reached, stopping gait");
          }
        }
      }
    }
  }
};

Skill *skill;

void loadBySkillName(const char *skillName) {
  // get lookup information from on-board EEPROM and read the data array from storage
  char lr = skillName[strlen(skillName) - 1];
  int skillIndex;
  skillIndex = skillList->lookUp(skillName);
  if (skillIndex != -1) {
    skill->offsetLR = (lr == 'L' ? 30 : (lr == 'R' ? -30 : 0));
    skill->buildSkill(skillList->get(skillIndex)->index);
    strcpy(newCmd, skill->skillName);

    if (lr == 'R'                                                 // 'R' must mirror
        || ((lr == 'X' || lr != 'L')                              // 'L' should not mirror
            && ((random(10) > 7 && random(10) > 5) || coinFace))  // 1/5 chance to random otherwise flip everytime
    ) {
      skill->mirror();  // mirror the direction of a behavior
    }

    coinFace = !coinFace;
    skill->transformToSkill(skill->nearestFrame());
    for (byte i = 0; i < HEAD_GROUP_LEN; i++)
      targetHead[i] = currentAng[i] - currentAdjust[i];
  }
}
