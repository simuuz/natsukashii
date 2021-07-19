#pragma once
#include "ch1.h"
#include "ch2.h"
#include "ch3.h"
#include "ch4.h"
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

constexpr int FREQUENCY = 48000;
constexpr int CHANNELS = 2;
constexpr int SAMPLES = 4096;

namespace natsukashii::core
{
struct Apu {
	~Apu();
	Apu(bool skip);
	void Reset();
	void Step(u8 cycles);

	CH1 ch1;
	CH2 ch2;
	CH3 ch3;
	CH4 ch4;
	
	bool left_enable, right_enable;
	u8 left_volume, right_volume;
	u8 nr51;
	u8 ReadIO(u16 addr);
	void WriteIO(u16 addr, u8 val);
	bool skip;
	u32 sample_clock = 0;
	int buffer_pos = 0;
	u8 frame_sequencer_position = 0;
	float buffer[SAMPLES * 2];
	SDL_AudioDeviceID device;
	bool apu_enabled = false;
};
} // natsukashii::core