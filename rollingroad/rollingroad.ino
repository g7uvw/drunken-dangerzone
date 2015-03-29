// Rolling Road 
// (c) David Mills 2014

#include <FreqMeasure.h>
int potval = 0, oldpotvol = 0;
double sum=0;
int count=0;
float previousfrequency = 0;
float currentfrequency = 0;
const int ledPin = 13;
const int brakepin = 23;
void setup()
{
 
 analogWriteResolution(10);          // 10 bit PWM resolution (0 - 1023)
 analogWriteFrequency(23,46875);     // pin 23 as output, PWM frequency 46875Hz
 FreqMeasure.begin();                // start measuring the speed pulses
 pinMode(ledPin, OUTPUT);
}

void loop()
{
  
 // Car will be up to speed before the pot is set to provide some drag on the road. 
 // Driver brings the car up to speed then adjusts the pot until they feel some bite.
 // At this point it's up to this code to keep the road at this speed by providing more
 // braking as the car tries to speed up.
 
 //read the pot
 potval = analogRead(0);             // analog 0, pin 14 on teensy 3.1 
 
 if (potval < oldpotval)             // reducing load from road, prior to speeding up?
 {
    if (FreqMeasure.available())
    {
      //invalidate old frequency 
      previousfrequency = FreqMeasure.countToFrequency(FreqMeasure.read());
    }
 }
 
 oldpotval = potval;                // save the current pot value for checking next loop around
 digitalWriteFast(ledPin, LOW);     // set the LED off

 
 if (FreqMeasure.available())
   {
    sum += FreqMeasure.read();
    count++;
    if (count > 2)
      {
        currentfrequency = FreqMeasure.countToFrequency(sum / count);
        if (currentfrequency > previousfrequency)    // we're still speeding up
          {
            digitalWriteFast(ledPin, HIGH);   // set the LED on
            // going faster this time around, lengthen the brake pulses.
            analogWrite(brakepin,(potval*2)); 
          }
         previousfrequency = currentfrequency;
       }
   }
 else
   analogWrite(brakepin,potval);
}
