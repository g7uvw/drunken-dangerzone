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

#ifndef DCF77_h
#define DCF77_h

/**
 * Turn debugging on or off
 */
//#define DCF_DEBUG 1
/**
 * Number of milliseconds to elapse before we assume a "1",
 * if we receive a falling flank before - its a 0.
 */
#define DCF_split_millis 140
/**
 * There is no signal in second 59 - detect the beginning of 
 * a new minute.
 */
#define DCF_sync_millis 1200
/**
 * DCF time format struct
 */
struct DCF77Buffer {
		unsigned long long prefix	:21	;
		unsigned long long Min		:7	;	//7 Bits für die Minuten
		unsigned long long P1			:1	;	//Parity Minuten
		unsigned long long Hour		:6	;	//6 Bits für die Stunden
		unsigned long long P2			:1	;	//Parity Stunden
		unsigned long long Day		:6	;	//6 Bits für den Tag
		unsigned long long Weekday	:3	;	//3 Bits für den Wochentag 
		unsigned long long Month		:5	;	//3 Bits für den Monat
		unsigned long long Year		:8	;	//8 Bits für das Jahr **eine 5 für das Jahr 2005**
		unsigned long long P3			:1	;	//Parity von P2
};

class DCF77 {	
	private:
		unsigned char DCF77Pin;
		unsigned char blinkPin;
		unsigned char previousSignalState;
		int previousFlankTime;
		int bufferPosition;
		unsigned long long dcf_rx_buffer;
		void appendSignal(unsigned char);
		void addSecond();
		void finalizeBuffer(void);
	public:
		unsigned char ss;
		unsigned char mm;
		unsigned char hh;
		unsigned char day;
		unsigned char mon;
		unsigned int year;
	  /**
	   * Initialize the DCF77 library. Provide the pin number of the 
	   * pin where the DCF77 signal occurs.
	   */
		DCF77(unsigned char dcfPin);
		/**
		 * Should be called every 15 ms in order to sample the DCF signal.
		 * Returns the current state of the signal: 1 for HIGH, 0 for LOW.
		 */
		int scanSignal(void);
		/**
		 * dumps the current time to serial.
		 */
		void serialDumpTime(); 
};

#endif
