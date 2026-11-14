#include "esp32-hal.h"
#include <stdio.h>

void dealWithExceptions() {
#ifdef GYRO_PIN

  // Handle turning exception regardless of gyroBalanceQ status
  if (imuException == IMU_EXCEPTION_TURNING) {
    PTL("EXCEPTION: turning target reached");
    // Stop the robot and make it stand up
    tQueue->addTask('k', "up");
    needTurning = false;  // Reset flag after creating task
    PTL("endTurn");
    prev_imuException = imuException;
    print6Axis();
  }
  
  // Handle other IMU exceptions only when gyroBalanceQ is true
  if (gyroBalanceQ) {
    // if (imuException == IMU_EXCEPTION_FLIPPED || (skill->period == 1 && fabs(xyzReal[2]) >= 15) || (skill->period > 1
    // && abs(xyzReal[2]) >= 20)) {
    //   delay(50);
    // }
    if (imuException && imuException != IMU_EXCEPTION_TURNING) {  // the gyro reaction switch can be toggled on/off by the 'g' token
      switch (imuException) {
        case IMU_EXCEPTION_LIFTED:
          {
            if (skill->period == 1 && prev_imuException != IMU_EXCEPTION_LIFTED) {
              if (ypr[1] < -50)
                strcpy(newCmd, "lifted");
              else
                strcpy(newCmd, "dropped");
              loadBySkillName(newCmd);
              token = 'k';
            }
            break;
          }
        case IMU_EXCEPTION_FREEFALL:
          {
            PTL("EXCEPTION free fall");
            strcpy(newCmd, "lnd");
            loadBySkillName(newCmd);
            shutServos();  // does not shut the P1S servo.while token p in serial monitor can.? ? ?
            delay(1000);
            token = 'k';
            strcpy(newCmd, "up");
            newCmdIdx = -1;
            break;
          }
        case IMU_EXCEPTION_FLIPPED:
          {
            PTL("EXCEPTION: Fall over");
            //  for (int m = 0; m < 2; m++)
            //    meow(30 - m * 12, 42 - m * 12, 20);
            token = 'k';
            manualHeadQ = false;
            strcpy(newCmd, "rc");
            newCmdIdx = -2;
            // tQueue->addTaskToFront('k', "rc");
            break;
          }
        case IMU_EXCEPTION_KNOCKED:
          {
            if (tQueue->cleared() && skill->period == 1) {
              PTL("EXCEPTION: Knocked");
              tQueue->addTask('k', "knock");
#if defined NYBBLE && defined ULTRASONIC
              if (!moduleActivatedQ[0]) {  // serial2 may be used to connect serial2 rather than the RGB ultraconic sensor
                int8_t clrRed[] = { 125, 0, 0, 0, 0, 126 };
                int8_t clrBlue[] = { 0, 0, 125, 0, 0, 126 };
                tQueue->addTask('C', clrRed, 1);
                tQueue->addTask('C', clrBlue);
              }
#endif
              tQueue->addTask('k', "up");
            }
            break;
          }
        case IMU_EXCEPTION_PUSHED:
          {
            PTL("EXCEPTION: Pushed");
            // Acceleration Real
            //      ^ head
            //        ^ x+
            //        |
            //  y+ <------ y-
            //        |
            //        | x-
            if (skill->period == 1 && strncmp(lastCmd, "vtF", 2)) {
              char xSymbol[] = { '^', 'v' };
              char ySymbol[] = { '<', '>' };
              char xDirection = xSymbol[sign(ARX) > 0];
              char yDirection = ySymbol[sign(ARY) > 0];
              float forceAngle = atan(float(fabs(ARX)) / ARY) * degPerRad;
              PT(fabs(ARX) > fabs(ARY) ? xDirection : yDirection);
              PTHL(" ForceAngle:", forceAngle);
              if (tQueue->cleared()) {
                if (xDirection == '^') {
                  // tQueue->addTask('i', yDirection == '<' ? "0 -75" : "0 75");
                  if (fabs(forceAngle) < 60)
                    // tQueue->addTask('i', yDirection == '<' ? "0 45" : "0 -45");
                    tQueue->addTask('k', yDirection == '<' ? "wkL" : "wkR", 700);
                  // tQueue->addTask('i', "");
                  else {
                    tQueue->addTask('k', "wkF", 700);
                    // tQueue->addTask('i', "");
                    tQueue->addTask('k', "bkF", 500);
                  }
                } else {
                  // tQueue->addTask('k', yDirection == '<' ? "bkR" : "bkL", 1000);
                  if (fabs(forceAngle) < 60)
                    tQueue->addTask('k', yDirection == '<' ? "wkR" : "wkL", 700);
                  else {
                    tQueue->addTask('k', "bkF", 500);
                    tQueue->addTask('k', "wkF", 700);
                  }
                }
              }
              tQueue->addTask('k', "up");
              delayPrevious = runDelay;
              runDelay = delayException;
              PTL();
            }
            break;
          }
        case IMU_EXCEPTION_OFFDIRECTION:
          {
            PTL("EXCEPTION: off direction");
            // char *currentGait = skill->skillName;  // it may not be gait
            // char gaitDirection = currentGait[strlen(currentGait) - 1];
            float yawDiff = int(ypr[0] - previous_ypr[0]) % 180;
            if (tQueue->cleared()) {
              if (skill->period <= 1 || !strcmp(skill->skillName, "vtF")) {  // not gait or stepping
                tQueue->addTask('k', yawDiff > 0 ? "vtR" : "vtL", round(fabs(yawDiff) * 15));
                // tQueue->addTask('k', "up", 100);
                delayPrevious = runDelay;
                runDelay = delayException;
              } else {
                // if (gaitDirection == 'L' || gaitDirection == 'R')   //turning gait
                previous_ypr[0] = ypr[0];
              }
              // else {
              //   if (gaitDirection == 'L' || gaitDirection == 'R') {  //turning gait
              //     previous_ypr[0] = ypr[0];
              //   } else {
              //     currentGait[strlen(currentGait) - 1] = yawDiff > 0 ? 'R' : 'L';
              //     PTL(currentGait);
              //     tQueue->addTask('k', currentGait, round(fabs(yawDiff) * 15));
              //     currentGait[strlen(currentGait) - 1] = 'F';
              //     PTL(currentGait);
              //     tQueue->addTask('k', currentGait);
              //   }
              // }
            }
            break;
          }
        default:
          {
            break;
          }
      }
      // if (imuException != IMU_EXCEPTION_PUSHED)
      prev_imuException = imuException;
      print6Axis();
      // readIMU();  // flush the IMU to avoid static readings and infinite loop

      // if (tQueue->lastTask == NULL) {
      //   if (strcmp(lastCmd, "") && strcmp(lastCmd, "lnd") && *strGet(newCmd, -1) != 'L' && *strGet(lastCmd, -1) !=
      //   'R') {
      //     PTH("save last task ", lastCmd);
      //     tQueue->lastTask = new Task('k', lastCmd);
      //   }
      // }
    } else {
#ifndef ROBOT_ARM
      if (tQueue->cleared() && prev_imuException == IMU_EXCEPTION_LIFTED) {
        // strcpy(newCmd, "dropRec");
        // loadBySkillName(newCmd);
        // token = 'k';
        tQueue->addTask('k', "dropRec", 500);
      }
#endif
      prev_imuException = imuException;
    }
  }
  if (tQueue->cleared() && runDelay <= delayException)
    runDelay = delayMid;
#endif
}

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
      case T_GYRO:
        {
#ifdef GYRO_PIN
      if (token == T_GYRO)
      {
        if (cmdLen == 0)
        {
          gyroBalanceQ = !gyroBalanceQ;
          token = gyroBalanceQ ? 'G' : 'g'; // G for activated gyro
          resetAdjust();
        }
        else
        {
          if (newCmd[0] == C_GYRO_CALIBRATE)
          {
            shutServos();
            PTLF("Setting updateGyroQ to false...");
            updateGyroQ = false;
            if (newCmd[1] != C1_GYRO_CALIBRATE_IMMEDIATELY)
            {
              PTLF("\nPut the robot FLAT on the table and don't touch it during calibration.");
            }
            
            // Wait for IMU task to terminate completely
            if (TASK_imu != NULL) {
              PTLF("Waiting for IMU task to terminate before calibration...");
              // Wait for IMU task to terminate
              while (eTaskGetState(TASK_imu) != eDeleted) {
                  if(eTaskGetState(TASK_imu)== eReady){
                      TASK_imu = NULL;
                      break;
                  }
                vTaskDelay(100 / portTICK_PERIOD_MS);
              }
            }
            
            // Create calibration task to run on core 0
            xTaskCreatePinnedToCore(
                taskCalibrateImuUsingCore0,         // Task function
                "taskCalibrateImuUsingCore0",       // Task name
                1800,                               // Task stack size (restored to original value)
                NULL,                               // Task parameters
                1,                                  // Task priority
                &taskCalibrateImuUsingCore0_handle, // Task handle
                0                                   // Task core number to run on
            );
            
            // Wait for updateGyroQ to be re-set to true, indicating calibration completion
            unsigned long waitStart = millis();
            const unsigned long maxWaitTime = 10000; // Maximum waiting time 10 seconds
            
            while (!updateGyroQ) {
              if (millis() - waitStart > maxWaitTime) {
                PTLF("Timeout waiting for calibration to complete. Forcing continuation.");
                updateGyroQ = true; // Force continuation
                break;
              }
              vTaskDelay(100 / portTICK_PERIOD_MS);
            }
            
            PTLF("Calibration completed, allowing IMU to stabilize...");
            delay(3000); // Allow IMU to stabilize after calibration
            createIMUTask();
          }
          else
          {
            byte i = 0;
            while (newCmd[i] != '\0')
            {
              if (toupper(newCmd[i]) == C_GYRO_FINENESS)
              {                                               // if newCmd[i] is 'f' or 'F'
                fineAdjustQ = (newCmd[i] == C_GYRO_FINENESS); // if newCmd[i] == T_GYRO_FINENESS, fineAdjustQ is true. else newCmd[i] == C_GYRO_FINENESS_OFF, fineAdjustQ is false.
                token = fineAdjustQ ? 'G' : 'g';              // G for activated gyro
              }
              else if (toupper(newCmd[i]) == C_GYRO_BALANCE){  // if newCmd[i] is 'b' or 'B'
                gyroBalanceQ = (newCmd[i] == C_GYRO_BALANCE); // if newCmd[i] == T_GYRO_FINENESS, gyroBalanceQ is true. else is false.
                if (!gyroBalanceQ) {
                  gyroBalanceQ = false;
                  printToAllPorts('g');
                  resetAdjust();
                }
                else if (!updateGyroQ) {
                  updateGyroQ = true;
                  createIMUTask();
                }
              }
              else if (toupper(newCmd[i]) == C_GYRO_UPDATE) { // if newCmd[i] is 'u' or 'U' - Update gyro reading
                bool newState = (newCmd[i] == C_GYRO_UPDATE); // if newCmd[i] == 'U', updateGyroQ is true. else 'u', updateGyroQ is false.
                if (!newState) {
                  updateGyroQ = false;
                  gyroBalanceQ = false;
                  printToAllPorts('g');
                  resetAdjust();
                }
                else if (newState && !updateGyroQ) {
                  updateGyroQ = true;
                  gyroBalanceQ = true;
                  createIMUTask();
                }
              }
              else if (toupper(newCmd[i]) == C_PRINT)
              {                                      // if newCmd[i] is 'p' or 'P'
                printGyroQ = (newCmd[i] == C_PRINT); // if newCmd[i] == T_GYRO_PRINT, always print gyro. else only print once
                print6Axis();
              }
              else if (newCmd[i] == '?')
              {
                PTF("Gyro state:");
                PTT(" Update-", updateGyroQ);
                PTT(" Balance-", gyroBalanceQ);
                PTT(" Print-", printGyroQ);
                PTTL(" Frequency-", fineAdjustQ);
              }
              i++;
            }
            imuSkip = fineAdjustQ ? IMU_SKIP : IMU_SKIP_MORE;
            runDelay = fineAdjustQ ? delayMid : delayShort;
          }
        }
      }
#endif
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
      case T_INDEXED_SEQUENTIAL_ASC:    // move multiple indexed joints to angles once at a time (ASCII format entered in
                                        // the serial monitor)
      case T_INDEXED_SIMULTANEOUS_ASC:  // move multiple indexed joints to angles simultaneously (ASCII format entered
                                        // in the serial monitor)
      case T_TILT:  // tilt the robot, format: t axis angle. 0:yaw, 1:pitch, 2:roll
      case T_BALANCE_SLOPE:
        {
          if (token == T_INDEXED_SIMULTANEOUS_ASC && cmdLen == 0)
            manualHeadQ = false;
          else {
            int targetFrame[DOF + 1];
            // arrayNCPY(targetFrame, currentAng, DOF);
            for (int i = 0; i < DOF; i++) {
              targetFrame[i] = currentAng[i] - (gyroBalanceQ ? currentAdjust[i] : 0);
            }
            targetFrame[DOF] = '~';

            char *cmdForParsing = new char[cmdLen + 1];
            // Use strncpy with bounds checking to prevent buffer overflow
            strncpy(cmdForParsing, newCmd, cmdLen);
            cmdForParsing[cmdLen] = '\0';  // Ensure null termination
            if (token == T_SERVO_CALIBRATE && lastToken != T_SERVO_CALIBRATE) {
              #ifdef T_SERVO_MICROSECOND
                                setServoP(P_HARD);
                                workingStiffness = false;
              #endif
              #ifdef VOICE
                                if (newCmdIdx == 2) {  // only deactivate the voice module via serial port

                                  char setCmd[] = "Ad~";  // turn off voice
                                  set_voice(setCmd);
                                }
              #endif
                                strcpy(newCmd, "calib");//it will override the newCmd, so we need to backup it with originalCmd
                                loadBySkillName(newCmd);
                              }
            char *pch;
            pch = strtok(cmdForParsing, " ,");
            nonHeadJointQ = false;
            do {  // it supports combining multiple commands at one time
              // for example: "m8 40 m8 -35 m 0 50" can be written as "m8 40 8 -35 0 50"
              // the combined commands should be less than four. string len <=30 to be exact.
              int target[2] = {};
              int inLen = 0;
              for (byte b = 0; b < 2 && pch != NULL; b++) {
                target[b] = atoi(pch);  //@@@ cast
                pch = strtok(NULL, " ,\t");
                inLen++;
              }
              // PTHL( target[0],target[1]);
              if ((token == T_INDEXED_SEQUENTIAL_ASC || token == T_INDEXED_SIMULTANEOUS_ASC) && target[0] >= 0
                  && target[0] < DOF) {
                targetFrame[target[0]] = target[1];
                if (target[0] < 4) {
                  targetHead[target[0]] = target[1];
                  manualHeadQ = true;
                } else
                  nonHeadJointQ = true;
              }
              if (token == T_INDEXED_SEQUENTIAL_ASC) {
                transform(targetFrame, 1, 1);
                delay(10);
              } else if (token == T_BALANCE_SLOPE) {
                if (inLen == 2) {
                  balanceSlope[0] = max(-2, min(2, target[0]));
                  balanceSlope[1] = max(-2, min(2, target[1]));
                }
              }
              // delay(5);
            } while (pch != NULL);

            // Release dynamically allocated memory
            delete[] cmdForParsing;

            // For calibration commands, print calibration values after the loop
            if (token == T_SERVO_CALIBRATE) {
              printToAllPorts(range2String(DOF));
              printToAllPorts(list2String(servoCalib));
            }
            if ((token == T_INDEXED_SEQUENTIAL_ASC || token == T_INDEXED_SIMULTANEOUS_ASC)
                && (nonHeadJointQ || lastToken != T_SKILL)) {
              // printToAllPorts(token);
              transform(targetFrame, 1, transformSpeed);  // if (token == T_INDEXED_SEQUENTIAL_ASC) it will be useless
              skill->convertTargetToPosture(targetFrame);
            }
            delete[] pch;
          }
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
      case T_LISTED_BIN:  // list of all 16 joint: angle0, angle2,... angle15 (binary encoding)
        {
          transform((int8_t *)newCmd, 1, transformSpeed);  // need to add angleDataRatio if the angles are large
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
            + max(0, int(runDelay
#ifdef GYRO_PIN
                         - gyroBalanceQ * (max(fabs(ypr[1]) / 2, fabs(ypr[2])) / 20)  // accelerate gait when tilted
                             / (!fineAdjustQ && !mpuQ ? 4 : 1)                        // reduce the adjust if not using mpu6050
#endif
                         )));
    }
    if (skill->period < 0) {
      if (!strcmp(skill->skillName, "fd")) {  // need to optimize logic to combine "rest" and "fold"
        shutServos();
        gyroBalanceQ = false;
        printToAllPorts('g');
        idleTimer = 0;
        token = '\0';
      } else {
        if (interruptedDuringBehavior) {
          loadBySkillName("up");
        } else
          skill->convertTargetToPosture(currentAng);
      }
      for (int i = 0; i < DOF; i++)
        currentAdjust[i] = 0;
      printToAllPorts(token);  // behavior can confirm completion by sending the token back
#ifdef GYRO_PIN
      if (xyzReal[2] > 0 && (fabs(ypr[1]) > 45 || fabs(ypr[2]) > 45)) {  // wait for imu to update
        while (fabs(ypr[1]) > 10 || fabs(ypr[2]) > 10) {
          // print6Axis();
          delay(IMU_PERIOD);
        }
      }
#endif
    }
  }

  {
    delay(1);  // avoid triggering WDT
  }
}
