/* BiBoard V0
    PWM:
                        |--------------------------------
                        |    PWM[0]           PWM[6]    |
                        |    PWM[1]           PWM[7]    |
                        |    PWM[2]           PWM[8]    |
                        |-----------                    |
                        |           |                   |
                        |   ESP32   |  IMU         USB-C|~~~~Tail~~~~
                        |           |                   |
                        |-----------                    |
                        |    PWM[3]           PWM[9]    |
                        |    PWM[4]           PWM[10]   |
                        |    PWM[5]           PWM[11]   |
                        |-------------------------------|

    Pin Name    |   ESP32 Pin   |   Arduino Pin Name    |   Alternative Function
    PWM[0]          GPIO4               4                   GPIO / Ain / Touch
    PWM[1]          GPIO5               5                   GPIO / VSPI SS
    PWM[2]          GPIO18              18                  GPIO / VSPI SCK
    -----------------------------------------------------------------------------
    PWM[3]          GPIO32              32                  GPIO / Ain / Touch
    PWM[4]          GPIO33              33                  GPIO / Ain / Touch
    PWM[5]          GPIO19              19                  GPIO / VSPI MISO
    -----------------------------------------------------------------------------
    PWM[6]          GPIO2               2                   boot pin, DO NOT PUT HIGH WHEN BOOT!
    PWM[7]          GPIO15              15                  GPIO / HSPI SS / Ain Touch
    PWM[8]          GPIO13              13                  built-in LED / GPIO / HSPI MOSI / Ain / Touch
    -----------------------------------------------------------------------------
    PWM[9]          GPIO12              12                  GPIO / HSPI MISO / Ain / Touch
    PWM[10]         GPIO14              14                  GPIO / HSPI SCK / Ain / Touch
    PWM[11]         GPIO27              27                  GPIO / Ain / Touch

    I2C:

    Pin Name    |   ESP32 Pin   |   Arduino Pin Name    |   Alternative Function
    I2C-SCL         GPIO22              22                  Fixed - ICM20600 - Pulled
    I2C-SDA         GPIO21              21                  Fixed = ICM20600 - Pulled

    System default, nothing to declaration!

    Other Peripherals:

    Pin Name    |   ESP32 Pin   |   Arduino Pin Name    |   Alternative Function
    IR_Remote       GPIO23              23                  Fixed - VS1838B IR
    DAC_Out         GPIO25              25                  Fixed - PAM8302
    IMU_Int         GPIO26              26                  Fixed - MPU6050 Interrupt

    System default, nothing to declare!
*/

/* BiBoard2
  IMU_Int     27
  BUZZER      14
  VOLTAGE     4
  RGB LED     15
  GREEN-LED   5
*/

/*  Total DOF            Walking DOF
                   Nybble    Bittle    Cub
   BiBoard  (12)  skip 0~4  skip 0~4    12
   BiBoard2 (16)  skip 0~8  skip 0~8  skip0~4
*/

// #define RevB
#define RevDE
#define SERIAL_TIMEOUT 10  // 5 may cut off the message
#define SERIAL_TIMEOUT_LONG 200
#define BOARD "B10"

#define DATE "260207"  // YYMMDD
String SoftwareVersion = "";
String uniqueName = "";

// I2C EEPROM configuration - global setting for all boards
// Uncomment the following line to enable I2C EEPROM storage instead of ESP32 Flash
// #define I2C_EEPROM_ADDRESS 0x54  // Address of i2c eeprom chip

#define BIRTHMARK '@'  // Send '!' token to reset the birthmark in the EEPROM so that the robot will know to restart and reset
// #define BT_BLE  // toggle Bluetooth Low Energy (BLE）
// #define BT_SSP  // toggle Bluetooth Secure Simple Pairing (BT_SSP)
// #define WIFI_MANAGER  // toggle WiFi Manager. It should be always off for now
// #define WEB_SERVER // toggle web server
// #define SHOW_FPS // toggle FPS display
#ifndef VT
// #define GYRO_PIN  // toggle the Inertia Measurement Unit (IMU), i.e. the gyroscope
#endif
#define SERVO_FREQ 240


