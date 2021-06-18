#include "apu.h"
#include "Bus.h"

uint8_t len_table[16][2] =
{
	{0x0A, 0xFE},
	{0x14, 0x02},
	{0x28, 0x04},
	{0x50, 0x06},
	{0xA0, 0x08},
	{0x3C, 0x0A},
	{0x0E, 0x0C},
	{0x1A, 0x0E},
	{0x0C, 0x10},
	{0x18, 0x12},
	{0x30, 0x14},
	{0x60, 0x16},
	{0xC0, 0x18},
	{0x48, 0x1A},
	{0x10, 0x1C},
	{0x20, 0x1E}
};

uint8_t duty_table[4][8] =
{
	{0, 1, 0, 0, 0, 0, 0, 0},
	{0, 1, 1, 0, 0, 0, 0, 0},
	{0, 1, 1, 1, 1, 0, 0, 0},
	{1, 0, 0, 1, 1, 1, 1, 1}
};

void NesApu::apu_write(uint16_t addr, uint8_t data)
{
	switch (addr)
	{
	case 0x4000:
	{
		p1.decay_V = data & 0xF;
		p1.decay_enabled = !((data >> 4) & 0x1);
		p1.decay_loop = (data >> 5) & 0x1;
		p1.len_enabled = !((data >> 5) & 0x1);
		p1.duty = (data >> 6) & 0x3;
		break;
	}
	case 0x4001:
	{
		p1.write_to_sweep_register = true;
		p1.sweep_shift = data & 0x7;
		p1.sweep_negate = (data >> 3) & 0x1;
		p1.sweep_period = (data >> 4) & 0x7;
		p1.sweep_enable = ((data >> 7) & 0x1) && (p1.sweep_shift != 0);
		break;
	}
	case 0x4002:
	{
		p1.period = data;
		break;
	}
	case 0x4003:
	{
		p1.write_to_fourth_channel = true;
		p1.period |= ((data & 0x7) << 3);
		if (SQ1_enable) {
			p1.len_counter = len_table[(((data >> 3) & 0x1F) >> 4) & 0xF][(((data >> 3) & 0x1F) >> 3) & 0x1];
		}
		p1.duty_counter = 0;
		p1.freq_counter = p1.period;
		break;
	}
	case 0x4004:
	{
		p2.decay_V = data & 0xF;
		p2.decay_enabled = !((data >> 4) & 0x1);
		p2.decay_loop = (data >> 5) & 0x1;
		p2.len_enabled = !((data >> 5) & 0x1);
		p2.duty = (data >> 6) & 0x3;
		break;
	}
	case 0x4005:
	{
		p2.write_to_sweep_register = true;
		p2.sweep_shift = data & 0x7;
		p2.sweep_negate = (data >> 3) & 0x1;
		p2.sweep_period = (data >> 4) & 0x7;
		p2.sweep_enable = ((data >> 7) & 0x1) && (p2.sweep_shift != 0);
		break;
	}
	case 0x4006:
	{
		p2.period = data;
		break;
	}
	case 0x4007:
	{
		p2.write_to_fourth_channel = true;
		p2.period |= ((data & 0x7) << 3);
		if (SQ2_enable) {
			p2.len_counter = len_table[(((data >> 3) & 0x1F) >> 4) & 0xF][(((data >> 3) & 0x1F) >> 3) & 0x1];
		}
		p2.duty_counter = 0;
		p2.freq_counter = p2.period;
		break;
	}
	case 0x4008:
	{
		t.linear_control = data & 0x80;
		t.length_enabled = !(data & 0x80);
		t.linear_load =    data & 0x7f;
	}
	case 0x400A:
	{
		t.freq_timer = data & 0xff;
	}
	case 0x400B:
	{
		t.freq_timer |= ((data & 0x7) << 3);

		if (Triangle_enable)
			t.length_counter = len_table[(((data >> 3) & 0x1F) >> 4) & 0xF][(((data >> 3) & 0x1F) >> 3) & 0x1];

		t.linear_reload = true;
	}
	case 0x4015:
	{
		SQ1_enable = data & 0x1;
		SQ2_enable = data & 0x2;
		Triangle_enable = data & 0x4;
		Noise_enable = data & 0x8;
		DMC_enable = data & 0x10;
		if (!SQ1_enable) { p1.len_counter = 0; }
		if (!SQ2_enable) { p2.len_counter = 0; }
		if (!Triangle_enable) { t.length_counter = 0; }
	}
	case 0x4017:
	{
		mode = data & 0x80;
		irq_disable = data & 0x40;
		if (mode)
		{
			/*mode 1 is immediately clocked*/
			clock_Sweep();
			clock_Envelope();
			clock_LengthCounter();

			SQ2clock_Sweep();
			SQ2clock_Envelope();
			SQ2clock_LengthCounter();
		}
		if (irq_disable) {
			irq_pending = false;
		}
		break;
	}
	default:
		printf("Addr here is not handled yet. Only working with sq1 right now =)");
		break;
	}
}

