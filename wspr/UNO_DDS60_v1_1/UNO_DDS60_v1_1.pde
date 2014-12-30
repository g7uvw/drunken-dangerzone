/*
Arduino UNO DDS-60 WSPR/QRSS Controller
 Generates QRSS and WSPR coordinated frequency hopping transmissions 
 on 6 thru 160 meters synchronized by either GPS or WWVB time data.
 
 v1.0 19 September 2011
 v1.1 24 November 2011 Reversed DDS-60 Data and Load pins 4 and 6 to 
                       correct schematic error.
 
 The on-chip generation of the WSPR message algorithm is the work of 
 Andy Talbot, G4JNT. Portions of the GPS receive code were influenced
 by Igor Gonzalez Martin's Arduino tutorial.
 
 Copyright (C) 2011,  Gene Marcus W3PM GM4YRE
 
 Permission is granted to use, copy, modify, and distribute this software
 and documentation for non-commercial purposes.
 _________________________________________________________________________
 
 UNO Digital Pin Allocation
 D0  GPS RX
 D1  PB2 Cal+/Hour set
 D2  PB1 Cal enable/Min set/TX inhibit
 D3  PB3 Cal-/time set
 D4  DDS60 data
 D5  DDS60 clock
 D6  DDS60 load
 D7  Band BCD0
 D8  Band BCD1
 D9  Band BCD2
 D10 Band BCD3
 D11 WWVB RX
 D12 QRSS TX/band select
 D13 TX
 A0/D14 LCD D7
 A1/D15 LCD D6
 A2/D16 LCD D5
 A3/D17 LCD D4
 A4/D18 LCD enable
 A5/D19 LCD RS 
 ------------------------------------------------------------
 */
// include the library code:
#include <LiquidCrystal.h>
#include <string.h>
#include <ctype.h>
#include <EEPROM.h>
#include <avr/interrupt.h>  
#include <avr/io.h>
#include <MsTimer2.h>
#include <StopWatch.h>

StopWatch MySW;

//__________________________________________________________________________________________________
// ENTER WSPR DATA:
char call[7] = "W3PM";    
char locator[5] = "EM64"; // Use 4 character locator e.g. "EM64"
byte power = 10; // Min = 0 dBm, Max = 43 dBm, steps 0,3,7,10,13,17,20,23,27,30,33,37,40,43
//__________________________________________________________________________________________________

//__________________________________________________________________________________________________
// LOAD BAND FREQUENCY DATA:
unsigned long band[10] ={
  1838100,  // timeslot 0  00,20,40 minutes after hour
  3594100,  // timeslot 1  02,22,42 minutes after hour
  50294500, // timeslot 2  04,24.44 minutes after hour
  7040100,  // timeslot 3  06,26,46 minutes after hour
  10140200, // timeslot 4  08,28,48 minutes after hour
  14097100, // timeslot 5  10,30,50 minutes after hour
  18106100, // timeslot 6  12,32,52 minutes after hour
  21096100, // timeslot 7  14,34,54 minutes after hour
  24926100, // timeslot 8  16,36,56 minutes after hour
  28126100  // timeslot 9  18,38,58 minutes after hour
};

//__________________________________________________________________________________________________
// LOAD TRANSMIT TIME SLOT DATA: ( 0=idle, 1=transmit WSPR, 2=transmit QRSS )
// Note: Ensure QRSS message length does not exceed QRSS allocated transmit window(s)
const byte TransmitFlag[10] ={
  0, // timeslot 0
  2, // timeslot 1
  2, // timeslot 2 
  2, // timeslot 3
  1, // timeslot 4
  1, // timeslot 5 
  1, // timeslot 6
  1, // timeslot 7
  1, // timeslot 8
  1  // timeslot 9
};


//__________________________________________________________________________________________________
// LOAD QRSS DATA:

// QRSS frequency (Hz)
unsigned long QRSSfreq = 10140060;

// QRSS message
// A "space" should be adde at the end of message to
// separate message when looping
const char QRSSmsg[ ] = "W3PM ";

// QRSS frequency shift in Hz
const byte QRSSshift = 4;

// QRSS dot length in seconds
const byte QRSSdot = 3;

// QRSS patterns:
// 0 = ID loop; 1 = sinewave; 2 = interrupted sinewave; 3 = sawtooth
// 4 = waves; 5 = hills; 6 = herringbone; 7 = upramp ; 8 = downramp
// 9 = upramp sawtooth; 10 = downramp sawtooth; 11 = M's; 12 = W's
const byte Pattern = 4;

//__________________________________________________________________________________________________

const char SyncVec[162] = {
  1,1,0,0,0,0,0,0,1,0,0,0,1,1,1,0,0,0,1,0,0,1,0,1,1,1,1,0,0,0,0,0,0,0,1,0,0,1,0,1,0,0,0,0,0,0,1,0,
  1,1,0,0,1,1,0,1,0,0,0,1,1,0,1,0,0,0,0,1,1,0,1,0,1,0,1,0,1,0,0,1,0,0,1,0,1,1,0,0,0,1,1,0,1,0,1,0,
  0,0,1,0,0,0,0,0,1,0,0,1,0,0,1,1,1,0,1,1,0,0,1,1,0,1,0,0,0,1,1,1,0,0,0,0,0,1,0,1,0,0,1,1,0,0,0,0,
  0,0,0,1,1,0,1,0,1,1,0,0,0,1,1,0,0,0
};

