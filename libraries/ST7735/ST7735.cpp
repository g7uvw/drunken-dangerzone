// Graphics library by ladyada/adafruit with init code from Rossum 
// MIT license

#include "ST7735.h"
#include "glcdfont.c"
#include <avr/pgmspace.h>
#include "pins_arduino.h"
#include "wiring_private.h"
#include <SPI.h>


ST7735::ST7735(uint8_t cs, uint8_t rs, uint8_t sid, 
               uint8_t sclk, uint8_t rst) {
    _cs = cs;
    _rs = rs;
    _sid = sid;
    _sclk = sclk;
    _rst = rst;
    
    setRotation(0);
}

ST7735::ST7735(uint8_t cs, uint8_t rs,  uint8_t rst) {
    _cs = cs;
    _rs = rs;
    _sid = 0;
    _sclk = 0;
    _rst = rst;

    setRotation(0);
}


inline void ST7735::spiwrite(uint8_t c) {
    
    //Serial.println(c, HEX);
    
    if (!_sid) {
        SPI.transfer(c);
        return;
    }
    
    volatile uint8_t *sclkportreg = portOutputRegister(sclkport);
    volatile uint8_t *sidportreg = portOutputRegister(sidport);
    
    int8_t i;
    
    *sclkportreg |= sclkpin;
    
    for (i=7; i>=0; i--) {
        *sclkportreg &= ~sclkpin;
        //SCLK_PORT &= ~_BV(SCLK);
        
        if (c & _BV(i)) {
            *sidportreg |= sidpin;
            //digitalWrite(_sid, HIGH);
            //SID_PORT |= _BV(SID);
        } else {
            *sidportreg &= ~sidpin;
            //digitalWrite(_sid, LOW);
            //SID_PORT &= ~_BV(SID);
        }
        
        *sclkportreg |= sclkpin;
        //SCLK_PORT |= _BV(SCLK);
    }
}


void ST7735::writecommand(uint8_t c) {
    *portOutputRegister(rsport) &= ~ rspin;
    //digitalWrite(_rs, LOW);
    
    *portOutputRegister(csport) &= ~ cspin;
    //digitalWrite(_cs, LOW);
    
    //Serial.print("C ");
    spiwrite(c);
    
    *portOutputRegister(csport) |= cspin;
    //digitalWrite(_cs, HIGH);
}


void ST7735::writedata(uint8_t c) {
    *portOutputRegister(rsport) |= rspin;
    //digitalWrite(_rs, HIGH);
    
    *portOutputRegister(csport) &= ~ cspin;
    //digitalWrite(_cs, LOW);
    
    //Serial.print("D ");
    spiwrite(c);
    
    *portOutputRegister(csport) |= cspin;
    //digitalWrite(_cs, HIGH);
} 


void ST7735::setAddrWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    writecommand(ST7735_CASET);  // column addr set
    writedata(0x00);
    writedata(x0+2);   // XSTART 
    writedata(0x00);
    writedata(x1+2);   // XEND
    
    writecommand(ST7735_RASET);  // row addr set
    writedata(0x00);
    writedata(y0+1);    // YSTART
    writedata(0x00);
    writedata(y1+1);    // YEND
    
    writecommand(ST7735_RAMWR);  // write to RAM
}

void ST7735::pushColor(uint16_t color) {
    *portOutputRegister(rsport) |= rspin;
    *portOutputRegister(csport) &= ~ cspin;
    
    spiwrite(color >> 8);    
    spiwrite(color);   
    
    *portOutputRegister(csport) |= cspin;
}

