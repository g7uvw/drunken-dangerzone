/*
  Example animated analogue meters using a ILI9341 TFT LCD screen

  Needs Font 2 (also Font 4 if using large scale label)

  Make sure all the display driver and pin comnenctions are correct by
  editting the User_Setup.h file in the TFT_eSPI library folder.

  #########################################################################
  ###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE LIBRARY ######
  #########################################################################
*/

#include <M5Stack.h> 
#include "DHT12.h"
#include <Wire.h>     //The DHT12 uses I2C comunication.
DHT12 dht12;          //Preset scale CELSIUS and ID 0x5c.

#define TFT_GREY 0x5AEB

#define LOOP_PERIOD 100 // Display updates every 10Sec

float ltx = 0;    // Saved x coord of bottom of needle
uint16_t osx = 120, osy = 120; // Saved x & y coords
uint32_t updateTime = 0;       // time for next update

int old_analog =  -999; // Value last displayed
int old_digital = -999; // Value last displayed

int value[6] = {0, 0, 0, 0, 0, 0};
int old_value[6] = { -1, -1, -1, -1, -1, -1};
int d = 0;

void setup(void) {
  M5.begin();
  // M5.Lcd.setRotation(2);
    Wire.begin();
  M5.Lcd.fillScreen(TFT_BLACK);

  analogMeter(); // Draw analogue meter

  // Draw 6 linear meters
  byte d = 40;
  plotLinear("T", 0, 160);
  updateTime = millis(); // Next update time
}


void loop() {
  if (updateTime <= millis()) {
    updateTime = millis() + LOOP_PERIOD;


    value[0] = dht12.readHumidity();//map(analogRead(A0), 0, 1023, 0, 100); // Test with value form Analogue 0
    plotNeedle(value[0], 0);
    value[0] = dht12.readTemperature();
    plotPointer();

  }
}


// #########################################################################
//  Draw the analogue meter on the screen
// #########################################################################
void analogMeter()
{
  // Meter outline
  M5.Lcd.fillRect(0, 0, 239, 126, TFT_GREY);
  M5.Lcd.fillRect(5, 3, 230, 119, TFT_WHITE);

  M5.Lcd.setTextColor(TFT_BLACK);  // Text colour

  // Draw ticks every 5 degrees from -50 to +50 degrees (100 deg. FSD swing)
  for (int i = -50; i < 51; i += 5) {
    // Long scale tick length
    int tl = 15;

    // Coodinates of tick to draw
    float sx = cos((i - 90) * 0.0174532925);
    float sy = sin((i - 90) * 0.0174532925);
    uint16_t x0 = sx * (100 + tl) + 120;
    uint16_t y0 = sy * (100 + tl) + 140;
    uint16_t x1 = sx * 100 + 120;
    uint16_t y1 = sy * 100 + 140;

    // Coordinates of next tick for zone fill
    float sx2 = cos((i + 5 - 90) * 0.0174532925);
    float sy2 = sin((i + 5 - 90) * 0.0174532925);
    int x2 = sx2 * (100 + tl) + 120;
    int y2 = sy2 * (100 + tl) + 140;
    int x3 = sx2 * 100 + 120;
    int y3 = sy2 * 100 + 140;

    // Yellow zone limits
    //if (i >= -50 && i < 0) {
    //  M5.Lcd.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_YELLOW);
    //  M5.Lcd.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_YELLOW);
    //}

    // Green zone limits
    if (i >= 0 && i < 25) {
      M5.Lcd.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_GREEN);
      M5.Lcd.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_GREEN);
    }

    // Orange zone limits
    if (i >= 25 && i < 50) {
      M5.Lcd.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_ORANGE);
      M5.Lcd.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_ORANGE);
    }

    // Short scale tick length
    if (i % 25 != 0) tl = 8;

    // Recalculate coords incase tick lenght changed
    x0 = sx * (100 + tl) + 120;
    y0 = sy * (100 + tl) + 140;
    x1 = sx * 100 + 120;
    y1 = sy * 100 + 140;

    // Draw tick
    M5.Lcd.drawLine(x0, y0, x1, y1, TFT_BLACK);

    // Check if labels should be drawn, with position tweaks
    if (i % 25 == 0) {
      // Calculate label positions
      x0 = sx * (100 + tl + 10) + 120;
      y0 = sy * (100 + tl + 10) + 140;
      switch (i / 25) {
        case -2: M5.Lcd.drawCentreString("0", x0, y0 - 12, 2); break;
        case -1: M5.Lcd.drawCentreString("25", x0, y0 - 9, 2); break;
        case 0: M5.Lcd.drawCentreString("50", x0, y0 - 6, 2); break;
        case 1: M5.Lcd.drawCentreString("75", x0, y0 - 9, 2); break;
        case 2: M5.Lcd.drawCentreString("100", x0, y0 - 12, 2); break;
      }
    }

    // Now draw the arc of the scale
    sx = cos((i + 5 - 90) * 0.0174532925);
    sy = sin((i + 5 - 90) * 0.0174532925);
    x0 = sx * 100 + 120;
    y0 = sy * 100 + 140;
    // Draw scale arc, don't draw the last part
    if (i < 50) M5.Lcd.drawLine(x0, y0, x1, y1, TFT_BLACK);
  }

  M5.Lcd.drawString("%RH", 5 + 230 - 40, 119 - 20, 2); // Units at bottom right
  M5.Lcd.drawCentreString("%RH", 120, 70, 4); // Comment out to avoid font 4
  M5.Lcd.drawRect(5, 3, 230, 119, TFT_BLACK); // Draw bezel line

  plotNeedle(0, 0); // Put meter needle at 0
}

