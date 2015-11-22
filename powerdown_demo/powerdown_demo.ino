/// @dir powerdown_demo
/// Sample code to completely power down a JeeNode (std or micro).
// 2010-08-16 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

#include <JeeLib.h>
#define LEDPIN 3       // LED signal pin

void setup () 
{
  pinMode(LEDPIN, OUTPUT);

}
   
//sleep ISR
ISR(WDT_vect) { Sleepy::watchdogEvent(); } // Setup for low power waiting

void loop () {

  int i=0;
  
  for(i=0;i<5;i++)
  {
    digitalWrite(LEDPIN,1);
    delay(1000);
    digitalWrite(LEDPIN,0);
    delay(1000);
  }
  
  for(i=0;i<5;i++)
  {
    digitalWrite(LEDPIN,1);
    Sleepy::loseSomeTime(1000);
    digitalWrite(LEDPIN,0);
    Sleepy::loseSomeTime(1000);
  }
    
  
 // stop responding to interrupts
    cli();
    
    // zzzzz... this code is now in the Ports library
    Sleepy::powerDown();


}
