/*
 Get pressure, altitude, and temperature from the BMP085.
 Serial.print it out at 9600 baud to serial monitor.
 Fixed for Arduino 1.0+ by iLabBali.com
 Based largely on code by Jim Lindblom via the repost at
 http://bildr.org/2011/06/bmp085-arduino/
*/
#include <stddef.h>
#include <Wire.h>
#include "DHT.h"
#include <ST7735.h>
#include <SPI.h>
#define DHTPIN 2     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define BMP085_ADDRESS 0x77  // I2C address of BMP085
DHT dht(DHTPIN, DHTTYPE);

//LCD
#define sclk 13    // for MEGAs use pin 52
#define mosi 11    // for MEGAs use pin 51
#define cs 10   // for MEGAs you probably want this to be pin 53
#define dc 9
#define rst 8  // you can also connect this to the Arduino reset

// Color definitions
#define	BLACK           0x0000
#define	BLUE            0x001F
#define	RED             0xF800
#define	GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0  
#define WHITE           0xFFFF


class SMA {
public:
	SMA(unsigned int period) :
		period(period), window(new double[period]), head(NULL), tail(NULL),
				total(0) {
		//assert(period >= 1);
	}
	~SMA() {
		delete[] window;
	}
 
	// Adds a value to the average, pushing one out if nescessary
	void add(double val) {
		// Special case: Initialization
		if (head == NULL) {
			head = window;
			*head = val;
			tail = head;
			inc(tail);
			total = val;
			return;
		}
 
		// Were we already full?
		if (head == tail) {
			// Fix total-cache
			total -= *head;
			// Make room
			inc(head);
		}
 
		// Write the value in the next spot.
		*tail = val;
		inc(tail);
 
		// Update our total-cache
		total += val;
	}
 
	// Returns the average of the last P elements added to this SMA.
	// If no elements have been added yet, returns 0.0
	double avg() const {
		ptrdiff_t size = this->size();
		if (size == 0) {
			return 0; // No entries => 0 average
		}
		return total / (double) size; // Cast to double for floating point arithmetic
	}
 
private:
	unsigned int period;
	double * window; // Holds the values to calculate the average of.
 
	// Logically, head is before tail
	double * head; // Points at the oldest element we've stored.
	double * tail; // Points at the newest element we've stored.
 
	double total; // Cache the total so we don't sum everything each time.
 
	// Bumps the given pointer up by one.
	// Wraps to the start of the array if needed.
	void inc(double * & p) {
		if (++p >= window + period) {
			p = window;
		}
	}
 
	// Returns how many numbers we have stored.
	ptrdiff_t size() const {
		if (head == NULL)
			return 0;
		if (head == tail)
			return period;
		return (period + tail - head) % period;
	}
};


ST7735 tft = ST7735(cs, dc, rst);

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

// Add these to the top of your program
const float p0 = 101325;     // Pressure at sea level (Pa)
const float currentAltitude = 1580.08; // current altitude in METERS
const float ePressure = p0 * pow((1-currentAltitude/44330), 5.255);  // expected pressure (in Pa) at altitude
float weatherDiff;
float humid;
float temp2;
 SMA pressure(10);

void setup(){
  Serial.begin(9600);
  tft.initR();               // initialize a ST7735R chip
  tft.writecommand(ST7735_DISPON);
  tft.fillScreen(BLACK);
  tft.setTextColor(BLUE);
  tft.setTextSize(1);
  tft.setCursor(0,0);
  tft.println("dave@webshed.org");
  tft.println("Temperature, Humidity");
  tft.println("& Air Pressure data.");
 
  
  Wire.begin();
  bmp085Calibration();
  dht.begin();
  delay(2000);
tft.fillScreen(BLACK);
}

void loop()
{
  tft.fillScreen(BLACK);
  tft.setTextColor(YELLOW);
  tft.setTextSize(2);
   tft.setCursor(0,0);
   // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  pressure.add(h);
  if (isnan(t) || isnan(h)) {
    Serial.println("Failed to read from DHT");
    tft.println("Failed to read from DHT");
  } else {
    Serial.print("Temperature: "); 
    Serial.print(t);
    Serial.print(" C");
    Serial.print("\t");
    Serial.print("Humidity: "); 
    Serial.print(pressure.avg());
    Serial.println("%");
    
    tft.println("Temp: "); 
    tft.print(t);
    tft.print(" C");
    tft.println(" ");
    tft.setTextColor(GREEN);
    tft.println("Humidity: "); 
    tft.print(h);
    tft.println("%");
  }
  
  float temperature = bmp085GetTemperature(bmp085ReadUT()); //MUST be called first
  float pressure = bmp085GetPressure(bmp085ReadUP());
  float atm = pressure / 101325; // "standard atmosphere"
  float altitude = calcAltitude(pressure); //Uncompensated caculation - in Meters 
 
  Serial.print("Temperature: ");
  Serial.print(temperature, 2); //display 2 decimal places
  Serial.print(" C");
  Serial.print("\t");

  Serial.print("Pressure: ");
  Serial.print(pressure, 0); //whole number only.
  Serial.println(" Pa");
  tft.setTextColor(YELLOW);
  tft.println("Temp: ");
  tft.print(temperature, 2); //display 2 decimal places
  tft.print(" C");
  tft.println(" ");
  tft.setTextColor(BLUE);
  tft.println("Pressure: ");
  tft.print(pressure, 0); //whole number only.
  tft.println(" Pa");

  //Serial.print("Standard Atmosphere: ");
  //Serial.println(atm, 4); //display 4 decimal places

  //Serial.print("Altitude: ");
  //Serial.print(altitude, 2); //display 2 decimal places
  //Serial.println(" M");

  //Serial.println();//line break
  //weatherDiff = pressure - ePressure;
  //if(weatherDiff > 250)
    //Serial.println("Sunny!");
  //else if ((weatherDiff <= 250) || (weatherDiff >= -250))
    //Serial.println("Partly Cloudy");
  //else if (weatherDiff > -250)
    //Serial.println("Rain :-(");
  Serial.println();

  delay(2000); //wait a second and get values again.
  //tft.setRotation(tft.getRotation()+1);
}

// Stores all of the bmp085's calibration values into global variables
// Calibration values are required to calculate temp and pressure
// This function should be called at the beginning of the program
void bmp085Calibration()
{
  Serial.write("\n\nCalibrating ... ");
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
  Serial.write("Calibrated\n\n");
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

float calcAltitude(float pressure){

  float A = pressure/101325;
  float B = 1/5.25588;
  float C = pow(A,B);
  C = 1 - C;
  C = C /0.0000225577;

  return C;
}

void testdrawtext(char *text, uint16_t color) {
  tft.drawString(0, 0, text, color);
}