// Tutorial: https://bittle.petoi.com/11-tutorial-on-creating-new-skills
#ifdef BITTLE
#define MODEL "Bittle X"
#include "InstinctBittleESP.h"
#define REGULAR P1L
#define KNEE P1L

#define HEAD
#define TAIL  // the robot arm's clip is assigned to the tail joint
#define LL_LEG
#endif

#ifdef BiBoard_V1_0
#define ESP_PWM
#define PWM_NUM 12
// #define INTERRUPT_PIN 26  // use pin 2 on Arduino Uno & most boards
#define BUZZER 2
#define BT_CLIENT  // toggle Bluetooth client (BLE） for Micro:Bit
// #define IR_PIN 23

#define LOW_VOLTAGE 7.0  // for 2S 7.4V power
#define NO_BATTERY_VOLTAGE 6.8
#define LOW_VOLTAGE2 5.0  // for 6V power
#define NO_BATTERY_VOLTAGE2 4.8

#ifdef RevDE
// #define VOLTAGE 37  // rev D
#define ANALOG2 35  // rev D
#endif

#define ANALOG1 34
#define ANALOG3 36
#define ANALOG4 39
// #define BACKTOUCH_PIN 38
#define VOICE_RX 26
#define VOICE_TX 25
#define UART_RX2 9
#define UART_TX2 10
#define SERIAL_VOICE Serial1
#define IMU_MPU6050
#define IMU_ICM42670

// #define PWM_LED_PIN 27

// L:Left-R:Right-F:Front-B:Back---LF, RF, RB, LB
const uint8_t PWM_pin[PWM_NUM] = {
    18, 14, 5,  27,  // head or shoulder roll
    23, 4,  12, 33,  // shoulder pitch
    19, 15, 13, 32  // knee
};
#endif

#define MAX_READING 4096.0  // to compensate the different voltage level of boards
#define BASE_RANGE 1024.0
double rate = 1.0 * MAX_READING / BASE_RANGE;

#define DOF 16
#define WALKING_DOF 8
#define GAIT_ARRAY_DOF 8

enum ServoModel_t {
  G41 = 0,
  P1S,
  P1L,
  P2K,
  P50
};

ServoModel_t servoModelList[] = {
  REGULAR, REGULAR, REGULAR, REGULAR,
  REGULAR, REGULAR, REGULAR, REGULAR,
  REGULAR, REGULAR, REGULAR, REGULAR,
  KNEE,    KNEE,    KNEE,    KNEE
};

bool newBoard = false;
#include <math.h>
/*  Token (T_) and Character (C_) Command list.
      Character Commands modify the behavior of the Token that they are associated with in reaction().
      Tokens are stored in the global "token" char variable and Character Commands are stored in the global char array
   newCmd[0].

      Directive macros that #define Character Commands have the following NAME format (all uppercase letters):
        C{char-array-position}_{association-name}_{character-command-name}
            where:
              {char-array-position} = 0, 1, 2, ...  indicates the location of the Character Command in the char array
   newCmd[0], and therefore indicates where the Character Command must be placed after the token. Note:  "C_" is the
   same as "C0_". {association-name} =  the Token name or the Token behavior name that the Character Command is
   associated with. This allows some Character Commands to be associated with multiple tokens Examples include: C_PRINT,
   C_FOLLOW series. {character-command-name} = unique name for the character command.

      This adds a Character Command sequencing paradigm to the token system where the ability to append multiple
   Character Commands after a Token provides additional flexibility.

    Change Log:
      Renamed T_CALIBRATE to T_SERVO_CALIBRATE        to make its behavior more obvious now that we are calibrating the
   IMU.

*/
#define T_ABORT 'a'            // abort the calibration values
#define T_BEEP \
  'b'  // b note1 duration1 note2 duration2 ... e.g. b12 8 14 8 16 8 17 8 19 4. bVolume will change the volume of the
       // sound, in scale of 0~10. 0 will mute all sound effect. e.g. b3. a single 'b' will toggle all sound on/off
