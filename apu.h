#ifndef APU_H
#define APU_H

#include "SDL_audio.h"
#include "SDL.h"
#include <iostream>
#include <cstdio>

constexpr int DEVICE_SAMPLES = 512;
constexpr float NES_SAMPLING_RATE = 1789773;
constexpr float DESIRED_SAMPLING_RATE = 44100;

typedef struct PULSE
{
	double output_sample = 0.0;

	bool sweep_enable = false;
	bool sweep_negate = false;
	bool write_to_fourth_channel = false;
	bool write_to_sweep_register = false;

	uint16_t period;
	uint8_t duty;
	uint8_t duty_counter;

	uint8_t decay_V;
	uint8_t decay_hidden_vol;
	uint8_t decay_counter;
	uint8_t decay_enabled;
	uint8_t decay_loop;
	uint8_t len_enabled;

	uint8_t sweep_divider;
	uint8_t sweep_counter;
	uint8_t sweep_period;
	uint8_t sweep_shift;

	uint8_t len_counter;
	uint16_t freq_counter;

}PULSE;

typedef struct TRIANGLE
{
	double output_sample = 0.0;
	bool linear_reload;
	bool ultrasonic;
	bool clock_triunit;
	uint8_t tri_step;
	uint8_t linear_counter;
	uint8_t length_counter;
	uint8_t linear_control;
	uint8_t length_enabled;
	uint8_t linear_load;
	uint8_t freq_timer;
	uint8_t freq_counter;
}TRIANGLE;

typedef struct NOISE
{
	double output_sample;
}NOISE;

typedef struct DMC
{
	double output_sample;
}DMC;

class Bus;

class NesApu
{
private:
	Bus* busPtr;

	//General
	uint8_t step;
	int cpu_cycles;
	int cycles_to_next_sequence;
	bool mode = false;
	bool SQ1_enable = false;
	bool SQ2_enable = false;
	bool DMC_enable = false;
	bool Noise_enable = false;
	bool Triangle_enable = false;
	bool irq_pending = false;
	bool irq_disable = false;
	bool switch_flip = false;
	
	PULSE p1;
	PULSE p2;
	TRIANGLE t;
	NOISE n;
	DMC d;


public:
	NesApu();

	//Attaches APU to the NES
	void ConnectTotBus(Bus* ptr) {
		busPtr = ptr;
	}

	//Register read/writes
	uint8_t apu_read(uint16_t);
	void apu_write(uint16_t, uint8_t);

	//
	bool isSweepForcingSilence();
	void clock_FrameSequencer();
	void clock_LengthCounter();
	void clock_Sweep();
	void clock_Envelope();

	bool SQ2isSweepForcingSilence();
	void SQ2clock_LengthCounter();
	void SQ2clock_Sweep();
	void SQ2clock_Envelope();

	void clock_LinearCounter();
	void TRIclock_LengthCounter();
	Sint16 getSample();
	//

	void step_apu(int);
	void step_pulse(int);
	void step_triangle(int);
};

#endif