void ST7735::drawPixel(uint8_t x, uint8_t y,uint16_t color) {
  if ((x >= TFTWIDTH) || (y >= TFTHEIGHT)) return;
    
    // check rotation, move pixel around if necessary
  switch (rotation) {
  case 1:
    swap(x, y);
    x = TFTWIDTH - x - 1;
    break;
  case 2:
    x = TFTWIDTH - x - 1;
    y = TFTHEIGHT - y - 1;
    break;
  case 3:
    swap(x, y);
    y = TFTHEIGHT - y - 1;
    break;
  }
  
  setAddrWindow(x,y,x+1,y+1);
  
  // setup for data
  *portOutputRegister(rsport) |= rspin;
  *portOutputRegister(csport) &= ~ cspin;
  
  spiwrite(color >> 8);    
  spiwrite(color);   
  
  *portOutputRegister(csport) |= cspin;
  
}


void ST7735::fillScreen(uint16_t color) {
  setAddrWindow(0, 0, TFTWIDTH-1, TFTHEIGHT-1);
  
  // setup for data
  *portOutputRegister(rsport) |= rspin;
  *portOutputRegister(csport) &= ~ cspin;
  
  for (uint8_t x=0; x < TFTWIDTH; x++) {
    for (uint8_t y=0; y < TFTHEIGHT; y++) {
      spiwrite(color >> 8);    
      spiwrite(color);    
    }
  }
  
  *portOutputRegister(csport) |= cspin;
}

void ST7735::initB(void) {
    // set pin directions
    pinMode(_rs, OUTPUT);
    
    if (_sclk) {
        pinMode(_sclk, OUTPUT);
        sclkport = digitalPinToPort(_sclk);
        sclkpin = digitalPinToBitMask(_sclk);
        
        pinMode(_sid, OUTPUT);
        sidport = digitalPinToPort(_sid);
        sidpin = digitalPinToBitMask(_sid);
    } else {
        // using the hardware SPI
        SPI.begin();
        SPI.setDataMode(SPI_MODE3);
    }
    // toggle RST low to reset; CS low so it'll listen to us
    pinMode(_cs, OUTPUT);
    digitalWrite(_cs, LOW);
    cspin = digitalPinToBitMask(_cs);
    csport = digitalPinToPort(_cs);
    
    rspin = digitalPinToBitMask(_rs);
    rsport = digitalPinToPort(_rs);
    
    if (_rst) {
        pinMode(_rst, OUTPUT);
        digitalWrite(_rst, HIGH);
        delay(500);
        digitalWrite(_rst, LOW);
        delay(500);
        digitalWrite(_rst, HIGH);
        delay(500);
    }
    
    writecommand(ST7735_SWRESET); // software reset
    delay(50);
    writecommand(ST7735_SLPOUT);  // out of sleep mode
    delay(500);
    
    writecommand(ST7735_COLMOD);  // set color mode
    writedata(0x05);        // 16-bit color
    delay(10);
    
    writecommand(ST7735_FRMCTR1);  // frame rate control
    writedata(0x00);  // fastest refresh
    writedata(0x06);  // 6 lines front porch
    writedata(0x03);  // 3 lines backporch
    delay(10);
    
    writecommand(ST7735_MADCTL);  // memory access control (directions)
    writedata(0x08);  // row address/col address, bottom to top refresh
    madctl = 0x08;
    
    writecommand(ST7735_DISSET5);  // display settings #5
    writedata(0x15);  // 1 clock cycle nonoverlap, 2 cycle gate rise, 3 cycle oscil. equalize
    writedata(0x02);  // fix on VTL
    
    writecommand(ST7735_INVCTR);  // display inversion control
    writedata(0x0);  // line inversion
    
    writecommand(ST7735_PWCTR1);  // power control
    writedata(0x02);      // GVDD = 4.7V 
    writedata(0x70);      // 1.0uA
    delay(10);
    writecommand(ST7735_PWCTR2);  // power control
    writedata(0x05);      // VGH = 14.7V, VGL = -7.35V 
    writecommand(ST7735_PWCTR3);  // power control
    writedata(0x01);      // Opamp current small 
    writedata(0x02);      // Boost frequency
    
    
    writecommand(ST7735_VMCTR1);  // power control
    writedata(0x3C);      // VCOMH = 4V
    writedata(0x38);      // VCOML = -1.1V
    delay(10);
    
    writecommand(ST7735_PWCTR6);  // power control
    writedata(0x11); 
    writedata(0x15);
    
    writecommand(ST7735_GMCTRP1);
    writedata(0x09);
    writedata(0x16);
    writedata(0x09);
    writedata(0x20);
    writedata(0x21);
    writedata(0x1B);
    writedata(0x13);
    writedata(0x19);
    writedata(0x17);
    writedata(0x15);
    writedata(0x1E);
    writedata(0x2B);
    writedata(0x04);
    writedata(0x05);
    writedata(0x02);
    writedata(0x0E);
    writecommand(ST7735_GMCTRN1);
    writedata(0x0B); 
    writedata(0x14); 
    writedata(0x08); 
    writedata(0x1E); 
    writedata(0x22); 
    writedata(0x1D); 
    writedata(0x18); 
    writedata(0x1E); 
    writedata(0x1B); 
    writedata(0x1A); 
    writedata(0x24); 
    writedata(0x2B); 
    writedata(0x06); 
    writedata(0x06); 
    writedata(0x02); 
    writedata(0x0F); 
    delay(10);
    
    writecommand(ST7735_CASET);  // column addr set
    writedata(0x00);
    writedata(0x02);   // XSTART = 2
    writedata(0x00);
    writedata(0x81);   // XEND = 129
    
    writecommand(ST7735_RASET);  // row addr set
    writedata(0x00);
    writedata(0x02);    // XSTART = 1
    writedata(0x00);
    writedata(0x81);    // XEND = 160
    
    writecommand(ST7735_NORON);  // normal display on
    delay(10);
    
    writecommand(ST7735_DISPON);
    delay(500);
    cursor_y = cursor_x = 0;
    textsize = 1;
    textcolor = 0xFFFF;
}



