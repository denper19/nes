#pragma once
#include "mem.h"
#include <iostream>

class Mapper
{
private:
	const char* rom = NULL;
	mem6502 header;
	uint8_t prgRomSize;
	uint8_t chrRomSize;
	uint8_t prgRamSize, prgRamPresence;
	uint8_t mapperNum, mapperNum1, mapperNum2;
	uint8_t trainer;
public:
	void LoadRom(const char* file);
	virtual void mapperLoad(const char*);
	virtual uint8_t mapperRead(int);
	virtual void mapperWrite(uint8_t);
	virtual uint8_t mapperReadCHR(uint16_t);
	virtual uint8_t mapperReadRam(uint16_t);
};

