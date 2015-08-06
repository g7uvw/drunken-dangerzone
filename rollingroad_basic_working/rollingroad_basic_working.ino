// Rolling Road 
// (c) David Mills 2015
// Second day of tests...
#include <FreqMeasure.h>

// bug brake values still sent when road speed = 0;


// Pin mappings 
const int LEDpin = 13;
const int BRAKEpin = A14; //was 23
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
 //analogWriteFrequency(BRAKEpin,PWM_F);   
//pinMode(3,INPUT); 
 FreqMeasure.begin();                // start measuring the speed pulses
 pinMode(LEDpin, OUTPUT);            // LED setup
 pinMode(READYpin,OUTPUT);
 

}

void loop()
{
  start:
 // Car will be up to speed before the pot is set to provide some drag on the road. 
 // Driver brings the car up to speed then adjusts the pot until they feel some bite.
 // At this point it's up to this code to keep the road at this speed by providing more
 // braking as the car tries to speed up.
 
 // read the pot & wait for it to become stable (value doesn't change by more than +/- 10)
 // apply braking too, as that's what the driver will detected when setting the pot


	
// quick in range code from : https://stackoverflow.com/questions/3964017/checking-if-integer-falls-in-range-using-only-operator
// Short answer:
// if (num < max && !(num <= min)) { // stuff to do }
// This will return true if "num" is between "min" and "max" but is not equal to either of them.
// If you need it to include "min" and "max" in the range check, use:
// if (num <= max && !(num < min)) { // stuff to do }
// This works because...
// !(A > B)  == (A <= B)  // If not greater than B, must be less than or equal to B
// !(A >= B) == (A < B)   // If not greater or equal to B, must be less than B
// !(A < B)  == (A >= B)  // If not less than B, must be greater or equal to B
// !(A <= B) == (A > B)   // If not less than or equal to B, must be greater than B



//debugBlink(READYpin,2);
//while(!(unsigned) (POTaverage-(POTaverage-10)) <= ((POTaverage+10)-(POTaverage-10)))
//while(!(unsigned) (POTaverage-(POTaverage-10)) <= ((oldPOTaverage+10)-(oldPOTaverage-10)))

if (getFREQaverage(2) < 10)
   roadenable = 0;
else
   roadenable = 1;
   
if(!roadenable)
     goto start;
   
while (POTaverage < 100)
{
 POTaverage = getPOTaverage();
 Serial.print("Waiting for car to get up to speed - POT Average: ");
 Serial.println(POTaverage, DEC);
}

// I'm not sure what sort of noise we'll get on the POT reading, so for now see if it
// is in the range +/- 10 of what it started as. This can be tweeked in the final install.
// uncomment serial.print lines for debugging this.
while (!(POTaverage <= POTaverage+10 && !(POTaverage < POTaverage-10))) 
{
 POTaverage = getPOTaverage();
 Serial.print("POT Average: ");
 Serial.println(POTaverage, DEC);
 analogWrite(BRAKEpin,roadenable *(POTaverage + PWM_extra));  // Add in the extra so there's no sudden jump if the pot get changed when the road is braking
}

//debugBlink(READYpin,4);
// when we make it here, the driver should have finished setting pot for his choice of speed
// get a baseline frequency (speed)

basefrequency = getFREQaverage(5);  // take 5 samples
basespeed = FreqToMPH(basefrequency);
 Serial.print("Speed: ");
 Serial.println(basespeed, 3);
//debugBlink(READYpin,6);
digitalWriteFast(READYpin, HIGH);    // set the READY LED on
controllingroad = true;

while (controllingroad)
{
  delay(100);
  POTaverage = getPOTaverage();              // read pot
  if ((POTaverage < (oldPOTaverage - 5)) || (  POTaverage > (oldPOTaverage + 5) ))    // pot has been changed, probably going to change car speed, need to recalibrate
    {
      controllingroad = false;
      Serial.println("Pot Value changed");
      Serial.print("OldPOT Average: ");
      Serial.print(oldPOTaverage,DEC);
      Serial.print("POT Average: ");
      Serial.println(POTaverage,DEC);
    }
  oldPOTaverage = POTaverage;
  currentspeed = FreqToMPH(getFREQaverage(10));
  if (currentspeed < 5)
  {
    analogWrite(BRAKEpin,0);   // redule pulse length a bit
    controllingroad = false;
    goto start;
  }
    
  if (currentspeed > (basespeed+1))       // road going faster than it was...
    {
      analogWrite(BRAKEpin,roadenable * (POTaverage + PWM_extra));   // longer pulses
      if (currentspeed > previousspeed) //still getting faster, so increase the PWM ontime.
         {
           PWM_extra+=1;    
           Serial.print("Road speeding up, setting PWM to ");
           Serial.println(POTaverage + PWM_extra, DEC);
           Serial.print("Speed MPH ");
           Serial.println(currentspeed,DEC);
         }
      if  (currentspeed < (previousspeed-1))    // still faster than base freq, but slowing down
         {
           //if (PWM_extra > 10)
              PWM_extra-=1;
           analogWrite(BRAKEpin,roadenable * (POTaverage + PWM_extra));   // redule pulse length a bit
         }  
      digitalWriteFast(LEDpin, HIGH);         // set the LED on
      previousspeed = currentspeed;
    }
  else
  {
   Serial.print("Road speed normal, PWM value ");
   Serial.println(POTaverage, DEC);
   Serial.print("Speed MPH ");
   Serial.println(currentspeed,DEC);
    analogWrite(BRAKEpin,roadenable * POTaverage-2);        // standard pulses
    digitalWriteFast(LEDpin, LOW);           // set the LED on
    PWM_extra = 0;                           // reset the PWM extra length to something low
  }
}

// no longer controlling the road, the speedpot has changed, so we need new frequncy values
digitalWriteFast(READYpin, LOW);    // set the READY LED off


// loop back to start
}



