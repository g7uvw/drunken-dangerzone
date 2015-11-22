// QRSS Transmitter from Si5351
// George Smart, M1GEO.
//  here: http://www,george-smart.co.uk/wiki/Arduino_QRSS
//

#include "si5351.h"
#include "Wire.h"

Si5351 si5351;

// QRSS Timings in milliseconds
#define CW_DIT  53
#define CW_DAH  (CW_DIT*3)
#define QRSS_DIT  7200
#define QRSS_DAH  (QRSS_DIT*3)

#include <stdint.h>

#define CW      0
#define QRSS    1
#define ASK     0
#define FSK     1

unsigned long next_time;

#define OFFST -13000 // units of 0.01Hz
#define SHIFT +1000
#define FREQA (14449200000 + OFFST)
#define FREQB (FREQA + SHIFT)

void modulate(int a) {
  if (a==0) {
    si5351.set_freq(0, SI5351_PLL_FIXED, SI5351_CLK0);
  }
  if (a==1) {
    si5351.set_freq(FREQA, SI5351_PLL_FIXED, SI5351_CLK0);
  }
  if (a==2) {
    si5351.set_freq(FREQB, SI5351_PLL_FIXED, SI5351_CLK0);
  }
  
}

void setup()
{
  
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0);
  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA);

  // Setup RS232
  Serial.begin(9600);

  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  sendDah(CW,ASK); 
  sendDah(CW,ASK); 
  sendDah(CW,ASK); 
  sendDah(CW,ASK); 
  sendDah(CW,ASK); 
  sendDah(CW,ASK); 
  sendDah(CW,ASK); 
  sendDah(CW,ASK); 
  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
}

void loop()
{
  
  next_time = millis() + 600000;
  
  modulate(0);
  delay(1000);
  
  sendPattern("-- .---- --. . --- / .. -. / .--- --- ----- .---- -.-. -. / .-.-", CW, ASK);

  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 

  sendPattern(".--. ... . / --.- ... .-.. / .-.-", CW, ASK);
  
  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  
  sendBattery();
  
  modulate(0);
  delay(1000);

  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  waitDah(CW); 
  
  sendPattern("-- .---- --. . ---", QRSS, FSK);

  // wait for the remiaining 10 minutes slot
  modulate(1);
  while (millis() < next_time);
}


void waitDit(short T) {
  (T) ? delay(QRSS_DIT) : delay(CW_DIT);
}

void waitDah(short T) {
  (T) ? delay(QRSS_DAH) : delay(CW_DAH);
}

void sendDitASK(short T) {
  modulate(1);
  waitDit(T);
  modulate(0);
  waitDit(T);  // always end on a dit duration
}

void sendDahASK(short T) {
  modulate(1);
  waitDah(T);
  modulate(0);
  waitDit(T);  // always end on a dit duration
}

void sendDitFSK(short T) {
  modulate(2);
  waitDit(T);
  modulate(1);
  waitDit(T);  // always end on a dit duration
}

void sendDahFSK(short T) {
  modulate(2);
  waitDah(T);
  modulate(1);
  waitDit(T);  // always end on a dit duration
}

void sendDit(short T, short M) {
  (M) ? sendDitFSK(T) : sendDitASK(T);
}  

void sendDah(short T, short M) {
  (M) ? sendDahFSK(T) : sendDahASK(T);
}

void sendPattern(String str, short T,  short M) {
  int strLen = str.length();
  Serial.print(strLen);
  Serial.print(" elements: ");
  
  if (M && T) {
    modulate(1);
    waitDah(T);
  }
  
  int i=0;
  for(i=0;i<strLen;i++) {
    switch (str.charAt(i)) {
    case '-':
      Serial.print("-");
      sendDah(T,M);
      break;
    case '.':
      Serial.print(".");
      sendDit(T,M);
      break;
    case '/':
      Serial.print("/");
      waitDah(T);
      waitDah(T);
      waitDah(T);
      break;
    case ' ':
      Serial.print(" ");
      waitDah(T);
      break;
    default: 
      Serial.print("(I didn't recognise '");
      Serial.print(str.charAt(i));
      Serial.print("'.  Permitted Chars are -  .  (space)  or  / )");
    }
  }
  
  if (M && T) {
    modulate(1);
    waitDah(T);
  } 
}

void sendNumber(int d) {

  switch (d) {
    case 1:
      sendPattern(".---- ", CW, ASK); // 1
      break;
    case 2:
      sendPattern("..--- ", CW, ASK); // 2
      break;
    case 3:
      sendPattern("...-- ", CW, ASK); // 3
      break;
    case 4:
      sendPattern("....- ", CW, ASK); // 4
      break;
    case 5:
      sendPattern("..... ", CW, ASK); // 5
      break;
    case 6:
      sendPattern("-.... ", CW, ASK); // 6
      break;  
    case 7:
      sendPattern("--... ", CW, ASK); // 7
      break;
    case 8:
      sendPattern("---.. ", CW, ASK); // 8
      break;
    case 9:
      sendPattern("----. ", CW, ASK); // 9
      break;
    case 0:
      sendPattern("----- ", CW, ASK); // 0
      break;
    case '.':
      sendPattern(".-.-.- ", CW, ASK); // .
      break;      
  } 
}

void sendBattery() {
  int sensorValue = analogRead(A0);
  float voltage = 3 * sensorValue * (5.0 / 1024.0);
  int value = voltage*100;
  int num[4];
  
  Serial.println(sensorValue);
  Serial.println(voltage);
  Serial.println(value);
  
  sendPattern("-... .- - - /", CW, ASK);
  int i = 0;
  while (value > 0) {
   int digit = value % 10;
   num[i]=digit;
   value /= 10;
   i++;
  }
  
  for (i=4;i>=0;i--) {
    if (i==1) {
      sendNumber('.');
    }
    sendNumber(num[i]);
  }
  
  
  sendPattern("...- / .-.-", CW, ASK);
}