#define T_BEEP_BIN \
  'B'  // B note1 duration1 note2 duration2 ... e.g. B12 8 14 8 16 8 17 8 19 4. a single 'B' will toggle all sound
       // on/off
#define T_SERVO_CALIBRATE \
  'c'  // send the robot to calibration posture for attaching legs and fine-tuning the joint offsets. c jointIndex1
       // offset1 jointIndex2 offset2 ... e.g. c0 7 1 -4 2 3 8 5
#define T_COLOR \
  'C'  // change the eye colors of the RGB ultrasonic sensor. a single 'C' will cancel the manual eye colors
#define T_REST 'd'  // set the robot to rest posture and shut down all the servos. "d index" can turn off a single servo
#define T_SERVO_FEEDBACK \
  'f'  // return the servo's position info if the chip supports feedback. e.g. f8 returns the 8th joint's position. A
       // single 'f' returns all the joints' position
#define C_FOLLOW 'F'
#define C_FOLLOW_OFF 'f'
#define C_LEARN 'l'  // Should be named C_SERVO_FEEDBACK_LEARN since it is only associated with T_SERVO_FEEDBACK?
#define C_REPLAY 'r'  // Should be named C_SERVO_FEEDBACK_REPLAY since it is only associated with T_SERVO_FEEDBACK?
#define T_SERVO_FOLLOW 'F'  // make the other legs follow the moved legs

#define T_GYRO 'g'  // gyro-related commands. by itself, is a toggle to turn on or off the gyro function
// These Character (C_) commands apply to the T_GYRO Token
#define C_GYRO_UPDATE 'U'                 // update the gyro data
#define C_GYRO_UPDATE_OFF 'u'             // stop updating the gyro data
#define C_GYRO_FINENESS 'F'               // increase the frequency of gyroscope sampling
#define C_GYRO_FINENESS_OFF 'f'           // reduce the frequency of gyroscope sampling to accelerate motion
#define C_GYRO_BALANCE 'B'                // turn on the gyro balancing
#define C_GYRO_BALANCE_OFF 'b'            // turn off the gyro balancing
#define C_GYRO_CALIBRATE 'c'              // calibrate the IMU. enter "gc"
#define C1_GYRO_CALIBRATE_IMMEDIATELY 'i' // calibrate the IMU immediately. enter "gci"

// These Character (C_) commands apply to various tokens that have a print capability (e.g. T_GYRO, T_SERVO_FEEDBACK)
#define C_PRINT 'P'  // Continuously print data
#define C_PRINT_OFF 'p'  // Print data once then stop

#define T_HELP_INFO 'h'  // hold the loop to check printed information.
#define T_INDEXED_SIMULTANEOUS_ASC \
  'i'  // i jointIndex1 jointAngle1 jointIndex2 jointAngle2 ... e.g. i0 70 8 -20 9 -20. a single 'i' will free the head
       // joints if it were previously manually controlled.
#define T_INDEXED_SIMULTANEOUS_BIN 'I'  // I jointIndex1 jointAngle1 jointIndex2 jointAngle2 ... e.g. I0 70 8 -20 9 -20
#define T_JOINTS 'j'  // A single "j" returns all angles. "j Index" prints the joint's angle. e.g. "j 8" or "j11".
#define T_JOYSTICK 'J'
#define T_SKILL 'k'
#define T_SKILL_DATA 'K'
#define T_BALANCE_SLOPE 'l'           // change the slope of the balancing adjustment in roll and pitch directions. \
                                      // default "l 1 1". the numbers allows [-2,-1,0,1,2]