/*
Load Morse code send data. 
 The first five bits are data representing the Morse character. 
 
 1 = dit
 0 = dah
 
 The last three bits represent the number of bits in the Morse character.
 */
const byte Morse[37] = {
  B11111101,	//	0		
  B01111101,	//	1	
  B00111101,	//	2		
  B00011101,	//	3		
  B00001101,	//	4	
  B00000101,	//	5		
  B10000101,	//	6		
  B11000101,	//	7		
  B11100101,	//	8		
  B11110101,	//	9		
  B01000010,	//	A		
  B10000100,	//	B		
  B10100100,	//	C		
  B10000011,	//	D		
  B00000001,	//	E		
  B00100100,	//	F		
  B11000011,	//	G		
  B00000100,	//	H		
  B00000010,	//	I		
  B01110100,	//	J		
  B10100011,	//	K		
  B01000100,	//	L		
  B11000010,	//	M		
  B10000010,	//	N		
  B11100011,	//	O		
  B01100100,	//	P		
  B11010100,	//	Q		
  B01000011,	//	R		
  B00000011,	//	S		
  B10000001,	//	T		
  B00100011,	//	U		
  B00010100,	//	V		
  B01100011,	//	W		
  B10010100,	//	X		
  B10110100,	//	Y		
  B11000100,	//	Z
  B00000000     //      sp		
};

#define InhibitButton  2
#define CalSetButton   2
#define CalUpButton    1
#define CalDwnButton   3
#define TimeSetButton  3
#define MinSetButton   2
#define HourSetButton  1


// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(19,18,17,16,15,14);

// configure variables
int GPSpin = 0;             // RX PIN
int wwvbPin=11;             // WWVB receiver input pin
int txPin = 13;             // TX (HIGH on transmit)
int QRSStxPin = 12;         // QRSS TX/band select (HIGH on transmit)
int BandBCD0pin = 7;
int BandBCD1pin = 8;
int BandBCD2pin = 9;
int BandBCD3pin = 10;
int byteGPS=-1;
char buffer[300] = "";
char StartCommand[7] = "$GPGGA";
int IndiceCount=0,StartCount=0,counter=0;
int indices[13];
int second=0,minute=0,hour=0;
int pulseWidth=0,countSync=0,bitcount=0;
int wwvbData=0,wwvbHour=0,wwvbMin=0;
int Lat10,Lat1,NS,Lon100,Lon10,Lon1,EW,validGPSflag;
byte pinState = 0;
int mSecTimer,mSecTimer2,sat1,sat10;

// Load WSPR symbol frequency offsets
unsigned int OffsetFreq[4] = {
  0,   // 0 Hz  
  35,  // 1.46 Hz
  70,  // 2.93 Hz
  105  // 4.39 Hz
};
int val = 0;     // variable for reading the pin status
byte DATA = 6;   //DATA on DDS60 pin 3
byte CLOCK = 5;  //CLOCK on DDS60 pin 2
byte LOAD = 4;   //LOAD on DDS60 pin 1
byte count = 0,start = 0;
unsigned long mask = 1; // bitmask
byte temp = 1;
int inc = 3;
byte w0 = 1;
unsigned long FreqWord,TempWord,TempFreq,SineDelay;
unsigned long TenMHz = 10000000;
long CalFactor;
char buf[10];
volatile byte bb,i,j,ii,value,timeslot;
byte symbol[162];
byte c[11];                // encoded message
byte sym[170];             // symbol table 162
byte symt[170];            // symbol table temp
byte RXflag=1;    // GPS/WWVB receiver control 0 = disable, 1 = enable
int MsgLength;
volatile byte QRSScount,MorseByte,ByteMask,MorseBitCount,QRSSflag,WWVBflag1,WWVBflag2;
volatile byte MorseBit,MorseChar,SpaceCount,CharFlag,WordSpaceFlag;
volatile byte InhibitFlag = 0,GPSinhibitFlag = 0; // 1 will inhibit transmitter
unsigned long n1;    // encoded callsign
unsigned long m1;    // encodes locator
char cnt1;
unsigned char cc1;
float pi=3.14,wavecount=0;
double wave;

