//#include "txrom.h"
//
//void txrom::mapperLoad(const char*)
//{
//	CartridgeData.setSize(prgRomSize * 0x4000 + chrRomSize * 0x2000 + prgRamSize * 0x2000 + 0x2000);
//	CartridgeData.load(fileAddress, 0, prgRomSize * 0x4000 + chrRomSize * 0x2000 + prgRamSize * 0x2000 + 0x2000);
//}
//
//uint8_t txrom::mapperRead(uint16_t addr, uint8_t data)
//{
//	uint32_t mapper_addr = 0;
//
//	if (addr >= 0x8000 && addr >= 0x9FFE)
//	{
//
//	}
//	else if (addr >= 0xA000 && addr <= 0xBFFE)
//	{
//
//	}
//	else if (addr >= 0xC000 && addr <= 0xDFFE)
//	{
//
//	}
//	else if (addr >= 0xE000 && addr <= 0xFFFE)
//	{
//
//	}
//	else
//	{
//
//	}
//}