void ST7735::initR(void) {
    // set pin directions
    pinMode(_rs, OUTPUT);
    
    if (_sclk) {
        pinMode(_sclk, OUTPUT);
        sclkport = digitalPinToPort(_sclk);
        sclkpin = digitalPinToBitMask(_sclk);
        
        pinMode(_sid, OUTPUT);
        sidport = digitalPinToPort(_sid);
        sidpin = digitalPinToBitMask(_sid);
    } else {
        // using the hardware SPI
        SPI.begin();
        SPI.setDataMode(SPI_MODE3);
    }
    // toggle RST low to reset; CS low so it'll listen to us
    pinMode(_cs, OUTPUT);
    digitalWrite(_cs, LOW);
    cspin = digitalPinToBitMask(_cs);
    csport = digitalPinToPort(_cs);
    
    rspin = digitalPinToBitMask(_rs);
    rsport = digitalPinToPort(_rs);
    
    if (_rst) {
        pinMode(_rst, OUTPUT);
        digitalWrite(_rst, HIGH);
        delay(500);
        digitalWrite(_rst, LOW);
        delay(500);
        digitalWrite(_rst, HIGH);
        delay(500);
    }
    
    writecommand(ST7735_SWRESET); // software reset
    delay(150);
    
    writecommand(ST7735_SLPOUT);  // out of sleep mode
    delay(500);
    
    writecommand(ST7735_FRMCTR1);  // frame rate control - normal mode
    writedata(0x01);  // frame rate = fosc / (1 x 2 + 40) * (LINE + 2C + 2D)
    writedata(0x2C); 
    writedata(0x2D); 
    
    writecommand(ST7735_FRMCTR2);  // frame rate control - idle mode
    writedata(0x01);  // frame rate = fosc / (1 x 2 + 40) * (LINE + 2C + 2D)
    writedata(0x2C); 
    writedata(0x2D); 
    
    writecommand(ST7735_FRMCTR3);  // frame rate control - partial mode
    writedata(0x01); // dot inversion mode
    writedata(0x2C); 
    writedata(0x2D); 
    writedata(0x01); // line inversion mode
    writedata(0x2C); 
    writedata(0x2D); 
    
    writecommand(ST7735_INVCTR);  // display inversion control
    writedata(0x07);  // no inversion
    
    writecommand(ST7735_PWCTR1);  // power control
    writedata(0xA2);      
    writedata(0x02);      // -4.6V
    writedata(0x84);      // AUTO mode
    
    writecommand(ST7735_PWCTR2);  // power control
    writedata(0xC5);      // VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
    
    writecommand(ST7735_PWCTR3);  // power control
    writedata(0x0A);      // Opamp current small 
    writedata(0x00);      // Boost frequency
    
    writecommand(ST7735_PWCTR4);  // power control
    writedata(0x8A);      // BCLK/2, Opamp current small & Medium low
    writedata(0x2A);     
    
    writecommand(ST7735_PWCTR5);  // power control
    writedata(0x8A);    
    writedata(0xEE);     
    
    writecommand(ST7735_VMCTR1);  // power control
    writedata(0x0E);  
    
    writecommand(ST7735_INVOFF);    // don't invert display
    
    writecommand(ST7735_MADCTL);  // memory access control (directions)
    writedata(0xC8);  // row address/col address, bottom to top refresh
    madctl = 0xC8;
    
    writecommand(ST7735_COLMOD);  // set color mode
    writedata(0x05);        // 16-bit color
    
    writecommand(ST7735_CASET);  // column addr set
    writedata(0x00);
    writedata(0x00);   // XSTART = 0
    writedata(0x00);
    writedata(0x7F);   // XEND = 127
    
    writecommand(ST7735_RASET);  // row addr set
    writedata(0x00);
    writedata(0x00);    // XSTART = 0
    writedata(0x00);
    writedata(0x9F);    // XEND = 159
    
    writecommand(ST7735_GMCTRP1);
    writedata(0x02);
    writedata(0x1c);
    writedata(0x07);
    writedata(0x12);
    writedata(0x37);
    writedata(0x32);
    writedata(0x29);
    writedata(0x2d);
    writedata(0x29);
    writedata(0x25);
    writedata(0x2B);
    writedata(0x39);
    writedata(0x00);
    writedata(0x01);
    writedata(0x03);
    writedata(0x10);
    writecommand(ST7735_GMCTRN1);
    writedata(0x03); 
    writedata(0x1d); 
    writedata(0x07); 
    writedata(0x06); 
    writedata(0x2E); 
    writedata(0x2C); 
    writedata(0x29); 
    writedata(0x2D); 
    writedata(0x2E); 
    writedata(0x2E); 
    writedata(0x37); 
    writedata(0x3F); 
    writedata(0x00); 
    writedata(0x00); 
    writedata(0x02); 
    writedata(0x10); 
    
    writecommand(ST7735_DISPON);
    delay(100);
    
    writecommand(ST7735_NORON);  // normal display on
    delay(10);
    cursor_y = cursor_x = 0;
    textsize = 1;
    textcolor = 0xFFFF;
}