//******************************************************************
// Clock - interrupt routine used as master timekeeper
// Timer1 Overflow Interrupt Vector, called every second
// Timer 1 is also used for QRSS timing
ISR(TIMER1_COMPA_vect) {
  second++ ;
  if (second == 60) {
    minute++ ;
    second=0 ;
  }
  if (minute == 60) {
    hour++;
    minute=0 ;
  }
  if (hour == 24) {
    hour=0 ;
  }
  displaytime();
  if(InhibitFlag == 0)
  {
    if(bitRead(minute,0) == 0 & second == 0) 
    {
      if (minute < 20) {
        timeslot = minute/2;
      }
      else{
        if (minute < 40) {
          timeslot = (minute-20)/2;
        }
        else {
          timeslot = (minute -40)/2;
        }
      }
      setfreq();
      transmit();
    }
    if(QRSSflag == 1)
    { // QRSS transmit routine begins here:
      if(start < 4)
      { // Transmit QRSS base frequency for four seconds to start QRSS transmission
        TempFreq = QRSSfreq;
        TempWord = TempFreq*pow(2,32)/180000000;
        DisplayFreq();
        TempWord = (TempFreq+(CalFactor*(TempFreq/pow(10,7))))*pow(2,32)/180000000;
        TransmitSymbol();
        start++;
        QRSScount=0;
        MorseChar=0;
        MorseBit=0;
        CharFlag=0; 
      }
      else
      {
        MsgLength = (strlen(QRSSmsg));
        if (MorseChar>MsgLength-1)
        {
          if(Pattern >= 1)
          {
            i=0;
            ii=0;
            j=0;
            wavecount = 0;          
            if(Pattern == 1) wavecount = 270;//Offset to start on mark frequency
            QRSSflag = 0;      //Disable QRSS process
            TIMSK2 = 2;        //Timer2 enabled
          }
          else
            QRSScount=0;
          MorseChar=0;
          MorseBit=0;
          CharFlag=0;
        }
        else
          if((QRSSmsg[MorseChar] >= 97) & (QRSSmsg[MorseChar] <= 122)) 
          {
            temp = QRSSmsg[MorseChar]-87;          
          }
          else
            if((QRSSmsg[MorseChar] >= 65) & (QRSSmsg[MorseChar] <= 90))
            { 
              temp = QRSSmsg[MorseChar] - 55;  
            }
            else
              temp = QRSSmsg[MorseChar] - 48; 
        ByteMask = B00000111;
        MorseBitCount = ByteMask & Morse[temp]; 
        if(QRSSmsg[MorseChar] == 32)
        {
          WordSpace(); 
        }
        else
          if(bitRead(Morse[temp],(7-MorseBit)) == HIGH)
          {
            Dah();
          }
          else
          {
            Dit();
          }
        if (CharFlag >= 1)
        {
          CharSpace(); 
        }
      }
    }
  }
  else{
  }
  //  }
};


//******************************************************************
//Timer2 Overflow Interrupt Vector, called every mSec to increment
//the mSecTimer2 used for WSPR transmit timing.
//Timer2 is also used for QRSS pattern generation

ISR(TIMER2_COMPA_vect) {

  if (TransmitFlag[timeslot] == 2)
  { //QRSS pattern generator routines start here 
    ii++;
    if(ii > (QRSSdot*15))
    {
      TempFreq = QRSSfreq;
      TempWord = TempFreq*pow(2,32)/180000000;
      DisplayFreq();
      TempWord = (TempFreq+(CalFactor*(TempFreq/pow(10,7))))*pow(2,32)/180000000;
      switch(Pattern){
      case 1:  // sinewave
        wave = sin(wavecount*(pi/180));
        wave = (wave+1)/2;
        break;
      case 2:  // interrupted sinewave
        if(j == HIGH)
        {
          wave = 0.25;
        }
        else
          wave = sin(wavecount*(pi/180));
        wave = (wave+1)/2;
        break;        
      case 3:  // sawtooth
        if(wavecount < 180)
        {
          wave = wavecount/180;
        }
        else
          wave = 2-(wavecount/180);
        break;
      case 4:  // waves
        if(wavecount < 180)
        {
          wave = pow((wavecount/180),2);
        }
        else
          wave = pow((2-(wavecount/180)),2);
        break;         
      case 5:  // hills
        if(wavecount < 180)
        {
          wave = sqrt(wavecount/180);
        }
        else
          wave = sqrt(2-(wavecount/180));
        break; 
      case 6:  // herringbone
        if(wavecount < 180)
        {
          wave = wavecount/180;
        }
        else
          wave = 1.2 -(wavecount/180);
        break; 
      case 7:  // upramp
        wave = wavecount/360;
        break; 
      case 8:  // downramp
        wave = 1-(wavecount/360);
        break; 
      case 9:  // upramp sawtooth
        if(wavecount < 300)
        {
          wave = wavecount/300;
        }
        else
          wave = 1-((wavecount-300)/60);
        break; 
      case 10:  // downramp sawtooth
        if(wavecount < 60)
        {
          wave = wavecount/60;
        }
        else
          wave = 1-((wavecount-60)/300);          
        break;    
      case 11:  // M's
        if(i == 2)
        {
          wave = 0;
        }
        else
        {
          if(wavecount < 180)
          {
            wave = wavecount/180;
          }
          else
            wave = 2-(wavecount/180);
        }
        break;
      case 12:  // W's
        if(i == 2)
        {
          wave = 1;
        }
        else
        {
          if(wavecount < 180)
          {
            wave = 1-(wavecount/180);
          }
          else
            wave = (wavecount-180)/180;
        }
        break;
      }
      TempFreq = QRSSshift*(wave*QRSSfreq/pow(10,7))*pow(2,32)/180000000;
      TempWord = TempWord + TempFreq;
      TransmitSymbol();
      ii=0;
      wavecount++;
      if(wavecount>359)
      {
        wavecount=0;
        i++;
        if(i > 2) i=0;
        j=!j;
      }
    } 
  }
  else
  {// WSPR symbol tranmission routine begins here:
    mSecTimer2++;
    if(mSecTimer2 > 681){
      mSecTimer2 = 0;
      if(bb < 3) { // Begin 2 second delay - actually 0.682mSec * 3
        TempWord = FreqWord;
        TransmitSymbol();
        bb++;
      }
      else
      {
        // Begin 162 WSPR symbol transmission
        if (count < 162) 
        {
          TempWord = FreqWord + OffsetFreq[sym[count]];
          TransmitSymbol();
          count++;             //Increments the interrupt counter
        }
        else
        {
          TIMSK2 = 0;   // Disable WSPR timer
          digitalWrite(txPin,LOW); // External transmit control OFF 
          TempWord=0;            // Turn off transmitter
          TransmitSymbol(); 
          RXflag = 1;          //Turn GPS/WWVB receiver back on 
          lcd.setCursor(9,1);      
          lcd.print("IDLE         ");
        }
      }
    }
  }
};  


