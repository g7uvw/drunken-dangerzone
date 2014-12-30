#include "BH1750.h"
#include "Arduino.h"



BH1750::BH1750(byte address, Mode opmode)
{
	
	_mymode = opmode;
	BH1750::_address = address;

}

void BH1750::begin(void)
{
	if (!_initialised)
	{
	poweron();
	reset();
	}
	setmode();
	_initialised = true;
}


void BH1750::reset()
{
	//Serial.print("In reset, address = ");
	Wire.beginTransmission(BH1750::_address);
  	Wire.write(Mode(RESET));
  	Wire.endTransmission();	
  	delay(100);
	
}

void BH1750::setmode(void)
{
	//Serial.print("In setmode, address = ");
	//Serial.println(BH1750::_address);
	//Serial.print("Mode = ");
	//Serial.println(Mode(_mymode));
	Wire.beginTransmission(BH1750::_address);
	Wire.write(Mode(_mymode));
	Wire.endTransmission();	
	delay(100);
	
}

void BH1750::poweron(void)
{
	//Serial.print("In Poweron, address = ");
	//Serial.println(BH1750::_address);
	Wire.beginTransmission(BH1750::_address);
  	Wire.write(Mode(POWER_ON));
  	Wire.endTransmission();
  	delay(100);
}

void BH1750::poweroff(void)
{
	Wire.beginTransmission(BH1750::_address);
  	Wire.write(Mode(POWER_DOWN));
  	Wire.endTransmission();	
}

unsigned int BH1750::read(void)
{

	unsigned int data = 0;
	Wire.beginTransmission(BH1750::_address);
	Wire.requestFrom(BH1750::_address, 2);
  	while(Wire.available())
  	{
  	//Serial.print(data,DEC);
  	//Serial.print(" ");
    data <<=8;
    data|= Wire.read();  
  	}
  	//Serial.println();
  	Wire.endTransmission();  
  	return data / 1.2;		//from datasheet conv to Lux
  	
  	//there's nothing to really be gained by doing calc as a float, it seems
  	//float ans = 1.0*data;
  	//return (int) (ans/1.2);  //from datasheet conv to Lux
}