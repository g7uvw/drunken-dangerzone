

// Rolling Road 
// (c) David Mills 2015
// Second day of tests...
#include <FreqMeasure.h>
#include <FreqMeasure2.h>
#include <Wire.h>

//pressure stuff
#define BMP085_ADDRESS 0x77  //I2C address fr the pressure sensor.
const unsigned char OSS = 3;  // Oversampling Setting
// Calibration values
int16_t ac1;
int16_t ac2;
int16_t ac3;
uint16_t ac4;
uint16_t ac5;
uint16_t ac6;
int16_t b1;
int16_t b2;
int16_t mb;
int16_t mc;
int16_t md;

// b5 is calculated in bmp085GetTemperature(...), this variable is also used in bmp085GetPressure(...)
// so ...Temperature(...) must be called before ...Pressure(...).
long b5; 


// bug brake values still sent when road speed = 0;


// Pin mappings 
const int LEDpin = 13;
const int BRAKEpin = A14; //was 23
const int READYpin = 22;
const int POTpin = 0;  // Analog 0 (PIN 14 on Teensy 3.1)
const int TORQUEpin = A1;

// Braking POT variables
int potval = 0, oldpotval = 0;
const int numReadings = 10;
int POTreadings[numReadings];      // the readings from the analog input
int POTindex = 0;                  // the index of the current reading
int POTtotal = 0;                  // the running total
int POTaverage = 0;                // the average
int oldPOTaverage = 0;             // previous average

// Torque variables
int TORQUEreadings[numReadings];      // the readings from the analog input
int TORQUEindex = 0;                  // the index of the current reading
int TORQUEtotal = 0;                  // the running total
int TORQUEaverage = 0;

// frequency stuff
float basefrequency = 0;
float currentfrequency = 0;
float previousfrequency = 0;

//speed stuff
float basespeed = 0;
float currentspeed = 0;
float previousspeed = 0;

//revs stuff
float revs = 0;

//control booleans
boolean controllingroad = true;
int roadenable = 0;       //multiplier for the analog writes.

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
int getRMPaverage(void);
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
 FreqMeasure2.begin();
 pinMode(LEDpin, OUTPUT);            // LED setup
 pinMode(READYpin,OUTPUT);
 //Wire.begin();
 //bmp085Calibration();
 //float temperature = bmp085GetTemperature(bmp085ReadUT()); //MUST be called first
 //float pressure = bmp085GetPressure(bmp085ReadUP());
 //data_packet.pressure = (uint16_t) pressure / 100;
 pinMode(6, INPUT_PULLUP);
}

void loop()
{
  Serial.println("start");
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
 Serial.println("hello");
  delay(10);   
  
  goto start;
} 

Serial.println("hello2");

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

//check recording status. PIN is grounded for recordidng enable.

if (digitalRead(6))
    data_packet.recording = 0;
else
    data_packet.recording = 1;

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
  revs = getRPMaverage(10);
  
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

//transmit the data to the PC

 data_packet.calibrating = 0;
 data_packet.highrange = 1;      
 data_packet.torque = TORQUEaverage;
 data_packet.speedo = (uint16_t) currentspeed;
 data_packet.revs = (uint16_t) revs;

 char pBuffer[uBufSize];
 memcpy(pBuffer, &data_packet, uBufSize);
 
 for(int i = 0; i<uBufSize;i++) 
 {
   Serial.print(pBuffer[i]);
 }   

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

