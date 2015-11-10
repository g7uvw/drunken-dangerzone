// Rolling Road 
// (c) David Mills 2015
// 28/8/2015 using this
//fuck me it works!
// tested working on bench on 15/09/2015
// sent to CCK on 16/09/2015

//this is the one we'e using on 22/09/2015
// seems to work first time, but jerks on subsequent runs after a full stop. Seems to happen if the chip isn't reset
// added a control bool to see if the road has ever loaded up - if so and we're back at low pot and low speed, then 
// we've probably had a run already and then gone full stop.

// basic testing shows this reset on zero isn't working. Test on bench again.

// 28/10/15 - still an issue where if speed drops to around zero then comes back to basespeed, the PWM value is too high
// looks like the base speed is being recalculated at the lower speed.

// Do the brake val based on currentspeed - basespeed. may need to scale the value.
// think of it as PLL or FLL and brake is the error val for correction.





#define NO_BINARY_OUTPUT false
#define DEBUGGING false

#include <FreqMeasure.h>
//#include <Wire.h>

// bug brake values still sent when road speed = 0;


// Pin mappings 
const int LEDpin = 13;
const int BRAKEpin = A14; //was 23
const int READYpin = 22;
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
//int TORQUEreadings[numReadings];      // the readings from the analog input
//int TORQUEindex = 0;                  // the index of the current reading
//int TORQUEtotal = 0;                  // the running total
int TORQUEaverage = 0;

// frequency stuff
float basefrequency = 0;
float currentfrequency = 0;
float previousfrequency = 0;

//speed stuff
float basespeed = 0;
float currentspeed = 0;
float previousspeed = 0;
float speederror = 0;

//control booleans
boolean controllingroad = true;
int roadenable = 0;       //multiplier for the analog writes.
boolean once_worked = 0;  //if we've ever loaded up, this is true. If true at start, then we're reset and need to zero everything
boolean freewheel = 0;

// data structure
struct data_t{
        uint8_t     calibrating;    // non zero means we're calibrating the strain guage
        uint8_t     recording;      // non zero means record
        uint8_t     highrange;      // non zero means Torque is set to high range
        uint8_t     padding;       // just to pad the stuct
        uint16_t    torque;         // from strain guage
        uint16_t    speedo;          // MPH from controller board
        uint16_t    revs;
        uint16_t    humidity;
        uint16_t    pressure;
    } __attribute__((__packed__)) data_packet;
    
unsigned long uBufSize = sizeof(data_t);

// Function protoypes
int getPOTaverage(void);
int getTORQUEaverage(void);
int getFREQaverage(void);
//void debugBlink(const int pin, unsigned int flashcount);
float FreqToMPH(float freq);
void Serial_Update();
void setBrake(int brake);


// PWM Setup - may need tweeking in the field.
const unsigned int PWM_F = 50;
//const unsigned int PWM_F = 46875; // too fast for SCR driver brick
int PWM_extra = 10;


