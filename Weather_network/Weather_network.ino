// Read a bunch of sensors and send the results out over NRF24L link
// Max data size is 32 bytes, so we can send a few packets if needed.

#include <stddef.h>
#include <Wire.h>
#include "DHT.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <RF24Network.h>
#include <RF24.h>

// Sensor Types for Bit Mask
#define DS18X20	0x01
#define DHT_11	0x02
#define DHT_22	0x04
#define SHT_11	0x08
#define SHT_15	0x10
#define BMP_085	0x20
#define LUX	0x40


// Humidity sensor setup
#define DHTPIN 2     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define BMP085_ADDRESS 0x77  // I2C address of BMP085
DHT dht(DHTPIN, DHTTYPE);

// DS18x20 sensor configuration
// Data wire is plugged into port 3 on the Arduino
#define ONE_WIRE_BUS 3
#define TEMPERATURE_PRECISION 12 //12 bits if DS18B20 sensors
#define DS_NumSensors 2  //number of DS18x20 sensors we're using.

OneWire oneWire(ONE_WIRE_BUS);  
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature DS_sensors(&oneWire);
// arrays to hold device addresses
byte DS_Addrs [DS_NumSensors][8];  // Somewhere to store the device addresses.
int  DS_Temps [DS_NumSensors];     // Somewhere to store temps - should really be malloc'ed based on num of detected devices
//byte DS_Discovered;   // How many sensors we actually see on the bus, should be same as DS_NumSensors if all is well.
//byte DS_Degraded;    // if DS_Discovered != DS_NumSensors, then consider the bus degraded and flag it.

// Network initialisation
RF24 radio(9,10);
RF24Network network(radio);
const uint16_t this_node = 1;
const uint16_t base_station = 0;
#define RF_Channel 25  // what Radio channel we're using.

const unsigned char OSS = 3;  // Oversampling Setting
// Calibration values
int ac1;
int ac2;
int ac3;
unsigned int ac4;
unsigned int ac5;
unsigned int ac6;
int b1;
int b2;
int mb;
int mc;
int md;

// b5 is calculated in bmp085GetTemperature(...), this variable is also used in bmp085GetPressure(...)
// so ...Temperature(...) must be called before ...Pressure(...).
long b5; 


struct Data_Packet {
	byte SourceID;        //1 byte
        byte NumberOfPackets; //1 byte
        byte ThisPacketID;    //1 byte
        byte SensorStatus;    //1 byte
        int  RESERVED;        //2 bytes
        int  DATA[13];        //26 bytes
};




Data_Packet data;
byte DS_Discovered;

void setup()
{
  Serial.begin(57600);
  // init radio
  SPI.begin();
  radio.begin();
  network.begin(RF_Channel, this_node);
  radio.setRetries(10,10);          // wait a max of 2500us between retries, retry 10 times
  radio.setDataRate(RF24_250KBPS);  // should be fast enough & give greater range
  radio.setPALevel(RF24_PA_MAX);    // set TX power to 0 dbm
    
  DS_sensors.begin();
  
  DS_Discovered = DS_DetectDevices();
  if (DS_Discovered != DS_NumSensors)
    firstpacket.SensorStatus = DS18X20;
  else
   firstpacket.SensorStatus = 0;
    
  for (byte i=0; i < DS_Discovered; i++)
  {
    DS_sensors.setResolution(DS_Addrs[i], 12);      
  }
  
  Wire.begin();
  bmp085Calibration();
  dht.begin();
  delay(2000);
}

void loop ()
{
 
 network.update();  //Network housekeeping
 
 // DHT11 / 22 sensors take about 2s to warm up and about 250ms to read
 // read as floats, convert to ints to save space.
 volatile float tmp;
 tmp = dht.readHumidity();
 int DHT_h = 100 * tmp;
 tmp = dht.readTemperature();
 int DHTt_t = 100 * tmp;
 
 //get the temp and pressuer from the BMP085 device
 tmp = bmp085GetTemperature(bmp085ReadUT()); //MUST be called first
 int BMP_t = tmp * 100;
 tmp = bmp085GetPressure(bmp085ReadUP());
 int BMP_p = tmp * 100;
 
 //get temps from the DS18x20 sensors
 DS_sensors.requestTemperatures();
 
 for (byte i = 0; i <  DS_Discovered; i++)
 {
   tmp = DS_sensors.getTempC(DS_Addrs[i]);
    // error handling - if we're out of sensible limits set flag in firstpacket
    if ((tmp > 60) || (tmp == -127))
    {
        firstpacket.SensorStatus = DS18X20;
    }
    
   DS_Temps[i] = 100 * tmp;
 }
 
 
 

}


//////////////////////////////////////////////////////
// Helper functions under here
//////////////////////////////////////////////////////


