#include <RadioTime.h>



RadioTime DCF = RadioTime();

void setup(){
   Serial.begin(9600);
   DCF.Init(13); // flash LED on pin 13 to show input pulses
}
  
void loop(){
  static int secs, oldsecs;

   secs = DCF.service();
   if(secs == rxNO_SIGNAL){
	Serial.print("Waiting for signal ");
	while( DCF.service() == rxNO_SIGNAL)   // wait here for a signal
	   delay(100);  
   }    
   else if(secs != oldsecs ){
	Serial.print(DCF.hour);
	Serial.print(":");
	Serial.print(DCF.minute);
	Serial.print(":");
	Serial.println(DCF.second);
   }
} 
