#include "Wire.h"
#include "BH1750.h"
#define BH1750_address 0x23

BH1750 Sensor(BH1750_address, CONT_HR_HALF);
void setup()
{
  Wire.begin();
  Serial.begin(9600);
  delay(300);
}

void loop()
{
  
  float Lux = Sensor.read();
  float Wattsm2 = Lux/683.0;
  Serial.print(Lux,4);     
  Serial.println("[lx]"); 
  Serial.print(Wattsm2,4);
  Serial.println("[Watts/m^2]"); 
  delay(1000);
}