// #########################################################################
// Update needle position
// This function is blocking while needle moves, time depends on ms_delay
// 10ms minimises needle flicker if text is drawn within needle sweep area
// Smaller values OK if text not in sweep area, zero for instant movement but
// does not look realistic... (note: 100 increments for full scale deflection)
// #########################################################################
void plotNeedle(int value, byte ms_delay)
{
  M5.Lcd.setTextColor(TFT_BLACK, TFT_WHITE);
  char buf[8]; dtostrf(value, 4, 0, buf);
  M5.Lcd.drawRightString(buf, 40, 119 - 20, 2);

  if (value < -10) value = -10; // Limit value to emulate needle end stops
  if (value > 110) value = 110;

  // Move the needle util new value reached
  while (!(value == old_analog)) {
    if (old_analog < value) old_analog++;
    else old_analog--;

    if (ms_delay == 0) old_analog = value; // Update immediately id delay is 0

    float sdeg = map(old_analog, -10, 110, -150, -30); // Map value to angle
    // Calcualte tip of needle coords
    float sx = cos(sdeg * 0.0174532925);
    float sy = sin(sdeg * 0.0174532925);

    // Calculate x delta of needle start (does not start at pivot point)
    float tx = tan((sdeg + 90) * 0.0174532925);

    // Erase old needle image
    M5.Lcd.drawLine(120 + 20 * ltx - 1, 140 - 20, osx - 1, osy, TFT_WHITE);
    M5.Lcd.drawLine(120 + 20 * ltx, 140 - 20, osx, osy, TFT_WHITE);
    M5.Lcd.drawLine(120 + 20 * ltx + 1, 140 - 20, osx + 1, osy, TFT_WHITE);

    // Re-plot text under needle
    M5.Lcd.setTextColor(TFT_BLACK);
    M5.Lcd.drawCentreString("%RH", 120, 70, 4); // // Comment out to avoid font 4

    // Store new needle end coords for next erase
    ltx = tx;
    osx = sx * 98 + 120;
    osy = sy * 98 + 140;

    // Draw the needle in the new postion, magenta makes needle a bit bolder
    // draws 3 lines to thicken needle
    M5.Lcd.drawLine(120 + 20 * ltx - 1, 140 - 20, osx - 1, osy, TFT_RED);
    M5.Lcd.drawLine(120 + 20 * ltx, 140 - 20, osx, osy, TFT_MAGENTA);
    M5.Lcd.drawLine(120 + 20 * ltx + 1, 140 - 20, osx + 1, osy, TFT_RED);

    // Slow needle down slightly as it approaches new postion
    if (abs(old_analog - value) < 10) ms_delay += ms_delay / 5;

    // Wait before next update
    delay(ms_delay);
  }
}

// #########################################################################
//  Draw a linear meter on the screen
// #########################################################################
void plotLinear(char *label, int x, int y)
{
  int w = 36;
  M5.Lcd.drawRect(x, y, w, 155, TFT_GREY);
  M5.Lcd.fillRect(x + 2, y + 19, w - 3, 155 - 38, TFT_WHITE);
  M5.Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
  M5.Lcd.drawCentreString(label, x + w / 2, y + 2, 2);

  for (int i = 0; i < 110; i += 10)
  {
    M5.Lcd.drawFastHLine(x + 20, y + 27 + i, 6, TFT_BLACK);
  }

  for (int i = 0; i < 110; i += 50)
  {
    M5.Lcd.drawFastHLine(x + 20, y + 27 + i, 9, TFT_BLACK);
  }

  M5.Lcd.fillTriangle(x + 3, y + 127, x + 3 + 16, y + 127, x + 3, y + 127 - 5, TFT_RED);
  M5.Lcd.fillTriangle(x + 3, y + 127, x + 3 + 16, y + 127, x + 3, y + 127 + 5, TFT_RED);

  M5.Lcd.drawCentreString("---", x + w / 2, y + 155 - 18, 2);
}

// #########################################################################
//  Adjust  linear meter pointer position
// #########################################################################
void plotPointer(void)
{
  int dy = 187;
  byte pw = 16;
  int i = 0;

  M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);


    char buf[8]; dtostrf(value[i], 4, 0, buf);
    M5.Lcd.drawRightString(buf, i * 40 + 36 - 5, 187 - 27 + 155 - 18, 2);

    int dx = 3 + 40 * i;
    if (value[i] < 0) value[i] = 0; // Limit value to emulate needle end stops
    if (value[i] > 100) value[i] = 100;

    while (!(value[i] == old_value[i])) {
      dy = 187 + 100 - old_value[i];
      if (old_value[i] > value[i])
      {
        M5.Lcd.drawLine(dx, dy - 5, dx + pw, dy, TFT_WHITE);
        old_value[i]--;
        M5.Lcd.drawLine(dx, dy + 6, dx + pw, dy + 1, TFT_RED);
      }
      else
      {
        M5.Lcd.drawLine(dx, dy + 5, dx + pw, dy, TFT_WHITE);
        old_value[i]++;
        M5.Lcd.drawLine(dx, dy - 6, dx + pw, dy - 1, TFT_RED);
      }
    }
  
}

