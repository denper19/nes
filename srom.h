#ifndef  SROM
#define SROM
#include "cartridge.h"
#include "mem.h"
#include <vector>
class srom
{
private:
	const char* file = NULL;
	mem6502 header;
	mem6502 CartridgeData;
	uint8_t BankSelect;
	uint8_t prgRomSize;
	uint8_t control;
	uint8_t chrLO;
	uint8_t chrHI;
	uint8_t CHR;
	uint8_t prgLO;
	uint8_t prgHI;
	uint8_t PRG;
	uint8_t mregister;
	uint8_t reg_control;
	uint8_t reg_chr0, reg_chr1;
	uint8_t reg_prg;
	uint8_t counter;
public:
	uint8_t mapper;
	uint8_t fourscreen_mapper;
	uint8_t prgRamSize;
	uint8_t chrRomSize;
	void recalculate();
	void mapperLoad(const char*);
	uint8_t mapperRead(int);
	void mapperWrite(int, int);
	uint8_t mapperReadCHR(int);
	uint8_t mapperReadRAM(int);
};

#endif // ! SROM


