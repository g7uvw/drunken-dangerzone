//hold car at 30mph
#include <FreqMeasure.h>

// bug brake values still sent when road speed = 0;


// Pin mappings 
const int LEDpin = 13;
const int BRAKEpin = 23;
const int READYpin = 22;
const int POTpin = 0;  // Analog 0 (PIN 14 on Teensy 3.1)

// Braking POT variables
int potval = 0, oldpotval = 0;
const int numReadings = 10;
int POTreadings[numReadings];      // the readings from the analog input
int POTindex = 0;                  // the index of the current reading
int POTtotal = 0;                  // the running total
int POTaverage = 0;                // the average
int oldPOTaverage = 0;             // previous average

// frequency stuff
float basefrequency = 0;
float currentfrequency = 0;
float previousfrequency = 0;

//speed stuff
float basespeed = 0;
float currentspeed = 0;
float previousspeed = 0;

//control booleans
boolean controllingroad = true;
int roadenable = 0;       //multiplier for the analog writes.

// Function protoypes
int getPOTaverage(void);
int getFREQaverage(void);
void debugBlink(const int pin, unsigned int flashcount);
float FreqToMPH(float freq);

// PWM Setup - may need tweeking in the field.
const unsigned int PWM_F = 50;
//const unsigned int PWM_F = 46875; // too fast for SCR driver brick
int PWM_extra = 10;


void setup()
{
 Serial.begin(9600);
 Serial.println("Starting Rolling Road");
 analogWriteResolution(10);          // 16, was 10 bit PWM resolution (0 - 1023)
 analogWriteFrequency(BRAKEpin,PWM_F);   
//pinMode(3,INPUT); 
 FreqMeasure.begin();                // start measuring the speed pulses
 pinMode(LEDpin, OUTPUT);            // LED setup
 pinMode(READYpin,OUTPUT);
}

void loop()
{
  basespeed = FreqToMPH(getFREQaverage(2));
  
  while (basespeed < 29)
  {
    Serial.print("Speed = ");
    Serial.println(basespeed);
  }
  while()
  {
    currentspeed = FreqToMPH(getFREQaverage(2));
    if (currentspeed > 30.5)
    {
      Serial.print("Faster than 30MPH : ");
      Serial.println(currentspeed,2);
      PWM_extra++;
    }
  
 if (currentspeed < previousspeed)
   {
     Serial.print("Slowing down : ");
      Serial.println(currentspeed,2);
      PWM_extra--;
   }
   
  
   
  
  
  previousspeed = currentspeed;
}


