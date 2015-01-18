// Archives datalogger

/* This uses the GPL SD card library. The rest of the code aside from the libraries and the 
sleep stuff (see later) is so simple I can't claim copyright or anything on it. Do what you want with it.
Except remove this notice
David Mills 2015 http://webshed.org
*/


// Sleepy stuff comes from :
/*
 Watchdog Sleepy Bones (the skeleton is the essential armature for any living thing)
 This is the basic structure that you need to Sleep_Mode_Pwr_Down/Watchdog interrupt wake
 Provided for general consumption by Rachel's Electronics http://www.rachelselectronics.com
 Under the CC licence here (http://creativecommons.org/licenses/by-sa/3.0/us/)
 Rachel's Electronics: Making the World Safe for Robots! -Joel Murphy joel@joelmurphy.net
 
          Built in part from code posted by: 
           * KHM 2008 / Lab3/  Martin Nawrath nawrath@khm.de
           * Kunsthochschule fuer Medien Koeln
           * Academy of Media Arts Cologne 
             http://interface.khm.de/index.php/lab/experiments/sleep_watchdog_battery/
*/


//includes
#include "DHT.h"
#include "RTClib.h"
#include <Wire.h>
//#include <SD.h>  //buggy uses too much power
#include "SdFat.h"
//#include <avr/sleep.h>
//#include <avr/wdt.h>
#include <JeeLib.h>  // Include library containing low power functions

//sleep details
#define sleepTime  3                       //number of 8 second sleep cycles
volatile byte wdt=0;                       //used to cound number of sleep cycles


//pins

#define LEDPIN 3       // SLED signal pin
#define DHTPIN 4       // Sensor pin
#define chipSelect 10  // SD card driver

//devices
#define DHTTYPE DHT22   

//objects
DHT dht(DHTPIN, DHTTYPE);
RTC_DS1307 rtc;
SdFat SD;
ISR(WDT_vect) { Sleepy::watchdogEvent(); } // Setup for low power waiting
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
   //Serial.begin(9600);

  //all setup now - kick the watchdog to get it ready to wake us 
  //setup_watchdog();                         // set prescaller and enable interrupt                  
  //set_sleep_mode(SLEEP_MODE_PWR_DOWN);       // sleep mode is set here Power Down uses the least current
                                             // system clock is turned off, so millis won't be reliable!
  //delay(10);
}

void loop()
{
  
      
  //Sleepy::loseSomeTime(10000L);      // Instead of delay(1000); 
  //system_sleep();
  digitalWrite(LEDPIN,1);
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  // check if returns are valid, if they are NaN (not a number) then something went wrong!
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
  
   // check if returns are valid, if they are NaN (not a number) then something went wrong!
  //if (isnan(t) || isnan(h)) {
  //  Serial.println("Failed to read from DHT");
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
  int hh = (h*100);
  int tt = (t*100);
  
  //save the time 
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
  digitalWrite(LEDPIN,0);
  //delay(5000);
  //sleep for ~10min
  for (byte i = 0; i < 10; ++i)
      Sleepy::loseSomeTime(60000);
}


//sleep code
//void system_sleep() {
//  ADCSRA |= (0<<ADEN);                     // disable ADC
//  sleep_enable();                          // enable sleeping
//  sleep_mode();                            // activate system sleep
//  // sleeping ... 
//  // first action after leaving WDT Interrupt Vector:
//  if (wdt == sleepTime) {                  // sleep for this number times 8 seconds
//    sleep_disable();                       // disable sleep  
//    //ADCSRA |= (1<<ADEN);                   // switch ADC on    
//    wdt = 0;                               // reset watchdog counter   
//  }
//}


//void setup_watchdog() {
//cli(); //disable global interrupts
//  MCUSR = 0x00;  //clear all reset flags 
//  //set WD_ChangeEnable and WD_resetEnable to alter the register
//  WDTCSR |= (1<<WDCE) | (1<<WDE);   // this is a timed sequence to protect WDTCSR
//  // set new watchdog timeout value to 1024K cycles (~8.0 sec)
//  WDTCSR = (1<<WDP3) | (1<<WDP0);
//  //enable watchdog interrupt
//  WDTCSR |= (1<<WDIE);    
//sei(); //enable global interrupts
//}

// Watchdog Interrupt Service Routine. 
// Very first thing after sleep wakes with WDT Interrupt
//ISR(WDT_vect) {
//  wdt++;  // increment the watchdog timer
//}