uint8_t ST7735::width() {
  return _width;
}

uint8_t ST7735::height() {
  return _height;
}

// draw a string from memory

void ST7735::drawString(uint8_t x, uint8_t y, char *c, 
                        uint16_t color, uint8_t size) {
    while (c[0] != 0) {
        drawChar(x, y, c[0], color, size);
        x += size*6;
        c++;
        if (x + 5 >= _width) {
            y += 10;
            x = 0;
        }
    }
}
// draw a character
void ST7735::drawChar(uint8_t x, uint8_t y, char c, 
                      uint16_t color, uint8_t size) {
    for (uint8_t i =0; i<5; i++ ) {
        uint8_t line = pgm_read_byte(font+(c*5)+i);
        for (uint8_t j = 0; j<8; j++) {
            if (line & 0x1) {
                if (size == 1) // default size
                    drawPixel(x+i, y+j, color);
                else {  // big size
                    fillRect(x+i*size, y+j*size, size, size, color);
                } 
            }
            line >>= 1;
        }
    }
}


// fill a circle
void ST7735::fillCircle(uint8_t x0, uint8_t y0, uint8_t r, uint16_t color) {
    drawVerticalLine(x0, y0-r, 2*r+1, color);
    fillCircleHelper(x0, y0, r, 3, 0, color);
}

