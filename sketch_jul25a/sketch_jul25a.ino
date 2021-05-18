#include "DFRobot_IL0376F_SPI.h"
#include <DHTesp.h>

DFRobot_IL0376F_SPI epaper;
DHTesp dht;

#define EPAPER_CS  D3
#define Font_CS  D6
#define EPAPER_DC  D8
#define EPAPER_BUSY     D7

// If your sensor is CREAM uncomment this line
#define DHTTYPE DHTesp::DHT22   // Cream humidity sensor

// DHT Sensor Pin
const int DHTPin = 13;

uint8_t peakT = 0, peakH = 0;

void setup() {
  // put your setup code here, to run once:
  epaper.begin(EPAPER_CS, Font_CS, EPAPER_DC, EPAPER_BUSY);
  dht.setup(DHTPin, DHTesp::DHT22);
  Serial.begin(115200);

}

void loop() {
  // put your main code here, to run repeatedly:
  TempAndHumidity TH;
  TH = dht.getTempAndHumidity();

  if (TH.temperature > peakT)
    peakT = TH.temperature;

  if (TH.humidity > peakH)
    peakH = TH.humidity;
  String Tpoint = String(TH.temperature) + "   " + String(peakT);
  String Hpoint = String(TH.humidity) + "   " + String(peakH);

  Serial.print(Tpoint);
  Serial.println();
  Serial.print(Hpoint);
  Serial.println();
  char T[10] ={'\0','\0','\0','\0','\0','\0','\0','\0','\0','\0'};
  sprintf(T,"%s",Tpoint.c_str());
  char H[10] = {'\0','\0','\0','\0','\0','\0','\0','\0','\0','\0'};
  sprintf(H,"%s",Hpoint.c_str());  

  epaper.fillScreen(BLACK);
  //epaper.disString(22,10,1,"Hello",BLACK);
  epaper.disString(22,10,1,T,WHITE);
  epaper.disString(22,33,1,H,WHITE);
  epaper.disString(22,50,1,T,WHITE);
  epaper.flush(PART);
  delay(5000);
}
