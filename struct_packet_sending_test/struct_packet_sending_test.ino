 
 struct data_t{
        uint8_t     calibrating;    // non zero means we're calibrating the strain guage
        uint8_t     recording;      // non zero means record
        uint8_t     highrange;      // non zero means Torque is set to high range
        uint8_t     padding;       // just to pad the stuct
        uint16_t    torque;         // from strain guage
        uint16_t    speedo;          // MPH from controller board
        uint16_t    revs;
        uint16_t    humidity;
        uint16_t    pressure;
    } __attribute__((__packed__)) data_packet;
    
     unsigned long uBufSize = sizeof(data_t);
     
void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(115200); 
  data_packet.calibrating = 0;
  data_packet.recording = 1;
  data_packet.highrange = 1;
  data_packet.padding = 0;
  data_packet.torque = 250;
  data_packet.speedo = 1;
  data_packet.revs = 6;
  data_packet.humidity = 0;
  data_packet.pressure = 0;
}

void loop() {
while (data_packet.speedo < 110)
{
  char pBuffer[uBufSize];
   memcpy(pBuffer, &data_packet, uBufSize);
 
   for(int i = 0; i<uBufSize;i++) 
   {
     Serial.print(pBuffer[i]);
   }
  data_packet.speedo++;
  data_packet.revs = data_packet.speedo / 10;
  
  delay(100);       
}
data_packet.speedo = 0;
delay(100);
}
