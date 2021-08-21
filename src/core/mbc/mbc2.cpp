#include "mem.h"

namespace natsukashii::core
{
MBC2::MBC2(std::vector<u8>& rom, const std::string& savefile) : rom(rom)
{
  std::ifstream file{savefile, std::ios::binary};
  file.unsetf(std::ios::skipws);

  if (!file.is_open())
  {
    ram.fill(0);
  }
  else
  {
    file.read((char*)ram.data(), EXTRAM_SZ);
    file.close();
  }
}

u8 MBC2::Read(u16 addr)
{
  switch (addr)
  {
  case 0 ... 0x3fff:
    return rom[addr];
  case 0x4000 ... 0x7fff:
    return rom[(0x4000 * romBank + (addr - 0x4000)) % rom.size()];
  case 0xa000 ... 0xbfff:
    return ramEnable ? (0xf0 | (ram[addr & 0x1ff] & 0xf)) : 0xff;
  }
}

void MBC2::Write(u16 addr, u8 val)
{
  switch(addr)
  {
  case 0 ... 0x3fff:
    if(bit<u16, 8>(addr))
    {
      romBank = val;
      romBank = (romBank == 0) ? 1 : romBank;
    }
    else
    {
      ramEnable = (val & 0xf) == 0xa;
    }
    break;
  case 0xa000 ... 0xbfff:
    if(ramEnable)
    {
      ram[addr & 0x1ff] = val & 0xf;
    }
    break;
  }
}

void MBC2::Save(const std::string& filename)
{
  FILE* file = fopen(filename.c_str(), "wb");
  fwrite(ram.data(), 1, EXTRAM_SZ, file);
  fclose(file);
}
} // natsukashii::core