
#include "si5351.h"
#include "Wire.h"
#define freq 14449200000
Si5351 si5351;

void setup()
{
  // Start serial and initialize the Si5351
  Serial.begin(57600);

  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0);
  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA);
}

void loop()
{
  
 for(;;)
 {
   
   on();
   delay(3000);
   off();
   delay(3000);
 }
  
  
}

void on()
{
  si5351.set_freq(freq, SI5351_PLL_FIXED, SI5351_CLK0);
}

void off()
{
  si5351.set_freq(0, SI5351_PLL_FIXED, SI5351_CLK0);
}
