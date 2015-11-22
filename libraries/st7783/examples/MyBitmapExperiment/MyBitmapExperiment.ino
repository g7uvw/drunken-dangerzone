#include <SD.h>
#include "TFTLCD.h"

#define LCD_CS A3    
#define LCD_CD A2    
#define LCD_WR A1    
#define LCD_RD A0   
#define LCD_RESET A4
//Duemilanove/Diecimila/UNO/etc ('168 and '328 chips) microcontoller:

#define	BLACK           0x0000
#define	BLUE            0x001F
#define	RED             0xF800
#define	GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0 
#define WHITE           0xFFFF

#define SD_CS 10     // Set the chip select line to whatever you use (10 doesnt conflict with the library)

// our TFT wiring
TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

// the file itself
File bmpFile;

// information we extract about the bitmap file
int bmpWidth, bmpHeight;
uint8_t bmpDepth, bmpImageoffset;

//int loads = 0;

#define ROTATION 3
#define BUFFPIXEL 60
#define BYTES_PER_PIXEL 3

uint8_t picBuffer[BYTES_PER_PIXEL * BUFFPIXEL];  // 3 * pixels to buffer
int bufferIndex = BYTES_PER_PIXEL * BUFFPIXEL;

void setup()
{
  Serial.begin(9600);
 
  tft.reset();
  tft.initDisplay(); 

  tft.setRotation(ROTATION);
  

  Serial.print("Initializing SD card...");
  pinMode(10, OUTPUT);

  tft.setRotation(ROTATION);
  
  if (!SD.begin(10)) {
    Serial.println("failed!");
    while(1);
  }
  Serial.println("SD OK!");
 
}


/*
  Callback for displayBitmap function to load data into buffer
*/
unsigned int loadBitmapData(void *data)
{
  File *sdFile = (File *) data;
  int bytesRead = sdFile->read(picBuffer, 2 * BUFFPIXEL);
  return bytesRead / 2;
}


/*
  Loads a raw 16-bit bitmap with the format
  
  4-byte width
  4-byte height
  raw 16-bit pixel data in R5G6B5 format
  
*/

void displayBitmap(const char *name, int x, int y)
{
  File sdFile = SD.open(name);
  
  if( ! sdFile )
  {
    Serial.println("Error opening file.");
    return; 
  }
  
  uint32_t width = read32( sdFile );
  uint32_t height = read32( sdFile );
  
  Serial.print("Width: ");
  Serial.println(width, DEC);
  
  uint32_t time = millis();
  uint16_t bx, ex, by, ey;
  
  tft.setViewport(x, y, x + width - 1, y + height - 1);
  tft.goTo(x, y);
 
  int bytesRead = ( loadBitmapData(&sdFile) );
  tft.bulkWrite( (uint16_t *) picBuffer, bytesRead, loadBitmapData, &sdFile);
  
 // tft.setViewport(bx, ex, by, ey);
  tft.setDefaultViewport();
  
  Serial.print(millis() - time, DEC);
  Serial.println(" ms");
  
  sdFile.close();
}


/*
 Slightly modified routine for drawing bitmaps (From original samples).
 
 Watch out, as this one uses some global variables
*/
void bmpdraw(const char *name, int x, int y) {

  File sdFile = SD.open(name);
  if( ! sdFile )
  {
    Serial.println("Error opening file.");
    return; 
  }  
  
  if( ! bmpReadHeader(sdFile) )
  {
    Serial.println("Error reading header.");
    return; 
  }
  
  sdFile.seek(bmpImageoffset);
  
  uint32_t diskTime = 0;
  uint32_t tempTime;
  
  uint32_t time = millis();
  uint16_t p;
  uint8_t g, b;
  int i, j;
 
  Serial.print("rotation = "); Serial.println(tft.getRotation(), DEC);
  
  tft.goTo(0,0);
  for (i=0; i< bmpHeight; i++) {
    // bitmaps are stored with the BOTTOM line first so we have to move 'up'
    tft.goTo(0, bmpHeight - i - 1);

    for (j=0; j<bmpWidth; j++) {
      // read more pixels

      if (bufferIndex >= BYTES_PER_PIXEL * BUFFPIXEL) {
        tempTime =  millis();
        sdFile.read(picBuffer, BYTES_PER_PIXEL * BUFFPIXEL);
        diskTime += millis() - tempTime;
        bufferIndex= 0;
      }
      
      p = tft.Color565( picBuffer[bufferIndex+2], picBuffer[bufferIndex+1], picBuffer[bufferIndex]);
      bufferIndex += 3;

      tft.writeData(p);
    }
  }
 
 uint32_t totalTime = millis();
 Serial.print("Total time: ");
 Serial.println(totalTime - time, DEC);
 Serial.print("Disk Time: ");
 Serial.println(diskTime, DEC);
 
 sdFile.close();
}


/*
  From example code.  Reads bitmap header.  Uses global variables.

*/
boolean bmpReadHeader(File f) {
   // read header
  uint32_t tmp;
  
  if (read16(f) != 0x4D42) {
    // magic bytes missing
    return false;
  }
 
  // read file size
  tmp = read32(f);  
  Serial.print("size 0x"); Serial.println(tmp, HEX);
  
  // read and ignore creator bytes
  read32(f);
  
  bmpImageoffset = read32(f);  
  Serial.print("offset "); Serial.println(bmpImageoffset, DEC);
  
  // read DIB header
  tmp = read32(f);
  Serial.print("header size "); Serial.println(tmp, DEC);
  bmpWidth = read32(f);
  bmpHeight = read32(f);

  
  if (read16(f) != 1)
    return false;
    
  bmpDepth = read16(f);
  Serial.print("bitdepth "); Serial.println(bmpDepth, DEC);


  Serial.print("compression "); Serial.println(tmp, DEC);

  return true;
}


/*********************************************/

// These read data from the SD card file and convert them to big endian 
// (the data is stored in little endian format!)


// LITTLE ENDIAN!

uint16_t read16(File &f) {
  uint16_t d;
  uint8_t b;
  b = f.read();
  d = f.read();
  d <<= 8;
  d |= b;
  return d;
}


// LITTLE ENDIAN!
uint32_t read32(File &f) {
  uint32_t d;
  uint16_t b;
 
  b = read16(f);
  d = read16(f);
  d <<= 16;
  d |= b;
  return d;
}




void loop()
{
  // 16-bit top-down display
   displayBitmap("out.lcd", 0, 0);
   delay(3000);

   tft.fillScreen(BLACK);
   delay(1000);
   
   // 24-bit bottom-up display
   bmpdraw("sunset.bmp", 0, 0);
   delay(3000);
   
   tft.fillScreen(WHITE);
   delay(1000);
   
   // 16-bit top-down display
   displayBitmap("sunset.lcd", 0, 0);
   delay(3000);
}

