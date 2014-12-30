#include <DCF77.h>

#define redPin 13


int DCF77Pin=7;
int blinkPin=13;
int seconds=0;
int previousSecond =0;
int minutes=0;
int hours=0;
DCF77 myDCF=DCF77(DCF77Pin);


void setup(void) {
  pinMode(blinkPin, OUTPUT);
}



void loop(void) {
  int DCFsignal = myDCF.scanSignal();
  if (DCFsignal) {
    digitalWrite(blinkPin, HIGH);
  } else {
    digitalWrite(blinkPin, LOW);
  }
  hours=myDCF.hh;
  minutes=myDCF.mm;
  seconds=myDCF.ss;
  if (seconds != previousSecond) 
    myDCF.serialDumpTime();
  delay(20);
  previousSecond = seconds;
}
