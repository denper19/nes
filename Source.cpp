#include "apu.h"
#include "cpu.h"
#include "Bus.h"
#include "ppu.h"
#include "mem.h"
#include <queue>
#undef main

int main()
{
	Bus main;
	cpu6502 cpu;
	NesPPU ppu;
	NesApu apu;
	main.ConnectBus(&cpu, &ppu, &apu);
	ppu.ConnectToBus(&main);
	cpu.ConnectTotBus(&main);
	apu.ConnectTotBus(&main);
	main.BusMapperSet();
	main.init();
	main.run();
	return 0;
}


