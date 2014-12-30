/*
  RadioTime.h- Radio time code interface- Version 0.1
  DCF decode based on the Arduino DCF77 decoder v0.2 Copyright (C) 2006 Mathias Dalheimer (md@gonium.net)
  Input Capture support and modifications to a C++ class Copyright (c) 2008 Michael Margolis.  All right reserved.  
*/

/******* Sketch to receive radio clock time via Timer1 Input Capture interrupt handling *********************************
 * functionality: measure length of pulses on the ICP pin and decode using DCF77 format
 **************************************************************************************************************/

/* 
  
  The methods are:
  
 */

// ensure this library description is only included once
#ifndef RadioTime_h
#define RadioTime_h

#include <inttypes.h>
typedef uint8_t byte;
typedef uint8_t boolean;

#define rxNO_SIGNAL -1
#define rxRESET      0
#define rxNEW_MINUTE 60

/*
typedef enum {
	 rxNoSignal,      // no signal received in last 4 seconds
     rxAcquiring,     // signal being received but time not yet fully decoded 
	 rxTimeValid,     // time data is valid and ready to use 
	 rxNewMinute      // this is the first service since the start of a new minute;
  } rxSignalState_t;
*/

class RadioTime
{
private:	// User declarations
  // Clock variables 
//  byte previousSecond;
  byte bufferPosition;  // was int !!
  unsigned long long dcf_rx_buffer;

  void ResetState();
  void processSignal(byte signal);
  boolean finalizeBuffer(void);
public:		// User declarations
  // time vars: the received time is stored here!
  char tzOffset;  // offset in hours to the current time zone (uk = -1)
  volatile byte second;
  volatile byte minute;
  volatile byte hour;
  volatile byte day;
  volatile byte dow; // day of week
  volatile byte month;
  volatile unsigned int year;
  
  RadioTime();
  void Init();  	
  void Init(byte pin); // initialise with radio signal mirrored on given pin

  char service(); // must be called at least twice a second, returns -1 if waiting for signal, else second count, 60 is new minute
};

#endif
