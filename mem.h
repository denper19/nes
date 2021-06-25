#ifndef MEM6502_H
#define MEM6502_H

#include <vector>
#include <fstream>
#include <cstdint>

class mem6502
{
private:

	std::vector<uint8_t> mem;

public:
	mem6502();
	mem6502(int size);
	void setSize(int size);
	void mem6502init();
	uint8_t operator[](int)const;
	uint8_t& operator[](int);
	void load(char const*, int start, int bytes);
	void contents();
};

#endif // MEM6502_H