/* Arduino DCF77 library v0.1
 * Copyright (C) 2006 Mathias Dalheimer (md@gonium.net)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#undef abs
#include "WConstants.h"
#include "HardwareSerial.h"
// include this library's description file
#include "DCF77.h"

DCF77::DCF77(unsigned char dcfPin){
	DCF77Pin = dcfPin;
	previousSignalState=0;
	previousFlankTime=0;
	bufferPosition=0;
	dcf_rx_buffer=0;
	ss=mm=hh=day=mon=year=0;
	Serial.begin(9600);
#ifdef DCF_DEBUG 
	Serial.println("Initializing DCF77 lib");
	Serial.print("Using DCF77 pin #");
	Serial.println(DCF77Pin);
#endif
	pinMode(DCF77Pin, INPUT);
}

void DCF77::appendSignal(unsigned char signal) {
#ifdef DCF_DEBUG
	Serial.print(", appending value ");
	Serial.print(signal);
	Serial.print(" at position ");
	Serial.println(bufferPosition);
#endif
	dcf_rx_buffer = dcf_rx_buffer | ((unsigned long long) signal << bufferPosition);
  bufferPosition++;
	if (bufferPosition > 59) {
		finalizeBuffer();
	}
}

void DCF77::finalizeBuffer(void) {
#ifdef DCF_DEBUG
	Serial.println("Finalizing Buffer");
#endif
  if (bufferPosition == 59) {
  	struct DCF77Buffer *rx_buffer;
		rx_buffer = (struct DCF77Buffer *)(unsigned long long)&dcf_rx_buffer;
		//convert the received bits from BCD
		mm = rx_buffer->Min-((rx_buffer->Min/16)*6);
		hh = rx_buffer->Hour-((rx_buffer->Hour/16)*6);
		day= rx_buffer->Day-((rx_buffer->Day/16)*6); 
		mon= rx_buffer->Month-((rx_buffer->Month/16)*6);
		year= 2000 + rx_buffer->Year-((rx_buffer->Year/16)*6);
	}
	// reset stuff
	ss = 0;
	bufferPosition = 0;
	dcf_rx_buffer=0;
}

void DCF77::serialDumpTime(void){
  Serial.print("Time: ");
  Serial.print(hh, DEC);
  Serial.print(":");
  Serial.print(mm, DEC);
  Serial.print(":");
  Serial.print(ss, DEC);
  Serial.print(" Date: ");
	Serial.print(day, DEC);
	Serial.print(".");
	Serial.print(mon, DEC);
	Serial.print(".");
	Serial.println(year, DEC);
}

int DCF77::scanSignal(void){
	unsigned char DCFsignal = digitalRead(DCF77Pin);
	if (DCFsignal != previousSignalState) {
	  if (DCFsignal == 1) {
			/* We detected a raising flank, increase per one second! */
			addSecond();
#ifdef DCF_DEBUG
			serialDumpTime();
#endif
			int thisFlankTime=millis();
			if (thisFlankTime - previousFlankTime > DCF_sync_millis) {
#ifdef DCF_DEBUG
				Serial.println("####");
				Serial.println("#### Begin of new Minute!!!");
				Serial.println("####");
#endif
				finalizeBuffer();
			}
      previousFlankTime=thisFlankTime;
#ifdef DCF_DEBUG
      Serial.print(previousFlankTime);
	    Serial.print(": DCF77 Signal detected, ");
#endif
	  } else {
			/* or a falling flank */
	    int difference=millis() - previousFlankTime;
#ifdef DCF_DEBUG
      Serial.print("duration: ");
	    Serial.print(difference);
#endif
			if (difference < DCF_split_millis) {
				appendSignal(0);
			} else {
				appendSignal(1);
			}
    }
    previousSignalState = DCFsignal;
  }
	return DCFsignal;
}

/**
 * ########################################################
 * ###                 Helpers below                    ###
 * ########################################################
 */

/**
 * Add one second to the hh:mm:ss variables.
 */
void DCF77::addSecond() {
	ss++;
	if (ss==60) {
		ss=0;
		mm++;
		if (mm==60) {
			mm=0;
			hh++;
			if (hh==24) 
				hh=0;
		}
	}
}
