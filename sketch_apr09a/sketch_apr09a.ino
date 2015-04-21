
// Pin mappings 
const int LEDpin = 13;
const int BRAKEpin = 23;
const int READYpin = 22;
const int POTpin = 0;  // Analog 0 (PIN 14 on Teensy 3.1)


// Braking POT variables
int potval = 0, oldpotval = 0;
const int numReadings = 10;
int POTreadings[numReadings];      // the readings from the analog input
int POTindex = 0;                  // the index of the current reading
int POTtotal = 0;                  // the running total
int POTaverage = 0;                // the average
int oldPOTaverage = 0;             // previous average

int getPOTaverage(void);

// PWM Setup - may need tweeking in the field.
const unsigned int PWM_F = 50;
//const unsigned int PWM_F = 46875; // too fast for SCR driver brick
int PWM_extra = 10;

void setup()
{
 Serial.begin(9600);
 Serial.println("Starting Rolling Road");
 analogWriteResolution(10);          // 10 bit PWM resolution (0 - 1023)
 analogWriteFrequency(BRAKEpin,PWM_F);   
//pinMode(3,INPUT); 
 //FreqMeasure.begin();                // start measuring the speed pulses
 pinMode(LEDpin, OUTPUT);            // LED setup
 pinMode(READYpin,OUTPUT);
}

void loop()
{
//  while (!(POTaverage <= POTaverage+10 && !(POTaverage < POTaverage-10))) 
//{
 POTaverage = getPOTaverage();
 Serial.print("POT Average: ");
 Serial.println(POTaverage, DEC);
 analogWrite(BRAKEpin,POTaverage);  // Add in the extra so there's no sudden jump if the pot get changed when the road is braking
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