NesApu::NesApu()
{
	busPtr = NULL;
	cpu_cycles = 0;
	cycles_to_next_sequence = 0;
}

uint8_t NesApu::apu_read(uint16_t addr)
{
	if (addr == 0x4015)
	{
		uint8_t output = 0;
		if (p1.len_counter != 0) output |= 0x1;
		if (p2.len_counter != 0) output |= 0x2;
		if (t.length_counter != 0) output |= 0x4;
		if (irq_pending) output |= 0x40;
		irq_pending = false;
		return output;
	}
	else
	{
		printf("Only reads from address 0x4015, Err in apu_read in apu.cpp| return 0");
		return 0;
	}
}

void NesApu::clock_FrameSequencer()
{
	if (!mode)
	{
		//4-step sequence

		switch (step)
		{
		case 0:
			clock_Envelope();

			SQ2clock_Envelope();

			clock_LinearCounter();
			break;
		case 1:
			clock_Sweep();
			clock_LengthCounter();
			clock_Envelope();

			SQ2clock_Sweep();
			SQ2clock_LengthCounter();
			SQ2clock_Envelope();

			clock_LinearCounter();
			TRIclock_LengthCounter();
			break;
		case 2:
			clock_Envelope();

			SQ2clock_Envelope();

			clock_LinearCounter();
			break;
		case 3:
			if (!irq_disable)
				irq_pending = true;
			clock_Sweep();
			clock_Envelope();
			clock_LengthCounter();

			SQ2clock_Sweep();
			SQ2clock_Envelope();
			SQ2clock_LengthCounter();

			TRIclock_LengthCounter();
			clock_LinearCounter();
			//wrap back to 0
			step = 0;
			return;
		}
	}
	else if (mode)
	{
		//5-step sequence
		/*
			Clocked immediately when mode 5
		*/
		switch (step)
		{
		case 0:
			clock_Envelope();

			SQ2clock_Envelope();

			clock_LinearCounter();
			break;
		case 1:
			clock_Sweep();
			clock_LengthCounter();
			clock_Envelope();

			SQ2clock_Sweep();
			SQ2clock_LengthCounter();
			SQ2clock_Envelope();

			clock_LinearCounter();
			TRIclock_LengthCounter();
			break;
		case 2:
			clock_Envelope();

			SQ2clock_Envelope();

			clock_LinearCounter();
			break;
		case 3:
			break;
		case 4:
			clock_Envelope();
			clock_LengthCounter();
			clock_Sweep();

			SQ2clock_Envelope();
			SQ2clock_LengthCounter();
			SQ2clock_Sweep();

			clock_LinearCounter();
			TRIclock_LengthCounter();
			//wrap back to 0
			step = 0;
			return;
		}
	}

	step++;
}

void NesApu::clock_LengthCounter()
{
	if (p1.len_enabled && (p1.len_counter > 0)){
			p1.len_counter--;
	}
}

void NesApu::SQ2clock_LengthCounter()
{
	if (p2.len_enabled && (p2.len_counter > 0)){
			p2.len_counter--;
	}
}

void NesApu::clock_Envelope()
{
	if (p1.write_to_fourth_channel)
	{
		p1.write_to_fourth_channel = false;
		p1.decay_hidden_vol = 0xF;
		p1.decay_counter = p1.decay_V;
	}
	else
	{
		if (p1.decay_counter > 0)
			p1.decay_counter--;
		else
		{
			p1.decay_counter = p1.decay_V;
			if (p1.decay_hidden_vol > 0)
				p1.decay_hidden_vol--;
			else if (p1.decay_loop)
				p1.decay_hidden_vol = 0xF;
		}
	}
}

void NesApu::SQ2clock_Envelope()
{
	if (p2.write_to_fourth_channel)
	{
		p2.write_to_fourth_channel = false;
		p2.decay_hidden_vol = 0xF;
		p2.decay_counter = p2.decay_V;
	}
	else
	{
		if (p2.decay_counter > 0)
			p2.decay_counter--;
		else
		{
			p2.decay_counter = p2.decay_V;
			if (p2.decay_hidden_vol > 0)
				p2.decay_hidden_vol--;
			else if (p2.decay_loop)
				p2.decay_hidden_vol = 0xF;
		}
	}
}

void NesApu::clock_LinearCounter()
{
	if (t.linear_reload)
		t.linear_counter = t.linear_load;
	else if (t.linear_counter > 0)
		t.linear_counter--;
	if (!t.linear_control)
		t.linear_reload = false;
}

void NesApu::TRIclock_LengthCounter()
{
	if (t.length_enabled && (t.length_counter > 0)) {
		t.length_counter--;
	}
}

