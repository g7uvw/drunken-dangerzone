//#include <SoftwareSerial.h>
#include <stdint.h>
#include <TinyGPS.h>

/* This sample code demonstrates the normal use of a TinyGPS object.
   It requires the use of SoftwareSerial, and assumes that you have a
   4800-baud serial GPS device hooked up on pins 3(rx) and 4(tx).
*/

TinyGPS gps;
//SoftwareSerial nss(3, 4);

static void gpsdump(TinyGPS &gps);
static bool feedgps();
static void print_float(float val, float invalid, int len, int prec);
static void print_int(unsigned long val, unsigned long invalid, int len);
static void print_date(TinyGPS &gps);
static void print_str(const char *str, int len);
void signOn(TinyGPS &gps);

char temp[32];
int year;
byte ret,month, day, hour, minute, second, hundredths, Nsatellites, band,lastsecond = 0;
unsigned long TXfreq, fix_age,fail;

//wspr bits
char call[7] = "G7UVW";    
byte power = 10; // Min = 0 dBm, Max = 43 dBm, steps 0,3,7,10,13,17,20,23,27,30,33,37,40,43
char locator[5]; //filled in by GPS
static unsigned long WSPR_TX_BANDS_FREQS[] = {1.838000e6, 3.594000e6, 7.040000e6, 10.140100e6, 14.097000e6, 18.106000e6, 21.096000e6, 24.926000e6, 28.126000e6};

// WSPR Sync Data - Don't edit this!!
const char SyncVec[162] = {
  1,1,0,0,0,0,0,0,1,0,0,0,1,1,1,0,0,0,1,0,0,1,0,1,1,1,1,0,0,0,0,0,0,0,1,0,0,1,0,1,0,0,0,0,0,0,1,0,
  1,1,0,0,1,1,0,1,0,0,0,1,1,0,1,0,0,0,0,1,1,0,1,0,1,0,1,0,1,0,0,1,0,0,1,0,1,1,0,0,0,1,1,0,1,0,1,0,
  0,0,1,0,0,0,0,0,1,0,0,1,0,0,1,1,1,0,1,1,0,0,1,1,0,1,0,0,0,1,1,1,0,0,0,0,0,1,0,1,0,0,1,1,0,0,0,0,
  0,0,0,1,1,0,1,0,1,1,0,0,0,1,1,0,0,0
};

byte c[11];                // encoded message
char sym[170];             // symbol table 162
byte symt[170];            // symbol table temp
unsigned long n1;    // encoded callsign
unsigned long m1;    // encodes locator

// end wspr bits

//DDS bits
#define DDS_CLOCK 125000000
#define  CLOCK  8  //pin connections for DDS
#define  LOAD 9 
#define  DATA  10
#define  RESET 11
int  DDS_error = 0;

