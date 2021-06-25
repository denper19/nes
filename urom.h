#ifndef UROM
#define UROM

#include "cartridge.h"
#include "mem.h"
#include <vector>

class urom : public Mapper
{
private:
	const char* file = NULL;
	mem6502 header;
	mem6502 CartridgeData;
	uint8_t BankSelect;
	uint8_t prgRomSize;
public:
	uint8_t mapper;
	uint8_t fourscreen_mapper = 0;
	uint8_t chrRomSize;
	void mapperLoad(const char*);
	uint8_t mapperRead(int);
	void mapperWrite(uint8_t);
	uint8_t mapperReadCHR(uint16_t);
};
#endif // !UROM