//******************************************************************
//******************************************************************

void setup()
{
  MySW.start();

  //Set up Timer2 to fire every mSec (WSPR timer)
  TIMSK2 = 0;        //Disable timer during setup
  TCCR2A = 2;        //CTC mode
  TCCR2B = 4;        //Timer Prescaler set to 64
  OCR2A = 247;       // Timer set for 1 mSec with correction factor

  //Set up Timer1A to fire every second (master clock)
  TCCR1B = 0;        //Disable timer during setup
  TIMSK1 = 2;        //Timer1 Interrupt enable
  TCCR1A = 0;        //Normal port operation, Wave Gen Mode normal
  TCCR1B = 12;       //Timer prescaler to 256 - CTC mode
  OCR1A = 62377;     //Timer set for 1000 mSec using correction factor
  // 62500 is nominal for 1000 mSec. Decrease variable to increase clock speed

  // set up the LCD for 16 columns and 2 rows 
  lcd.begin(16, 2); 

  //Set up GPS pin
  pinMode(GPSpin, INPUT);
  digitalWrite(GPSpin, HIGH); // internal pull-up enabled

  //Set up WWVB pin
  pinMode(wwvbPin, INPUT);
  digitalWrite(wwvbPin, HIGH); // internal pull-up enabled

  // Set up TX pin to output
  pinMode(txPin, OUTPUT);
  pinMode(QRSStxPin, OUTPUT);

  //Set up band select pins
  pinMode(BandBCD0pin, OUTPUT);
  pinMode(BandBCD1pin, OUTPUT);
  pinMode(BandBCD2pin, OUTPUT);
  pinMode(BandBCD3pin, OUTPUT);

  // Set GPS input to 4800 baud
  Serial.begin(4800);

  // Initialize a buffer for received data
  for (int i=0;i<300;i++){       
    buffer[i]=' ';
  }   

  // Set up transmit inhibit interrupt
  pinMode(InhibitButton, INPUT);
  digitalWrite(InhibitButton, HIGH); // internal pull-up enabled

  // Set up calibration pins 
  pinMode(CalSetButton, INPUT);    // declare pushbutton as input 
  digitalWrite(CalSetButton, HIGH); // internal pull-up enabled
  pinMode(CalUpButton, INPUT);    // declare pushbutton as input 
  digitalWrite(CalUpButton, HIGH); // internal pull-up enabled
  pinMode(CalDwnButton, INPUT);    // declare pushbutton as input 
  digitalWrite(CalDwnButton, HIGH); // internal pull-up enabled

  // Set up TimeSet pins
  pinMode(TimeSetButton, INPUT);    // declare pushbutton as input 
  digitalWrite(TimeSetButton, HIGH); // internal pull-up enabled  
  pinMode(HourSetButton, INPUT);     // declare pushbutton as input
  digitalWrite(HourSetButton, HIGH); // internal pull-up enabled
  pinMode(MinSetButton, INPUT);   // declare pushbutton as input
  digitalWrite(MinSetButton, HIGH); // internal pull-up enabled

  // Set up DDS-60
  pinMode (DATA, OUTPUT);   // sets pin 38 as OUPUT
  pinMode (CLOCK, OUTPUT);  // sets pin 37 as OUTPUT
  pinMode (LOAD, OUTPUT);   // sets pin 36 as OUTPUT

    // Turn on LCD 
  lcd.display();

  // turn off transmitter
  TempWord = 0;
  TransmitSymbol();

  // Display "IDLE" on LCD
  lcd.setCursor(9,1);
  lcd.print("IDLE   ");

  // Load Cal Factor
  for(j=0; j<3; j++) {  // Step through 4 bytes to make 32 bit word
    temp = EEPROM.read(50+j); // Get bytes 50 through 53
    for (i=0; i<8; i++){ // Set through 8 bit to make 1 byte
      bitWrite(CalFactor,i*j+i,(bitRead(temp,i))); // Assemble bits to make 32 bit word
    }
  }

  setfreq(); 

  // Begin WSPR message calculation
  encode_call();
  encode_locator();
  encode_conv();
  interleave_sync();


  // Calibration process follows:
  if(digitalRead(CalSetButton) == LOW)
  {
    FreqWord = TenMHz*pow(2,32)/180000000;
    TempWord = FreqWord;
    TransmitSymbol();
    CalFactor = 0;
    detachInterrupt(0);  // Disable transmit inhibit interrupt
    TIMSK0 = 0;          // Disable timer0 interrupt (WWVB)    
    TIMSK1 = 0;          // Disable timer1 interrupt (master clock)
    TIMSK2 = 0;          // Disable timer2 interrupt (WSPR)
    QRSSflag = 0;        // Disable QRSS process

    lcd.setCursor(0,0);
    lcd.print("Adjust to 10MHz");
    lcd.setCursor(0,1);
    lcd.print("Cal Factor= ");
    lcd.setCursor(12,1);
    lcd.print(CalFactor);
    calibrate();
  }

  // Manual time set process follows:
  if(digitalRead(TimeSetButton) == LOW)
  {
    TIMSK1 = 0;        // Interrupt disable to stop clock
    hour = 0;
    minute = 0;
    second = 0;
    displaytime();
    detachInterrupt(0);  // Disable transmit inhibit interrupt
    timeset();
  }

  attachInterrupt(0, TXinhibit, LOW);  // TX inhibit pin on interrupt 1 - pin 3
}

