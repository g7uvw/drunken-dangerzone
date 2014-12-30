#include <RadioTime.h>

RadioTime MSF = RadioTime();


void setup(){
   Serial.begin(9600);
   MSF.Init(13); // flash LED on pin 13 to show input pulses
}
  
void loop(){
  static int secs, oldsecs;

   secs = MSF.service();
   if(secs == rxNO_SIGNAL){
	Serial.print("Waiting for signal ");
	while( MSF.service() == rxNO_SIGNAL)   // wait here for a signal
	   delay(100);  
   }    
   else if(secs != oldsecs ){
	Serial.print(MSF.hour);
	Serial.print(":");
	Serial.print(MSF.minute);
	Serial.print(":");
	Serial.println(MSF.second);
   }
}
