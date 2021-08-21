#pragma once
#include <array>
#include <iterator>
#include <vector>
#include <map>
#include "common.h"

constexpr int BOOTROM_SZ = 0x100;
constexpr int EXTRAM_SZ = 0x2000;
constexpr int WRAM_SZ = 0x2000;
constexpr int HRAM_SZ = 0x7f;
constexpr int ROM_SZ_MIN = 0x8000;

namespace natsukashii::core
{
using namespace natsukashii::util;
class Cart
{
public:
  virtual u8 Read(u16 addr) { return 0xff; }
  virtual void Write(u16 addr, u8 val) { }
  virtual void Clear() { }
  virtual void Save(const std::string& filename) { }
  virtual u8* GetROM() { }
  virtual u8* GetRAM() { }
  virtual void SetRam(std::ifstream& rhs) { }
};

class NoMBC : public Cart
{
public:
  explicit NoMBC(std::vector<u8>& rom);
  u8 Read(u16 addr) override;
  void Write(u16 addr, u8 val) override;
  void Save(const std::string& filename) override {}
  u8* GetROM() override { return rom.data(); }
  u8* GetRAM() override { u8 ram[EXTRAM_SZ]{0xff}; return ram; }
  void SetRam(std::ifstream& rhs) override { }
private:
  std::vector<u8> rom;
};

class MBC1 : public Cart
{
public:
  MBC1(std::vector<u8>& rom, const std::string& savefile);
  u8 Read(u16 addr) override;
  void Write(u16 addr, u8 val) override;
  void Save(const std::string& filename) override;
  u8* GetROM() override { return rom.data(); }
  u8* GetRAM() override { return ram.data(); }
  void SetRam(std::ifstream& rhs) override {
    rhs.read((char*)ram.data(), EXTRAM_SZ);
  }
private:
  u8 romBank = 1;
  u8 ramBank = 1;
  u8 romSize;
  u8 ramSize;
  bool mode = false;
  bool ramEnable = false;
  static constexpr u16 bitmasks[7] = {0x1, 0x3, 0x7, 0xf, 0x1f, 0x1f, 0x1f};
  static constexpr u32 RAM_SIZES[6] = {0, 2 * 1024, 8 * 1024, 32 * 1024, 128 * 1024, 64 * 1024};
  
  std::array<u8, EXTRAM_SZ> ram;
  std::vector<u8> rom;
};

class MBC2 : public Cart
{
public:
  MBC2(std::vector<u8>& rom, const std::string& savefile);
  u8 Read(u16 addr) override;
  void Write(u16 addr, u8 val) override;
  void Save(const std::string& filename) override;
  u8* GetROM() override { return rom.data(); }
  u8* GetRAM() override { return ram.data(); }
  void SetRam(std::ifstream& rhs) override {
    rhs.read((char*)ram.data(), EXTRAM_SZ);
  }
private:
  u8 romBank = 1;
  bool ramEnable = false;

  std::array<u8, EXTRAM_SZ> ram;
  std::vector<u8> rom;
};

class MBC3 : public Cart
{
public:
  MBC3(std::vector<u8>& rom, const std::string& savefile);
  u8 Read(u16 addr) override;
  void Write(u16 addr, u8 val) override;
  void Save(const std::string& filename) override;
  u8* GetROM() override { return rom.data(); }
  u8* GetRAM() override { return ram.data(); }
  void SetRam(std::ifstream& rhs) override {
    rhs.read((char*)ram.data(), EXTRAM_SZ);
  }
private:
  u8 ramBank = 0;
  u8 romBank = 0;

  std::array<u8, EXTRAM_SZ> ram;
  std::vector<u8> rom;
  bool ramEnable = false;
};

class MBC5 : public Cart
{
public:
  MBC5(std::vector<u8>& rom, const std::string& savefile);
  u8 Read(u16 addr) override;
  void Write(u16 addr, u8 val) override;
  void Save(const std::string& filename) override;
  u8* GetROM() override { return rom.data(); }
  u8* GetRAM() override { return ram.data(); }
  void SetRam(std::ifstream& rhs) override {
    rhs.read((char*)ram.data(), EXTRAM_SZ);
  }
private:
  u16 romBank = 1;
  u8 ramBank = 1;
  std::array<u8, EXTRAM_SZ> ram;
  std::vector<u8> rom;
  bool ramEnable = false;
};

class Mem
{
public:
  ~Mem();
  Mem(bool skip, std::string bootrom_path);
  void LoadROM(std::string filename);
  void SaveState(std::ofstream& savestate);
  void LoadState(std::ifstream& loadstate);
  void Reset();
  u8 Read(u16 addr);
  void Write(u16 addr, u8 val);

  u8 ie = 0;
  bool skip;

  struct Joypad
  {
    union
    {
      struct
      {
        unsigned:1;
        unsigned:1;
        unsigned select_btn:1;
        unsigned select_dpad:1;
        unsigned btn_start_down:1;
        unsigned btn_select_up:1;
        unsigned btn_a_left:1;
        unsigned btn_b_right:1;
      };

      u8 raw;
    };

    Joypad() : raw(0xff) {}
    void write(u8 val) {
      raw = val;
      raw |= (0b11000000);
    }
  };

  struct IO
  {
    u8 bootrom = 1, tac = 0, tima = 0, tma = 0, intf = 0, div = 0;
    u8 nr41, nr42, nr43, nr44, nr50, nr51, nr52;
    Joypad joy;
  } io;

  friend class Ppu;
  friend class Bus;
  bool rom_opened = false;
  void DoInputs(int key);
  std::string savefile;
private:
  bool held = false;
  Cart* cart = nullptr;
  void LoadBootROM(std::string filename);

  void WriteIO(u16 addr, u8 val);
  u8 ReadIO(u16 addr);

  u8 bootrom[BOOTROM_SZ];
  u8 wram[WRAM_SZ];
  u8 hram[HRAM_SZ];
  std::vector<u8> rom;

  bool dpad = false;
  bool button = false;

  void HandleJoypad(u8 val);
};
}  // namespace natsukashii::core