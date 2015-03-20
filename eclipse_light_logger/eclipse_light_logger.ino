#include "Wire.h"
#include "BH1750.h"
#include "SdFat.h"
#include "RTClib.h"
#include <JeeLib.h>  // Include library containing low power functions

//pins
#define LEDPIN 3       // LED signal pin
#define DHTPIN 4       // Sensor pin
#define chipSelect 10  // SD card driver

//sleep ISR
ISR(WDT_vect) { Sleepy::watchdogEvent(); } // Setup for low power waiting

byte BH1750_Addr = 0x23;
unsigned int data = 0;
RTC_DS1307 rtc;
SdFat SD;

BH1750 Sensor ( BH1750_Addr, CONT_HR_1);

void setup()
{
  Wire.begin();
  rtc.begin();
  Serial.begin(9600);
  SD.begin(chipSelect);
  Sensor.begin();
  delay(300);
  
}

void loop()
{
   DateTime now = rtc.now();
  char timebuffer[16];
  char filename[32];
  
  //make the filename from the date
  sprintf(filename, "%04u%02u%02u.csv", now.year(), now.month(), now.day());
  
  // format the time into the string
  sprintf(timebuffer, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
   // make a string for assembling the data to log:
  String dataString = "";
  int hh = Sensor.read();      //multiply up the readings to make an INT, floats are a pain
  
  //save the time & readings
  dataString += timebuffer;
  dataString += ",";
  dataString += String(hh);

 
  // open the file. Reopening and closing each loop is inefficient,
  // but is easiest to implement and realiable for poweroff.
  File dataFile = SD.open(filename, FILE_WRITE);
  // if the file is available, write to it:
  if (dataFile) {
  dataFile.println(dataString);
  dataFile.close();
  }
 // data = Sensor.read();
  Serial.println(hh,DEC);
  delay (1000);
  
}


