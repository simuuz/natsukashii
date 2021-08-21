#include <bus.h>

#include <utility>

namespace natsukashii::core
{
Bus::Bus(bool skip, std::string bootrom_path) : mem(skip, std::move(bootrom_path)), ppu(skip), apu(skip) {}

void Bus::LoadROM(std::string path) {
  this->mem.LoadROM(std::move(path));
  romopened = mem.rom_opened;
}

void Bus::Reset() {
  ppu.Reset();
  mem.Reset();
  apu.Reset();
}

u8 Bus::ReadByte(u16 addr) {
  switch(addr) {
  case 0x8000 ... 0x9fff:
    return ppu.vram_lock ? 0xff : ppu.vram[addr & 0x1fff];
  case 0xfe00 ... 0xfe9f:
    return ppu.oam_lock ? 0xff : ppu.oam[addr & 0xff];
  case 0xff40 ... 0xff4b:
    return ppu.ReadIO(addr);
  case 0xff10 ... 0xff3f:
    return apu.ReadIO(addr);
  default:
    return mem.Read(addr);
  }
}

u8 Bus::NextByte(u16& pc, u8& cycles) {
  u8 val = ReadByte(pc);
  cycles += 4;
  pc++;
  return val;
}

u16 Bus::ReadHalf(u16 addr) {
  return (ReadByte(addr + 1) << 8) | ReadByte(addr);
}

u16 Bus::NextHalf(u16& pc, u8& cycles) {
  u8 low = ReadByte(pc);
  u8 high = ReadByte(pc + 1);
  cycles += 8;
  pc += 2;

  return (high << 8) | low;
}

void Bus::WriteByte(u16 addr, u8 val) {
  switch(addr) {
  case 0x8000 ... 0x9fff:
    if(!ppu.vram_lock) ppu.vram[addr & 0x1fff] = val;
    break;
  case 0xfe00 ... 0xfe9f:
    if(!ppu.oam_lock) ppu.oam[addr & 0xff] = val;
    break;
  case 0xff40 ... 0xff4b:
    ppu.WriteIO(mem, addr, val, mem.io.intf);
    break;
  case 0xff10 ... 0xff3f:
    apu.WriteIO(addr, val);
    break;
  default:
    mem.Write(addr, val);
  }
}

void Bus::WriteHalf(u16 addr, u16 val) {
  WriteByte(addr + 1, val >> 8);
  WriteByte(addr, val);
}

void Bus::SaveState(std::ofstream& savestate) {
  mem.SaveState(savestate);
  ppu.SaveState(savestate);
}

void Bus::LoadState(std::ifstream& loadstate) {
  mem.LoadState(loadstate);
  ppu.LoadState(loadstate);
}
}  // namespace natsukashii::core