// draw a circle outline
void ST7735::drawCircle(uint8_t x0, uint8_t y0, uint8_t r, 
                        uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    
    drawPixel(x0, y0+r, color);
    drawPixel(x0, y0-r, color);
    drawPixel(x0+r, y0, color);
    drawPixel(x0-r, y0, color);
    
    while (x<y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        drawPixel(x0 + x, y0 + y, color);
        drawPixel(x0 - x, y0 + y, color);
        drawPixel(x0 + x, y0 - y, color);
        drawPixel(x0 - x, y0 - y, color);
        
        drawPixel(x0 + y, y0 + x, color);
        drawPixel(x0 - y, y0 + x, color);
        drawPixel(x0 + y, y0 - x, color);
        drawPixel(x0 - y, y0 - x, color);
        
    }
}

// draw a triangle!
void ST7735::drawTriangle(uint8_t x0, uint8_t y0,
                          uint8_t x1, uint8_t y1,
                          uint8_t x2, uint8_t y2, uint16_t color)
{
    drawLine(x0, y0, x1, y1, color);
    drawLine(x1, y1, x2, y2, color);
    drawLine(x2, y2, x0, y0, color); 
}

// fill a triangle!
void ST7735::fillTriangle ( int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t color)
{
    if (y0 > y1) {
        swap(y0, y1); swap(x0, x1);
    }
    if (y1 > y2) {
        swap(y2, y1); swap(x2, x1);
    }
    if (y0 > y1) {
        swap(y0, y1); swap(x0, x1);
    }
    
    int32_t dx1, dx2, dx3; // Interpolation deltas
    int32_t sx1, sx2, sy; // Scanline co-ordinates
    
    sx2=(int32_t)x0 * (int32_t)1000; // Use fixed point math for x axis values
    sx1 = sx2;
    sy=y0;
    
    // Calculate interpolation deltas
    if (y1-y0 > 0) dx1=((x1-x0)*1000)/(y1-y0);
    else dx1=0;
    if (y2-y0 > 0) dx2=((x2-x0)*1000)/(y2-y0);
    else dx2=0;
    if (y2-y1 > 0) dx3=((x2-x1)*1000)/(y2-y1);
    else dx3=0;
    
    // Render scanlines (horizontal lines are the fastest rendering method)
    if (dx1 > dx2)
    {
        for(; sy<=y1; sy++, sx1+=dx2, sx2+=dx1)
        {
            drawHorizontalLine(sx1/1000, sy, (sx2-sx1)/1000, color);
        }
        sx2 = x1*1000;
        sy = y1;
        for(; sy<=y2; sy++, sx1+=dx2, sx2+=dx3)
        {
            drawHorizontalLine(sx1/1000, sy, (sx2-sx1)/1000, color);
        }
    }
    else
    {
        for(; sy<=y1; sy++, sx1+=dx1, sx2+=dx2)
        {
            drawHorizontalLine(sx1/1000, sy, (sx2-sx1)/1000, color);
        }
        sx1 = x1*1000;
        sy = y1;
        for(; sy<=y2; sy++, sx1+=dx3, sx2+=dx2)
        {
            drawHorizontalLine(sx1/1000, sy, (sx2-sx1)/1000, color);
        }
    }
}