void setup()
{
  pinMode (DATA,  OUTPUT); 
  pinMode (CLOCK, OUTPUT); 
  pinMode (LOAD,  OUTPUT); 
  pinMode (RESET, OUTPUT); 
  
  Serial.begin(4800);
  //nss.begin(4800);
   
  AD9850_init();
  AD9850_reset();
  
  signOn(gps);
 // locator[5]= 'JO01';
    Serial.println(F("Generating WSPR frame"));
  GenerateWSPR();
  Serial.println();

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


//void loop()
//{
//  
//  fail++;
//  ret = feedgps();
//  
//  if (fail == 90000) {
//    Serial.println(F("GPS: No Data."));
//  }
//  
//  
//  if (ret>0) {
//    Nsatellites = gps.satellites();
//    gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &fix_age);
//    if (fix_age == TinyGPS::GPS_INVALID_AGE) {
//      Serial.println(F("GPS: No Fix."));
//     
//    } else {
//      
//      sprintf(temp, "Date %02d/%02d/%02d, Time %02d:%02d:%02d (Sats: %02d, GPS Age: %lu ms, GPS Feed: %lu).", day, month, year, hour, minute, second, Nsatellites, fix_age, fail);
//      Serial.println(temp);
//      Serial.print(F("hello"));
//           
//   if ( (minute % 2 == 0) && (second >= 1) && (second <= 4) ) {  // start transmission between 1 and 4 seconds, on every even minute
//          Serial.print(F("Selecting Band & Frequency"));
//          //pick a band
//           band = random(0, (sizeof(WSPR_TX_BANDS_FREQS) / sizeof(unsigned long)));
//            //pick a frequency on that band
//          TXfreq = (WSPR_TX_BANDS_FREQS[band]+DDS_error) + random(1, 100);
//          Serial.print(F("Transmitting WSPR on: "));
//          Serial.println(TXfreq - DDS_error);
//          
//          WSPR_TX();
//          Serial.println(F("Finished."));
//      }
//    delay(500);
//    }
//    fail = 0;
//  }
//}
  
  
  
 // gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &fix_age);
 // if (fix_age == TinyGPS::GPS_INVALID_AGE) {
 //     Serial.println("GPS: No Fix.");
 // }
  //if (lastsecond != second)
 // {
    //sprintf(temp, "Date %02d/%02d/%02d, Time %02d:%02d:%02d.", day, month, year, hour, minute, second);
    //Serial.println(temp);
    //Serial.println();
 // }
 // lastsecond = second;
  

 /* if ( (minute % 2 == 0) && (second >= 1) && (second <= 4) ) 
  {
     //pick a band
   band = random(0, (sizeof(WSPR_TX_BANDS_FREQS) / sizeof(unsigned long)));
  //pick a frequency on that band
  TXfreq = (WSPR_TX_BANDS_FREQS[band]+DDS_error) + random(1, 100);
  Serial.print("Transmitting WSPR on: ");
  Serial.println(TXfreq - DDS_error);
  WSPR_TX();
  }
  }
}*/

void WSPR_TX()
{ 
  int i;
  for (i=0;i<162;i++) 
  {
    if ( sym[i] >=0 && sym[i]<=3)
    {
      SetFrequency(TXfreq + (sym[i] *2));  //fix when DDS class is fixed for fractional Hz output
    }
    delay(682); 
  }
  SetFrequency(1);  //jitter if off entirely
  Serial.println("Finished");
  //feedgps();
}


void signOn(TinyGPS &gps)
{
  //Serial.print("Testing TinyGPS library v. "); Serial.println(TinyGPS::library_version());
  //Serial.println("With added Maidenhead Locator calc");
  //Serial.println("Waiting for valid GPS data");
  Serial.println();
  //Serial.println(F("Sats HDOP Latitude Longitude Fix  Date       Time       Date Alt     Course Speed Card    Chars Sentences Checksum  Maidenhead"));
  //Serial.println(F("          (deg)    (deg)     Age                        Age  (m)     --- from GPS ----    RX    RX        Fail       Locator"));
  //Serial.println("------------------------------------------------------------------------------------------------------------------------------");
  
  dumpPos(gps);
  Serial.println();
  dumpPos(gps);
  
}

static void dumpPos(TinyGPS &gps)

{
  float flat, flon;
  unsigned long age;
  const int pairs=4;
  //const int pairs=2;
  const double scaling[]={360.,360./18.,(360./18.)/10.,((360./18.)/10.)/24.,(((360./18.)/10.)/24.)/10.,((((360./18.)/10.)/24.)/10.)/24.,(((((360./18.)/10.)/24.)/10.)/24.)/10.};
  int i;
  int index;
  char m[9];
  
 
  gps.f_get_position(&flat, &flon, &age);
  while (flat == TinyGPS::GPS_INVALID_F_ANGLE)
  {
    feedgps();
    
    gps.f_get_position(&flat, &flon, &age);
  }
  
  gpsdump(gps);
  
  for (i=0;i<pairs;i++)
  {
    index = (int)floor(fmod((180.0+flon),scaling[i])/scaling[i+1]);
    m[i*2] = (i&1) ? 0x30+index : (i&2) ? 0x61+index : 0x41+index;
    index = (int)floor(fmod((90.0+flat),(scaling[i]/2))/(scaling[i+1]/2));
    m[i*2+1] = (i&1) ? 0x30+index : (i&2) ? 0x61+index : 0x41+index;
  }
  m[pairs*2]=0;
  print_str(m,8);
  for (i=0;i<4;i++)
  {
    locator[i] = m[i];
  }
  locator[4] = '\0';
  print_str(locator,5);
}



static void gpsdump(TinyGPS &gps)
{
  float flat, flon;
  unsigned long age, date, time, chars = 0;
  unsigned short sentences = 0, failed = 0;
 // static const float LONDON_LAT = 51.508131, LONDON_LON = -0.128002;
  
  print_int(gps.satellites(), TinyGPS::GPS_INVALID_SATELLITES, 5);
  print_int(gps.hdop(), TinyGPS::GPS_INVALID_HDOP, 5);
  gps.f_get_position(&flat, &flon, &age);
  print_float(flat, TinyGPS::GPS_INVALID_F_ANGLE, 9, 5);
  print_float(flon, TinyGPS::GPS_INVALID_F_ANGLE, 10, 5);
  print_int(age, TinyGPS::GPS_INVALID_AGE, 5);

  print_date(gps);

  print_float(gps.f_altitude(), TinyGPS::GPS_INVALID_F_ALTITUDE, 8, 2);
  print_float(gps.f_course(), TinyGPS::GPS_INVALID_F_ANGLE, 7, 2);
  print_float(gps.f_speed_kmph(), TinyGPS::GPS_INVALID_F_SPEED, 6, 2);
  print_str(gps.f_course() == TinyGPS::GPS_INVALID_F_ANGLE ? "*** " : TinyGPS::cardinal(gps.f_course()), 6);
 // print_int(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0UL : (unsigned long)TinyGPS::distance_between(flat, flon, LONDON_LAT, LONDON_LON) / 1000, 0xFFFFFFFF, 9);
  //print_float(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : TinyGPS::course_to(flat, flon, 51.508131, -0.128002), TinyGPS::GPS_INVALID_F_ANGLE, 7, 2);
  //print_str(flat == TinyGPS::GPS_INVALID_F_ANGLE ? "*** " : TinyGPS::cardinal(TinyGPS::course_to(flat, flon, LONDON_LAT, LONDON_LON)), 6);

  gps.stats(&chars, &sentences, &failed);
  print_int(chars, 0xFFFFFFFF, 6);
  print_int(sentences, 0xFFFFFFFF, 10);
  print_int(failed, 0xFFFFFFFF, 9);
  //Serial.println();
}

static void print_int(unsigned long val, unsigned long invalid, int len)
{
  char sz[32];
  if (val == invalid)
    strcpy(sz, "*******");
  else
    sprintf(sz, "%ld", val);
  sz[len] = 0;
  for (int i=strlen(sz); i<len; ++i)
    sz[i] = ' ';
  if (len > 0) 
    sz[len-1] = ' ';
  Serial.print(sz);
  feedgps();
}

static void print_float(float val, float invalid, int len, int prec)
{
  char sz[32];
  if (val == invalid)
  {
    strcpy(sz, "*******");
    sz[len] = 0;
        if (len > 0) 
          sz[len-1] = ' ';
    for (int i=7; i<len; ++i)
        sz[i] = ' ';
    Serial.print(sz);
  }
  else
  {
    Serial.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1);
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i=flen; i<len; ++i)
      Serial.print(" ");
  }
  feedgps();
}

