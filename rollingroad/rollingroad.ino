// Rolling Road 
// (c) David Mills 2015

#include <FreqMeasure.h>

// Pin mappings 
const int LEDpin = 13;
const int BRAKEpin = 23;
const int READYpin = 22;
const int POTpin = 0;  // Analog 0 (PIN 14 on Teensy 3.1)

// Braking POT variables
int potval = 0, oldpotval = 0;
const int numReadings = 20;
int POTreadings[numReadings];      // the readings from the analog input
int POTindex = 0;                  // the index of the current reading
int POTtotal = 0;                  // the running total
int POTaverage = 0;                // the average
int oldPOTaverage = 0;             // previous average

// frequency stuff
float basefrequency = 0;
float currentfrequency = 0;

//control booleans
boolean controllingroad = true;

// Function protoypes
int getPOTaverage(void);
int getFREQaverage(void);

void setup()
{
 analogWriteResolution(10);          // 10 bit PWM resolution (0 - 1023)
 analogWriteFrequency(23,46875);     // pin 23 as output, PWM frequency 46875Hz
 FreqMeasure.begin();                // start measuring the speed pulses
 pinMode(LEDpin, OUTPUT);  
}

void loop()
{
  
 // Car will be up to speed before the pot is set to provide some drag on the road. 
 // Driver brings the car up to speed then adjusts the pot until they feel some bite.
 // At this point it's up to this code to keep the road at this speed by providing more
 // braking as the car tries to speed up.
 
 // read the pot & wait for it to become stable (value doesn't change by more than +/- 2)
 // apply braking too, as that's what the driver will detected when setting the pot

while(!(unsigned) (POTaverage-(POTaverage-2)) <= ((POTaverage+2)-(POTaverage-2)))
{
 POTaverage = getPOTaverage();
 analogWrite(BRAKEpin,POTaverage);
}

// when we make it here, the driver should have finished setting pot for his choice of speed
// get a baseline frequency (speed)

basefrequency = getFREQaverage(50);  // take 50 samples
digitalWriteFast(READYpin, HIGH);    // set the READY LED on
controllingroad = true;

while (controllingroad)
{
  POTaverage = getPOTaverage();              // read pot
  if (POTaverage < oldPOTaverage)            // pot has been reduced, probably going to change car speed, need to recalibrate
    controllingroad = false;
    
  currentfrequency = getFREQaverage(10);
  if (currentfrequency > basefrequency)       // road is speeding up
    {
      analogWrite(BRAKEpin,(POTaverage*2));   // longer pulses
      digitalWriteFast(LEDpin, HIGH);         // set the LED on
    }
  else
  {
    analogWrite(BRAKEpin,POTaverage);        // standard pulses
    digitalWriteFast(LEDpin, LOW);           // set the LED on
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
