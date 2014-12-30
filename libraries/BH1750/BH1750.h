#ifndef BH1750_H
#define BH1750_H

#include "Arduino.h"
#include "Wire.h"

//something odd with Arduino compiler - these have to be declared as hex to work.
enum Mode {
	POWER_DOWN = 0,
	POWER_ON = 0x01,
	RESET = 0x07,
	CONT_HR_1 = 0x10,
	CONT_HR_HALF = 0x11,
	CONT_LR = 0x13,
	SS_HR_1 = 0x20,
	SS_HR_HALF = 0x21,
	SS_LR = 0x23
};

class BH1750
{
public:
	BH1750(byte address, Mode opmode);
	void begin();
	void reset();
	unsigned int read();
	void poweroff();
	void poweron();
	void setmode(void);
	

private:
	bool _initialised;
	byte _mymode;
	short _address;
};
#endif
