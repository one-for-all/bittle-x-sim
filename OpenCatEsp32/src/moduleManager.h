int8_t indexOfModule(char moduleName) {
  for (byte i = 0; i < sizeof(moduleList) / sizeof(char); i++)
    if (moduleName == moduleList[i])
      return i;
  return -1;
}
bool moduleActivatedQfunction(char moduleCode) {
  return moduleActivatedQ[indexOfModule(moduleCode)];
}

int8_t activeModuleIdx() {  // designed to work if only one active module is allowed
  for (byte i = 0; i < sizeof(moduleList) / sizeof(char); i++)
    if (moduleActivatedQ[i])
      return i;
  return -1;
}

void initModule(char moduleCode) {
  bool successQ = true;
  int8_t index = indexOfModule(moduleCode);
  switch (moduleCode) {
    case EXTENSION_GROVE_SERIAL:
      {
        PTLF("Start Serial2");
        Serial2.begin(115200, SERIAL_8N1, 9, 10);
        Serial2.setTimeout(SERIAL_TIMEOUT);
        break;
      }
  }
  moduleActivatedQ[index] = successQ;
}

void stopModule(char moduleCode) {
  switch (moduleCode) {
    case EXTENSION_GROVE_SERIAL:
      {
        Serial2.end();
        PTL("Stop Serial 2");
        break;
      }
  }
}
void showModuleStatus() {
  byte moduleCount = sizeof(moduleList) / sizeof(char);
  printListWithoutString((char *)moduleList, moduleCount);
  printListWithoutString(moduleActivatedQ, moduleCount);
  moduleDemoQ = (moduleActivatedQfunction(EXTENSION_DOUBLE_LIGHT)
                 || moduleActivatedQfunction(EXTENSION_DOUBLE_TOUCH)
                 || moduleActivatedQfunction(EXTENSION_GESTURE)
                 || moduleActivatedQfunction(EXTENSION_DOUBLE_IR_DISTANCE)
                 || moduleActivatedQfunction(EXTENSION_CAMERA)
                 || moduleActivatedQfunction(EXTENSION_PIR)
                 // || moduleActivatedQfunction(EXTENSION_BACKTOUCH)
                 // || moduleActivatedQfunction(EXTENSION_ULTRASONIC)
                 || moduleActivatedQfunction(EXTENSION_QUICK_DEMO));
}

void reconfigureTheActiveModule(char *moduleCode) {
  if (moduleCode[0] == '?') {
    showModuleStatus();
    return;
  }
  bool statusChangedQ = false;

  // Determine target module and operation type based on the elegant original design
  char targetModule = moduleCode[0];
  bool isCloseOnlyOperation = false;

  // Handle 'X' or 'X~' -> close all
  if (strlen(moduleCode) == 0 || (strlen(moduleCode) >= 1 && (moduleCode[0] == '~' || moduleCode[0] == '\n'))) {
    targetModule = '-'; // Use '-' to represent close all, avoiding confusion
  }
  // Handle 'Xc' -> close specific module (lowercase)
  else if (strlen(moduleCode) >= 1 && islower(moduleCode[0])) {
    targetModule = toupper(moduleCode[0]); // Convert to uppercase for module identification
    isCloseOnlyOperation = true; // Only close this specific module, don't enable anything
  }
  // Handle 'XC' -> enable specific module (uppercase) - use original logic
  for (byte i = 0; i < sizeof(moduleList) / sizeof(char); i++) {                                               // disable unneeded modules
    if (!moduleActivatedQ[i]) continue;

    // For lowercase: only process target module; For original logic: only process non-target modules
    if ((isCloseOnlyOperation && moduleList[i] != targetModule) ||
        (!isCloseOnlyOperation && moduleList[i] == targetModule)) continue;

    // Original logic: protect voice and backtouch unless closing all
    if (!isCloseOnlyOperation &&
        (moduleList[i] == EXTENSION_VOICE || moduleList[i] == EXTENSION_BACKTOUCH) &&
        targetModule != '-') continue;

    // Unified disable logic
    PTHL("- disable", moduleNames[i]);
    stopModule(moduleList[i]);
    moduleActivatedQ[i] = false;
    statusChangedQ = true;
  }

  // Original logic: enable target module (skip for close-only operations)
  if (!isCloseOnlyOperation) {
    for (byte i = 0; i < sizeof(moduleList) / sizeof(char); i++) {
      if (moduleList[i] == targetModule && !moduleActivatedQ[i]) {
        PTHL("+  enable", moduleNames[i]);
        moduleActivatedQ[i] = true;
        statusChangedQ = true;
      }
    }
  }

  if (statusChangedQ){  // if the status of the modules has changed, show the new status
#ifndef I2C_EEPROM_ADDRESS
    config.putBytes("moduleState", moduleActivatedQ, sizeof(moduleList) / sizeof(char));
#endif
    showModuleStatus();
    for (byte i = 0; i < sizeof(moduleList) / sizeof(char); i++) {
      if (moduleList[i] == targetModule && moduleActivatedQ[i]) {
        initModule(moduleList[i]);
      }
    }
  }
}

