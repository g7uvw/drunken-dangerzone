// Simple Ethernet & LCD testing

// Includes
#include <SPI.h>
#include <Ethernet.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
//#include "openweathermap.h"
#include "HttpClient.h"

// Pins
#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0
#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin

#define YP A1  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 7   // can be a digital pin
#define XP 6   // can be a digital pin



//Ethernet shield attached to pins 10, 11, 12, 13

// Colours
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

// touchscreen 
#define TS_MINX 150
#define TS_MINY 120
#define TS_MAXX 920
#define TS_MAXY 940
#define MINPRESSURE 10
#define MAXPRESSURE 1000

// Ethernet defaults if DHCP doesn't work
byte mac[] = { 0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };
IPAddress ip(192,168,1, 177);
IPAddress gateway(192,168,1, 1);
IPAddress subnet(255, 255, 0, 0);

// init objects
Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
EthernetServer server(23);



void setup(void)
{
  Serial.begin(9600);
  Serial.println(F("Ethernet & TFT LCD test"));
  tft.reset();
  uint16_t identifier = tft.readID();
  tft.begin(identifier);
  tft.fillScreen(BLACK);
  tft.setTextColor(WHITE);  
  tft.setTextSize(1);
  tft.println(F("Hello World!"));
  Serial.println(F("Hello World!"));
  tft.println(F("Trying to get an IP address using DHCP"));
  Serial.println(F("Trying to get an IP address using DHCP"));
  if (Ethernet.begin(mac) == 0) {
    tft.println(F("Failed to configure Ethernet using DHCP"));
    // initialize the ethernet device not using DHCP:
    Ethernet.begin(mac, ip, gateway, subnet);
  }
  tft.print("My IP address: ");
  Serial.print("My IP address: ");
  ip = Ethernet.localIP();
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    tft.print(ip[thisByte], DEC);
    Serial.print(ip[thisByte], DEC);
    tft.print("."); 
    Serial.print(".");
  }
  
    pinMode(13, OUTPUT);
    
}

void loop(void)
{
  digitalWrite(13, HIGH);
  Point p = ts.getPoint();
  digitalWrite(13, LOW);

  // if sharing pins, you'll need to fix the directions of the touchscreen pins
  //pinMode(XP, OUTPUT);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  //pinMode(YM, OUTPUT);

  // we have some minimum pressure we consider 'valid'
  // pressure of 0 means no pressing!

  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    
    Serial.print("X = "); Serial.print(p.x);
    Serial.print("\tY = "); Serial.print(p.y);
    Serial.print("\tPressure = "); Serial.println(p.z);
    
  tft.setTextColor(YELLOW);  
  tft.setTextSize(2);
  tft.print("X = "); tft.print(p.x);
  tft.print("\tY = "); tft.print(p.y);
  tft.print("\tPressure = "); tft.println(p.z);
  }
  
 
   //weather_response_t resp = weather->cachedUpdate();
   // if (resp.isSuccess) {
   //     tft.print(resp.temp_low);
   //     tft.print("-");
   //     tft.print(resp.temp_high);
   //     tft.println(resp.descr);
   // }
}