// draw a rounded rectangle!
void ST7735::drawRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r,
                           uint16_t color) {
    // smarter version
    drawHorizontalLine(x+r, y, w-2*r, color);
    drawHorizontalLine(x+r, y+h-1, w-2*r, color);
    drawVerticalLine(x, y+r, h-2*r, color);
    drawVerticalLine(x+w-1, y+r, h-2*r, color);
    // draw four corners
    drawCircleHelper(x+r, y+r, r, 1, color);
    drawCircleHelper(x+w-r-1, y+r, r, 2, color);
    drawCircleHelper(x+w-r-1, y+h-r-1, r, 4, color);
    drawCircleHelper(x+r, y+h-r-1, r, 8, color);
}

// fill a rounded rectangle!
void ST7735::fillRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r,
                           uint16_t color) {
    // smarter version
    fillRect(x+r, y, w-2*r, h, color);
    
    // draw four corners
    fillCircleHelper(x+w-r-1, y+r, r, 1, h-2*r-1, color);
    fillCircleHelper(x+r, y+r, r, 2, h-2*r-1, color);
}

// used to do circles and roundrects!
void ST7735::fillCircleHelper(uint16_t x0, uint16_t y0, uint16_t r, uint8_t cornername, uint16_t delta,
                              uint16_t color) {
    
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    
    while (x<y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        if (cornername & 0x1) {
            drawVerticalLine(x0+x, y0-y, 2*y+1+delta, color);
            drawVerticalLine(x0+y, y0-x, 2*x+1+delta, color);
        }
        if (cornername & 0x2) {
            drawVerticalLine(x0-x, y0-y, 2*y+1+delta, color);
            drawVerticalLine(x0-y, y0-x, 2*x+1+delta, color);
        }
    }
}

uint16_t ST7735::Color565(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t c;
    c = r >> 3;
    c <<= 6;
    c |= g >> 2;
    c <<= 5;
    c |= b >> 3;
    
    return c;
}

void ST7735::drawCircleHelper(uint16_t x0, uint16_t y0, uint16_t r, uint8_t cornername,
                              uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    
    
    while (x<y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        if (cornername & 0x4) {
            drawPixel(x0 + x, y0 + y, color);
            drawPixel(x0 + y, y0 + x, color);
        } 
        if (cornername & 0x2) {
            drawPixel(x0 + x, y0 - y, color);
            drawPixel(x0 + y, y0 - x, color);
        }
        if (cornername & 0x8) {
            drawPixel(x0 - y, y0 + x, color);
            drawPixel(x0 - x, y0 + y, color);
        }
        if (cornername & 0x1) {
            drawPixel(x0 - y, y0 - x, color);
            drawPixel(x0 - x, y0 - y, color);
        }
    }
}

uint8_t ST7735::getRotation() {
    return rotation;
}

void ST7735::setRotation(uint8_t m) {
    
    m%= 4;  // cant be higher than 3
    rotation = m;
    switch (m) {
        case 0:
            _width = TFTWIDTH; 
            _height = TFTHEIGHT;
            break;
        case 1:
            _width = TFTHEIGHT; 
            _height = TFTWIDTH;
            break;
        case 2:
            _width = TFTWIDTH; 
            _height = TFTHEIGHT;
            break;
        case 3:
            _width = TFTHEIGHT; 
            _height = TFTWIDTH;
            break;
    }
}

// draw a rectangle
void ST7735::drawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, 
                      uint16_t color) {
    // smarter version
    drawHorizontalLine(x, y, w, color);
    drawHorizontalLine(x, y+h-1, w, color);
    drawVerticalLine(x, y, h, color);
    drawVerticalLine(x+w-1, y, h, color);
}