//******************************************************************
//******************************************************************

void loop()
{
  if (RXflag == 1)
  {   
    if (digitalRead(wwvbPin) == LOW) wwvbStart() ;

    else
      GPSprocess();
  }
}

//******************************************************************
void encode() 
{
  encode_call();
  encode_locator();
  encode_conv();
  interleave_sync();
};
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

//******************************************************************
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

//******************************************************************
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

//******************************************************************
// convolutional encoding of message array c[] into a 162 bit stream
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

//******************************************************************
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

//******************************************************************
// interleave reorder the 162 data bits and and merge table with the sync vector
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

//******************************************************************
// Determine time slot and load band frequency data. Display 
// frequency and calculate frequency word for DDS
void setfreq()
{
  // Select band port pin and set HIGH for active band
  if(bitRead(timeslot,0)==1)digitalWrite(BandBCD0pin,HIGH);
  else
    digitalWrite(BandBCD0pin,LOW);
  if(bitRead(timeslot,1)==1)digitalWrite(BandBCD1pin,HIGH);
  else
    digitalWrite(BandBCD1pin,LOW);
  if(bitRead(timeslot,2)==1)digitalWrite(BandBCD2pin,HIGH);
  else
    digitalWrite(BandBCD2pin,LOW);
  if(bitRead(timeslot,3)==1)digitalWrite(BandBCD3pin,HIGH);
  else
    digitalWrite(BandBCD3pin,LOW);

  // Print frequency to the LCD
  lcd.setCursor(0,0);

  ltoa(band[timeslot],buf,10);

  if (buf[7]==0) {
    lcd.print(buf[0]);
    lcd.print(',');
    lcd.print(buf[1]);
    lcd.print(buf[2]);
    lcd.print(buf[3]);
    lcd.print('.');
    lcd.print(buf[4]);
    lcd.print(buf[5]);
    lcd.print(buf[6]);
    lcd.print(" KHz  ");
  }
  else {
    lcd.print(buf[0]);
    lcd.print(buf[1]);
    lcd.print(',');
    lcd.print(buf[2]);
    lcd.print(buf[3]);
    lcd.print(buf[4]);
    lcd.print('.');
    lcd.print(buf[5]);
    lcd.print(buf[6]);
    lcd.print(buf[7]);
    lcd.print(" KHz  ");
  }

  FreqWord = (band[timeslot]+(CalFactor*(band[timeslot]/pow(10,7))))*pow(2,32)/180000000;

  return;
}


//******************************************************************
// Determine if it is time to transmit. If so, determine if it is
// time to transmit the QRSS or the WSPR message

void transmit()
{
  if(GPSinhibitFlag == 1 || InhibitFlag ==1)return;
  else
    if(WWVBflag1 == 1 && WWVBflag2 != 1)return;
    else
      if (TransmitFlag[timeslot] == 1)
      { // Start WSPR transmit process
        start = 0;
        RXflag = 0;     // Disable GPS/WWVB receiver
        WWVBflag1 = 0;  // Disable WWVB receive timing
        QRSSflag = 0;   // Disable QRSS process
        lcd.setCursor(9,1); 
        lcd.print("WSPR TX");
        digitalWrite(txPin,HIGH); // External transmit control ON
        digitalWrite(QRSStxPin,LOW); // External QRSS transmit control OFF
        bb =0;
        count = 0;            // Start WSPR symbol transmit process
        TIMSK0 = 0;   //Disable WWVB timer
        TIMSK2 = 2;   //Enable timer2 interrupt 
        return;
      }
      else
        if (TransmitFlag[timeslot] == 2)
        { // Start QRSS transmit process
          if(TIMSK2 == 2)return; // Do not disturb while transmitting waveform
          else
            lcd.setCursor(9,1); 
          lcd.print("QRSS TX"); 
          RXflag = 0; //Disable GPS/WWVB receiver
          TIMSK2 = 0;          // Disable timer2 interrupt (WSPR)
          QRSSflag = 1;  // Enable QRSS process
          digitalWrite(txPin,HIGH); // External transmit control ON
          digitalWrite(QRSStxPin,HIGH); // External QRSS transmit control ON
          return;
        }
        else  
      { // Turn off transmitter and idle
        lcd.setCursor(9,1);
        TIMSK2 = 0; //Turn off WSPR timer
        QRSSflag = 0; //Disable QRSS process
        start = 0;
        lcd.print("IDLE   ");
        digitalWrite(txPin,LOW); // External transmit control OFF
        digitalWrite(QRSStxPin,LOW); // External QRSS transmit control OFF
        RXflag = 1; //Enable GPS/WWVB receiver
        TempWord=0;
        TransmitSymbol();
        return;
      }
}

