#include "Wire.h"
#include "BH1750.h"

byte BH1750_Addr = 0x23;
unsigned int data = 0;

BH1750 Sensor ( BH1750_Addr, CONT_HR_1);

void setup()
{
  Wire.begin();
  Serial.begin(9600);
  Sensor.begin();
  delay(300);
  
}

void loop()
{
  data = Sensor.read();
  Serial.println(data,DEC);
  delay (500);
  
}


