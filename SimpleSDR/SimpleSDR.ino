#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <complex.h>
#include <limits.h>
#include <math.h>


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


    // ====== calculate parameters for frequency translation and the decimating FIR filter ===
      //modulation: can be 'l', 'u', 'a', 'f' for LSB, USB, AM, FM respectively
    char modulation = 'a'; // fixed AM for now
    
      //d_shift_phase: how much the LO phase is changed from sample to sample when generating the LO (for the shift)
    float d_shift_phase = 0;//((atoi(argv[2])-atoi(argv[1]))/samp_rate)*2*M_PI; 
     
      //shift_phase: actual LO phase (for shift)
    float shift_phase = 0; 
     
      //deicmate_taps_length: how many FIR filter taps we need to reach a given transition BW (should be odd, thus the |1)
    const int decimate_taps_length = (int)(4.0f/(decimate_transition_bw/samp_rate)) | 1; 
    
     //decimate_taps: FIR filter taps for the decimating FIR filter:
    __complex__ float* decimate_taps = (__complex__ float*) malloc(2*sizeof(float)*decimate_taps_length); 
     
      //decimate_buffer: the last <decimate_taps_length> pieces of shifted input samples, on which the decimating FIR is applied
    __complex__ float* decimate_buffer = (__complex__ float*) calloc(2*sizeof( float),decimate_taps_length); 
     
      //decimate_taps_middle: the index of the item in the exact middle of <decimate_taps>
    int decimate_taps_middle = decimate_taps_length/2; 
      
      //decimate_factor: only 1 out of <decimate_factor> samples will remain at the output of the decimating FIR filter
    int decimate_factor = samp_rate / output_rate; 
      
      //decimate_dshift: in case of SSB mode, we shift the filter taps in the frequency domain, so that the filter will have an asymmetric transmission curve
    const __complex__ float decimate_dshift = (modulation=='u'?1:-1) * ((ssb_bw/2)/samp_rate)*2*M_PI;
    
      //decimate_cutoff_rate: the cutoff frequency (in proportion to the sampling frequency) of the decimating FIR filter
    const float decimate_cutoff_rate = (modulation=='u'||modulation=='l') ? (ssb_bw/2)/samp_rate : (amfm_bw/2)/samp_rate;
    
void setup() {


    // ====== design the FIR filter ===
      //calculate the tap in the middle:
    decimate_taps[decimate_taps_middle] = 2 * M_PI * decimate_cutoff_rate * hamming(0); 
      //calculate the remaining taps:
    for(int i=1; i<=decimate_taps_middle; i++)  
        decimate_taps[decimate_taps_middle-i] = decimate_taps[decimate_taps_middle+i] = (sin(2*M_PI*decimate_cutoff_rate*i)/i) * hamming((float)i/decimate_taps_middle);
      //in case of SSB, do the shift:
    if(modulation=='u'||modulation=='l') for(int i=0; i<decimate_taps_length; i++) { 
        decimate_taps[i] *= (sinf(shift_phase) + cosf(shift_phase) * I); 
        shift_phase += decimate_dshift;
        while(shift_phase>2*M_PI) shift_phase-=2*M_PI;
    }
    
    // ====== normalize FIR filter ===
    float decimate_taps_sum = 0; 
      //we sum up the absolute value of taps:
    for(int i=0; i<decimate_taps_length; i++) decimate_taps_sum += cabsf(decimate_taps[i]); 
      //and we divide by the sum:
    for(int i=0; i<decimate_taps_length; i++) decimate_taps[i] /= decimate_taps_sum; 
    int decimate_counter = 0;
    float last_phi = 0;
}

void loop() {
  
        // ====== load input samples (I and Q) ===
        int input_i = getchar(); 
        int input_q = getchar();
        __complex__ float input = ((float)input_i/(UCHAR_MAX/2.0)-1.0) + I*((float)input_q/(UCHAR_MAX/2.0)-1.0); 

        // ====== apply frequency translation (a.k.a. shift) ===
        //we increase the current LO phase by <d_shift_phase>:
        shift_phase += d_shift_phase; 
        //we bring the current LO phase back into the range (0, 2*pi]:
        while(shift_phase>2*M_PI) shift_phase-=2*M_PI;
        //we multiply the input sample by the output of the LO (thus this is a mixer):
        input *= sinf(shift_phase) + cosf(shift_phase) * I;

        // ====== apply decimating FIR filter ===
        // we write the shifted input sample at the end of <decimate_buffer>: 
        // 1. we fill the end of <decimate_buffer> with <decimate_taps_length> samples, 
        // 2. we apply the decimating FIR filter once, 
        // 3. we move the data in the buffer back <decimate_taps_length> samples, and start again.
        decimate_buffer[decimate_taps_length-decimate_factor+decimate_counter] = input; 
        if(++decimate_counter >= decimate_factor) {
            //we only run this part 1 time out of <decimate_factor> times of getting here:
            decimate_counter = 0;
            //we apply the decimating FIR filter, the result of which is <decimated>:
            __complex__ float decimated = CMPLX(0,0); 
            for(int i=0; i<=decimate_taps_length; i++) decimated += decimate_buffer[i] * decimate_taps[i];
            //we shift the items in the buffer back <decimate_taps_length> samples:
            memmove(decimate_buffer, decimate_buffer+decimate_factor, (decimate_taps_length-decimate_factor)*sizeof(complex float));

            // ====== apply demodulator ===
            short demodulator_output;
            if(modulation=='f') { 
                //we apply the FM demodulator; phi is the current phase of the <decimated> sample:
                float phi = cargf(decimated);
                //we calculate the phase change between each <decimated> sample:
                float dphi = phi - last_phi; 
                //...so we need to remember the last phase of it next time:
                last_phi = phi; 
                //we want the phase to be in the range [-pi, pi]:
                while(dphi<-M_PI) dphi += 2*M_PI;
                while(dphi>M_PI) dphi -= 2*M_PI;
                //we rescale the input to a 16-bit short, and there we have our audio sample:
                demodulator_output = (SHRT_MAX-1)*(dphi/M_PI);
            }
            //we apply the AM demodulator, which just calculates the absolute value of each <decimated> sample:
            else if(modulation=='a') demodulator_output = cabsf(decimated) * SHRT_MAX; 
            //we apply the SSB demodulator, which just calculates the real value of each <decimated> sample 
            //(this works as we already removed the other sideband at the decimating FIR filter):
            else demodulator_output = crealf(decimated) * SHRT_MAX; 
            //we write the audio sample to stdout:
            fwrite(&demodulator_output, sizeof(short), 1, stdout); 
            //we exit the program if the task supplying the input data on stdin is closed:
            if(feof(stdin)) break; 
        }
    }


}



