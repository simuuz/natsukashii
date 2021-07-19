#pragma once
#include "core.h"
#include <nfd.hpp>
#include <thread>

namespace natsukashii::frontend
{
using namespace natsukashii::core;

constexpr float aspect_ratio_gb = (float)WIDTH / (float)HEIGHT;
struct MainWindow
{
  MainWindow(std::string title);
  ~MainWindow();
  void Run();
  void OpenFile();
  void UpdateTexture();
  void MenuBar();

  std::thread emu_thread;
  void PingEmuThread();
  void WaitEmuThread();

  bool running = true;
  mINI::INIFile file;
  mINI::INIStructure ini;
  GLFWwindow* window = nullptr;
  std::unique_ptr<Core> core;
  int key;
  unsigned int id;
};
} // natsukashii::frontend