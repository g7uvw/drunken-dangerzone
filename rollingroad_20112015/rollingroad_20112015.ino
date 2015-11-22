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
#define NO_BINARY_OUTPUT true
#define DEBUGGING true


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
    digitalWrite(REDLED, HIGH);
    
    while (!roadenable)
    {
        if (getFREQaverage(10) < 100)    //wait for some pulses from the road
            roadenable = false;
        else
            roadenable = true;
        
        // Zero everything
        POTindex = 0;                   // the index of the current reading
        POTtotal = 0;                   // the running total
        POTaverage = 0;                 // the average
        oldPOTaverage = 0;              // previous average
        TORQUEaverage = 0;
        
        basefrequency = 0;
        currentfrequency = 0;
        previousfrequency = 0;
        
        basespeed = 0;
        currentspeed = 0;
        previousspeed = 0;
        
        Brake_extra = 0;                // Set the Brake to zero too
        setBrake(0);
        
        
        //we do the calibration stuff here.
        // read the Torque ADC channel and transmit the data.
        TORQUEaverage = getTORQUEaverage(TORQUEaverage);
        data_packet.calibrating = 1;
        data_packet.recording = 0;
        data_packet.highrange = 1;
        data_packet.padding = 0;
        data_packet.torque = TORQUEaverage;
        data_packet.speedo = 0;
        data_packet.revs = 0;
        
        char pBuffer[uBufSize];
        memcpy(pBuffer, &data_packet, uBufSize);
        
        for(unsigned int i = 0; i<uBufSize;i++)
        {
            Serial.print(pBuffer[i]);
        }
        
        if (DEBUGGING)
        {
            Serial.println("calibrating");
            delay(100);
        }
    }
    
    // if we make it here, we have speed pulses, now wait for the car to come up to speed
    while (!(POTaverage <= POTaverage+3 && !(POTaverage < POTaverage-3)))
        
    {
        digitalWrite(REDLED, HIGH);         // turn the LED on (HIGH is the voltage level)
        digitalWrite(GREENLED, HIGH);       // turn the LED on (HIGH is the voltage level)
        digitalWrite(YELLOWLED, HIGH);      // turn the LED on (HIGH is the voltage level)
        
        POTaverage = getPOTaverage();
        setBrake(POTaverage);               // Brake comes in smoothly with the POT
        Brake_extra = 0;
        
        basefrequency = getFREQaverage(5);  // take 5 samples
        basespeed = FreqToMPH(basefrequency);
        
        TORQUEaverage = getTORQUEaverage(TORQUEaverage);
        data_packet.torque = TORQUEaverage;
        
        data_packet.calibrating = 0;
        data_packet.recording = 0;          // non zero means record
        data_packet.highrange = 1;          // non zero means Torque is set to high range
        data_packet.padding = 0;            // just to pad the stuct
        data_packet.torque = TORQUEaverage; // from strain guage
        data_packet.revs = (uint16_t)basefrequency;
        data_packet.humidity = 63;
        data_packet.pressure = 1000;
        data_packet.speedo = (uint16_t)basespeed;
        
        Serial_Update();
        
        oldPOTaverage = POTaverage;         // Save previous POT value
        
        
        if (DEBUGGING)
        {
            Serial.print("Waiting for car to get up to speed ");
            Serial.println(basespeed,2);
            Serial.print("POT average ");
            Serial.println(POTaverage, 1);
        }
        
    }
    
    //digitalWrite(REDLED, LOW);              // Turn off the LEDS
    //digitalWrite(GREENLED, LOW);
    //digitalWrite(YELLOWLED, LOW);
    
    // If we make it here, we have speed pulses and the POT is set
    
    controllingroad = true;
    
    // We should sit in this loop while the following conditions are true
    //      as long as the speed is > 5mph
    //      as long at the current speed is not 5mph below the base speed
    //      as long as the pot isn't set to < 10
    
    while (controllingroad)
    {
        // set the status LEDS
        if (freewheel)
        {
            digitalWrite(GREENLED, LOW);    // turn the LED off (LOW is the voltage level)
            digitalWrite(YELLOWLED, HIGH);       // turn the LED on (HIGH is the voltage level)
        }
        else
        {
            digitalWrite(GREENLED, HIGH);   // turn the LED on (HIGH is the voltage level)
            digitalWrite(REDLED, LOW);          // turn the LED on (HIGH is the voltage level)
            digitalWrite(YELLOWLED, LOW);       // turn the LED on (HIGH is the voltage level)
        }
        
        // check the recording switch
        if (digitalRead(6))
            data_packet.recording = 0;
        else
            data_packet.recording = 1;
        
        // Do what's needed to the brake
        if (freewheel)
            setBrake(0);
        else
            setBrake(POTaverage + Brake_extra);
        
        // Get the Torque value
        TORQUEaverage = getTORQUEaverage(TORQUEaverage);
        
        // check to see if the pot has changed again
        oldPOTaverage = POTaverage;
        POTaverage = getPOTaverage();
        while (!(POTaverage <= POTaverage+3 && !(POTaverage < POTaverage-3)))
        {
            digitalWrite(GREENLED, HIGH);   // turn the LED on (HIGH is the voltage level)
            digitalWrite(YELLOWLED, HIGH);
            
            POTaverage = getPOTaverage();

            basefrequency = getFREQaverage(5);  // take 5 samples
            basespeed = FreqToMPH(basefrequency);
            
            setBrake(POTaverage);
            Brake_extra = 0;
            
            if (DEBUGGING)
            {
                Serial.println("Pot Value changed");
                Serial.print("OldPOT Average: ");
                Serial.println(oldPOTaverage);
                Serial.print("POT Average: ");
                Serial.println(POTaverage);
            }
        }
        
        // POT value has either not changed, or else has changed and we have a new basespeed
        // Now work out what the road needs to do
        
        previousspeed = currentspeed;                   // save old speed
        currentspeed = FreqToMPH(getFREQaverage(10));   // get new speed
        speederror = currentspeed - basespeed;
        
        if (DEBUGGING)
        {
            Serial.print("Speed error = ");
            Serial.println(speederror,2);
        }
        
        if (currentspeed < 5)                    // speed too low, turn off controls
        {
            setBrake(0);
            Brake_extra = 0;
            controllingroad = false;
            freewheel = true;
            
            if (DEBUGGING)
            {
                Serial.println("Road speed below 5 MPH");
            }
            
            break;                              // exit to main loop
        }
        
        if (currentspeed < (basespeed - 5))      // 5 mph less than base speed, turn off brake
        {
            freewheel = true;
            Brake_extra = 0;
            setBrake(0);
            digitalWrite(GREENLED, LOW);
            digitalWrite(YELLOWLED, HIGH);
            
            if (DEBUGGING)
            {
                Serial.println("currentspeed < basespeed-5");
                Serial.println("Freewheeling");
            }
        }
        
        else
        {
            Brake_extra = speederror * SCALEFACTOR;
            setBrake(POTaverage + Brake_extra);
            freewheel = false;
        }
        
        data_packet.calibrating = 0;
        data_packet.highrange = 1;
        data_packet.torque = TORQUEaverage;
        data_packet.speedo = (uint16_t) currentspeed;
        data_packet.revs = (uint16_t) currentfrequency;
    
        Serial_Update();
    
        if (DEBUGGING)
        {
          Serial.print("Torque = ");
          Serial.println(TORQUEaverage);
        }
    }
}





