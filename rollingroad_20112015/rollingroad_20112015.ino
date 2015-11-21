// Rolling Road v2.0
// v3.0 if I ever need to make it will be PID control.
// (c) David Mills - dave@webshed.org

#include <FreqMeasure.h>
#include "vars.h"
#include "struct.h"
#include "rollingroad.h"

// Scale factor
#define SCALEFACTOR 1000

// Debugging defines
#define NO_BINARY_OUTPUT false
#define DEBUGGING false


void setup()
{
    Serial.begin(115200);
    Serial.println("Starting Rolling Road");
    analogWriteResolution(10);          //10 bit PWM resolution (0 - 1023)
    analogWriteFrequency(BRAKEpin,PWM_F);
    pinMode(3,INPUT);
    FreqMeasure.begin();                // start measuring the speed pulses
    
    pinMode(REDLED,OUTPUT);
    pinMode(GREENLED,OUTPUT);
    pinMode(YELLOWLED,OUTPUT);
}

void loop()
{
    // Wait for the car to get up to speed, speed OK is signified when the pot value is >0 and stable
start:
    digitalWrite(YELLOWLED, !digitalRead(YELLOWLED));
    
    while (!roadenable)
    {
        if (getFREQaverage(10) < 100)    //wait for some pulses from the road
            roadenable = false;
        else
            roadenable = true;
        
        if (once_worked)                   // zero eveything.
        {
            POTindex = 0;                  // the index of the current reading
            POTtotal = 0;                  // the running total
            POTaverage = 0;                // the average
            oldPOTaverage = 0;             // previous average
            TORQUEaverage = 0;
            
            basefrequency = 0;
            currentfrequency = 0;
            previousfrequency = 0;
            
            basespeed = 0;
            currentspeed = 0;
            previousspeed = 0;
            
            Brake_extra = 0;
            
            setBrake(0);
        }
        
        //we do the calibration stuff here.
        // read the Torque ADC channel and transmit the data.
        TORQUEaverage = getTORQUEaverage();
        data_packet.calibrating = 1;
        data_packet.recording = 0;
        data_packet.highrange = 1;
        data_packet.padding = 0;
        data_packet.torque = TORQUEaverage;
        data_packet.speedo = 0;
        data_packet.revs = 0;
        
        char pBuffer[uBufSize];
        memcpy(pBuffer, &data_packet, uBufSize);
        
        for(int i = 0; i<uBufSize;i++)
        {
            Serial.print(pBuffer[i]);
        }
        
        if (DEBUGGING)
        {
            Serial.println("calibrating");
            delay(100);
        }
        
        //goto start;
    }

}
