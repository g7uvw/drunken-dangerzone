

// Libraries
#include <stdint.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h>

static void gpsdump(TinyGPS &gps);
static bool feedgps();
static void print_float(float val, float invalid, int len, int prec);
static void print_int(unsigned long val, unsigned long invalid, int len);
static void print_date(TinyGPS &gps);
static void print_str(const char *str, int len);
void signOn(TinyGPS &gps);

#define DDS_REF   125000000 // Hz. 
#define  CLOCK  8  //pin connections for DDS
#define  LOAD 9 
#define  DATA  10
#define  RESET 11
int  DDS_error = 0;                  

static unsigned long WSPR_TX_BANDS_FREQS[] = {1.838000e6, 3.594000e6, 7.040000e6, 10.140100e6, 14.097000e6, 18.106000e6, 21.096000e6, 24.926000e6, 28.126000e6};


char call[7] = "G7UVW";    
byte power = 10; // Min = 0 dBm, Max = 43 dBm, steps 0,3,7,10,13,17,20,23,27,30,33,37,40,43
char locator[5]; //filled in by GPS

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

// DDS/Arduino Connections
#define DDS_LOAD  8
#define DDS_CLOCK 9
#define DDS_DATA  10
#define LED       13
#define GPS_LED   12
#define PA_ON     11



// GPS Stuff
TinyGPS gps;
//SoftwareSerial nss(3, 4);  //GPS RX 3, TX 4




// Variables
unsigned long  TXfreq = 0;
unsigned long startT = 0, stopT = 0;
int year;
byte month, day, hour, minute, second, hundredths, Nsatellites, ret;
unsigned long fix_age, fail;
char sz[32];
byte band;
//byte randPA;


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
 // dumpPos(gps);
 Serial.println(F("Generating WSPR frame"));
  GenerateWSPR();
  Serial.println();
  Serial.println("OK");
  signOn(gps);
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
      band = random(0, (sizeof(WSPR_TX_BANDS_FREQS) / sizeof(unsigned long)));  // random, select a band.
      TXfreq = (WSPR_TX_BANDS_FREQS[band]+DDS_error) + random(1, 100);
      Serial.print("Transmitting WSPR on: ");
      Serial.println(TXfreq - DDS_error);
      WSPR_TX();
      
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


static bool feedgps()
{
  while (Serial.available()) {
    if (gps.encode(Serial.read())) {
      return true;
    }
  }
  return false;
}

void signOn(TinyGPS &gps)
{

  Serial.println(F("Waiting for valid GPS data"));
  Serial.println();
  Serial.print(F("Time and Date : "));
  print_date(gps);
  Serial.println();
  Serial.print(F("Longform Maidenhead Locator : "));
  Maidenhead(gps);
  Serial.println();
  Serial.print(F("WSPR Locator : "));
  print_str(locator,5);
  Serial.println();
  
  
}



static void Maidenhead(TinyGPS &gps)

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
  //print_str(locator,5);
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
  feedgps();
}

static void print_str(const char *str, int len)
{
  int slen = strlen(str);
  for (int i=0; i<len; ++i)
    Serial.print(i<slen ? str[i] : ' ');
  feedgps();
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


