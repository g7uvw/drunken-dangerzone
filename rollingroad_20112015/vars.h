// Pin mappings
const int REDLED = 9;
const int GREENLED = 7;
const int YELLOWLED = 5;

const int BRAKEpin = A14; //was 23
const int POTpin = 0;  // Analog 0 (PIN 14 on Teensy 3.1)
const int TORQUEpin = 1; // Analog 1 (PIN 15 on Teensy 3.1)

// Braking POT variables
int potval = 0, oldpotval = 0;
const int numReadings = 10;
int POTreadings[numReadings];      // the readings from the analog input
int POTindex = 0;                  // the index of the current reading
int POTtotal = 0;                  // the running total
int POTaverage = 0;                // the average
int oldPOTaverage = 0;             // previous average

// Torque variables
unsigned TORQUEaverage = 0;

// frequency variables
float basefrequency = 0;
float currentfrequency = 0;
float previousfrequency = 0;

//speed variables
float basespeed = 0;
float currentspeed = 0;
float previousspeed = 0;
float speederror = 0;

//control booleans
boolean controllingroad = true;
boolean roadenable = false;       //multiplier for the analog writes.
boolean once_worked = false;  //if we've ever loaded up, this is true. If true at start, then we're reset and need to zero everything
boolean freewheel = true;


// PWM Setup - may need tweeking in the field.
const unsigned int PWM_F = 50;
int Brake_extra = 0;
