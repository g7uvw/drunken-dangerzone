//AD9850 DDS test

#define DDS_CLOCK 125000000

#define  CLOCK  8  //pin connections for DDS
#define  LOAD 9 
#define  DATA  10
#define  RESET 11

unsigned char MARTIN1_VIS = 00101100;
unsigned long base_frequency = 14230000;    //20m SSTV calling

void setup()
{
  pinMode (DATA,  OUTPUT); 
  pinMode (CLOCK, OUTPUT); 
  pinMode (LOAD,  OUTPUT); 
  pinMode (RESET, OUTPUT); 
  AD9850_init();
  AD9850_reset();

}

void loop()
{
Send_VIS_code();
Send_sync();
int lines = 0;
for (lines = 0;lines<256; lines++)
{
 Bars(); 
}
}

void Send_VIS_code()
{
 /*
The VIS code is sent as:            
30 mS start of 1200 Hz.          
7 data bits sent LSB first,                  
30 mS each, 1100 Hz for 1,1300 Hz for 0.          
Even parity bit of 30 mS.          
30 mS stop of 1200 Hz.
*/

 int i;
 SetFrequency(base_frequency + 1200);
 delay(30);
 
 for (i=0;i<8;i++)
 {
   if(MARTIN1_VIS & (1<<i))
   {
     SetFrequency(base_frequency + 1100);
   }
   else
   {
     SetFrequency(base_frequency + 1300);
   }
   delay(30);
   }
   
 SetFrequency(base_frequency + 1200);
 delay(30);
}

void Send_sync()
{
 
 SetFrequency(base_frequency + 1200);
 delay(5);
}

void Bars()
{
  Send_sync();
  //Green
  SetFrequency(base_frequency + 2300);
  delay(62);
  SetFrequency(base_frequency + 1500);
  delay(62);
  
  //Blue
  SetFrequency(base_frequency + 2300);
  delay(28);
  SetFrequency(base_frequency + 1500);
  delay(28);
  
  //Red
  SetFrequency(base_frequency + 2300);
  delay(36);
  SetFrequency(base_frequency + 1500);
  delay(36);
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