#define T_LISTED_BIN 'L'              // a list of the DOFx joint angles: angle0 angle1 angle2 ... angle15
#define T_INDEXED_SEQUENTIAL_ASC 'm'  // m jointIndex1 jointAngle1 jointIndex2 jointAngle2 ... e.g. m0 70 0 -70 8 -20 9 -20
#define T_INDEXED_SEQUENTIAL_BIN 'M'  // M jointIndex1 jointAngle1 jointIndex2 jointAngle2 ... e.g. M0 70 0 -70 8 -20 9 -20
#define T_NAME 'n'                    // customize the Bluetooth device's broadcast name. e.g. nMyDog will name the device as "MyDog" \
                                      // it takes effect the next time the board boosup. it won't interrupt the current connecton.
// #define T_MELODY 'o'
#define T_SIGNAL_GEN 'o'  // signal generator for joint movements
#define T_CPG 'r'      //Oscillator for Central Pattern Generator (ASCII)
#define T_CPG_BIN 'Q'  //Oscillator for Central Pattern Generator (Binary)
#define T_PAUSE 'p'  // pause
#define T_POWER 'P'  // power, print the voltage
#define T_TASK_QUEUE 'q'
#define T_ROBOT_ARM 'R'
#define T_SAVE 's'
#define T_TILT 't'
#define T_TEMP 'T'  // call the last skill data received from the serial port
#define T_MEOW 'u'
// #define T_SERVO_MICROSECOND 'w' // PWM width modulation
#define T_WIFI_INFO 'w'
// #define T_XLEG 'x'
#define T_LEARN 'x'
#define T_RANDOM_MIND 'z'  // toggle random behaviors

#define T_READ 'R'  // read pin     R
#define T_WRITE 'W'  // write pin                      W
#define TYPE_ANALOG 'a'  //            Ra(analog read)   Wa(analog write)
#define TYPE_DIGITAL 'd'  //            Rd(digital read)  Wd(digital write)

#define T_RESET '!'
#define T_QUERY '?'
#define C_QUERY_PARTITION 'p'
#define T_ACCELERATE '.'
#define T_DECELERATE ','

#define T_EXTENSION 'X'
#define EXTENSION_GROVE_SERIAL 'S'  // connect to Grove UART2
#define EXTENSION_VOICE \
  'A'  // connect to Grove UART2 (on V0_*: a slide switch can choose the voice or the Grove), or UART1 (on V1). Hidden
       // on board.
#define EXTENSION_DOUBLE_TOUCH 'T'  // connect to ANALOG1, ANALOG2
#define EXTENSION_DOUBLE_LIGHT 'L'  // connect to ANALOG1, ANALOG2
#define EXTENSION_DOUBLE_IR_DISTANCE 'D'  // connect to ANALOG3, ANALOG4
#define EXTENSION_PIR 'I'  // connect to ANALOG3
#define EXTENSION_BACKTOUCH 'B'  // connect to BACKTOUCH_PIN
#define EXTENSION_ULTRASONIC 'U'  // connect to Grove UART2
#define EXTENSION_GESTURE 'G'  // connect to Grove I2C
#define EXTENSION_CAMERA 'C'  // connect to Grove I2C
#define EXTENSION_QUICK_DEMO 'Q'  // activate the quick demo at the end of OpenCatEsp32.ino

#define T_CAMERA_PRINT 'P'
#define T_CAMERA_PRINT_OFF 'p'
#define T_CAMERA_REACTION 'R'
#define T_CAMERA_REACTION_OFF 'r'

#define IMU_EXCEPTION_FLIPPED -1
#define IMU_EXCEPTION_LIFTED -2
#define IMU_EXCEPTION_KNOCKED -3
#define IMU_EXCEPTION_PUSHED -4
#define IMU_EXCEPTION_OFFDIRECTION -5
#define IMU_EXCEPTION_FREEFALL -6
#define IMU_EXCEPTION_TURNING -7

