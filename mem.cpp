#include "mem.h"

mem6502::mem6502()
{
	mem.resize(65536);
	mem6502init();
}

mem6502::mem6502(int size)
{
	mem.resize(size);
	mem6502init();
}

void mem6502::setSize(int size)
{
	mem.resize(size);
	mem6502init();
}

void mem6502::mem6502init()
{
	for (int i = 0; i < mem.size(); i++)
	{
		mem[i] = 0xEA;
	}
}

void mem6502::load(char const* filepath, int start, int bytes)
{
	std::vector<uint8_t> result(bytes);
	std::ifstream file(filepath, std::ios::in | std::ios::binary);
	if (file.is_open())
	{
		printf("File read done\n\n");
		file.read((char*)& result[0], bytes);
		for (int i = 0; i < bytes; i++)
		{
			mem[start + i] = result[i];
		}
	}
	else
	{
		printf("File could not be opened\n\n");
	}
}

uint8_t mem6502::operator[](int addr)const
{
	return mem[addr];
}

uint8_t& mem6502::operator[](int addr)
{
	return mem[addr];
}

void mem6502::contents()
{
	for (int i = 0; i < 65536; i++)
	{
		printf("%x ", mem[i]);
	}
}