byte DS_DetectDevices() {
  byte i=0;
  byte ii=0; 
  while ((i < DS_NumSensors) && (oneWire.search(DS_Addrs[i]))) 
  {        
    i++;
  }
  
  for (ii=0; ii < i; ii++) {
    Serial.print("Sensor ");
    Serial.print(ii);  
    Serial.print(": ");                          
    printAddress(DS_Addrs[ii]);                  // print address from each device address arry.
  }
  Serial.print("\r\n");
  return ii;                 // return total number of devices found.
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

// Stores all of the bmp085's calibration values into global variables
// Calibration values are required to calculate temp and pressure
// This function should be called at the beginning of the program
void bmp085Calibration()
{
  //Serial.write("\n\nCalibrating ... ");
  ac1 = bmp085ReadInt(0xAA);
  ac2 = bmp085ReadInt(0xAC);
  ac3 = bmp085ReadInt(0xAE);
  ac4 = bmp085ReadInt(0xB0);
  ac5 = bmp085ReadInt(0xB2);
  ac6 = bmp085ReadInt(0xB4);
  b1 = bmp085ReadInt(0xB6);
  b2 = bmp085ReadInt(0xB8);
  mb = bmp085ReadInt(0xBA);
  mc = bmp085ReadInt(0xBC);
  md = bmp085ReadInt(0xBE);
  //Serial.write("Calibrated\n\n");
}

// Calculate temperature in deg C
float bmp085GetTemperature(unsigned int ut){
  long x1, x2;

  x1 = (((long)ut - (long)ac6)*(long)ac5) >> 15;
  x2 = ((long)mc << 11)/(x1 + md);
  b5 = x1 + x2;

  float temp = ((b5 + 8)>>4);
  temp = temp /10;

  return temp;
}

// Calculate pressure given up
// calibration values must be known
// b5 is also required so bmp085GetTemperature(...) must be called first.
// Value returned will be pressure in units of Pa.
long bmp085GetPressure(unsigned long up){
  long x1, x2, x3, b3, b6, p;
  unsigned long b4, b7;

  b6 = b5 - 4000;
  // Calculate B3
  x1 = (b2 * (b6 * b6)>>12)>>11;
  x2 = (ac2 * b6)>>11;
  x3 = x1 + x2;
  b3 = (((((long)ac1)*4 + x3)<<OSS) + 2)>>2;

  // Calculate B4
  x1 = (ac3 * b6)>>13;
  x2 = (b1 * ((b6 * b6)>>12))>>16;
  x3 = ((x1 + x2) + 2)>>2;
  b4 = (ac4 * (unsigned long)(x3 + 32768))>>15;

  b7 = ((unsigned long)(up - b3) * (50000>>OSS));
  if (b7 < 0x80000000)
    p = (b7<<1)/b4;
  else
    p = (b7/b4)<<1;

  x1 = (p>>8) * (p>>8);
  x1 = (x1 * 3038)>>16;
  x2 = (-7357 * p)>>16;
  p += (x1 + x2 + 3791)>>4;

  long temp = p;
  return temp;
}

// Read 1 byte from the BMP085 at 'address'
char bmp085Read(byte address)
{
  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(address);
  Wire.endTransmission();
  Wire.requestFrom(BMP085_ADDRESS, 1);
  while(!Wire.available()) {};
  return Wire.read();
}

// Read 2 bytes from the BMP085
// First byte will be from 'address'
// Second byte will be from 'address'+1
int bmp085ReadInt(byte address)
{
  unsigned char msb, lsb;

  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(address);
  Wire.endTransmission();

  Wire.requestFrom(BMP085_ADDRESS, 2);
  while(Wire.available()<2)
    ;
  msb = Wire.read();
  lsb = Wire.read();

  return (int) msb<<8 | lsb;
}

// Read the uncompensated temperature value
unsigned int bmp085ReadUT(){
  unsigned int ut;

  // Write 0x2E into Register 0xF4
  // This requests a temperature reading
  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write((byte)0xF4);
  Wire.write((byte)0x2E);
  Wire.endTransmission();

  // Wait at least 4.5ms
  delay(5);

  // Read two bytes from registers 0xF6 and 0xF7
  ut = bmp085ReadInt(0xF6);
  return ut;
}

// Read the uncompensated pressure value
unsigned long bmp085ReadUP(){

  unsigned char msb, lsb, xlsb;
  unsigned long up = 0;

  // Write 0x34+(OSS<<6) into register 0xF4
  // Request a pressure reading w/ oversampling setting
  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(0xF4);
  Wire.write(0x34 + (OSS<<6));
  Wire.endTransmission();

  // Wait for conversion, delay time dependent on OSS
  delay(2 + (3<<OSS));

  // Read register 0xF6 (MSB), 0xF7 (LSB), and 0xF8 (XLSB)
  msb = bmp085Read(0xF6);
  lsb = bmp085Read(0xF7);
  xlsb = bmp085Read(0xF8);

  up = (((unsigned long) msb << 16) | ((unsigned long) lsb << 8) | (unsigned long) xlsb) >> (8-OSS);

  return up;
}

void writeRegister(int deviceAddress, byte address, byte val) {
  Wire.beginTransmission(deviceAddress); // start transmission to device 
  Wire.write(address);       // send register address
  Wire.write(val);         // send value to write
  Wire.endTransmission();     // end transmission
}

int readRegister(int deviceAddress, byte address){

  int v;
  Wire.beginTransmission(deviceAddress);
  Wire.write(address); // register to read
  Wire.endTransmission();

  Wire.requestFrom(deviceAddress, 1); // read a byte

  while(!Wire.available()) {
    // waiting
  }

  v = Wire.read();
  return v;
}