void setup()
{
 Serial.begin(115200);
 Serial.println("Starting Rolling Road");
 analogWriteResolution(10);          //10 bit PWM resolution (0 - 1023)
 analogWriteFrequency(BRAKEpin,PWM_F);   
pinMode(3,INPUT); 
 FreqMeasure.begin();                // start measuring the speed pulses
 //FreqMeasure2.begin();
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
{
  if (once_worked)  //zero eveything.
  {
    POTindex = 0;                  // the index of the current reading
    POTtotal = 0;                  // the running total
    POTaverage = 0;                // the average
    oldPOTaverage = 0;             // previous average
    TORQUEaverage = 0;

// frequency stuff
    basefrequency = 0;
    currentfrequency = 0;
    previousfrequency = 0;

  //speed stuff
    //basespeed = 0;
    currentspeed = 0;
    previousspeed = 0;
    
    PWM_extra = 0;
    
    setBrake(0);
    //analogWrite(BRAKEpin,0); 
  }
  
//we do the calibration stuff here.
     // read the Torque ADC channel and transmit the data.
  TORQUEaverage = getTORQUEaverage();
  data_packet.calibrating = 1;
  data_packet.recording = 0;
  data_packet.highrange = 1;
  data_packet.padding = 0;
  data_packet.torque = TORQUEaverage;
  data_packet.speedo = 0;
  data_packet.revs = 0;
  
  char pBuffer[uBufSize];
 memcpy(pBuffer, &data_packet, uBufSize);
 
 for(int i = 0; i<uBufSize;i++) 
 {
   Serial.print(pBuffer[i]);
 }
 Serial.println("calibrating");
  delay(100);   
  goto start;
}
   
while (POTaverage < 50) 
{
  
  basefrequency = getFREQaverage(5);  // take 5 samples
  basespeed = FreqToMPH(basefrequency);
  data_packet.calibrating = 0;
  data_packet.recording = 0;      // non zero means record
  data_packet.highrange = 1;      // non zero means Torque is set to high range
  data_packet.padding = 0;       // just to pad the stuct
  data_packet.torque = 0;         // from strain guage
  data_packet.revs = 0;
  data_packet.humidity = 78;
  data_packet.pressure = 0;
  data_packet.speedo = (uint16_t)basespeed;
  Serial_Update();
  
 POTaverage = getPOTaverage();
 
 if (DEBUGGING)
 {
   Serial.print("Waiting for car to get up to speed ");
   Serial.println(basespeed,2);
   Serial.print("POT average ");
   Serial.println(POTaverage, 1);
  }

}

// I'm not sure what sort of noise we'll get on the POT reading, so for now see if it
// is in the range +/- 10 of what it started as. This can be tweeked in the final install.
// uncomment serial.print lines for debugging this.
while (!(POTaverage <= POTaverage+10 && !(POTaverage < POTaverage-10))) 
{
 //if (!once_worked)
 //{
   basefrequency = getFREQaverage(5);  // take 5 samples
   basespeed = FreqToMPH(basefrequency); 
 //}
 data_packet.speedo = (uint16_t)basespeed; 
 Serial_Update(); 
 POTaverage = getPOTaverage();
 
 setBrake(roadenable *(POTaverage + PWM_extra));
 //analogWrite(BRAKEpin,roadenable *(POTaverage + PWM_extra));  // Add in the extra so there's no sudden jump if the pot get changed when the road is braking
 
 if (DEBUGGING)
 {
 Serial.print("POT Average: ");
 Serial.println(POTaverage, 2);
 }
 
}


// when we make it here, the driver should have finished setting pot for his choice of speed
// get a baseline frequency (speed)

if (!once_worked)
{
  basefrequency = getFREQaverage(5);  // take 5 samples
  basespeed = FreqToMPH(basefrequency);
}

data_packet.speedo = (uint16_t)basespeed;
TORQUEaverage = getTORQUEaverage();
data_packet.torque = TORQUEaverage;
Serial_Update();

digitalWriteFast(READYpin, HIGH);    // set the READY LED on
controllingroad = true;
once_worked = true;

if (DEBUGGING)
{
 Serial.println("back at top");
 Serial.print("Speed: ");
 Serial.println(basespeed, 3);
}

if (digitalRead(6))
    data_packet.recording = 0;
else
    data_packet.recording = 1;
    
while (controllingroad)
{
  
  if (!freewheel)
  {
    //Serial.println("not free");
    setBrake(POTaverage + PWM_extra);
    //analogWrite(BRAKEpin,POTaverage);  
  }
  
  
  if (DEBUGGING)
  {
    //delay(1000);
    delay(5);
    if (!freewheel)
    {
      Serial.print("Controlling road, PWM = ");
      Serial.println(POTaverage + PWM_extra, DEC);
    }
    Serial.print("Current Speed MPH ");
    Serial.println(currentspeed,2);
    Serial.println("------------");
  }
  
  delay(10); //was 100?
  POTaverage = getPOTaverage();              // read pot
  if ((POTaverage < (oldPOTaverage - 5)) || (  POTaverage > (oldPOTaverage + 5) ))    // pot has been changed, probably going to change car speed, need to recalibrate
    {
      controllingroad = false;
      basefrequency = getFREQaverage(5);  // take 5 samples
      basespeed = FreqToMPH(basefrequency);
      
      setBrake(roadenable * (POTaverage));
      
      //analogWrite(BRAKEpin,roadenable * (POTaverage));   
      if (DEBUGGING)
      {
        Serial.println("Pot Value changed");
        Serial.print("OldPOT Average: ");
        Serial.print(oldPOTaverage,2);
        Serial.print("POT Average: ");
        Serial.println(POTaverage,2);
      }
    }
    
  oldPOTaverage = POTaverage;
  previousspeed = currentspeed;    //save old speed
  currentspeed = FreqToMPH(getFREQaverage(10));  //get new speed
  TORQUEaverage = getTORQUEaverage();
  
  data_packet.torque = TORQUEaverage;
  
  if (currentspeed < 5) //was 5
  {
    setBrake(0);
    //analogWrite(BRAKEpin,0);   // Brake off
    controllingroad = false;
    
    if (DEBUGGING)
    {
      Serial.println("Road speed below 5 MPH");
    }
    
    goto start;
  }
  
  speederror = currentspeed - basespeed;
  
  if (currentspeed > (basespeed+0.5))       // road going faster than it was...
    {
      freewheel = false;
      PWM_extra+=10; 
      
      //check it isn't pointlessly growing
     // if (PWM_extra > 1024)
      //  {
      //    PWM_extra = 1024;
      //  }
        
      //Serial.println("curr > base");
      setBrake(roadenable * (POTaverage + PWM_extra));  
      //analogWrite(BRAKEpin,roadenable * (POTaverage + PWM_extra));   // longer pulses
      if (currentspeed > previousspeed) //still getting faster, so increase the PWM ontime.
         {
           PWM_extra+=10;
           setBrake(roadenable * POTaverage + PWM_extra);
           data_packet.speedo = (uint16_t)currentspeed;
           Serial_Update();
        if (DEBUGGING)
           {   
             Serial.println("Road speeding up");
             //Serial.println(POTaverage + PWM_extra, DEC);
             Serial.print("Current Speed MPH ");
             Serial.println(currentspeed,2);
             Serial.print("Previous Speed MPH ");
             Serial.println(previousspeed,2);
           }
         }
         
      //this was commented out - it did work without this, but then it didnn't
      // can't explain that!!!!
      
      if  (currentspeed < (previousspeed-0.5))    // still faster than base freq, but slowing down
         {
           if (PWM_extra > 10)
           {
            PWM_extra-=10;
            setBrake(roadenable * (POTaverage + PWM_extra));
           }
            //analogWrite(BRAKEpin,roadenable * (POTaverage + PWM_extra));   // reduce pulse length a bit
         }  
      //digitalWriteFast(LEDpin, HIGH);         // set the LED on
      //delay(1);
      previousspeed = currentspeed;
    }
      
      // we're pretty much at the base speed +/- a little bit
      // hold the brake where it was.
      //slacken range a bit... was 0.05
      if (currentspeed <= (basespeed + 0.05) && !(currentspeed < (basespeed - 0.1))) 
      
  //if (!(currentspeed < (basespeed - 0.1)) || (currentspeed > (basespeed + 0.05) ))
      {
        freewheel = false;
        data_packet.speedo = (uint16_t)currentspeed;
        Serial_Update();
      //Serial.println("In range");
        setBrake(roadenable * POTaverage);  
       // analogWrite(BRAKEpin,roadenable * POTaverage);        // standard pulses
        PWM_extra = 0;
        if (DEBUGGING)
        {
          Serial.print("Base speed MPH ");
          Serial.println(basespeed,2);
          Serial.print("Constant speed MPH ");
          Serial.println(currentspeed,2);
          //Serial.print("PWM = ");
          //Serial.println(roadenable * POTaverage, DEC);
        }
      }
      
      
  if (currentspeed < basespeed - 5)   // 5 mph less than base speed
 // if (currentspeed < previousspeed - 1)   // 1 mph less than before
      {
        freewheel = true;
        data_packet.speedo = (uint16_t)currentspeed;
        Serial_Update();  
        PWM_extra=0;
        setBrake(0);
        //analogWrite(BRAKEpin,0);   // turn brake off - we're freewheeling. was -1, might have been an issue
        
        if (DEBUGGING)
        {
          Serial.println("Freewheeling");
        }
      }
    
 data_packet.calibrating = 0;
 data_packet.highrange = 1;      
 data_packet.torque = TORQUEaverage;
 data_packet.speedo = (uint16_t) currentspeed;
 data_packet.revs = (uint16_t) currentfrequency;

 Serial_Update();
 
}

// no longer controlling the road, the speedpot has changed, so we need new frequncy values
digitalWriteFast(READYpin, LOW);    // set the READY LED off

 Serial_Update();
 if (DEBUGGING)
 {
   Serial.println("Not controlling road");
 }
 

// loop back to start
}

