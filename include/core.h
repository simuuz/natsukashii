#pragma once
#include "cpu.h"
#include "bus.h"
#include <condition_variable>
#include <mutex>
#include <atomic>

namespace natsukashii::core
{
struct Core
{
  Core(bool skip, std::string bootrom_path);
  void Run();
  void Reset();
  void Pause();
  void Stop();
  void LoadROM(std::string path);
  void SaveState(int slot);
  void LoadState(int slot);
  void RunAsync();
  void WaitPing();
  std::condition_variable emu_condition_variable;
  std::mutex emu_mutex;
  std::atomic <bool> run_emu_thread = false;
  Bus bus;
  Cpu cpu;
  int key;
  bool pause = false;
  bool init = false;
};
}  // namespace natsukashii::core