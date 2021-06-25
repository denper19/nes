#pragma once
#include "cartridge.h"
class txrom : public Mapper
{
private:
	uint8_t reg[8];
	mem6502 CartridgeData;
	uint8_t prgRomSize;
	uint8_t chrRomSize;
	uint8_t prgRamSize, prgRamPresence;
	uint8_t mapperNum, mapperNum1, mapperNum2;
	uint8_t trainer;
public:
	void mapperLoad(const char*);
	uint8_t mapperRead(uint16_t, uint8_t);
	void mapperWrite(uint8_t);
	uint8_t mapperReadCHR(uint16_t);
	uint8_t mapperReadRAM(int);
};