static void print_date(TinyGPS &gps)
{
  int year;
  byte month, day, hour, minute, second, hundredths;
  unsigned long age;
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
  if (age == TinyGPS::GPS_INVALID_AGE)
    Serial.print(F("*******    *******    "));
  else
  {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d %02d:%02d:%02d   ",
        month, day, year, hour, minute, second);
    Serial.print(sz);
  }
  print_int(age, TinyGPS::GPS_INVALID_AGE, 5);
  feedgps();
}

static void print_str(const char *str, int len)
{
  int slen = strlen(str);
  for (int i=0; i<len; ++i)
    Serial.print(i<slen ? str[i] : ' ');
  feedgps();
}

static void print_tbl(const char *str, int len)
{
  char sz[5];
  
  for (int i=0; i<len; ++i)
  sprintf(sz, "%d", str[i]);
  Serial.print(sz);
  //Serial.print(str[i]+48);
  feedgps();
}

static bool feedgps()
{
  while (Serial.available())
  {
    if (gps.encode(Serial.read()))
      return true;
  }
  return false;
}

// WSPR generation code here
void GenerateWSPR(void)
{
  encode_call();
  encode_locator();
  encode_conv();
  interleave_sync();
}

//******************************************************************
// normalize characters 0..9 A..Z Space in order 0..36
char chr_normf(char bc ) 
{
  char cc=36;
  if (bc >= '0' && bc <= '9') cc=bc-'0';
  if (bc >= 'A' && bc <= 'Z') cc=bc-'A'+10;
  if (bc >= 'a' && bc <= 'z') cc=bc-'a'+10;  
  if (bc == ' ' ) cc=36;

  return(cc);
}

