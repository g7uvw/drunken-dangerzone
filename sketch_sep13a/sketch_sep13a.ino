#include <LiquidCrystal.h>
#include <OneWire.h>
/*
 The circuit:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)
 */
 
 #define RS 8
 #define ENABLE 9
 #define D4 10
 #define D5 11
 #define D6 12
 #define D7 13
 


// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(RS,ENABLE,D4,D5,D6,D7);

void setup() 
{
  Serial.begin(9600); 
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  pinMode(A0, OUTPUT);
  digitalWrite(A0, LOW);
  

}

void loop() 
{
  
  if (Serial.available() > 0) 
  {
    int inByte = Serial.read();
     switch (inByte) 
     {
    case 'X':    
     pinMode(A0, OUTPUT);
     digitalWrite(A0, HIGH);
     lcd.print("X-Rays OFF");
     delay(1000);
     digitalWrite(A0, LOW);
     break;
     default:
     digitalWrite(A0, LOW);
     }
  }
    

}
