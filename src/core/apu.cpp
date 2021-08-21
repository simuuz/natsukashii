#include "apu.h"

namespace natsukashii::core
{
Apu::~Apu() {
	SDL_CloseAudioDevice(device);
}

void audio_callback(void* userdata, Uint8* buffer, int len) {
  if(((Apu*)userdata)->buffer_pos >= SAMPLES * 2) {
    ((Apu*)userdata)->buffer_pos = 0;
    u32 len = SAMPLES * CHANNELS * sizeof(float);
    while(SDL_GetQueuedAudioSize(((Apu*)userdata)->device) > len * 4) {	}
    SDL_QueueAudio(((Apu*)userdata)->device, buffer, len);
  }
}

Apu::Apu(bool skip) : skip(skip)
{
	SDL_Init(SDL_INIT_AUDIO);
	memset(buffer, 0, SAMPLES * 2);
	SDL_AudioSpec want = {
		.freq = FREQUENCY,
		.format = AUDIO_F32SYS,
		.channels = CHANNELS,
		.samples = SAMPLES,
		.callback = audio_callback,
		.userdata = this,
	}, have;

	device = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
	if (device == 0) {
		printf("Failed to open audio device: %s\n", SDL_GetError());
		exit(1);
	}
	
	SDL_PauseAudioDevice(device, 0);
}

void Apu::Reset()
{
	apu_enabled = false;
	ch1.reset();
	ch2.reset();
	ch3.reset();
	memset(buffer, 0, SAMPLES * 2);
	SDL_CloseAudioDevice(device);
	SDL_AudioSpec want = {
		.freq = FREQUENCY,
		.format = AUDIO_F32SYS,
		.channels = CHANNELS,
		.samples = SAMPLES,
		.callback = audio_callback,
		.userdata = this,
	}, have;

	device = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
	if (device == 0) {
		printf("Failed to open audio device: %s\n", SDL_GetError());
		exit(1);
	}
  
	SDL_PauseAudioDevice(device, 0);
}

void Apu::WriteIO(u16 addr, u8 value) {
	switch(addr & 0xff) {
    case 0x10 ... 0x14: ch1.write(addr, value); break;
    case 0x16 ... 0x19: ch2.write(addr, value); break;
    case 0x1A ... 0x1E: case 0x30 ... 0x3f:
			ch3.write(addr, value);
			break;
		case 0x20 ... 0x23: ch4.write(addr, value); break;
		case 0x24:
			left_volume = (value >> 4) & 7;
			right_volume = value & 7;
			left_enable = value & 0x80;
			right_enable = value & 8;
			break;
		case 0x25: nr51 = value; break;
		case 0x26: {
			bool enable = value >> 7;
			if(!enable && apu_enabled) {
				for(u16 i = 0xff10; i <= 0xff25; i++) {
					WriteIO(i, 0);
				}

				apu_enabled = false;
			} else if(enable && !apu_enabled) {
				apu_enabled = true;
				frame_sequencer_position = 0;
				ch1.duty_index = 0;
				ch2.duty_index = 0;
				ch3.wave_pos = 0;
			}
		} break;
  }
}

u8 Apu::ReadIO(u16 addr) {
  switch(addr & 0xff) {
    case 0x10 ... 0x14: return ch1.read(addr);
    case 0x16 ... 0x19: return ch2.read(addr);
    case 0x1A ... 0x1E: case 0x30 ... 0x3f:
			return ch3.read(addr);
		case 0x20 ... 0x23: return ch4.read(addr);
		case 0x24:
			return (left_enable ? 0x80 : 0) | (left_volume << 4) |
						 (right_enable ? 8 : 0) | right_volume;
		case 0x25:
			return nr51;
		case 0x26:
			return (((u8)apu_enabled << 7) & 0x70) | (u8)ch1.nr14.enabled | ((u8)ch2.nr24.enabled << 1) | ((u8)ch3.nr34.enabled << 2) | ((u8)ch4.nr44.enabled << 3);
  }
}

void Apu::Step(u8 cycles) {
	for(int i = 0; i < cycles; i++) {
		sample_clock++;
		ch1.tick();
		ch2.tick();
		//ch3.tick();

		if((sample_clock % 8192) == 0) {
			sample_clock = 0;
			switch(frame_sequencer_position) {
				case 0:
				ch1.step_length();
				ch2.step_length();
				ch3.step_length();
				break;
				case 1: case 3: case 5: break;
				case 2:
				ch1.step_length();	
				ch2.step_length();
				ch3.step_length();
				ch1.step_sweep();
				break;
				case 4:
				ch1.step_length();
				ch2.step_length();
				ch3.step_length();
				break;
				case 6:
				ch1.step_length();
				ch2.step_length();
				ch3.step_length();
				ch1.step_sweep();
				break;
				case 7:
				ch1.step_volume();
				ch2.step_volume();
				break;
			}

			frame_sequencer_position = (frame_sequencer_position + 1) & 7;
		}
		
		if((sample_clock % (4194304 / FREQUENCY)) == 0) {
			buffer[buffer_pos++] = (left_volume / 7) * ((float)((ch1.sample() + ch2.sample() /*+ ch3.sample()*/)) / 8);
			buffer[buffer_pos++] = (right_volume / 7) * ((float)((ch1.sample() + ch2.sample() /*+ ch3.sample()*/)) / 8);
		}
	}
}
}