char defaultLan = 'a';
char currentLan;
// bool updated[10];
float degPerRad = 180 / M_PI;
float radPerDeg = M_PI / 180;

// control related variables
#define IDLE_TIME 1000
long idleTimer = 0;
#define CHECK_BATTERY_PERIOD 1000  // every 1 seconds. 60 mins -> 3600 seconds
#define BATTERY_WARNING_FREQ 10  // every 10 seconds
byte batteryWarningCounter = 0;
float lastVoltage;
int uptime = -1;
int frame = 0;
int tStep = 1;
long loopTimer;
byte fps = 0;
// long wdtTimer;

// Gait cycle counting variables
int targetCycles = 0;       // Target number of gait cycles to complete
int completedCycles = 0;    // Number of gait cycles completed so far
bool cycleCountingMode = false;  // Whether cycle counting mode is active

char token;
char lastToken;
char lowerToken;
#define CMD_LEN 20
char *lastCmd = new char[CMD_LEN + 1];  // the last char must be '\0' for safe so CMD_LEN+1 elements are required
int cmdLen = 0;
byte newCmdIdx = 0;
int8_t periodGlobal = 0;
#define BUFF_LEN 2507  // 1524 =125*20+7=2507
char *newCmd = new char[BUFF_LEN + 1];
int spaceAfterStoringData = BUFF_LEN;
char buttonCmd[20];
int serialTimeout;
char terminator;
// int serialTimeout;
long lastSerialTime = 0;
String webResponse = "";

/*  These "Q" booleans are conditions that are checked to activate or deactivate different states.
    A condition set to true activates (turns on) a state.
*/
bool lowBatteryQ = false;  // true = lowBattery() has determined that the battery voltage is below a threshold (see
                           // above VOLTAGE macros).
bool autoLedQ = false;
bool updateGyroQ = true;
bool fineAdjustQ = true;
bool gyroBalanceQ = true;  // true = CONTINUOUSLY access recovery options in dealWithExceptions().
bool gyroBalanceQlag = true;  // to record the origal gyroBalanceQ value.
bool printGyroQ = false;  // true = CONTINUOUSLY access print6Axis() which prints Gyro (IMU) data.
bool readFeedbackQ = false;  // true = CONTINUOUSLY access servoFeedback() which prints servo angles.
bool followFeedbackQ = false;  // true = CONTINUOUSLY access servoFollow() which follows servo angles as servos are
                               // manipulated and automatically calls servoFeedback() to show updated servo angles.
bool walkingQ = false;
bool manualHeadQ = false;
bool nonHeadJointQ = false;
bool manualEyeColorQ = false;
bool rebootForWifiManagerQ = false;
bool cmdFromWeb = false;
// bool keepDirectionQ = true;

// Other booleans
bool interruptedDuringBehavior = false;
bool autoSwitch = false;
bool workingStiffness = true;
bool cameraLockI2c = false;
bool imuLockI2c = false;
bool gestureLockI2c = false;
bool eepromLockI2c = false;

#define HEAD_GROUP_LEN 4  // used for controlling head pan, tilt, tail, and other joints independent from walking
int targetHead[HEAD_GROUP_LEN];

bool imuUpdated;
int8_t imuException = 0;
int8_t prev_imuException = 0;
byte transformSpeed = 2;
float protectiveShift;  // reduce the wearing of the potentiometer

int8_t moduleList[] = {
    EXTENSION_GROVE_SERIAL,
    EXTENSION_VOICE,
    EXTENSION_DOUBLE_TOUCH,
    EXTENSION_DOUBLE_LIGHT,
    EXTENSION_DOUBLE_IR_DISTANCE,
    EXTENSION_PIR,
    EXTENSION_BACKTOUCH,
    EXTENSION_ULTRASONIC,
    EXTENSION_GESTURE,
    EXTENSION_CAMERA,
    EXTENSION_QUICK_DEMO,
};