//******************************************************************
void TransmitSymbol()
{
  for (mask = 1; mask>0; mask <<= 1) { //iterate through bit mask
    if (TempWord & mask){ // if bitwise AND resolves to true
      digitalWrite(DATA,HIGH); // send 1
    }
    else{ //if bitwise and resolves to false
      digitalWrite(DATA,LOW); // send 0
    }
    digitalWrite(CLOCK,HIGH); // Clock data in
    digitalWrite(CLOCK,LOW);
  }
  for (temp = 1; temp>0; temp <<= 1) { //iterate through bit mask
    if (w0 & temp){ // if bitwise AND resolves to true
      digitalWrite(DATA,HIGH); // send 1
    }
    else{ //if bitwise and resolves to false
      digitalWrite(DATA,LOW); // send 0
    } 
    digitalWrite(CLOCK,HIGH); // Clock data in
    digitalWrite(CLOCK,LOW);
  } 
  digitalWrite (LOAD, HIGH); // Turn on pin 36/1 LOAD FQ-UD
  digitalWrite (LOAD, LOW); // Turn pin 36 off
  return;
}


/*
_______________________________________________________________
 WWVB timing process starts here
 _______________________________________________________________
 */
void wwvbStart() {
  lcd.setCursor(9,1); 
  lcd.print("WWVB RX"); 
  TIMSK0 = 1;   //Enable timer0 interrupt
  bitcount=0;
  WWVBflag1 = 1;
  wwvbData = 0;
  wwvbLoop() ;
}

void pulsedetect() {
  while (digitalRead(wwvbPin) == HIGH) {
  };
  if (countSync >= 2) process();
}

void wwvbLoop() {
  // Search for double sync pulse to start process:
  MySW.reset();
  MySW.start();
  while (digitalRead(wwvbPin) == LOW) {
  };
  MySW.stop();
  Serial.println(MySW.value(),DEC);
  if (MySW.value() > 775 && MySW.value() < 825) {
    countSync++ ;
    if (countSync == 2){
      second = 0 ; // Set to top of minute plus one second
      pulsedetect(); 
    } 
  }
  else
  {
    countSync=0;
    loop(); // Timing error - start over
  }
}

void process() {
  // Double sync pulse found - continue with processing:
  MySW.reset();
  MySW.start();
  while (digitalRead(wwvbPin) == LOW) {
  };
  MySW.stop();
  Serial.println(MySW.value(),DEC);
  bitcount++ ;
  if (bitcount == 10) pulsedetect() ;//Discard unwanted pulse 
  if (MySW.value() > 775 && MySW.value() < 825){ 
    pulse3() ;
  }
  else
    if (MySW.value() > 475 && MySW.value() < 525) {
      wwvbData = wwvbData << 1;
      bitSet(wwvbData,0);
      pulsedetect(); // Get next bit 
    }
    else
      if (MySW.value() > 175 && MySW.value() < 225) {
        wwvbData = wwvbData << 1;
        bitClear(wwvbData,0);
        pulsedetect(); // Get next bit
      }
      else
      {
        TIMSK0 = 0; //Turn off timer0
        loop(); // Timing error - start over
      }
}

// Sync pulse detected 
void pulse3() {
  countSync++ ; 
  if (countSync < 4 ) pulsedetect() ;
  else
    if (countSync == 4)
    {
      wwvbHour = 0 ;
      wwvbMin = 0 ;
      if (bitRead(wwvbData,0)==1) wwvbHour +=1 ;
      if (bitRead(wwvbData,1)==1) wwvbHour +=2 ;
      if (bitRead(wwvbData,2)==1) wwvbHour +=4 ;
      if (bitRead(wwvbData,3)==1) wwvbHour +=8 ;
      if (bitRead(wwvbData,5)==1) wwvbHour +=10 ;
      if (bitRead(wwvbData,6)==1) wwvbHour +=20 ;

      if (bitRead(wwvbData,8)==1) wwvbMin +=1 ;
      if (bitRead(wwvbData,9)==1) wwvbMin +=2 ;
      if (bitRead(wwvbData,10)==1) wwvbMin +=4 ;
      if (bitRead(wwvbData,11)==1) wwvbMin +=8 ;
      if (bitRead(wwvbData,13)==1) wwvbMin +=10 ;
      if (bitRead(wwvbData,14)==1) wwvbMin +=20 ;
      if (bitRead(wwvbData,15)==1) wwvbMin +=40 ;
    }
    else
    {
      if (countSync == 7) // 50 seconds of valid data required before time update
      {
        WWVBflag2 = 1;
        if (hour != wwvbHour) hour = wwvbHour ;
        if (minute != wwvbMin) minute = wwvbMin ;
        TIMSK0 = 0; //Turn off timer0
      }
    }
  pulsedetect();
}

