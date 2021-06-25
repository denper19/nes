#include "cartridge.h"

void Mapper::LoadRom(const char* file)
{
	rom = file;

	header.load(rom, 0, 0x10);
	prgRomSize = header[4];
	chrRomSize = header[5];
	mapperNum1 = header[6];
	mapperNum2 = header[7];
	prgRamSize = header[8];
	prgRamPresence = header[10];

	mapperNum = ((mapperNum1 & 0xF0) >> 4) | (mapperNum2 & 0xF0);
	prgRamPresence = mapperNum1 & 0x2;
	trainer = mapperNum1 & 0x4;
}
