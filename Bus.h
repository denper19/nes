#ifndef  BUS_H
#define  BUS_H

#include <vector>
#include "urom.h"
#include "srom.h"
#include "cpu.h"
#include "ppu.h"
#include "apu.h"

class Bus
{
private:
	cpu6502* cpu;
	NesPPU* ppu;
	NesApu* apu;
	srom cartridge;
	uint8_t ram[65536];
	bool strobe;
	bool apu_raises_irq;
	uint8_t controller, controller_index;
	SDL_AudioDeviceID audio_device;
public:
	Bus();
	bool chrFilled() { return cartridge.chrRomSize; }
	uint8_t bus_read_ppu(int);
	void ConnectBus(cpu6502*, NesPPU*, NesApu*);
	uint16_t BusRead(uint16_t);
	void BusMapperSet();
	void BusWrite(uint16_t, uint8_t);
	uint8_t ask_cpu(uint16_t data);
	void bus_ppu_write(uint16_t addr, uint8_t data);
	void write_controller(uint8_t*);
	uint8_t read_controller(int index);
	void init();
	void run();
};

#endif // ! BUS_H


