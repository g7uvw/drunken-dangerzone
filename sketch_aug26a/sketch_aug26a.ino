#include "Wire.h" 
#include "math.h"

 
unsigned int mode = 0x10;

int BH1750_Device = 0x23; // I2C address for light sensor
int iCheck = 0;  // iCheck = 0 for Lux, iCheck = 1 for Foot-Candles
unsigned int Lux, Scaled_FtCd;
float FtCd, Wattsm2;
int IntPin = 0;  // Pin 2

 
 
void setup()
{
  Wire.begin();
  Serial.begin(9600);
  
  Configure_BH1750();
  //Attach the interrupt to the input pin and monitor for ANY Change
 
  delay(300);
 
}
 
void loop()
{
  int i;
  Lux = BH1750_Read();
  FtCd = Lux/10.764;
  Wattsm2 = Lux/683.0;
  Serial.print(Lux,DEC);     
  Serial.println("[lx]"); 
  //Serial.print(FtCd,2);     
  //Serial.println("[FC]");
  //Serial.print(Wattsm2,4);     
  //Serial.println("[Watts/m^2]"); 
  delay(1000);
}
 
unsigned int BH1750_Read() //
{
  unsigned int i=0;
  Wire.beginTransmission(BH1750_Device);
  Wire.requestFrom(BH1750_Device, 2);
  while(Wire.available()) //
  {
    i <<=8;
    i|= Wire.read();  
  }
  Wire.endTransmission();  
  return i/1.2;  // Convert to Lux
}
 

 
void Configure_BH1750() 
{
  Wire.beginTransmission(BH1750_Device);
  Wire.write(mode);      // Set resolution to 1 Lux
  Wire.endTransmission();
}
