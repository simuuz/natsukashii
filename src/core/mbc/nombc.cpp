#include "mem.h"

namespace natsukashii::core
{
NoMBC::NoMBC(std::vector<u8>& rom) : rom(rom) {}

u8 NoMBC::Read(u16 addr)
{
  if(addr >= 0 && addr < 0x8000)
      return rom[addr];
  return 0xff;
}

void NoMBC::Write(u16 addr, u8 val) {}
} // natsukashii::core