#include "Bus.h"

SDL_AudioCVT cvt;

Sint16 sound_buffer[735 * 6];

int readPointer = 0;

long long int readSamples = 0;

static void my_callback(void* userdata, Uint8* stream, int len) {

	Sint16* stream16 = (Sint16*)stream;

	for (int i = 0; i < (len / 2); i++)
	{
		stream16[i] = sound_buffer[readPointer];

		if ((readPointer + 1) >= (735 * 6)) { readPointer = 0; }
		else { readPointer++; }

		readSamples++;
	}
}

Bus::Bus()
{
	cpu = NULL;
	ppu = NULL;
	apu = NULL;

	for (int i = 0; i < 0xffff; i++) { ram[i] = 0xEA; }

	SDL_Init(SDL_INIT_AUDIO);

	// setting up our audio format:
	SDL_AudioSpec audio_spec = { 0 };
	audio_spec.freq = 44100; // sampling rate
	audio_spec.format = AUDIO_S16SYS; // sample format
	audio_spec.channels = 1; // number of channels
	audio_spec.samples = DEVICE_SAMPLES;
	audio_spec.callback = my_callback;
	audio_spec.userdata = NULL;

	audio_device = SDL_OpenAudioDevice(NULL, 0, &audio_spec, NULL, 0);

	apu_raises_irq = false;

}

void Bus::BusMapperSet()
{
	cartridge.mapperLoad("roms/tetris.nes");
	if (cartridge.prgRamSize != 0)
	{
		for (int i = 0; i < 0x2000; i++) { BusWrite(0x6000 + i, cartridge.mapperReadRAM(i)); }
	}
}

void Bus::bus_ppu_write(uint16_t addr, uint8_t data)
{
	ppu->ppu_write(addr, data);
}

void Bus::ConnectBus(cpu6502* cpuPtr, NesPPU* ppuPtr, NesApu* apuPtr)
{
	cpu = cpuPtr;
	ppu = ppuPtr;
	apu = apuPtr;
}

uint8_t Bus::ask_cpu(uint16_t addr)
{
	return (uint8_t)cpu->read(addr);
}

uint8_t Bus::bus_read_ppu(int addr)
{
	return cartridge.mapperReadCHR(addr);
}

uint16_t Bus::BusRead(uint16_t addr)
{
	uint8_t data = 0;

	if (addr >= 0x000 && addr < 0x2000)
	{
		data = ram[addr & 0x7FF];
	}
	else if (addr >= 0x2000 && addr < 0x4000)
	{
		data = ppu->ppu_read_registers(addr);
	}
	else if (addr >= 0x4000 && addr < 0x4020)
	{
		if (addr == 0x4015)
		{
			uint8_t flg = apu->apu_read(addr);
			apu_raises_irq = flg & 0x40;
		}
		else if (addr >= 0x4016 && addr <= 0x4017)
		{
			if (addr == 0x4016) {
				if (controller_index < 8) { return 0x40 | read_controller(controller_index++); }
				return 0x40;
			}
		}
		else
		{
			data = ram[addr];
		}
	}
	else if (addr >= 0x8000 && addr <= 0xFFFF)
	{
		data = cartridge.mapperRead(addr);
	}
	else
	{
		data = ram[addr];
	}

	return data;
}

uint8_t Bus::read_controller(int index)
{
	return (controller >> index) & 0x1;
}

void Bus::write_controller(uint8_t* keys)
{
	controller = 0;
	controller |= keys[SDL_SCANCODE_J] ? 0x80 : 0x00;
	controller |= keys[SDL_SCANCODE_K] ? 0x40 : 0x00;
	controller |= keys[SDL_SCANCODE_L] ? 0x20 : 0x00;
	controller |= keys[SDL_SCANCODE_I] ? 0x10 : 0x00;
	controller |= keys[SDL_SCANCODE_S] ? 0x08 : 0x00;//UP
	controller |= keys[SDL_SCANCODE_DOWN] ? 0x04 : 0x00;//DOWN
	controller |= keys[SDL_SCANCODE_A] ? 0x02 : 0x00;//LEFT
	controller |= keys[SDL_SCANCODE_D] ? 0x01 : 0x00;//RIGHT
}