void initModuleManager() {
  byte moduleCount = sizeof(moduleList) / sizeof(char);
  PTHL("Module count: ", moduleCount);
  for (byte i = 0; i < moduleCount; i++) {
    if (moduleActivatedQ[i]) {
      initModule(moduleList[i]);
    }
  }
  showModuleStatus();
}

void read_serial() {
  Stream *serialPort = NULL;
  // String source;
  if (moduleActivatedQ[0] && Serial2.available()) {
    serialPort = &Serial2;
  } else if (Serial.available()) {
    serialPort = &Serial;
    // source = "SER";
  }
  if (serialPort) {
    token = serialPort->read();
    lowerToken = tolower(token);
    newCmdIdx = 2;
    delay(1); // leave enough time for serial read
    terminator = (token >= 'A' && token <= 'Z') ? '~' : '\n';  // capitalized tokens use binary encoding for long data commands
                                                               //'~' ASCII code = 126; may introduce bug when the angle is 126 so only use angles <= 125
    serialTimeout = (token == T_SKILL_DATA || lowerToken == T_BEEP) ? SERIAL_TIMEOUT_LONG : SERIAL_TIMEOUT;
    lastSerialTime = millis();
    do {
      if (serialPort->available()) {
        do {
          if (((token == T_SKILL || lowerToken == T_INDEXED_SIMULTANEOUS_ASC || lowerToken == T_INDEXED_SEQUENTIAL_ASC) && cmdLen >= spaceAfterStoringData) || cmdLen > BUFF_LEN) {
            PTH("Cmd Length: ", cmdLen);
            PTF("OVF");
            do {
              serialPort->read();
            } while (serialPort->available());
            printToAllPorts(token);
            token = T_SKILL;
            strcpy(newCmd, "up");
            cmdLen = 2;
            return;
          }
          newCmd[cmdLen++] = serialPort->read();
          // PTHL(newCmd[cmdLen - 1], int8_t(newCmd[cmdLen - 1]));
        } while (serialPort->available());
        lastSerialTime = millis();
      }
    } while ((cmdLen == 0                                                                               // wait for at least 1 byte when cmdLen==0;
      || newCmd[cmdLen - 1] != terminator) && long(millis() - lastSerialTime) < serialTimeout);// the lower case tokens are encoded in ASCII and can be entered in Arduino IDE's serial monitor
                                                                                               // if the terminator of the command is set to "no line ending" or "new line", parsing can be different
                                                                                               // so it needs a timeout for the no line ending case
    // PTH("* " + source, long(millis() - lastSerialTime));
    if (!(token >= 'A' && token <= 'Z') || token == 'X' || token == 'R' || token == 'W') {  // serial monitor is used to send lower cased tokens by users
                                                                                            // delete the unexpected '\r' '\n' if the serial monitor sends line ending symbols
      leftTrimSpaces(newCmd, &cmdLen); // allow space between token and parameters, such as "k sit"
      for (int i = cmdLen - 1; i >= 0; i--) { // delete the '/r' and '/n' if the serial monitor is configured to send terminators
        if ((newCmd[i] == '\n') || (newCmd[i] == '\r')) {
          newCmd[i] = '\0';
          cmdLen--;
        }
      }
    }
    if (cmdLen > 0)
      cmdLen = (newCmd[cmdLen - 1] == terminator) ? cmdLen - 1 : cmdLen;  // delete the terminator if it exists
    newCmd[cmdLen] = (token >= 'A' && token <= 'Z') ? '~' : '\0';
    if (token >= 'A' && token <= 'Z')
      newCmd[cmdLen + 1] = '\0';
    newCmdIdx = 2;
  }
}

void readSignal() {
  moduleIndex = activeModuleIdx();
  read_serial();  //  newCmdIdx = 2

  long current = millis();
  if (newCmdIdx)
    idleTimer = millis() + IDLE_TIME;
  else if (token != T_SERVO_CALIBRATE && token != T_SERVO_FOLLOW && token != T_SERVO_FEEDBACK && current - idleTimer > 0) {
    if (moduleIndex == -1)  // no active module
      return;
    // powerSaver -> 4
    // other -> 5
    // randomMind -> 100
  }
}

// — read human sensors (top level) —
void readHuman() {
}
// — generate behavior by fusing all sensors and instruction
String decision() {
  return "";
}

void read_sound() {
}

void read_GPS() {
}

void readEnvironment() {
  read_sound();
  read_GPS();
}
