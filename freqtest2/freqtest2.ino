/* FreqMeasure - Example with serial output
 * http://www.pjrc.com/teensy/td_libs_FreqMeasure.html
 *
 * This example code is in the public domain.
 */
#include <FreqMeasure2.h>
#include <FreqMeasure.h>

// PWM Setup - may need tweeking in the field.
const unsigned int PWM_F = 3420;
const unsigned int PWM_F2 = 5000;
const int BRAKEpin = 23;
const int BRAKEpin2 = 24;


void setup() {
  Serial.begin(57600);
  FreqMeasure2.begin();
  FreqMeasure.begin();
   analogWriteResolution(10);          // 16, was 10 bit PWM resolution (0 - 1023)
   analogWriteFrequency(BRAKEpin,PWM_F);  
   analogWriteFrequency(BRAKEpin2,PWM_F2);  
}

double sum=0, sum2=0;
int count=0;
int count2=0;

void loop() {
  //Serial.println("hello");
  if (FreqMeasure.available()) {
    // average several reading together
    sum = sum + FreqMeasure.read();
    count = count + 1;
    if (count > 30) {
      float frequency = FreqMeasure.countToFrequency(sum / count);
      Serial.print("Channel 1: ");
      Serial.println(frequency);
      sum = 0;
      count = 0;
    }
  }
  if (FreqMeasure2.available()) {
    // average several reading together
    sum2 = sum2 + FreqMeasure2.read();
    count2 = count2 + 1;
    if (count > 30) {
      float frequency2 = FreqMeasure2.countToFrequency(sum2 / count);
      Serial.print("Channel 2: ");
      Serial.println(frequency2);
      sum2 = 0;
      count2 = 0;
    }
  }
}


