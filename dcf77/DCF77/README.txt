The DCF77 library for the Arduino platform is a way to read the current
time from the german DCF77 time signal. You need an additional receiver
in order to use this library, I use the Conrad no. 641138 with a 8k 
pull-up resistor.

You can find an example on how to use this package in the examples subdirectory. The sketch will read the signal and dump the time to serial.

Note: This is a beta release with some functionality missing. The 
current todo list is:
 * Use interrupt routines for detecting signal changes
 * Use interrupt routines to add seconds automatically (currently, the      
   59th second is shown properly)
 * Implement parity checks to prevent faulty time signals to be
   evaluated

Installation: You should copy this directory to you arduino installation
under <Arduino dir>/lib/targets/libraries.
	
Note: Currently, the library uses 6400 bytes (of the 7168 bytes an 
arduino NG provides). You may want to replace your ATMega8 with an 
ATMega168.

You are welcome to send me feedback to md.AT.gonium.net