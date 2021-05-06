#include "apu.h"
#include "Bus.h"

int dutytable[] = { 0b00000001, 0b00000011, 0b00001111, 0b11111100 };
int lenTable(uint8_t a) {
	switch (a) {
	case 0x00:return 10;
	case 0x02:return 20;
	case 0x04:return 40;
	case 0x06:return 80;
	case 0x0A:return 160;
	case 0x0C:return 60;
	case 0x0E:return 14;
	case 0x10:return 12;
	case 0x12:return 24;
	case 0x14:return 48;
	case 0x16:return 96;
	case 0x18:return 192;
	case 0x1A:return 72;
	case 0x1C:return 16;
	case 0x1E:return 32;
	case 0x1F:return 30;
	case 0x1D:return 28;
	case 0x1B:return 26;
	case 0x19:return 24;
	case 0x17:return 22;
	case 0x15:return 20;
	case 0x13:return 18;
	case 0x11:return 16;
	case 0x0F:return 14;
	case 0x0D:return 12;
	case 0x0B:return 10;
	case 0x09:return 8;
	case 0x07:return 6;
	case 0x05:return 4;
	case 0x03:return 2;
	case 0x01:return 254;
	}
}

void NesApu::Init()
{
	SDL_setenv("SDL_AUDIODRIVER", "directsound", 1);

	if (SDL_Init(SDL_INIT_AUDIO) < 0)
		printf("Error init Audio");

}

void NesApu::apu_write(uint16_t addr, uint8_t data)
{
	switch (addr)
	{
	case 0x4000:
		duty_table = dutytable[(data >> 5) & 0x2];
		decay_loop = data & 0x20;
		lenEn = !(data & 0x20);
		decayEn = !(data & 0x10);
		decay_V = data & 0xF;
		break;
	case 0x4001:
		sweep_timer = data & 0x70;
		sweep_neg = data & 0x8;
		sweep_shift = data & 0x7;
		sweep_reload = true;
		sweepEn = (data & 0x80) && (sweep_shift != 0);
		break;
	case 0x4002:
		freq_timer = data;
		break;
	case 0x4003:
		freq_timer = ((data & 0x7) << 8) + freq_timer;
		if (channelP1En)
		{
			lenCounter = lenTable(data & 0xF8);
		}
		freq_counter = freq_timer;
		duty_counter = 0;
		decay_reset_flag = true;
		break;
	case 0x4015:
		channelP1En = data & 0x1;
		if (!channelP1En)
			lenCounter = 0;
		break;
	case 0x4017:
		sequencer_mode = data & 0x80;
		irq_en = !(data & 0x40);
		next_seq_phase = 0;
		sequence_counter = ClocksToNextSequence();
		if (sequencer_mode)
		{
			Clock_QuarterFrame();
			Clock_HalfFrame();
		}
		if (!irq_en)
			irq_pending = false;
		break;
	}
}

uint8_t NesApu::apu_read(uint16_t)
{
	uint8_t output = 0;
	if (lenCounter != 0)
		output |= 0x01;

	if (irq_pending)
		output |= 0x40;

	irq_pending = false;

	return output;
}
void NesApu::pulse1()
{
	if (freq_counter > 0)
		--freq_counter;
	else
	{
		freq_counter = freq_timer;
		duty_counter = (duty_counter + 1) & 7;
	}
	if (dutytable[duty_counter] && lenCounter != 0 && !IsSweepForcingSilence())
	{
		if (decayEn)
			output = decay_hidden_vol;
		else
			output = decay_V;
	}
	else
		output = 0;
}

void NesApu::Clock_HalfFrame()
{
	if (sweep_reload)
	{
		sweep_counter = sweep_timer;
		sweep_reload = false;
	}
	else if (sweep_counter > 0)
	{
		--sweep_counter;
	}
	else
	{
		sweep_counter = sweep_timer;
		if (sweepEn && !IsSweepForcingSilence())
		{
			if (sweep_neg)
				freq_timer -= (freq_timer >> sweep_shift) + 1;
			else
				freq_timer += (freq_timer >> sweep_shift);
		}
	}

	if (lenEn && lenCounter > 0)
		--lenCounter;
}

void NesApu::Clock_QuarterFrame()
{
	if (decay_reset_flag)
	{
		decay_reset_flag = false;
		decay_hidden_vol = 0xF;
		decay_counter = decay_V;
	}
	else
	{
		if (decay_counter > 0)
			--decay_counter;
		else
		{
			decay_counter = decay_V;
			if (decay_hidden_vol > 0)
				--decay_hidden_vol;
			else if (decay_loop)
				decay_hidden_vol = 0xf;
		}
	}
}

bool NesApu::IsSweepForcingSilence()
{
	if (freq_timer < 8)
	{
		return true;
	}
	else if (!sweep_neg && (freq_timer + (freq_timer >> sweep_shift) >= 0x800))
		return true;
	else return false;
}

int NesApu::ClocksToNextSequence()
{
	static int counter = 0;
	++counter;
	if (counter > 4)
		counter = 0;
	switch (counter - 1)
	{
	case 0:
		return 3729;
	case 1:
		return 7457;
	case 2:
		return 11189;
	case 3:
		return 14915;
	}
}

void NesApu::step_apu()
{
	apu_cycles += 2;

	if (sequence_counter > 0)
	{
		--sequence_counter;
	}
	else
	{
		if (((apu_cycles == 3729) || (apu_cycles == 7466) || (apu_cycles == 11186) || (apu_cycles == 14915)) && sequencer_mode)
			Clock_HalfFrame();
		else if (((apu_cycles == 3729) || (apu_cycles == 7466) || (apu_cycles == 11186) || (apu_cycles == 14915)) && !sequencer_mode)
			Clock_QuarterFrame();
		++next_seq_phase;
		int max_phase = sequencer_mode == 0 ? 4 : 5;
		if (next_seq_phase > max_phase)
			next_seq_phase = 0;

		sequence_counter = ClocksToNextSequence();
	}
}

double NesApu::getSample()
{
	return finalSample;
}

