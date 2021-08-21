#include "mem.h"

namespace natsukashii::core
{
MBC1::MBC1(std::vector<u8>& rom, const std::string& savefile) : rom(rom)
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

  romSize = rom[0x148];
  ramSize = rom[0x149];
}

u8 MBC1::Read(u16 addr)
{
  u8 zeroBank = 0;
  u8 highBank = 0;
  switch (addr)
  {
  case 0 ... 0x3fff:
    if (mode)
    {
      switch (romSize)
      {
      case 0 ... 4:
        zeroBank = 0;
        break;
      case 5:
        setbit<u8, 5>(zeroBank, ramBank & 1);
        break;
      case 6:
        setbit<u8, 5>(zeroBank, ramBank & 1);
        setbit<u8, 6>(zeroBank, ramBank >> 1);
        break;
      }
      return rom[(0x4000 * zeroBank + addr) % rom.size()];
    }
    else
    {
      return rom[addr];
    }
    break;
  case 0x4000 ... 0x7fff:
    highBank = romBank & bitmasks[romSize];
    switch (romSize)
    {
    case 5:
      setbit<u8, 5>(highBank, ramBank & 1);
      break;
    case 6:
      setbit<u8, 5>(highBank, ramBank & 1);
      setbit<u8, 6>(highBank, ramBank >> 1);
      break;
    }
    return rom[(0x4000 * highBank + (addr - 0x4000)) % rom.size()];
  case 0xa000 ... 0xbfff:
    if (ramEnable)
    {
      if (ramSize == 0x01 || ramSize == 0x02)
      {
        return ram[(addr - 0xa000) % RAM_SIZES[ramSize]];
      }
      else if (ramSize == 0x03)
      {
        if (mode)
        {
        	return ram[(0x2000 * ramBank + (addr - 0xa000)) % EXTRAM_SZ];
        }
        else
        {
          return ram[(addr - 0xa000) % EXTRAM_SZ];
        }
      }
    }
    else
    {
      return 0xff;
    }
    break;
  }
}

void MBC1::Write(u16 addr, u8 val) 
{
  switch (addr)
  {
  case 0 ... 0x1fff:
    ramEnable = ((val & 0xf) == 0xa);
    break;
  case 0x2000 ... 0x3fff:
    romBank = val & bitmasks[romSize];
    romBank = (romBank == 0) ? 1 : romBank;
    break;
  case 0x4000 ... 0x5fff:
    ramBank = val & 3;
    break;
  case 0x6000 ... 0x7fff:
    mode = val & 1;
    break;
  case 0xa000 ... 0xbfff:
    if (ramEnable)
    {
      if (ramSize == 0x01 || ramSize == 0x02)
      {
        ram[(addr - 0xa000) % RAM_SIZES[ramSize]] = val;
      }
      else if (ramSize == 0x03)
      {
        if (mode)
        {
          ram[(0x2000 * ramBank + (addr - 0xa000)) % EXTRAM_SZ] = val;
        }
        else
        {
          ram[(addr - 0xa000) % EXTRAM_SZ] = val;
        }
      }
    }
    break;
  }
}

void MBC1::Save(const std::string& filename)
{
  FILE* file = fopen(filename.c_str(), "wb");
  fwrite(ram.data(), 1, EXTRAM_SZ, file);
  fclose(file);
}
} // natsukashii::core