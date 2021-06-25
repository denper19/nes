#include "urom.h"

void urom::mapperLoad(const char* fileAddress)
{
	BankSelect = 0;
	prgRomSize = 0;
	chrRomSize = 0;
	header.load(fileAddress, 0, 0x10);
	prgRomSize = header[4];
	chrRomSize = header[5];
	fourscreen_mapper = header[6] & 0x8;
	mapper = !(header[6] & 0x1) & !fourscreen_mapper;
	CartridgeData.setSize(prgRomSize * 0x4000 + chrRomSize * 0x2000 + 0x2000);
	CartridgeData.load(fileAddress, 0, prgRomSize * 0x4000 + chrRomSize * 0x2000 + 0x2000);
}

void urom::mapperWrite(uint8_t data)
{
	BankSelect = data & 0xF;
}

uint8_t urom::mapperReadCHR(uint16_t addr)
{
	return CartridgeData[0x10 + (prgRomSize - 1) * 0x4000 + addr];
}

uint8_t urom::mapperRead(int addr)
{
	int MapperAddr = 0;
	if (addr >= 0x8000 && addr <= 0xBFFF)
	{
		MapperAddr = BankSelect * 0x4000 + (addr & 0x3FFF);
	}
	else if (addr >= 0xC000 && addr <= 0xFFFF)
	{
		MapperAddr = (prgRomSize - 1) * 0x4000 + (addr & 0x3FFF);
	}
	return CartridgeData[0x10 + MapperAddr];
}