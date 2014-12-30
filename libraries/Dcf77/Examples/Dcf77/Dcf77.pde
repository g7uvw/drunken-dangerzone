/*
 Author          :	J.M. Lietaer, Belgium, Europe - jmlietaer (at) gmail (dot) com 
 User level      :      Total Arduino Noob
 Date written    :	March 06, 2010 
 Program	 :      Example program for DCF77 Library
                        DCF77 - Read data from a DCF77 module and display date/time 
 System          :      Arduino Duemilanove ATMEGA328 
 Software        :      Arduino IDE 0018 on Apple Mac OS X 10.6.2
 Tested with	 :      Pollin DCF1 - article number 810054
 Information	 :      Please feel free to comment and suggest improvements 
 Released under  :      Creative Commons Attribution - Share Alike 2.0 Belgium License
 */

/*
Example of output:
- Result of calling getDateTime()
   DCF77DATA 2010030661531CET
             CCYYMMDDdhhmmTTTT
- Result of calling version()
   DCF77VERS 0.1 beta
- Return of getDateTime() while capturing DCF77 data
   DCF77POLL
 */

#include "Dcf77.h"

Dcf77 dcf77(1); 

void setup() {
  Serial.begin(9600);
}

void loop() {
  const char *v = dcf77.getDateTime();
  if (strcmp (v,"DCF77POLL") != 0) {
    Serial.println(v);  
  }
}

