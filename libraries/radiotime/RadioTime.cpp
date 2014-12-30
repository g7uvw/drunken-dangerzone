/*
  RadioTime.cpp- Radio time code interface- Version 0.1
  DCF decode based on the Arduino DCF77 decoder v0.2 Copyright (C) 2006 Mathias Dalheimer (md@gonium.net)
  Input Capture support and modifications to a C++ class Copyright (c) 2008 Michael Margolis.  All right reserved.  
*/

/******* Sketch to receive radio clock time via Timer1 Input Capture interrupt handling *********************************
 * functionality: measure length of pulses on the ICP pin and decode using DCF77 format
 **************************************************************************************************************/

//#define DCF_DEBUG

#include <arduino.h>
#include <avr/interrupt.h>
#include "RadioTime.h"

 //extern void debugOut(char* s, byte icrPulseCount, byte icrGapCount, byte bufferPosition); 

#define icpPin 8        // ICP input pin on arduino

// Input capture uses Timer1, we prescale 16mhz clock by 1024 go give approx 16us tick
#define PRESCALE (_BV(CS12)| _BV(CS10))  // prescale = 1025, 1 tick = 64us
#define ICR1H_TIC   16   // number of milliseconds per each tic of the most significant byte of ICR counter)


// defines for pulse durations
#define DCF_PULSE0_MILLIS  100 
#define DCF_PULSE1_MILLIS  200  

#define DCF_BAD_SIG   ((DCF_PULSE0_MILLIS * 3) /4)   // pulse less than this are considered noise
#define DCF_BAD_SIG_THRESHOLD   (DCF_BAD_SIG/ICR1H_TIC)   // number of tics of TCNT1H for above period

#define DCF_PULSE1_THRESHOLD  (150/ICR1H_TIC)  // Halfway between the pulse durations

#define DCF_GAP_SECS_MILLIS 700 // gaps less then this are bad signal
#define DCF_GAP_SECS_COUNT    (DCF_GAP_SECS_MILLIS/ICR1H_TIC)   // tics of TCNT1H for minute gap

#define DCF_GAP_MIN_MILLIS 1700  // pulse greater than this is the minute marker gap)
#define DCF_GAP_MIN_COUNT    (DCF_GAP_MIN_MILLIS/ICR1H_TIC)   // tics of TCNT1H for minute gap

#define DCF_GAP_MAX_MILLIS 2000  // pulse greater than this are no signal)
#define DCF_GAP_MAX_COUNT      (DCF_GAP_MAX_MILLIS/ICR1H_TIC)   // tics of TCNT1H for minute gap


struct DCF77Buffer {
  unsigned long long prefix	:21;
  unsigned long long Min	:7;	// minutes
  unsigned long long P1		:1;	// parity minutes
  unsigned long long Hour	:6;	// hours
  unsigned long long P2		:1;	// parity hours
  unsigned long long Day	:6;	// day
  unsigned long long Weekday	:3;	// day of week
  unsigned long long Month	:5;	// month
  unsigned long long Year	:8;	// year (5 -> 2005)
  unsigned long long P3		:1;	// parity
};
struct DCFflags{
	unsigned char parity_flag	:1;
	unsigned char parity_min	:1;
	unsigned char parity_hour	:1;
	unsigned char parity_date	:1;
} flags;

// variables to store whats happening in the interrupt handlers 
static volatile byte  icrPulseCount;   // this stores the current ICR1 most significant byte
static volatile byte  icrGapCount;   // this stores the current ICR1 most significant byte, (TODO use to check if lost or invalid signal) 
static volatile boolean endOfMinute;  // flag set true if end of minute
//static volatile rxSignalState_t rxState;   //enumerated type indicating receive status

static char outPin;                //if >=0 then mirror signal changes on this pin

/* Overflow interrupt vector */
ISR(TIMER1_OVF_vect){                 // here if no input pulse detected after 4 seconds
  //rxState = rxNoSignal; 
}

/* ICR interrupt vector */
ISR(TIMER1_CAPT_vect){
  TCNT1 = 0;                            // reset the counter
  if( bit_is_set(TCCR1B ,ICES1)){       // was rising edge detected ?   
    icrPulseCount = ICR1/256;  
	if( outPin >=0)
       digitalWrite(outPin,LOW );      // yes, set our output pin low to indicate end of pulse
  }
  else {                                // falling edge was detected    
    icrGapCount = ICR1/256;        
	if(icrGapCount > DCF_GAP_MIN_COUNT) 
	   endOfMinute = true; 
	if( outPin >=0)
       digitalWrite(outPin,HIGH );       // set our output pin high to blink at start of pulse   
  }       
  TCCR1B ^= _BV(ICES1);                 // toggle bit value to trigger on the other edge    
}

 RadioTime::RadioTime( )
{ 
  outPin = -1; // default value for pin disable output
}

	
void RadioTime::Init(byte pin)
{
   outPin = pin;   // todo check this works with an without given pin
   pinMode(outPin, OUTPUT);
   this->Init();
}

void RadioTime::Init()
{
  pinMode(icpPin, INPUT);                // ICP pin (digital pin 8 on arduino) as input
  endOfMinute = false;
  ResetState();  
  icrPulseCount = icrGapCount = 0;
  //rxState = rxNoSignal; 
  second=minute=hour=day=dow=month=year=0;

  TCCR1A = 0 ;                    // this register set to 0!
  TCCR1B = PRESCALE ;    // NORMAL MODE!!, prescaller 1024, rising edge ICP1 
  TCCR1B |= _BV(ICES1);           // enable input capture     

  TIMSK1 =  _BV(ICIE1);                         // enable input capture interrupt for timer 1
  TIMSK1 |= _BV(TOIE1);                         // enable overflow interrupt to detect missing input pulses 
}

