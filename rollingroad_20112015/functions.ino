void Serial_Update()
{
    if (!NO_BINARY_OUTPUT)
    {
        data_packet.calibrating = 0;
        data_packet.highrange = 1;
        data_packet.torque = TORQUEaverage;
        data_packet.speedo = (uint16_t) currentspeed;
        data_packet.revs = (uint16_t) currentfrequency;
        char pBuffer[uBufSize];
        memcpy(pBuffer, &data_packet, uBufSize);
        
        for(unsigned int i = 0; i<uBufSize;i++)
        {
            Serial.print(pBuffer[i]);
        }
        //Serial.println();
        //delay(10);        //removed on 28/10/15 to speed things up.
    }
}

int getTORQUEaverage(unsigned int cta)
{
    float ta = 0.0;
    float fta = cta;
    for (unsigned int i = 0; i < 10; i++)
    {
        ta += analogRead(TORQUEpin);
    }
    ta /= 10.0;
    
    fta -= fta/200.0;
    fta += ta/200;
    return (unsigned int) fta;
}

int getPOTaverage(void)
{
    // zero out the POT array
    for (unsigned int i = 0; i < numReadings; i++)
        POTreadings[i] = 0;
    POTtotal = 0;
    for(unsigned int i = 0; i< numReadings ; i++)
    {
        POTtotal= POTtotal - POTreadings[POTindex];
        // read from the sensor:
        POTreadings[POTindex] = analogRead(POTpin);
        // add the reading to the total:
        POTtotal= POTtotal + POTreadings[POTindex];
        // advance to the next position in the array:
        POTindex++;
        
        // if we're at the end of the array...
        if (POTindex >= numReadings)
            // ...wrap around to the beginning:
            POTindex = 0;
    }
    
    // calculate the average:
    int average = POTtotal / numReadings;
    return average;
}


int getFREQaverage(int samples)
{
    double sum=0;
    int count=0;
    
    for (count = 0; count < samples; count++)
    {
        if (FreqMeasure.available())
        {
            sum += FreqMeasure.read();
        }
        else
            count--;  // no reading to be had, roll back in loop and try again
        //delay(5);  // small delay to help things along
    }
    
    int average = FreqMeasure.countToFrequency(sum / samples);
    currentfrequency = average;
    return average;
}



float FreqToMPH(float freq)
{
    // 60Hz on a roller diamter of 50cms is 1.57 m/s
    // 1.57 m/s = 3.512 MPH
    return  (freq/60) * 3.512;
}

void setBrake(int brake)
{
    analogWrite(BRAKEpin,brake);
    if (DEBUGGING)
    {
        Serial.print("setBrake: Setting PWM to ");
        Serial.println(brake, DEC);
    }
}