void Bus::BusWrite(uint16_t addr, uint8_t data)
{
	if (addr >= 0x000 && addr < 0x2000)
	{
		ram[addr & 0x7FF] = data;
	}
	else if (addr >= 0x2000 && addr < 0x4000)
	{
		ppu->ppu_write_registers(addr, data);
	}
	else if (addr >= 0x4000 && addr < 0x4020)
	{
		if (addr == 0x4014)
		{
			ppu->ppu_write_4014(data);
		}
		else if ((addr == 0x4015) || (addr == 0x4000) || (addr == 0x4001) || (addr == 0x4002) || (addr == 0x4003))
		{
			apu->apu_write(addr, data);
		}
		else if (addr >= 0x4016 && addr <= 0x4017)
		{
			if (addr == 0x0417)
			{
				apu->apu_write(addr, data);
			}

			if (addr == 0x4016)
			{
				strobe = data & 0x1;
				if (strobe)
					controller_index = 8;
				else
					controller_index = 0;
			}
		}
		else
		{
			ram[addr] = data;
		}
	}
	else if (addr >= 0x8000 && addr <= 0xFFFF)
	{
		cartridge.mapperWrite(addr, data);
	}
	else
	{
		ram[addr] = data;
	}
}

void Bus::init()
{
	cpu->reset();
	ppu->ppu_powerup();
}

void half_to_byte_array(Uint8* arr, int index, Sint16 value) {
	memcpy(arr + index, &value, sizeof(Sint16));
}

void Bus::run()
{
	int cycles = 0;

	int control_cycles = 0;

	bool running = true;

	writePointer = 0;

	InitialWritePointer = 0;

	SDL_BuildAudioCVT(&cvt, AUDIO_S16SYS, 1, NES_SAMPLING_RATE, AUDIO_S16SYS, 1, DESIRED_SAMPLING_RATE);

	SDL_assert(cvt.needed);

	cvt.len = DEVICE_SAMPLES * (NES_SAMPLING_RATE/DESIRED_SAMPLING_RATE) * sizeof(Sint16);

	cvt.buf = (Uint8*)SDL_malloc(cvt.len * cvt.len_mult);

	SDL_PauseAudioDevice(audio_device, 0);

	while (running)
	{
		if ((writeSamples - readSamples) < (4410 - 512 - 98))
		{
			ppu->horizontal_mirroring = cartridge.mapper;

			if (apu_raises_irq)
			{
				cpu->irq();
				apu_raises_irq = false;
			}

			if (ppu->check_for_nmi()) {
				cpu->nmi();
				ppu->set_nmi_occured(0);
			}

			cycles = cpu->step_instruction();

			for (int i = 0; i < cycles * 3; i++) {
				ppu->step_ppu();
			}


			Sint16* cvt_buf = (Sint16*)cvt.buf;

			apu->step_apu(cycles);

			for (int i = 0; i < cycles; i++)
			{

				cvt_buf[bufferWriteSamples] = apu->getSample();

				bufferWriteSamples++;

				if (bufferWriteSamples * 2 >= cvt.len)
				{
					SDL_ConvertAudio(&cvt);

					for (int j = 0; j < (cvt.len_cvt / 2); j++)
					{
						sound_buffer[writePointer] = cvt_buf[j];
						writePointer++;
						if (writePointer >= 4410)
							writePointer = 0;

						writeSamples++;
					}

					bufferWriteSamples = 0;
				}
			}

			control_cycles += cycles;

			cycles = 0;

			if (control_cycles > 29780) {
				SDL_Event event;
				while (SDL_PollEvent(&event)) {
					if (event.type == SDL_QUIT) {
						running = false;
					}
				}
				uint8_t* kb = (uint8_t*)SDL_GetKeyboardState(NULL);
				write_controller(kb);
				control_cycles = 0;
			}
		}
	}
	SDL_CloseAudioDevice(audio_device);
}