void RadioTime::ResetState() {
      bufferPosition = 0;
      dcf_rx_buffer=0;	  
}

/**
 * Append a signal to the dcf_rx_buffer. Argument is 0 or 1. An internal
 * counter shifts the writing position within the buffer. If position > 59,
 * a new minute begins -> time to call finalizeBuffer().
 */
void RadioTime::processSignal(byte signal) {
 
    dcf_rx_buffer = dcf_rx_buffer | ((unsigned long long) signal << bufferPosition);
    // Update the parity bits. First: Reset when minute, hour or date starts.
    if (bufferPosition ==  21 || bufferPosition ==  29 || bufferPosition ==  36) {
      flags.parity_flag = 0;
    }
    // save the parity when the corresponding segment ends
    if (bufferPosition ==  28) {
      flags.parity_min = flags.parity_flag;
    };
    if (bufferPosition ==  35) {
      flags.parity_hour = flags.parity_flag;
    };
    if (bufferPosition ==  58) {
      flags.parity_date = flags.parity_flag;
    };
    // When we received a 1, toggle the parity flag
    if (signal == 1) {
      flags.parity_flag = flags.parity_flag ^ 1;
    }
	//if( rxState == 0)
	//	rxState = rxAcquiring;  // flag returned in service routine to tell the app that we are receiving signals

    bufferPosition++;
#ifdef NEEDED
    if (bufferPosition > 59) {
        this->finalizeBuffer();
    }
#endif
  
}
/**
 * Evaluates the information stored in the buffer. This is where the DCF77
 * signal is decoded and the internal clock is updated.
 */

void TimeZoneOffset(){


}

boolean RadioTime::finalizeBuffer(void) {
  boolean ret = false;  // default return code unless data validates ok
  if (bufferPosition == 59) {
    struct DCF77Buffer *rx_buffer;
    rx_buffer = (struct DCF77Buffer *)(unsigned long long)&dcf_rx_buffer;
    if (flags.parity_min == rx_buffer->P1  &&
        flags.parity_hour == rx_buffer->P2  &&
        flags.parity_date == rx_buffer->P3 ) 
    { 
      //convert the received bits from BCD
      minute = rx_buffer->Min-((rx_buffer->Min/16)*6);
      hour = rx_buffer->Hour-((rx_buffer->Hour/16)*6);
      day= rx_buffer->Day-((rx_buffer->Day/16)*6); 
      dow= rx_buffer->Weekday-((rx_buffer->Weekday/16)*6); 
      month= rx_buffer->Month-((rx_buffer->Month/16)*6);
      year= 2000 + rx_buffer->Year-((rx_buffer->Year/16)*6);
      TimeZoneOffset(); // add or subtract time zone offset
	  ret = true;  // data is valid
    }
  } 
  // reset stuff
  second = 0;
  bufferPosition = 0;
  dcf_rx_buffer=0;
  return ret;
}

/**
 * Dump the time to the serial line.
 */
#ifdef NEEDED
void serialDumpTime(void){
  Serial.println("");
  Serial.print("Time: ");
  hour -=1;   // correct for UK time if using DCF
  if( hour == 0)  // TODO need to check for correct day month & year!!!!
     hour = 23; 
  Serial.print(hour, DEC);
  Serial.print(":");
  if(minute < 10)
     Serial.print("0"); 
  Serial.print(minute, DEC);
  Serial.print(":");
  if(ss < 10)
     Serial.print("0"); 
  Serial.print(second, DEC);
    Serial.print(" Day of week: ");
  Serial.print(dow, DEC);
  Serial.print(" Date: ");
  Serial.print(day, DEC);
  Serial.print(".");
  Serial.print(month, DEC);
  Serial.print(".");
  Serial.println(year, DEC);
}
#endif

char RadioTime::service(){  // returns second count , -1 if signal, 0 if reset by next minute signal 
 
  char ret;
  if( bufferPosition < 59)
     ret = bufferPosition+ 1;  // don't return 60 until minute signal received 
  else 
    ret = bufferPosition;

  if( icrPulseCount > 0){
	  if( icrPulseCount < DCF_BAD_SIG_THRESHOLD ){
//         debugOut("bad sig ", icrPulseCount, icrGapCount,bufferPosition); 
//		 debugOut("thresholds: ", DCF_BAD_SIG_THRESHOLD,  DCF_GAP_MIN_COUNT,  DCF_GAP_MAX_COUNT ); 
         ResetState();
		 ret = rxNO_SIGNAL;
	  }
	  else{
          unsigned char signal = icrPulseCount > DCF_PULSE1_THRESHOLD ? 1: 0;   // signal is 0 if duration is less than count (~150ms)
	      //debugOut("ok ", icrPulseCount, signal,bufferPosition); 	     
          processSignal(signal);
		  if( bufferPosition < 59)
	         ret = bufferPosition+ 1;  // don't return 60 until minute signal received 
           else 
	          ret = bufferPosition;
	  }
	  icrPulseCount = 0;  // reset the count ready for the next signal
  }
  if(endOfMinute ){
	  // end of minute
	   //debugOut("end of minute! ", icrPulseCount, icrGapCount,bufferPosition); 
	   endOfMinute = false;
	   icrGapCount = 0;  // reset the count ready for the next signal
       if(bufferPosition == 59) {  
          if( finalizeBuffer()) 	  
		     ret = rxNEW_MINUTE; // this indicates new minute 
		  else
              ret = rxRESET; // finalize did not validate the data	
        }
        else {       
		   ResetState();   // end of minute marker before all 59 pulses received so start over        
		   ret = rxRESET; // this will return 0		          
        }
   }
   return ret;
}