String moduleNames[] = {"Grove_Serial", "Voice",      "Double_Touch", "Double_Light ", "Double_IR_Distance ", "PIR",
                        "BackTouch",    "Ultrasonic", "Gesture",      "Camera",        "Quick_Demo"};
bool moduleActivatedQ[] = {0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0};
bool moduleDemoQ = false;
int8_t moduleIndex;
bool icmQ = false;
bool mpuQ = false;
bool MuQ = false;
bool Sentry1Q = false;
bool Sentry2Q = false;
bool GroveVisionQ = false;
bool eepromQ = false;
bool initialBoot = true;
bool coinFace = true;
bool safeRest = true;
bool soundState;
byte buzzerVolume;
float amplifierFactor = 100.0;  // to fit the actual amplifier range of BiBoard

int delayLong = 20;
int delayMid = 8;
int delayException = 5;
int delayShort = 3;
int delayStep = 1;
int delayPrevious;
int runDelay = delayMid;

int8_t middleShift[] = { 0, -90, 0, 0,
                         -45, -45, -45, -45,
                         55, 55, -55, -55,
                         -55, -55, -55, -55 };

// #define INVERSE_SERVO_DIRECTION
int8_t rotationDirection[] = {
  1, -1, -1, 1,
  1, -1, 1, -1,
  1, -1, -1, 1,
  -1, 1, 1, -1
};

int angleLimit[][2] = {
    {-120, 120},
    {-85, 85},
    {-120, 120}, {-120, 120},
    {-90, 60},   {-90, 60},   {-90, 90},  {-90, 90},
    {-200, 80},  {-200, 80},  {-80, 200}, {-80, 200}, {-80, 200}, {-80, 200}, {-80, 200}, {-80, 200},
};

int currentAng[DOF] = {0, 0, 0, 0, 0, 0, 0, 0, 75, 75, 75, 75, -55, -55, -55, -55};
int previousAng[DOF] = {0, 0, 0, 0, 0, 0, 0, 0, 75, 75, 75, 75, -55, -55, -55, -55};

int zeroPosition[DOF] = {};
int calibratedZeroPosition[DOF] = {};

int8_t servoCalib[DOF] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

int16_t mpuOffset[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

float expectedRollPitch[2];
float RollPitchDeviation[2];
float currentAdjust[DOF] = {};
int balanceSlope[2] = {1, 1};  // roll, pitch

#include "tools.h"
#include "QList/QList.h"
// #include "taskQueue.h"

/* Dependencies for displayNsvPartition() */
#include "esp_partition.h"  // To check ESP32 partitions
#include "esp_log.h"  // To check ESP32 partitions
#include "nvs_flash.h"  // To check namespaces in the nvs partition of the ESP32
#include "nvs.h"  // To check namespaces in the nvs partition of the ESP32
#include <set>  // To manage unique namespaces

// #include "sound.h"
#include <Wire.h>
#include "configConstants.h"
// #include "bluetoothManager.h"
#include "io.h"
#ifdef GYRO_PIN
// #include "imu.h"
#endif
#include "espServo.h"
#include "motion.h"
// #include "randomMind.h"
#include "skill.h"
#include "moduleManager.h"

#include "reaction.h"
// #include "qualityAssurance.h"

void initRobot() {
  Wire.begin();
  SoftwareVersion = SoftwareVersion + BOARD + "_" + DATE;
  PTL('k');
  PTLF("Flush the serial buffer...");
  PTL("\n* Start *");
  printToAllPorts(MODEL);
  PTF("Software version: ");
  printToAllPorts(SoftwareVersion);

  newBoard = newBoardQ();
  configSetup();

  servoSetup();
  lastCmd[0] = '\0';
  newCmd[0] = '\0';
  skill = new Skill();
  skillList = new SkillList();

  strcpy(newCmd, "rest");
  loadBySkillName("rest");

  initModuleManager();
  PTL("Ready!");
  idleTimer = millis();
}