int getPOTaverage(void)
{
 // zero out the POT array
  for (unsigned int i = 0; i < numReadings; i++)
   POTreadings[i] = 0;  
  POTtotal = 0;  
  for(unsigned int i = 0; i< numReadings ; i++)
  {
    POTtotal= POTtotal - POTreadings[POTindex];        
    // read from the sensor:  
    POTreadings[POTindex] = analogRead(POTpin);
    // add the reading to the total:
    POTtotal= POTtotal + POTreadings[POTindex];      
    // advance to the next position in the array:  
    POTindex++;                    

    // if we're at the end of the array...
    if (POTindex >= numReadings)              
      // ...wrap around to the beginning:
      POTindex = 0;                          
  }
  
  // calculate the average:
  int average = POTtotal / numReadings; 
  return average;
}


int getFREQaverage(int samples)
{
  double sum=0;
  int count=0;
  
  for (count = 0; count < samples; count++)
  {
    if (FreqMeasure.available())
     {
      sum += FreqMeasure.read();
     }
     else
       count--;  // no reading to be had, roll back in loop and try again
     delay(10);  // small delay to help things along
  }

    int average = FreqMeasure.countToFrequency(sum / samples);
    return average;
}

void debugBlink(const int pin, unsigned int flashcount)
{
  digitalWriteFast(pin, LOW);    // set the LED off
  for (int count=0; count<flashcount; count++)
  {
    digitalWriteFast(pin, HIGH);    // set the LED on
    delay(1000);
    digitalWriteFast(pin, LOW);    // set the LED off
    delay(1000);
  }
  digitalWriteFast(pin, LOW);    // set the LED off when we finish
}

float FreqToMPH(float freq)
{
  // 60Hz on a roller diamter of 50cms is 1.57 m/s
  // 1.57 m/s = 3.512 MPH
  
  return (freq/60) * 3.512;
}

