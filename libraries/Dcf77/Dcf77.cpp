/*
Source code   	:   Dcf77.cpp
Author        		:   J.M. Lietaer, Belgium, Europe - jmlietaer (at) gmail (dot) com 
Date written  	:   March 06, 2010 
Library       		:   DCF77
                  Read data from a DCF77 module and display date/time 
*/

/*
Update from	:   Bernd Schwind, Germany, Europe - b.schwind (at) edvschwind (dot) de
Date		:   2011.01.23

Changes		:	insert Check bit and Plausi Check
				Version change to 0.2 beta
*/

/*
--- TO DO ---
- add bit 0, 15..19
- check bit 20, 28, 35, 58 for overall validity of DCF77 data
- modify return string to return_code+current_string+extra_bits+???
 */

//#include "WProgram.h"
#include <pins_arduino.h>
#include "Dcf77.h"
#include "stdlib.h"

  boolean isPulse = false;
  unsigned long tStart = 0;
  unsigned long tStop = 0;
  int tGap = 0;
  int tPulse = 0;
  int iDateTime[58];
  int i = 0;
  int n;
  char cDateTime[60];

Dcf77::Dcf77(int pin) {
  _pin=pin;
}

const char* Dcf77::version() {
  return "DCF77VERS 0.2 beta";
}

int Dcf77::signal() {
  return analogRead(_pin);
}

const char* Dcf77::getDateTime() {
  char temp[15];

  if (Dcf77::signal() > 200) {
    if (not isPulse) {
      isPulse = true;
      tStop = millis();
      tGap = tStop - tStart;
      if (tGap > 1500) {
        // set chek bit P3
		if (i==58) { // tGap ~ 1910
		  if (tGap > 1870) {
		    iDateTime[58] = 0;
		  }
		  else { // tGap ~ 1810
		    iDateTime[58] = 1;
		  }
		}
		
        // for i = 0 .. 58 --- decode iDateTime
		Dcf77::decodeDateTime();
        // reset iDateTime[i]
        for (i = 0; i < 59; i++) {
          iDateTime[i] = 0;
        }
        i = 0;
      }
      else {
 	    strcpy(cDateTime, "DCF77POLL");
        if (tGap > 850) { // tGap ~ 910
          iDateTime[i] = 0;
          i++;
        }
        else {
          if (tGap > 650) { // tGap ~ 810
            iDateTime[i] = 1;
            i++;
          }
        }
      }
      tStart = millis();
    }
    tStop = millis();
  }
  else {
    if (isPulse) {
      isPulse = false;
      tStop = millis();
      tPulse = tStop - tStart;
      tStart = millis();
    }
    tStop = millis();
  }
  return cDateTime;
}

const char* Dcf77::decodeDateTime() { 

  char temp[5];

  strcpy(cDateTime, "");
  strcpy(cDateTime, "DCF77DATA ");

  int r = 0;
 
  boolean p1 = false;
  boolean p2 = false;
  boolean p3 = false;
  boolean error = false;

  // year
  r = iDateTime[50] * 1 + iDateTime[51] * 2 + iDateTime[52] * 4 + iDateTime[53] * 8 +  iDateTime[54] * 10 + iDateTime[55] * 20 + iDateTime[56] * 40 + iDateTime[57] * 80 ;
  n=sprintf(temp, "%d", r + 2000);
  strncat(cDateTime, temp, strlen(temp));
  // Plausi check
  if (r<11) {
    error = true;
  }
  // month
  r = iDateTime[45] * 1 + iDateTime[46] * 2 + iDateTime[47] * 4 + iDateTime[48] * 8 +  iDateTime[49] * 10;
  n=sprintf(temp, "%d", r);
  if (r < 10) {
    n=sprintf(temp, "0%d", r);
    strncat(cDateTime, temp, strlen(temp));
  }
  else {
    n=sprintf(temp, "%d", r);
    strncat(cDateTime, temp, strlen(temp));
  }
  // Plausi check
  if (r < 1 || r > 12) {
    error = true;
  }
  
  // day
  r = iDateTime[36] * 1 + iDateTime[37] * 2 + iDateTime[38] * 4 + iDateTime[39] * 8 +  iDateTime[40] * 10 + iDateTime[41] * 20;
  if (r < 10) {
    n=sprintf(temp, "0%d", r);
    strncat(cDateTime, temp, strlen(temp));
  }
  else {
    n=sprintf(temp, "%d", r);
    strncat(cDateTime, temp, strlen(temp));
  }
  // Plausi check
  if (r < 1 || r > 31) {
    error = true;
  }
  

  // day of week
  r = iDateTime[42] * 1 + iDateTime[43] * 2 + iDateTime[44] * 4;
  n=sprintf(temp, "%d", r);
  strncat(cDateTime, temp, strlen(temp));
  // Plausi check
  if (r < 1 || r > 7) {
    error = true;
  }
  //P3 
  for (n=36;n<=57;n++) {
    if (iDateTime[n] == 1) {
	  p3 = !p3;
	}
  }

  // hour
  r = iDateTime[29] * 1 + iDateTime[30] * 2 + iDateTime[31] * 4 + iDateTime[32] * 8 +  iDateTime[33] * 10 + iDateTime[34] * 20;
  if (r < 10) {
    n=sprintf(temp, "0%d", r);
    strncat(cDateTime, temp, strlen(temp));
  }
  else {
    n=sprintf(temp, "%d", r);
    strncat(cDateTime, temp, strlen(temp));
  }
  // Plausi check
  if (r > 23) {
    error = true;
  }
  //P2
  for (n=29;n<=34;n++) {
    if (iDateTime[n] == 1) {
	  p2 = !p2;
	}
  }

  // minute
  r = iDateTime[21] * 1 + iDateTime[22] * 2 + iDateTime[23] * 4 + iDateTime[24] * 8 +  iDateTime[25] * 10 + iDateTime[26] * 20 + iDateTime[27] * 40;
  n=sprintf(temp, "%d", r);

  if (r < 10) {
    n=sprintf(temp, "0%d", r);
    strncat(cDateTime, temp, strlen(temp));
  }
  else {
    n=sprintf(temp, "%d", r);
    strncat(cDateTime, temp, strlen(temp));
  }
  // Plausi check
  if (r > 59) {
    error = true;
  }
  //P1
  for (n=21;n<=27;n++) {
    if (iDateTime[n] == 1) {
	  p1 = !p1;
	}
  }

  if (iDateTime[17] == 1) {
    strncat(cDateTime, "CEST", 4);
  }
  if (iDateTime[18] == 1) {
    strncat(cDateTime, "CET ", 4);
  }
  
// check bits check
   
  if (iDateTime[17] == iDateTime[18]) {
    error = true;
  }
  
  if (iDateTime[20] != 1) {
    error = true;
  }
  if (iDateTime[28] != p1) {
    error = true;
  }
  if (iDateTime[35] != p2) {
    error = true;
  }
  if (iDateTime[58] != p3) {
    error = true;
  }
  
//if error ......  
  if (error) {
    strcpy(cDateTime, "DCF77POLL");
  }
  return cDateTime; 

}