// fill a rectangle
void ST7735::fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, 
                      uint16_t color) {
  
    
   // set the Address Window!
    
    if (rotation == 0) {
            setAddrWindow(x, y, x+w-1, y+h-1);
        }
        
    if (rotation == 1) {
        swap(x, y);
        x = TFTWIDTH - x - 1;
            setAddrWindow(x - h + 1, y, x, y + w + 1);
        }
        
    if (rotation == 2) {
        x = TFTWIDTH - x - w;
        y = TFTHEIGHT - y - h;
            setAddrWindow(x, y, x+w-1, y+h-1);
        }
        
    if (rotation == 3) {
        swap(x, y);
        y = TFTHEIGHT - y - w;
            setAddrWindow(x, y, x+h-1, y+w-1);
        }

    
    // setup for data
    digitalWrite(_rs, HIGH);
    digitalWrite(_cs, LOW);
    
    
    if (rotation == 0) {
    for (x=0; x < w; x++) {
        for (y=0; y < h; y++) {
            spiwrite(color >> 8);    
            spiwrite(color);    
        }
    }
    }
    if (rotation == 1) {
        for (x=0; x < h; x++) {
            for (y=0; y < w; y++) {
                spiwrite(color >> 8);    
                spiwrite(color);    
            }
        }
    }
    if (rotation == 2) {
        for (x=0; x < w; x++) {
            for (y=0; y < h; y++) {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             
                spiwrite(color >> 8);    
                spiwrite(color);    
            }
        }
    }
    if (rotation == 3) {
        for (x=0; x < h; x++) {
            for (y=0; y < w; y++) {
                spiwrite(color >> 8);    
                spiwrite(color);    
            }
        }
    }
    
    digitalWrite(_cs, HIGH);
}

void ST7735::drawVerticalLine(uint8_t x, uint8_t y, uint8_t length, uint16_t color)
{
  if (x >= _width) return;
  if (y+length >= _height) length = _height-y-1;
    
  drawFastLine(x,y,length,color,1);
}

void ST7735::drawHorizontalLine(uint8_t x, uint8_t y, uint8_t length, uint16_t color)
{
    if (y >= _height) return;
    if (x+length >= _width) length = _width-x-1;
    
    drawFastLine(x,y,length,color,0);
}

void ST7735::drawFastLine(uint8_t x, uint8_t y, uint8_t length, 
                          uint16_t color, uint8_t rotflag)
{
  if (rotation == 0) {
    if (rotflag) {  
      setAddrWindow(x, y, x, y+length);   // vertical
    } else {
      setAddrWindow(x, y, x+length, y+1); // horizontal
    }
      
  } else if (rotation == 1) {
    swap(x, y);
    x = TFTWIDTH - x - 1;
    if (rotflag) {
      setAddrWindow(x-length + 1, y, x, y); // vertical
    } else {
      setAddrWindow(x, y, x, y+length); //horizontal
    }
      
  } else if (rotation == 2) {
    x = TFTWIDTH - x - 1;
    y = TFTHEIGHT - y - 1;
    if (rotflag) {
      setAddrWindow(x, y-length + 1, x, y); // vertical
    } else {
      setAddrWindow(x-length + 1, y, x, y+1); //horizontal
    }
      
  } else if (rotation == 3) {
    swap(x, y);
    y = TFTHEIGHT - y - 1;
    if (rotflag) {
      setAddrWindow(x, y, x+length, y); // vertical
    } else {
       setAddrWindow(x, y-length + 1, x, y); // horizontal
    }
  }

  // setup for data
  digitalWrite(_rs, HIGH);
  digitalWrite(_cs, LOW);
  
  while (length--) {
    spiwrite(color >> 8);    
    spiwrite(color);    
  }
  digitalWrite(_cs, HIGH);
}


