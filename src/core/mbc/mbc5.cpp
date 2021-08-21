#include "mem.h"

namespace natsukashii::core
{
MBC5::MBC5(std::vector<u8>& rom, const std::string& savefile) : rom(rom.begin(), rom.end())
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

u8 MBC5::Read(u16 addr)
{
  switch (addr)
  {
  case 0 ... 0x3fff:
    return rom[addr];
  case 0x4000 ... 0x7fff:
    return rom[(0x4000 * (romBank & 0x1ff) + (addr - 0x4000)) % rom.size()];
  case 0xa000 ... 0xbfff:
    return ramEnable ? ram[(0x2000 * ramBank + (addr - 0xa000)) % EXTRAM_SZ] : 0xff;
  }
}

void MBC5::Write(u16 addr, u8 val)
{
  switch (addr)
  {
  case 0 ... 0x1fff:
    ramEnable = (val & 0xf) == 0xa;
    break;
  case 0x2000 ... 0x2fff:
    romBank = val & 0xff;
    break;
  case 0x3000 ... 0x3fff:
    setbit<u16, 8>(romBank, val & 1);
    break;
  case 0x4000 ... 0x5fff:
    ramBank = val;
    break;
  case 0xa000 ... 0xbfff:
    if(ramEnable)
    {
      ram[(0x2000 * ramBank + (addr - 0xa000)) % EXTRAM_SZ] = val;
    }
    break;
  default:
    break;
  }
}

void MBC5::Save(const std::string& filename)
{
  FILE* file = fopen(filename.c_str(), "wb");
  fwrite(ram.data(), 1, EXTRAM_SZ, file);
  fclose(file);
}
} // natsukashii::core