#include "esp32-hal.h"
#include <stdio.h>

void reaction() {  // Reminder:  reaction() is repeatedly called in the "forever" loop() of OpenCatEsp32.ino
  if (newCmdIdx) {
    // PTLF("-----");
    lowerToken = tolower(token);
    if (initialBoot) {  //-1 for marking the boot-up calibration state
      fineAdjustQ = true;
      // updateGyroQ = true;
      gyroBalanceQ = true;
      initialBoot = false;
    }

    if (token != T_REST && newCmdIdx < 5)
      idleTimer = millis();
    if (newCmdIdx < 5 && lowerToken != T_BEEP && token != T_MEOW && token != T_LISTED_BIN
        && token != T_INDEXED_SIMULTANEOUS_BIN && token != T_TILT && token != T_READ && token != T_WRITE
        && token != T_JOYSTICK && token != T_EXTENSION)
    if ((lastToken == T_SERVO_CALIBRATE || lastToken == T_REST || lastToken == T_SERVO_FOLLOW || !strcmp(lastCmd, "fd"))
        && token != T_SERVO_CALIBRATE) {
      // updateGyroQ = true;
      gyroBalanceQ = true;  // This is the default state for this "Q" boolean with all tokens except (T_SERVO_CALIBRATE
                            // && when lastToken is one of the listed values)
      printToAllPorts('G');
    }
    if (token != T_PAUSE && !tStep) {
      tStep = 1;
      printToAllPorts('p');
    }
    if (token != T_SERVO_FEEDBACK && token != T_SERVO_FOLLOW && measureServoPin != -1) {
      for (byte i = 0; i < DOF; i++)
        movedJoint[i] = 0;
      reAttachAllServos();
      measureServoPin = -1;
      readFeedbackQ = false;    // This is the default value for this "Q" boolean condition with all tokens except (those
                                // in the conditional && measureServoPin != -1)
      followFeedbackQ = false;  // This is the default state for this "Q" boolean condition with all tokens except
                                // (those in the conditional && measureServoPin != -1)
    }
    switch (token) {
      case T_HELP_INFO:
        {
          PTLF("* Please refer to docs.petoi.com.\nEnter any character to continue.");
          while (!Serial.available())
            ;
          break;
        }
      case T_QUERY:
        {
          if (cmdLen == 0) {
            printToAllPorts(MODEL);
            printToAllPorts(SoftwareVersion);
          } else {
            byte i = 0;
            while (newCmd[i] != '\0') {
              if (newCmd[i] == C_QUERY_PARTITION)
                displayNsvPartition();
              i++;
            }
          }
          break;
        }
      case T_NAME:
        {
          if (cmdLen > 16)
            printToAllPorts("ERROR! The name should be within 16 characters!");
          else if (cmdLen)
            customBleID(
              newCmd,
              cmdLen);  // customize the Bluetooth device's broadcast name. e.g. nMyDog will name the device as "MyDog"
                        // it takes effect the next time the board boosup. it won't interrupt the current connecton.
          printToAllPorts(
            config.getString("ID")
          );
          break;
        }
      case T_PAUSE:
        {
          tStep = !tStep;             // tStep can be -1
          token = tStep ? 'p' : 'P';  // P for pause activated
          if (tStep)
            token = T_SKILL;
          else
            shutServos();
          break;
        }
      case T_ACCELERATE:
        {
          runDelay = max(0, runDelay - 1);
          PTHL("Run delay", runDelay);
          break;
        }
      case T_DECELERATE:
        {
          runDelay = min(delayLong, runDelay + 1);
          PTHL("Run delay", runDelay);
          break;
        }
      case T_REST:
        {
          gyroBalanceQ = false;
          printToAllPorts('g');
          if (cmdLen == 0) {
            strcpy(newCmd, "rest");
            if (strcmp(newCmd, lastCmd)) {
              loadBySkillName(newCmd);
            }
            shutServos();
            manualHeadQ = false;
            readFeedbackQ = false;
          } else if (cmdLen == 1) {  // allow turning off a single joint
            shutServos(atoi(newCmd));
          }
          break;
        }
      case T_JOINTS:
        {  // show the list of current joint anles
          //          printRange(DOF);
          //          printList(currentAng);
          printToAllPorts('=');
          if (cmdLen)
            printToAllPorts(currentAng[atoi(newCmd)]);
          else {
            printToAllPorts(range2String(DOF));
            printToAllPorts(list2String(currentAng));
          }
          break;
        }
      case ';':
        {
          setServoP(P_SOFT);
          break;
        }
      case ':':
        {
          setServoP(P_HARD);
          break;
        }
      case T_SAVE:
        {
          PTLF("save offset");
          saveCalib(servoCalib);
          break;
        }
      case T_ABORT:
        {
          PTLF("aborted");
          config.getBytes("calib", servoCalib, DOF);
          break;
        }
      case T_RESET:
        {
          resetAsNewBoard('R');
          break;
        }
      // this block handles array like arguments
      case T_INDEXED_SEQUENTIAL_BIN:
      case T_INDEXED_SIMULTANEOUS_BIN:
      case T_READ:
      case T_WRITE:
        {  // indexed joint motions: joint0, angle0, joint1, angle1, ... (binary encoding)
          if (cmdLen < 2)
            manualHeadQ = false;
          else {
            int targetFrame[DOF + 1];
            for (int i = 0; i < DOF; i++) {
              targetFrame[i] = currentAng[i] - (gyroBalanceQ ? currentAdjust[i] : 0);
            }
            targetFrame[DOF] = '~';
            byte group = token == T_WRITE ? 3 : 2;
            for (int i = 0; i < cmdLen; i += group) {
              int8_t cmdIndex = (int8_t)newCmd[i];  // Convert to int8_t to avoid char subscript warnings
              if (cmdIndex >= 0 && cmdIndex < DOF) {
                targetFrame[cmdIndex] = (int8_t)newCmd[i + 1];
                if (cmdIndex < 4) {
                  targetHead[cmdIndex] = (int8_t)newCmd[i + 1];
                  manualHeadQ = true;
                } else
                  nonHeadJointQ = true;
              }
              if (token == T_INDEXED_SEQUENTIAL_BIN) {
                transform(targetFrame, 1, transformSpeed);
                delay(10);
              } else if (token == T_WRITE) {  // Write a/d pin value
                pinMode((uint8_t)newCmd[i + 1], OUTPUT);
                if (newCmd[i] == TYPE_ANALOG) {
                  analogWrite(
                    (uint8_t)newCmd[i + 1],
                    uint8_t(newCmd[i + 2]));  // analog value can go up to 255.
                                              // the value was packed as unsigned byte by ardSerial
                                              // but casted by readSerial() as signed char and saved into newCmd.
                } else if (newCmd[i] == TYPE_DIGITAL)
                  digitalWrite((uint8_t)newCmd[i + 1], (uint8_t)newCmd[i + 2]);
              } else if (token == T_READ) {  // Read a/d pin
                // 34 35 36 37 38 39 97 100
                // "  #  $  %  &  '  a  d
                // e.g. analogRead(35) = Ra# in the Serial Monitor
                //                     = [R,a,35] in the Python API
                printToAllPorts('=');
                pinMode((uint8_t)newCmd[i + 1], INPUT);
                if (newCmd[i] == TYPE_ANALOG)  // Arduino Uno: A2->16, A3->17
                  printToAllPorts(analogRead((uint8_t)newCmd[i + 1]));
                else if (newCmd[i] == TYPE_DIGITAL)
                  printToAllPorts(digitalRead((uint8_t)newCmd[i + 1]));
              }
            }
            if (nonHeadJointQ || lastToken != T_SKILL) {
              // printToAllPorts(token);
              transform(targetFrame, 1, transformSpeed);  // if (token == T_INDEXED_SEQUENTIAL_BIN) it will be useless
              skill->convertTargetToPosture(targetFrame);
            }
          }
          break;
        }
      case T_EXTENSION:
        {
          // PTH("cmdLen = ", cmdLen);
          if (newCmd[0] == '?')
            showModuleStatus();
          else if (newCmd[0] != 'U' || (newCmd[0] == 'U' && cmdLen == 1)) {  // when reading the distance from ultrasonic sensor, the cmdLen is 3.
            // and we don't want to change the activation status of the ultrasonic sensor behavior
            reconfigureTheActiveModule(newCmd);
          }

          // deal with the following command
          switch (newCmd[0]) {
          }
          break;
        }
      case T_LISTED_BIN:  // list of all 16 joint: angle0, angle2,... angle15 (binary encoding)
        {
          transform((int8_t *)newCmd, 1, transformSpeed);  // need to add angleDataRatio if the angles are large
          break;
        }
      case T_CPG:
        {
          // a: amplitude
          // t: loopStep
          // d: delay
          // p: phase
          // s: shift
// {20,-8,4,3,1,2,35,1,35,1},//small
// {36,-8,4,3,1,2,35,1,35,1},//large
// {16,-6,-4,3,1,3,21,51,31,1},//walk
// {20,-14,4,3,1,2,70,70,1,1},//bound
// {23,-10,8,3,3,1,70,70,1,1},//bound2
// {17,0,0,3,1,3,1,75,50,25},//heng
// {15,-4,-4,3,1,3,36,52,52,36},//heng2
// {18,0,0,3,1,2,35,46,35,46}//turnR
          updateCPG();
          break;
        }
      case T_CPG_BIN:
        {
          // Binary CPG parameters: 12 int8_t values + '~' terminator
          // Apply the parameters directly to CPG
          amplitude = (int8_t)newCmd[0];
          sideRatio = (int8_t)newCmd[1];
          stateSwitchAngle = (int8_t)newCmd[2];
          shift[0] = (int8_t)newCmd[3];
          shift[1] = (int8_t)newCmd[4];
          loopDelay = (int8_t)newCmd[5];
          skipStep[0] = (int8_t)newCmd[6];
          skipStep[1] = (int8_t)newCmd[7];
          phase[0] = (int8_t)newCmd[8];
          phase[1] = (int8_t)newCmd[9];
          phase[2] = (int8_t)newCmd[10];
          phase[3] = (int8_t)newCmd[11];

          if (cpg == NULL)
            cpg = new CPG(300, skipStep);
          else if (skipStep[0] != cpg->_skipStep[0] || skipStep[1] != cpg->_skipStep[1]) {
            delete cpg;
            cpg = new CPG(300, skipStep);
          }
          cpg->setPar(amplitude, sideRatio, stateSwitchAngle, loopDelay, shift, phase);
          cpg->printCPG();
          gyroBalanceQ = false;
          break;
        }
      case T_SIGNAL_GEN:  // resolution, speed, jointIdx, midpoint, amp, freq,phase
        {
          char *pch = strtok(newCmd, " ,");
          int inLen = 0;
          int8_t pars[60];  // allows 12 joints 5*12 = 60
          while (pch != NULL) {
            pars[inLen++] = atoi(pch);  //@@@ cast
            pch = strtok(NULL, " ,\t");
          }
          // for (int i = 0; i < inLen; i++)
          //   PTT(pars[i], ' ');
          // PTL();
          int8_t resolution = pars[0];
          int8_t speed = pars[1];
          signalGenerator(resolution, speed, pars + 2, inLen, 1);
          break;
        }
      case T_TEMP:
        {  // call the last skill data received from the serial port
          config.getBytes("tmp", newCmd, config.getBytesLength("tmp"));
          skill->buildSkill();
          skill->transformToSkill(skill->nearestFrame());
          printToAllPorts(token);
          token = T_SKILL;
          strcpy(newCmd, "tmp");
          break;
        }
      case T_SKILL_DATA:  // takes in the skill array from the serial port, load it as a regular skill object and run it
                          // locally without continuous communication with the master
        {
          int bufferLen = dataLen(newCmd[0]);
          config.putBytes("tmp", newCmd, bufferLen);
          skill->buildSkill();
          skill->transformToSkill(skill->nearestFrame());
          manualHeadQ = false;
          strcpy(newCmd, "tmp");
          break;
        }
      case T_SKILL:
        {
          // Parse skill command and arguments
          char skillName[CMD_LEN + 1];
          char *spacePos = strchr(newCmd, ' ');
          int timeOrAngle = 0;

          if (spacePos != NULL) {
            // Extract skill name (everything before the space)
            int skillNameLen = spacePos - newCmd;
            strncpy(skillName, newCmd, skillNameLen);
            skillName[skillNameLen] = '\0';

            // Extract argument (everything after the space)
            timeOrAngle = atoi(spacePos + 1);
          } else {
            // No arguments, use the full command as skill name
            // Use strncpy with bounds checking to prevent buffer overflow
            strncpy(skillName, newCmd, CMD_LEN);
            skillName[CMD_LEN] = '\0';  // Ensure null termination
          }

          if (!strcmp("x", skillName)        // x for random skill
              || strcmp(lastCmd, skillName)  // won't transform for the same gait.
              || skill->period <= 1) {    // skill->period can be NULL!
            // it's better to compare skill->skillName and newCmd.
            // but need more logics for non skill cmd in between
            if (!strcmp(skillName, "bk"))
              strcpy(skillName, "bkF");

            loadBySkillName(skillName);  // skillName will be overwritten as dutyAngles then recovered from skill->skillName

            skill->info();
            manualHeadQ = false;
          }
          break;
        }
      default:
        {
          printToAllPorts("Undefined token!");
          printToAllPorts(newCmd);
          break;
        }
    }

    if (lowerToken == T_SKILL && newCmd[0] != '\0') {// T_SKILL_DATA sets newCmd="tmp"; must update lastCmd so next "kup" triggers reload
      // if (skill->period > 0)
      // Use strncpy with bounds checking to prevent buffer overflow
      strncpy(lastCmd, newCmd, CMD_LEN);
      lastCmd[CMD_LEN] = '\0';  // Ensure null termination
    }

    if (token != T_SKILL || skill->period > 0) {  // it will change the token and affect strcpy(lastCmd, newCmd)
        printToAllPorts(token);                     // postures, gaits and other tokens can confirm completion by sending the token back
      if (lastToken == T_SKILL
          && (lowerToken == T_GYRO || lowerToken == T_INDEXED_SIMULTANEOUS_ASC || lowerToken == T_INDEXED_SEQUENTIAL_ASC
              || lowerToken == T_PAUSE || token == T_JOINTS || token == T_RANDOM_MIND || token == T_BALANCE_SLOPE
              || token == T_ACCELERATE || token == T_DECELERATE || token == T_TILT))
        token = T_SKILL;
    }
    resetCmd();
  }

  if (tolower(token) == T_SKILL) {
    skill->perform();
    if (skill->period > 1) {
      delay(delayShort
            + max(0, int(runDelay)));
    }
  //   if (skill->period < 0) {
  //     if (!strcmp(skill->skillName, "fd")) {  // need to optimize logic to combine "rest" and "fold"
  //       shutServos();
  //       gyroBalanceQ = false;
  //       printToAllPorts('g');
  //       idleTimer = 0;
  //       token = '\0';
  //     } else {
  //       if (interruptedDuringBehavior) {
  //         loadBySkillName("up");
  //       } else
  //         skill->convertTargetToPosture(currentAng);
  //     }
  //     for (int i = 0; i < DOF; i++)
  //       currentAdjust[i] = 0;
  //     printToAllPorts(token);  // behavior can confirm completion by sending the token back
  //   }
  }

  {
    delay(1);  // avoid triggering WDT
  }
}