// bresenham's algorithm - thx wikpedia
void ST7735::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, 
                      uint16_t color) {
    uint16_t steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) {
        swap(x0, y0);
        swap(x1, y1);
    }
    
    if (x0 > x1) {
        swap(x0, x1);
        swap(y0, y1);
    }
    
    uint16_t dx, dy;
    dx = x1 - x0;
    dy = abs(y1 - y0);
    
    int16_t err = dx / 2;
    int16_t ystep;
    
    if (y0 < y1) {
        ystep = 1;
    } else {
        ystep = -1;}
    
    for (; x0<=x1; x0++) {
        if (steep) {
            drawPixel(y0, x0, color);
        } else {
            drawPixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

void ST7735::setCursor(uint16_t x, uint16_t y) {
    cursor_x = x;
    cursor_y = y;
}

void ST7735::setTextSize(uint8_t s) {
    textsize = s;
}

void ST7735::setTextColor(uint16_t c) {
    textcolor = c;
}

#if ARDUINO >= 100
size_t ST7735::write(uint8_t c) {
#else
    void ST7735::write(uint8_t c) {
#endif
        if (c == '\n') {
            cursor_y += textsize*8;
            cursor_x = 0;
        } else if (c == '\r') {
            // skip em
        } else {
            drawChar(cursor_x, cursor_y, c, textcolor, textsize);
            cursor_x += textsize*6;
        }
#if ARDUINO >= 100
        return 1;
#endif
    }
    
    void ST7735::goHome(void) {
        setCursor (0,0);
    }


//////////
/*
 uint8_t ST7735::spiread(void) {
 uint8_t r = 0;
 if (_sid > 0) {
 r = shiftIn(_sid, _sclk, MSBFIRST);
 } else {
 //SID_DDR &= ~_BV(SID);
 //int8_t i;
 //for (i=7; i>=0; i--) {
 //  SCLK_PORT &= ~_BV(SCLK);
 //  r <<= 1;
 //  r |= (SID_PIN >> SID) & 0x1;
 //  SCLK_PORT |= _BV(SCLK);
 //}
 //SID_DDR |= _BV(SID);
 
 }
 return r;
 }
 
 
 
 void ST7735::dummyclock(void) {
 
 if (_sid > 0) {
 digitalWrite(_sclk, LOW);
 digitalWrite(_sclk, HIGH);
 } else {
 // SCLK_PORT &= ~_BV(SCLK);
 //SCLK_PORT |= _BV(SCLK);
 }
 }
 uint8_t ST7735::readdata(void) {
 *portOutputRegister(rsport) |= rspin;
 
 *portOutputRegister(csport) &= ~ cspin;
 
 uint8_t r = spiread();
 
 *portOutputRegister(csport) |= cspin;
 
 return r;
 
 } 
 
 uint8_t ST7735::readcommand8(uint8_t c) {
 digitalWrite(_rs, LOW);
 
 *portOutputRegister(csport) &= ~ cspin;
 
 spiwrite(c);
 
 digitalWrite(_rs, HIGH);
 pinMode(_sid, INPUT); // input!
 digitalWrite(_sid, LOW); // low
 spiread();
 uint8_t r = spiread();
 
 
 *portOutputRegister(csport) |= cspin;
 
 
 pinMode(_sid, OUTPUT); // back to output
 return r;
 }
 
 
 uint16_t ST7735::readcommand16(uint8_t c) {
 digitalWrite(_rs, LOW);
 if (_cs)
 digitalWrite(_cs, LOW);
 
 spiwrite(c);
 pinMode(_sid, INPUT); // input!
 uint16_t r = spiread();
 r <<= 8;
 r |= spiread();
 if (_cs)
 digitalWrite(_cs, HIGH);
 
 pinMode(_sid, OUTPUT); // back to output
 return r;
 }
 
 uint32_t ST7735::readcommand32(uint8_t c) {
 digitalWrite(_rs, LOW);
 if (_cs)
 digitalWrite(_cs, LOW);
 spiwrite(c);
 pinMode(_sid, INPUT); // input!
 
 dummyclock();
 dummyclock();
 
 uint32_t r = spiread();
 r <<= 8;
 r |= spiread();
 r <<= 8;
 r |= spiread();
 r <<= 8;
 r |= spiread();
 if (_cs)
 digitalWrite(_cs, HIGH);
 
 pinMode(_sid, OUTPUT); // back to output
 return r;
 }
 
 */
