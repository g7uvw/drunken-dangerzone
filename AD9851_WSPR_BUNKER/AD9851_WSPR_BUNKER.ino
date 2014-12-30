// WSPR Transmitter from AD9851
// George Smart, M1GEO.
//  here: http://www,george-smart.co.uk/wiki/Arduino_WSPR
//
// AD9851 Code
// Originally by Peter Marks
//  here: http://blog.marxy.org/2008/05/controlling-ad9851-dds-with-arduino.html
//
// GPS provided by TinyGPS
//  here: http://arduiniana.org/libraries/tinygps/
//

// DDS Reference Oscilliator Frequency, in Hz. (Remember, the PLL).
#define DDS_REF   180000000 // Hz. Bunker
                  

// DDS Offset in Hz
#define DDS_OSET  172  // Hz. Bunker

// WSPR Output Frequency
// Bottom end of each band.  TX frequency is between 10 and 190 Hz up from this, randomly.
static unsigned long WSPR_TX_BANDS_FREQS[] = {1.838000e6, 3.594000e6, 7.040000e6, 10.140100e6, 14.097000e6, 18.106000e6, 21.096000e6, 24.926000e6, 28.126000e6};

// WSPR Tone Data - line breaks in no parciular place, just to look pretty. (0-161).
static byte WSPR_DATA_BUNKER[] = {1,1,2,2,0,2,0,0,1,0,0,0,3,1,3,0,0,2,3,0,2,1,0,3,1,1,3,2,0,0,2,2,2,
              2,1,2,0,1,2,1,2,0,2,0,0,2,1,2,1,3,2,2,1,3,2,1,0,0,0,3,1,2,1,0,2,2,0,1,1,0,3,2,3,2,1,0,
              1,0,0,3,2,2,3,0,3,1,0,0,0,1,3,0,3,0,1,0,0,0,1,0,2,0,2,2,3,2,2,3,0,2,3,3,1,0,3,1,0,0,3,
              3,2,1,2,0,0,3,3,1,0,2,0,0,2,1,0,1,0,0,1,1,2,2,0,0,0,2,2,3,3,0,1,0,1,3,0,2,2,3,1,0,2,2}; // 162 bits

// DDS/Arduino Connections
#define DDS_LOAD  8
#define DDS_CLOCK 9
#define DDS_DATA  10
#define LED       13
#define GPS_LED   12
#define PA_ON     11

// Libraries
#include <stdint.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h>

// GPS Stuff
TinyGPS gps;
SoftwareSerial nss(3, 4);  //GPS RX 3, TX 4

// Variables
unsigned long  WSPR_TXF = 0;
unsigned long startT = 0, stopT = 0;
int year;
byte month, day, hour, minute, second, hundredths, Nsatellites, ret;
unsigned long fix_age, fail;
char sz[32];
byte randBand;
byte randPA;


void setup()
{
  int i=0;
  // Set all pins to output states
  pinMode (DDS_DATA,  OUTPUT);
  pinMode (DDS_CLOCK, OUTPUT);
  pinMode (DDS_LOAD,  OUTPUT);
  pinMode (LED,       OUTPUT);
  pinMode (GPS_LED,   OUTPUT);
  
  // Setup RS232
  Serial.begin(4800);
  
  sprintf(sz, "\nM1GEO Compiled %s %s\n", __TIME__, __DATE__);
  Serial.print(sz);
  
  Serial.print("Base TX Frequencies: ");
  for (i=0;i<(sizeof(WSPR_TX_BANDS_FREQS) / sizeof(unsigned long));i++) {
    sprintf(sz, "%lu ", WSPR_TX_BANDS_FREQS[i]);
    Serial.print(sz);
  }
  
  Serial.print("Hz.\nDDS Reset   ");
  delay(900);
  frequency(0);
  delay(100);
  Serial.println("OK");
  
  Serial.print("Hz.\nPA Reset   ");
  digitalWrite(PA_ON, HIGH);  // turn PA on
  delay(1000);
  digitalWrite(PA_ON, LOW);  // turn PA off
  Serial.println("OK");
}

