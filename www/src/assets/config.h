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

Servo servos[NUM_JOINTS];