void encode_call()
{
  unsigned long t1;

  // coding of callsign
  if (chr_normf(call[2]) > 9) 
  {
    call[5] = call[4];
    call[4] = call[3]; 
    call[3] = call[2];
    call[2] = call[1];
    call[1] = call[0];
    call[0] = ' ';
  }

  n1=chr_normf(call[0]);
  n1=n1*36+chr_normf(call[1]);
  n1=n1*10+chr_normf(call[2]);
  n1=n1*27+chr_normf(call[3])-10;
  n1=n1*27+chr_normf(call[4])-10;
  n1=n1*27+chr_normf(call[5])-10;

  // merge coded callsign into message array c[]
  t1=n1;
  c[0]= t1 >> 20;
  t1=n1;
  c[1]= t1 >> 12;
  t1=n1;
  c[2]= t1 >> 4;
  t1=n1;
  c[3]= t1 << 4;
}

void encode_locator()
{
  unsigned long t1;
  // coding of locator
  m1=179-10*(chr_normf(locator[0])-10)-chr_normf(locator[2]);
  m1=m1*180+10*(chr_normf(locator[1])-10)+chr_normf(locator[3]);
  m1=m1*128+power+64;

  // merge coded locator and power into message array c[]
  t1=m1;
  c[3]= c[3] + ( 0x0f & t1 >> 18);
  t1=m1;
  c[4]= t1 >> 10;
  t1=m1;
  c[5]= t1 >> 2;
  t1=m1;
  c[6]= t1 << 6;
}

void encode_conv()
{
  int bc=0;
  int cnt=0;
  int cc;
  unsigned long sh1=0;

  cc=c[0];

  for (int i=0; i < 81;i++) {
    if (i % 8 == 0 ) {
      cc=c[bc];
      bc++;
    }
    if (cc & 0x80) sh1=sh1 | 1;

    symt[cnt++]=parity(sh1 & 0xF2D05351);
    symt[cnt++]=parity(sh1 & 0xE4613C47);

    cc=cc << 1;
    sh1=sh1 << 1;
  }
}

byte parity(unsigned long li)
{
  byte po = 0;
  while(li != 0)
  {
    po++;
    li&= (li-1);
  }
  return (po & 1);
}

void interleave_sync()
{
  int ii,ij,b2,bis,ip;
  ip=0;

  for (ii=0;ii<=255;ii++) {
    bis=1;
    ij=0;
    for (b2=0;b2 < 8 ;b2++) {
      if (ii & bis) ij= ij | (0x80 >> b2);
      bis=bis << 1;
    }
    if (ij < 162 ) {
      sym[ij]= SyncVec[ij] +2*symt[ip];
      ip++;
    }
  }
}


void SetFrequency(unsigned long frequency)
{
  unsigned long tuning_word = (frequency * pow(2, 32)) / DDS_CLOCK;
  digitalWrite (LOAD, LOW); 

  shiftOut(DATA, CLOCK, LSBFIRST, tuning_word);
  shiftOut(DATA, CLOCK, LSBFIRST, tuning_word >> 8);
  shiftOut(DATA, CLOCK, LSBFIRST, tuning_word >> 16);
  shiftOut(DATA, CLOCK, LSBFIRST, tuning_word >> 24);
  shiftOut(DATA, CLOCK, LSBFIRST, 0x0);
  digitalWrite (LOAD, HIGH); 
}

void AD9850_init()
{

  digitalWrite(RESET, LOW);
  digitalWrite(CLOCK, LOW);
  digitalWrite(LOAD, LOW);
  digitalWrite(DATA, LOW);
}

void AD9850_reset()
{
  //reset sequence is:
  // CLOCK & LOAD = LOW
  //  Pulse RESET high for a few uS (use 5 uS here)
  //  Pulse CLOCK high for a few uS (use 5 uS here)
  //  Set DATA to ZERO and pulse LOAD for a few uS (use 5 uS here)

  // data sheet diagrams show only RESET and CLOCK being used to reset the device, but I see no output unless I also
  // toggle the LOAD line here.

  digitalWrite(CLOCK, LOW);
  digitalWrite(LOAD, LOW);

  digitalWrite(RESET, LOW);
  delay(5);
  digitalWrite(RESET, HIGH);  //pulse RESET
  delay(5);
  digitalWrite(RESET, LOW);
  delay(5);

  digitalWrite(CLOCK, LOW);
  delay(5);
  digitalWrite(CLOCK, HIGH);  //pulse CLOCK
  delay(5);
  digitalWrite(CLOCK, LOW);
  delay(5);
  digitalWrite(DATA, LOW);    //make sure DATA pin is LOW

    digitalWrite(LOAD, LOW);
  delay(5);
  digitalWrite(LOAD, HIGH);  //pulse LOAD
  delay(5);
  digitalWrite(LOAD, LOW);
  // Chip is RESET now
}