void loop()
{
  fail++;
  ret = feedgps();
  
  if (fail == 90000) {
    //digitalWrite (GPS_LED, LOW);
    Serial.println("GPS: No Data.");
  }
  
  if (ret>0) {
    Nsatellites = gps.satellites();
    gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &fix_age);
    if (fix_age == TinyGPS::GPS_INVALID_AGE) {
      Serial.println("GPS: No Fix.");
      //digitalWrite (GPS_LED, LOW);
    } else {
      //digitalWrite (GPS_LED, HIGH);
      sprintf(sz, "Date %02d/%02d/%02d, Time %02d:%02d:%02d (Sats: %02d, GPS Age: %lu ms, GPS Feed: %lu).", day, month, year, hour, minute, second, Nsatellites, fix_age, fail);
      Serial.println(sz);
      
//      randBand = random(0, (sizeof(WSPR_TX_BANDS_FREQS) / sizeof(unsigned long)));  // random, select a band.
//      randPA   = 0; //random(1, 10) % 2; // random, PA on or off!
//      
//      WSPR_TXF = (WSPR_TX_BANDS_FREQS[randBand]+DDS_OSET) + random(50, 150); // always choose a frequency, it mixes it all up a little with the pRNG.

      if ( (minute % 2 == 0) && (second >= 1) && (second <= 4) ) {  // start transmission between 1 and 4 seconds, on every even minute
      randBand = random(0, (sizeof(WSPR_TX_BANDS_FREQS) / sizeof(unsigned long)));  // random, select a band.
      randPA   = 0; //random(1, 10) % 2; // random, PA on or off!
      
      WSPR_TXF = (WSPR_TX_BANDS_FREQS[randBand]+DDS_OSET) + random(50, 150); // always choose a frequency, it mixes it all up a little with the pRNG.

        Serial.print("Beginning WSPR Transmission on ");
        Serial.print(WSPR_TXF-DDS_OSET);
        Serial.print(" Hz with PA ");
        if (randPA > 0) {
          Serial.print("on.     ");
        } else{
          Serial.print("off.     ");
        }
        wsprTX();
        Serial.println("Finished.");
      }
      //digitalWrite (LED, HIGH);
      delay(250);
      //digitalWrite (LED, LOW);
      delay(250);
    }
    fail = 0;
  }
}

void frequency(unsigned long frequency) {
  unsigned long tuning_word = (frequency * pow(2, 32)) / DDS_REF;
  digitalWrite (DDS_LOAD, LOW); // take load pin low

  for(int i = 0; i < 32; i++) {
    if ((tuning_word & 1) == 1)
      outOne();
    else
      outZero();
    tuning_word = tuning_word >> 1;
  }
  byte_out(0x09);
  digitalWrite (DDS_LOAD, HIGH); // Take load pin high again
}

void byte_out(unsigned char byte) {
  int i;

  for (i = 0; i < 8; i++) {
    if ((byte & 1) == 1)
      outOne();
    else
      outZero();
    byte = byte >> 1;
  }
}

void outOne() {
  digitalWrite(DDS_CLOCK, LOW);
  digitalWrite(DDS_DATA, HIGH);
  digitalWrite(DDS_CLOCK, HIGH);
  digitalWrite(DDS_DATA, LOW);
}

void outZero() {
  digitalWrite(DDS_CLOCK, LOW);
  digitalWrite(DDS_DATA, LOW);
  digitalWrite(DDS_CLOCK, HIGH);
}

void flash_led(unsigned int t, int l) {
  unsigned int i = 0;
  if (t > 25) {
    digitalWrite(l, HIGH);
    delay(2000);
    digitalWrite(l, LOW);
  } else {
    for (i=0;i<t;i++) {
      digitalWrite(l, HIGH);
      delay(250);
      digitalWrite(l, LOW);
      delay(250);
    }
  }
}

void wsprTXtone(int t) {
  if ((t >= 0) && (t <= 3) ) {
    frequency((WSPR_TXF + (t * 2))); // should really be 1.4648 Hz not 2.
  } else {
    Serial.print("Tone #");
    Serial.print(t);
    Serial.println(" is not valid.  (0 <= t <= 3).");
  }
}

void wsprTX() {
  int i = 0;

  if (randPA > 0) {
    digitalWrite(PA_ON, HIGH);  // turn PA on
  } else {
    digitalWrite(PA_ON, LOW);  // turn PA off, just to ensure.
  }

  digitalWrite(LED, HIGH);
  for (i=0;i<162;i++) {
    wsprTXtone( WSPR_DATA_BUNKER[i] );
    delay(682);  }
  frequency(0);
  digitalWrite(LED, LOW);
  digitalWrite(PA_ON, LOW);  // turn PA off
}

static bool feedgps()
{
  while (Serial.available()) {
    if (gps.encode(Serial.read())) {
      return true;
    }
  }
  return false;
}

