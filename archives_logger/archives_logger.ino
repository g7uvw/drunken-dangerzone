// Archives datalogger

/* This uses the GPL SD card library. The rest of the code aside from the libraries is so simple I can't claim 
copyright or anything on it. Do what you want with it.
Except remove this notice
David Mills Jan 2015 http://webshed.org
*/

//includes
#include "DHT.h"
#include "RTClib.h"
#include <Wire.h>
//#include <SD.h>  //buggy uses too much power
#include "SdFat.h"
#include <JeeLib.h>  // Include library containing low power functions

//pins
#define LEDPIN 3       // LED signal pin
#define DHTPIN 4       // Sensor pin
#define chipSelect 10  // SD card driver

//sleep ISR
ISR(WDT_vect) { Sleepy::watchdogEvent(); } // Setup for low power waiting

//devices
#define DHTTYPE DHT22   

//objects
DHT dht(DHTPIN, DHTTYPE);
RTC_DS1307 rtc;
SdFat SD;


void setup()
{
   // make sure the chipselect pin is enabled correctly 
   pinMode(chipSelect, OUTPUT);
   // Signal LED pin
   pinMode(LEDPIN, OUTPUT);
   
   // start drivers
   Wire.begin();
   rtc.begin();
   dht.begin();
   SD.begin(chipSelect);
   //Serial.begin(9600);  //uncoment for debugging
}

void loop()
{
  digitalWrite(LEDPIN,1);  //flash LED to show we're doing something - minimal UI!
  
  // Read DHT22 Sensor
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  // check for valid data
  if (isnan(t) || isnan(h)) 
  {  
      //flag the error with invalid data
      t = 0.0;
      h = 0.0;  
  }
  
  DateTime now = rtc.now();
  char timebuffer[16];
  char filename[32];
  
  //make the filename from the date
  sprintf(filename, "%04u%02u%02u.csv", now.year(), now.month(), now.day());
  
  // format the time into the string
  sprintf(timebuffer, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  
  //uncomment beflow for debugging
   //  // check for valid data
  //if (isnan(t) || isnan(h)) {
  //  Serial.println("Error reading DHT22");
  //} else {
  //  Serial.print("Humidity: "); 
  //  Serial.print(h);
  //  Serial.print(" %\t");
  //  Serial.print("Temperature: "); 
  //  Serial.print(t);
  //  Serial.println(" *C");
 // }
  
  // make a string for assembling the data to log:
  String dataString = "";
  int hh = (h*100);      //multiply up the readings to make an INT, floats are a pain
  int tt = (t*100);
  
  //save the time & readings
  dataString += timebuffer;
  dataString += ",";
  dataString += String(hh);
  dataString += ",";
  dataString += String(tt);
 
  // open the file. Reopening and closing each loop is inefficient,
  // but is easiest to implement and realiable for poweroff.
  File dataFile = SD.open(filename, FILE_WRITE);
  // if the file is available, write to it:
  if (dataFile) {
  dataFile.println(dataString);
  dataFile.close();
  }
  
  digitalWrite(LEDPIN,0);  //tun off the LED, we're done

  //sleep for ~10min
  for (byte i = 0; i < 10; ++i)
      Sleepy::loseSomeTime(60000);
}