/*
_______________________________________________________________
 GPS timing process starts here
_______________________________________________________________
 */
void GPSprocess()
{
  byte pinState = 0;
  byteGPS=Serial.read();         // Read a byte of the serial port
  if (byteGPS == -1) {           // See if the port is empty yet
    delay(100); 
  } 
  else {
    buffer[counter]=byteGPS;        // If there is serial port data, it is put in the buffer
    counter++;                      
    if (byteGPS==13){            // If the received byte is = to 13, end of transmission
      IndiceCount=0;
      StartCount=0;
      for (int i=1;i<7;i++){     // Verify the received command starts with $GPGGA
        if (buffer[i]==StartCommand[i-1]){
          StartCount++;
        }
      }
      if(StartCount==6){               // If yes, continue and process the data
        for (int i=0;i<300;i++){
          if (buffer[i]==','){    // check for the position of the  "," separator
            indices[IndiceCount]=i;
            IndiceCount++;
          }
          if (buffer[i]=='*'){    // ... and the "*"
            indices[12]=i;
            IndiceCount++;
          }
        }
        // Load time data
        temp = indices[0];
        hour = (buffer[temp+1]-48)*10 + buffer[temp+2]-48;
        minute = (buffer[temp+3]-48)*10 + buffer[temp+4]-48;
        second = (buffer[temp+5]-48)*10 + buffer[temp+6]-48; 
        // Load latitude and logitude data          
        temp = indices[1];
        Lat10 = buffer[temp+1]-48;
        Lat1 = buffer[temp+2]-48;
        temp = indices[2];
        NS = buffer[temp+1];
        temp = indices[3];
        Lon100 = buffer[temp+1]-48;
        Lon10 = buffer[temp+2]-48;
        Lon1 = buffer[temp+3]-48;         
        temp = indices[4];
        EW = buffer[temp+1];
        temp = indices[5];
        validGPSflag = buffer[temp+1]-48;
        temp = indices[6];
        sat10 = buffer[temp+1]-48;
        sat1 = buffer[temp+2]-48;
        if(validGPSflag ==1)GPSinhibitFlag = 0;
        else
        {
          GPSinhibitFlag=1;
        }
      }
      counter=0;                    // Reset the buffer
      for (int i=0;i<300;i++){    //  
        buffer[i]=' ';             
      }
      lcd.setCursor(9,1); 
      lcd.print("GPS    "); 
      lcd.setCursor(13,1); 
      lcd.print(sat10);
      lcd.print(sat1);       
    }
  } 
}

//******************************************************************
void calibrate()
{ // Process to determine frequency calibration factor
  while(digitalRead(CalSetButton) == LOW)
  {
    if (digitalRead(CalUpButton) == LOW)
    {
      for(TempWord=0; TempWord < 350000; TempWord++) {
      }; //crude debounce delay
      CalFactor++;
    };
    if (digitalRead(CalDwnButton) == LOW)
    {
      for(TempWord=0; TempWord < 350000; TempWord++) {
      }; //crude debounce delay
      CalFactor--;
    };
    TempWord = (TenMHz+CalFactor)*pow(2,32)/180000000;
    TransmitSymbol();
    lcd.setCursor(12,1);
    lcd.print(CalFactor);
    lcd.print("   ");
  }

  // Writes CalFactor to address 50 + 3 bytes of EEprom
  for (j=0; j<3; j++)  { // Step through 4 bytes to make 32 bit word
    for (i=0; i<8; i++) { // Step through 8 bit to make 1 byte
      bitWrite(temp,i,(bitRead(CalFactor,j*i+i))); // Assemble bits to make 1 byte
    }
    EEPROM.write(50+j,temp); // Write byte to EEprom
  }

  EEPROM.write(54,1);
  lcd.setCursor(9,1);
  lcd.print("IDLE   ");
  TempWord = 0; // turn off 10 MHz calibrate signal
  TransmitSymbol();
  setfreq(); 
  attachInterrupt(0, TXinhibit, LOW);  // TX inhibit pin on interrupt 1 - pin 3  
  TIMSK1 = 2;          // Enable Timer1 Interrupt 
}   


//******************************************************************
void timeset()
{ // Process to manually set master time clock
  lcd.setCursor(9,1);
  lcd.print("SetTime");
  while(digitalRead(TimeSetButton) == LOW)
  {
    if (digitalRead(MinSetButton) == LOW)
    {
      for(TempWord=0; TempWord < 750000; TempWord++) {
      }; //crude debounce delay
      minute++;
      if(minute > 59) minute = 0;
      displaytime();
    };
    if (digitalRead(HourSetButton) == LOW)
    {
      for(TempWord=0; TempWord < 750000; TempWord++) {
      }; //crude debounce delay
      hour++;
      if(hour > 23) hour = 0;
      displaytime();
    };
  }

  TempWord = 0;
  TransmitSymbol();
  lcd.setCursor(9,1);
  lcd.print("IDLE   ");
  attachInterrupt(0, TXinhibit, LOW);  // TX inhibit pin on interrupt 1 - pin 3
  TIMSK1 = 2;          // Enable Timer1 Interrupt 
}


