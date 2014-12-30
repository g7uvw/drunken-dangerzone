//Some I2C Functions

void i2c_send(uint8_t addr,uint8_t reg, uint8_t data)
{
  Wire.beginTransmission(addr);
  Wire.write(reg);
  //delay(100);
  Wire.write(data);
  Wire.endTransmission(); 
}

void i2c_send_word(uint8_t addr,uint8_t reg, uint8_t data0, uint8_t data1)
{

  Wire.beginTransmission(addr);
  Wire.write(reg);
  //delay(100);
  Wire.write(data1);  //data is sent MSB first
  //delay(100);
  Wire.write(data0);
  Wire.endTransmission(); 
}

