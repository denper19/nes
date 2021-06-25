#include "srom.h"

void srom::mapperLoad(const char* fileAddress)
{
	reg_control = 0;
	reg_chr0 = 0;
	reg_chr1 = 0;
	reg_prg = 0;
	prgRomSize = 0;
	chrRomSize = 0;
	header.load(fileAddress, 0, 0x10);
	prgRomSize = header[4];
	chrRomSize = header[5];
	prgRamSize = header[8];
	fourscreen_mapper = header[6] & 0x8;
	mapper = !(header[6] & 0x1) & !fourscreen_mapper;
	CartridgeData.setSize(prgRomSize * 0x4000 + chrRomSize * 0x2000 + prgRamSize * 0x2000 + 0x2000);
	CartridgeData.load(fileAddress, 0, prgRomSize * 0x4000 + chrRomSize * 0x2000 + prgRamSize * 0x2000 + 0x2000);
}

void srom::mapperWrite(int addr, int data)
{
	if (addr >= 0x8000)
	{
		if (data & 0x80)
		{
			mregister = 0;
			counter = 0;
			reg_control |= 0xC;
			recalculate();
		}
		else
		{
			mregister >>= 1;
			mregister += (data & 0x1) << 4;
			counter++;
			if (counter == 5)
			{
				if (addr >= 0x8000 && addr <= 0x9fff)
				{
					reg_control = mregister & 0x1f;
				}
				else if (addr >= 0xa000 && addr <= 0xbfff)
				{
					reg_chr0 = mregister & 0x1f;
				}
				else if (addr >= 0xc000 && addr <= 0xdfff)
				{
					reg_chr1 = mregister & 0x1f;
				}
				else if (addr >= 0xe000 && addr <= 0xffff)
				{
					reg_prg = mregister & 0x1f;
				}
				recalculate();
				mregister = 0;
				counter = 0;
			}
		}
	}
}

void srom::recalculate()
{
	//control = mregister & 0x1F;

	switch (reg_control & 0x3)
	{
	case 0:
	case 1:
	case 2:
		mapper = 0;
		break;
	case 3:
		mapper = 1;
		break;
	}

	if (reg_control & 0x10) { chrLO = reg_chr0 & 0x1f; }
	else { CHR = (reg_chr0 & 0x1e) >> 0; }

	if (reg_control & 0x10) { chrHI = reg_chr1 & 0x1f; }

	uint8_t mode = (reg_control >> 2) & 0x3;

	switch (mode)
	{
	case 0:
	case 1:
	{
		PRG = (reg_prg & 0xe) >> 1;
		break;
	}
	case 2:
	{
		prgLO = 0;
		prgHI = reg_prg & 0xf;
		break;
	}
	case 3:
	{
		prgLO = reg_prg & 0xf;
		prgHI = prgRomSize - 1;
		break;
	}
	}
}

uint8_t srom::mapperRead(int addr)
{
	int mapperAddr = 0;
	if (addr >= 0x8000)
	{
		if (reg_control & 0x8)
		{
			if (addr >= 0x8000 && addr <= 0xbfff)
				mapperAddr = prgLO * 0x4000 + (addr & 0x3fff);

			if (addr >= 0xc000 && addr <= 0xffff)
				mapperAddr = prgHI * 0x4000 + (addr & 0x3fff);
		}
		else
		{
			mapperAddr = PRG * 0x8000 + (addr & 0x7fff);
		}
	}
	return CartridgeData[0x10 + mapperAddr];
}

uint8_t srom::mapperReadCHR(int addr)
{
	if (chrRomSize != 0)
	{
		if (addr < 0x2000)
		{
			int mapperAddr = 0;
			if (reg_control & 0x10)
			{
				if (addr >= 0x000 && addr <= 0xfff)
				{
					mapperAddr = chrLO * 0x1000 + (addr & 0x0fff);
				}

				if (addr >= 0x1000 && addr <= 0x1fff)
				{
					mapperAddr = chrHI * 0x1000 + (addr & 0x0fff);
				}
			}
			else
			{
				mapperAddr = CHR * 0x2000 + (addr & 0x1fff);
			}
			return CartridgeData[0x10 + (prgRomSize) * 0x4000 + mapperAddr];
		}
	}
}

uint8_t srom::mapperReadRAM(int addr)
{
	return CartridgeData[0x4000 * (prgRomSize)+0x2000 * (chrRomSize)+addr];
}