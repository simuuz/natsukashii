#include "core.h"
#include <chrono>

using clk = std::chrono::high_resolution_clock;

namespace natsukashii::core
{
Core::Core(bool skip, std::string bootrom_path) : bus(skip, bootrom_path), cpu(skip, &bus) {}

void Core::Run() {
  while(cpu.total_cycles < 70224) {
    cpu.Step();
    bus.ppu.Step(cpu.cycles, bus.mem.io.intf);
    bus.apu.Step(cpu.cycles);
    bus.mem.DoInputs(key);
    cpu.HandleTimers();
  }

  cpu.total_cycles -= 70224;
}

void Core::LoadROM(std::string path) {
  cpu.Reset();
  bus.Reset();
  bus.LoadROM(path);
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

void Core::RunAsync() {
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