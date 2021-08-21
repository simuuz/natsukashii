#include <core.h>
#include <chrono>
#include <utility>

using clk = std::chrono::high_resolution_clock;

namespace natsukashii::core
{
Core::Core(bool skip, std::string bootrom_path) : bus(skip, std::move(bootrom_path)), cpu(skip, &bus) {}

void Core::Run() {
  while(cycles < scheduler.entries[0].time) {
    cycles += cpu.Step();
    cpu.HandleInterrupts(cycles);
    bus.mem.DoInputs(key);
  }
}

void Core::DispatchEvents() {
  int last_pos = 0;
  for(int i = 0; i < scheduler.pos; i++) {
    if(scheduler.entries[i].time > cycles) {
      last_pos = i;
      break;
    }

    switch(scheduler.entries[0].event) {
    case Event::None: case Event::APU:
      break;
    case Event::Timers:
      cpu.DispatchTimers(scheduler.entries[i].time, scheduler);
      break;
    case Event::PPU:
      bus.ppu.DispatchEvents(scheduler.entries[i].time, scheduler, bus.mem.io.intf);
      break;
    case Event::Panic:
      printf("Panic event! Achievement unlocked: \"How did we get here?\"\n");
      exit(1);
    }
  }

  scheduler.pop(last_pos);
}

void Core::LoadROM(std::string path) {
  cpu.Reset();
  bus.Reset();
  bus.LoadROM(std::move(path));
  init = true;
}

void Core::Reset() {
  cpu.Reset();
  bus.Reset();
}

void Core::Pause() {
  pause = !pause;
}

void Core::Stop() {
  cpu.Reset();
  bus.Reset();
  init = false;
}

void Core::SaveState(int slot) {
  cpu.SaveState(slot);
}

void Core::LoadState(int slot) {
  cpu.LoadState(slot);
}

[[noreturn]] void Core::RunAsync() {
  while (true) {
    WaitPing();
    Run();
    run_emu_thread = false;
  }
}

void Core::WaitPing() {
  std::unique_lock <std::mutex> lock (emu_mutex);
  emu_condition_variable.wait(lock, [&]{ return run_emu_thread == true; });
}

}  // namespace natsukashii::core