int getTORQUEaverage(void)
{
 // zero out the torque array
  for (unsigned int i = 0; i < numReadings; i++)
   TORQUEreadings[i] = 0;  
  TORQUEtotal = 0;  
  for(unsigned int i = 0; i< numReadings ; i++)
  {
    TORQUEtotal= TORQUEtotal - TORQUEreadings[TORQUEindex];        
    // read from the sensor:  
    TORQUEreadings[TORQUEindex] = analogRead(TORQUEpin);
    // add the reading to the total:
    TORQUEtotal= TORQUEtotal + TORQUEreadings[TORQUEindex];      
    // advance to the next position in the array:  
    TORQUEindex++;                    

    // if we're at the end of the array...
    if (TORQUEindex >= numReadings)              
      // ...wrap around to the beginning:
      TORQUEindex = 0;                          
  }
  
  // calculate the average:
  int average = TORQUEtotal / numReadings; 
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


int getRPMaverage(int samples)
{
  double sum=0;
  int count=0;
  
  for (count = 0; count < samples; count++)
  {
    if (FreqMeasure2.available())
     {
      sum += FreqMeasure2.read();
     }
     else
       count--;  // no reading to be had, roll back in loop and try again
     delay(5);  // small delay to help things along
  }

    int average = FreqMeasure2.countToFrequency(sum / samples);
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




//pressure stuff 
// Stores all of the bmp085's calibration values into global variables
// Calibration values are required to calculate temp and pressure
// This function should be called at the beginning of the program
void bmp085Calibration()
{
  ac1 = bmp085ReadInt(0xAA);
  Serial.print(ac1);Serial.print(" ");
  ac2 = bmp085ReadInt(0xAC);
Serial.print(ac2);Serial.print(" ");  
  ac3 = bmp085ReadInt(0xAE);
  Serial.print(ac3);Serial.print(" ");
  ac4 = bmp085ReadInt(0xB0);
  Serial.print(ac4);Serial.print(" ");
  ac5 = bmp085ReadInt(0xB2);
  Serial.print(ac5);Serial.print(" ");
  ac6 = bmp085ReadInt(0xB4);
  Serial.print(ac6);Serial.print(" ");
  b1 = bmp085ReadInt(0xB6);
  Serial.print(b1);Serial.print(" ");
  b2 = bmp085ReadInt(0xB8);
  Serial.print(b2);Serial.print(" ");
  mb = bmp085ReadInt(0xBA);
  Serial.print(mb);Serial.print(" ");
  mc = bmp085ReadInt(0xBC);
  Serial.print(mc);Serial.print(" ");
  md = bmp085ReadInt(0xBE);
Serial.print(md);Serial.println();
}

// Calculate temperature in deg C
float bmp085GetTemperature(unsigned int ut){
  long x1, x2;

  x1 = (((long)ut - (long)ac6)*(long)ac5) >> 15;
  x2 = ((long)mc << 11)/(x1 + md);
  b5 = x1 + x2;

  float temp = ((b5 + 8)>>4);
  temp = temp /10;

  return temp;
}

// Calculate pressure given up
// calibration values must be known
// b5 is also required so bmp085GetTemperature(...) must be called first.
// Value returned will be pressure in units of Pa.
long bmp085GetPressure(unsigned long up){
  long x1, x2, x3, b3, b6, p;
  unsigned long b4, b7;

  b6 = b5 - 4000;
  // Calculate B3
  x1 = (b2 * (b6 * b6)>>12)>>11;
  x2 = (ac2 * b6)>>11;
  x3 = x1 + x2;
  b3 = (((((long)ac1)*4 + x3)<<OSS) + 2)>>2;

  // Calculate B4
  x1 = (ac3 * b6)>>13;
  x2 = (b1 * ((b6 * b6)>>12))>>16;
  x3 = ((x1 + x2) + 2)>>2;
  b4 = (ac4 * (unsigned long)(x3 + 32768))>>15;

  b7 = ((unsigned long)(up - b3) * (50000>>OSS));
  if (b7 < 0x80000000)
    p = (b7<<1)/b4;
  else
    p = (b7/b4)<<1;

  x1 = (p>>8) * (p>>8);
  x1 = (x1 * 3038)>>16;
  x2 = (-7357 * p)>>16;
  p += (x1 + x2 + 3791)>>4;

  long temp = p;
  return temp;
}

// Read 1 byte from the BMP085 at 'address'
char bmp085Read(unsigned char address)
{
  unsigned char data;

  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(address);
  Wire.endTransmission();

  Wire.requestFrom(BMP085_ADDRESS, 1);
  while(!Wire.available())
    ;

  return Wire.read();
}

// Read 2 bytes from the BMP085
// First byte will be from 'address'
// Second byte will be from 'address'+1
int bmp085ReadInt(unsigned char address)
{
  unsigned char msb, lsb;

  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(address);
  Wire.endTransmission();

  Wire.requestFrom(BMP085_ADDRESS, 2);
  while(Wire.available()<2)
    ;
  msb = Wire.read();
  lsb = Wire.read();

  return (int) msb<<8 | lsb;
}

// Read the uncompensated temperature value
unsigned int bmp085ReadUT(){
  unsigned int ut;

  // Write 0x2E into Register 0xF4
  // This requests a temperature reading
  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(0xF4);
  Wire.write(0x2E);
  Wire.endTransmission();

  // Wait at least 4.5ms
  delay(5);

  // Read two bytes from registers 0xF6 and 0xF7
  ut = bmp085ReadInt(0xF6);
  return ut;
}

// Read the uncompensated pressure value
unsigned long bmp085ReadUP(){

  unsigned char msb, lsb, xlsb;
  unsigned long up = 0;

  // Write 0x34+(OSS<<6) into register 0xF4
  // Request a pressure reading w/ oversampling setting
  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(0xF4);
  Wire.write(0x34 + (OSS<<6));
  Wire.endTransmission();

  // Wait for conversion, delay time dependent on OSS
  delay(2 + (3<<OSS));

  // Read register 0xF6 (MSB), 0xF7 (LSB), and 0xF8 (XLSB)
  msb = bmp085Read(0xF6);
  lsb = bmp085Read(0xF7);
  xlsb = bmp085Read(0xF8);

  up = (((unsigned long) msb << 16) | ((unsigned long) lsb << 8) | (unsigned long) xlsb) >> (8-OSS);

  return up;
}

void writeRegister(int deviceAddress, byte address, byte val) {
  Wire.beginTransmission(deviceAddress); // start transmission to device 
  Wire.write(address);       // send register address
  Wire.write(val);         // send value to write
  Wire.endTransmission();     // end transmission
}

int readRegister(int deviceAddress, byte address){

  int v;
  Wire.beginTransmission(deviceAddress);
  Wire.write(address); // register to read
  Wire.endTransmission();

  Wire.requestFrom(deviceAddress, 1); // read a byte

  while(!Wire.available()) {
    // waiting
  }

  v = Wire.read();
  return v;
}


