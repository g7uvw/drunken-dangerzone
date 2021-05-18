#include <complex.h>
#include <limits.h>
#include <math.h>
#include <ADC.h>

const int readPin = A9; // ADC0

//samp_rate: sampling rate of the input signal (in samples per second)
const float samp_rate = 240000; 
//output_rate: sampling rate of the output audio signal (in samples per second)
const int output_rate = 48000; 
//ssb_bw: filter bandwidth for SSB in Hz
const float ssb_bw = 3000;
//amfm_bw: filter bandwidth for AM & FM in Hz
const float amfm_bw = 12000; 
//decimate_transition_bw: transition bandwidth for decimating FIR filter 
//note: a lower value will slow down processing but will "sharpen" the filter charactersitic
const float decimate_transition_bw = 800;

float hamming(float x) { return (0.54-0.46*(2*M_PI*(0.5+(x/2)))); } //hamming window function used for FIR filter design

ADC *adc = new ADC(); // adc object

void setup() {
  // put your setup code here, to run once:

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(readPin, INPUT);

    adc->setReference(ADC_REFERENCE::REF_1V2, ADC_0);
    adc->setAveraging(1); // set number of averages
    adc->setResolution(8); // set bits of resolution

    adc->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_HIGH_SPEED);
    adc->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_HIGH_SPEED);
    adc->enableInterrupts(ADC_0);
    adc->startContinuous(readPin, ADC_0);
    
}

void loop() {
  // put your main code here, to run repeatedly:

}