//******************************************************************
void displaytime()
{
  lcd.setCursor(0,1);
  lcd.display();
  if (hour < 10) lcd.print ("0");
  lcd.print (hour);
  lcd.print (":");
  if (minute < 10) lcd.print ("0");
  lcd.print (minute);
  lcd.print (":");
  if (second < 10) lcd.print ("0");
  lcd.print (second);
  lcd.print (" "); 
  return;  
}

//******************************************************************
void DisplayFreq()
{
  // Print frequency to the LCD
  lcd.setCursor(0,0);

  ltoa(TempFreq,buf,10);

  if (buf[7]==0) {
    lcd.print(buf[0]);
    lcd.print(',');
    lcd.print(buf[1]);
    lcd.print(buf[2]);
    lcd.print(buf[3]);
    lcd.print('.');
    lcd.print(buf[4]);
    lcd.print(buf[5]);
    lcd.print(buf[6]);
    lcd.print(" KHz ");
  }
  else {
    lcd.print(buf[0]);
    lcd.print(buf[1]);
    lcd.print(',');
    lcd.print(buf[2]);
    lcd.print(buf[3]);
    lcd.print(buf[4]);
    lcd.print('.');
    lcd.print(buf[5]);
    lcd.print(buf[6]);
    lcd.print(buf[7]);
    lcd.print(" KHz ");
  }
}


//******************************************************************
void Dah()
{
  TempFreq = QRSSfreq + QRSSshift;
  TempWord = TempFreq*pow(2,32)/180000000;
  DisplayFreq();
  TempWord = (TempFreq+(CalFactor*(TempFreq/pow(10,7))))*pow(2,32)/180000000;
  TransmitSymbol();
  QRSScount++;
  if(QRSScount > (QRSSdot*3))
  {
    TempFreq = QRSSfreq;
    TempWord = TempFreq*pow(2,32)/180000000;
    DisplayFreq();
    TempWord = (TempFreq+(CalFactor*(TempFreq/pow(10,7))))*pow(2,32)/180000000;
    TransmitSymbol();
    SpaceCount++;
    if(SpaceCount > QRSSdot)
    {
      QRSScount=0;
      MorseBit++;
      SpaceCount=0;
    }
  }
  if(MorseBit > MorseBitCount-1)
  {
    MorseBit=0;
    MorseChar++;
    CharFlag=1;
  }
  return;
}

//******************************************************************
void Dit()
{
  TempFreq = QRSSfreq + QRSSshift;
  TempWord = TempFreq*pow(2,32)/180000000;
  DisplayFreq();
  TempWord = (TempFreq+(CalFactor*(TempFreq/pow(10,7))))*pow(2,32)/180000000;
  TransmitSymbol();
  QRSScount++;
  if(QRSScount > QRSSdot)
  {
    TempFreq = QRSSfreq;
    TempWord = TempFreq*pow(2,32)/180000000;
    DisplayFreq();
    TempWord = (TempFreq+(CalFactor*(TempFreq/pow(10,7))))*pow(2,32)/180000000;
    TransmitSymbol();
    SpaceCount++;
    if(SpaceCount > QRSSdot)
    {
      QRSScount=0;
      MorseBit++;
      SpaceCount=0;
    }
  }
  if(MorseBit > MorseBitCount-1)
  {
    MorseBit=0;
    MorseChar++;
    CharFlag=1;
  }
  return;
}

//******************************************************************
void CharSpace()
{
  TempFreq = QRSSfreq;
  TempWord = TempFreq*pow(2,32)/180000000;
  DisplayFreq();
  TempWord = (TempFreq+(CalFactor*(TempFreq/pow(10,7))))*pow(2,32)/180000000;
  TransmitSymbol();
  CharFlag++;
  if(CharFlag > QRSSdot*2)
  {
    CharFlag=0;
    MorseBit=0;
    QRSScount=0;
    SpaceCount=0;
  }
  return;
}

//******************************************************************
void WordSpace()
{
  TempFreq = QRSSfreq;
  TempWord = TempFreq*pow(2,32)/180000000;
  DisplayFreq();
  TempWord = (TempFreq+(CalFactor*(TempFreq/pow(10,7))))*pow(2,32)/180000000;
  TransmitSymbol();
  CharFlag=0;
  WordSpaceFlag++;
  if(WordSpaceFlag > (QRSSdot*6))
  {
    MorseChar++;
    WordSpaceFlag=0;
    QRSScount=0;
    MorseBit=0;
  }
  return;
}

//******************************************************************
void TXinhibit()
{
  for(TempWord=0; TempWord < 750000; TempWord++) {
  }; //crude debounce delay
  InhibitFlag = !InhibitFlag;
  if(InhibitFlag == 1) // 1 turns OFF transmitter
  {
    lcd.setCursor(15,0);
    lcd.print("*");
    TIMSK0 = 0; //Turn off WWVB timer
    TIMSK2 = 0; //Turn off WSPR timer
    QRSSflag = 0; //Disable QRSS process
    start = 0;
    digitalWrite(txPin,LOW); // External transmit control OFF
    digitalWrite(QRSStxPin,LOW); // External QRSS transmit control OFF
    RXflag = 1; //Enable GPS/WWVB receiver
    TempWord=0; //Set DDS60 to 0Hz
    TransmitSymbol();
    return;      
  }
  else
    lcd.setCursor(15,0);
  lcd.print(" ");
}








































































