Sint16 NesApu::getSample()
{
	d.output_sample = 0.0;
	n.output_sample = 0.0;
	Sint16 result;
	//result is between 0.0 and 1.0
	double pulse_out = (95.52 / ((8128.0 / (p1.output_sample + p2.output_sample)) + 100));
	double tnd_out = 163.67 / (24329.0 / (3 * t.output_sample + 2 * n.output_sample + d.output_sample) + 100);
	//convert to signed 16 bit value
	result = (Sint16)std::floor((pulse_out + tnd_out) * 65535);
	result ^= 0x8000;
	return result;
}


void NesApu::clock_Sweep()
{
	if (p1.write_to_sweep_register)
	{
		p1.sweep_counter = p1.sweep_period;
		p1.write_to_sweep_register = false;
	}
	else if (p1.sweep_counter > 0)
		p1.sweep_counter--;
	else
	{
		p1.sweep_counter = p1.sweep_period;
		if (p1.sweep_enable && !isSweepForcingSilence())
		{
			if (p1.sweep_negate) 
			 p1.period -= (p1.period >>= p1.sweep_shift + 1);
			else 
			 p1.period += (p1.period >>= p1.sweep_shift);
		}
	}
}

void NesApu::SQ2clock_Sweep()
{
	if (p2.write_to_sweep_register)
	{
		p2.sweep_counter = p2.sweep_period;
		p2.write_to_sweep_register = false;
	}
	else if (p2.sweep_counter > 0)
		p2.sweep_counter--;
	else
	{
		p2.sweep_counter = p2.sweep_period;
		if (p2.sweep_enable && !SQ2isSweepForcingSilence())
		{
			if (p2.sweep_negate) 
			 p2.period -= (p2.period >>= p2.sweep_shift);
			else 
			 p2.period += (p2.period >>= p2.sweep_shift);
		}
	}
}

void NesApu::step_apu(int cycles)
{
	for (int i = 0; i < cycles; i++)
	{
		if (p1.freq_counter > 0)
		{
			p1.freq_counter--;
		}
		else
		{
			p1.freq_counter = p1.period;
			p1.duty_counter = (p1.duty_counter + 1) & 0x7;
		}

		if (p2.freq_counter > 0)
		{
			p2.freq_counter--;
		}
		else
		{
			p2.freq_counter = p2.period;
			p2.duty_counter = (p2.duty_counter + 1) & 0x7;
		}

		if (!isSweepForcingSilence() && (p1.len_counter != 0) && (duty_table[p1.duty][p1.duty_counter]))
		{
			if (p1.decay_enabled)
			{
				p1.output_sample = p1.decay_hidden_vol;
			}
			else
			{
				p1.output_sample = p1.decay_V;
			}
		}
		else
		{
			p1.output_sample = 0;
		}

		if (!SQ2isSweepForcingSilence() && (p2.len_counter != 0) && (duty_table[p2.duty][p2.duty_counter]))
		{
			if (p2.decay_enabled)
			{
				p2.output_sample = p2.decay_hidden_vol;
			}
			else
			{
				p2.output_sample = p2.decay_V;
			}
		}
		else
		{
			p2.output_sample = 0;
		}
	}

		if (cpu_cycles >= cycles_to_next_sequence)
		{
			clock_FrameSequencer();
			cpu_cycles -= cycles_to_next_sequence;
			cycles_to_next_sequence += 7458;
			if ((mode && (cycles_to_next_sequence >= 37281)) || (!mode && (cycles_to_next_sequence >= 29828)))
			{
				cycles_to_next_sequence = 0;
			}
		}
		else
		{
			cpu_cycles += cycles;
		}

	for (int i = 0; i < cycles; i++)
	{
		t.ultrasonic = false;
		if (t.freq_timer < 2 && t.freq_counter == 0)
			t.ultrasonic = true;

		t.clock_triunit = true;

		if (t.length_counter == 0) 
			t.clock_triunit = false;

		if (t.linear_counter == 0) 
			t.clock_triunit = false;

		if (t.ultrasonic) 
			t.clock_triunit = false;

		if (t.clock_triunit)
		{
			if (t.freq_counter > 0)
				t.freq_counter--;
			else
			{
				t.freq_counter = t.freq_timer;
				t.tri_step = (t.tri_step + 1) & 0x1F;
			}
		}

		if (t.ultrasonic)
			t.output_sample = 7.5;
		else if (t.tri_step & 0x10)
			t.output_sample = t.tri_step ^ 0x1F;
		else
			t.output_sample = t.tri_step;
	}
}

bool NesApu::isSweepForcingSilence()
{
	if (p1.period < 8){
		return true;
	}
	else if (!p1.sweep_negate && (((p1.period + (p1.period >> p1.sweep_shift)) >= 0x800))) {
		return true;
	}
	else { return false; }
}

bool NesApu::SQ2isSweepForcingSilence()
{
	if (p2.period < 8){
		return true;
	}
	else if (!p2.sweep_negate && (((p2.period + (p2.period >> p2.sweep_shift)) >= 0x800))) {
		return true;
	}
	else { return false; }
}