void Serial_Update()
{
  if (!NO_BINARY_OUTPUT)
  {
   data_packet.calibrating = 0;
   data_packet.highrange = 1;      
   data_packet.torque = TORQUEaverage;
   data_packet.speedo = (uint16_t) currentspeed;
   data_packet.revs = (uint16_t) currentfrequency;
   char pBuffer[uBufSize];
   memcpy(pBuffer, &data_packet, uBufSize);
 
   for(int i = 0; i<uBufSize;i++) 
   {
     Serial.print(pBuffer[i]);
   } 
   //Serial.println();
   //delay(10);        //removed on 28/10/15 to speed things up.
  }
}

int getTORQUEaverage(void)
{
  unsigned int ta = 0;
  for (unsigned int i = 0; i < 10; i++)
  {
      ta += analogRead(TORQUEpin);
  }
  ta /= 10;
  return ta;
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
     //delay(5);  // small delay to help things along
  }

    int average = FreqMeasure.countToFrequency(sum / samples);
    currentfrequency = average;
    return average;
}



float FreqToMPH(float freq)
{
  // 60Hz on a roller diamter of 50cms is 1.57 m/s
  // 1.57 m/s = 3.512 MPH
  return  (freq/60) * 3.512;
}

void setBrake(int brake)
{
  analogWrite(BRAKEpin,brake);   
  if (DEBUGGING)
           {   
             Serial.print("setBrake: Setting PWM to ");
             Serial.println(brake, DEC);
           }
}
