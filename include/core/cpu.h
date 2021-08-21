#pragma once
#include <bus.h>
#include <scheduler.h>

namespace natsukashii::core
{
class Cpu
{
public:
  Cpu(bool skip, Bus* bus);
  u8 Step();
  void Reset();
  void SaveState(int slot);
  void LoadState(int slot);
  Bus* bus;
  bool halt = false;
  void DispatchTimers(u64 time, Scheduler& scheduler);
  void HandleInterrupts(u64& cycles);
  bool skip;
  u8 opcode;
  struct registers
  {
    union
    {
      struct
      {
        u8 f, a;
      };

      u16 af = 0;
    };

    union
    {
      struct
      {
        u8 c, b;
      };

      u16 bc = 0;
    };

    union
    {
      struct
      {
        u8 e, d;
      };

      u16 de = 0;
    };

    union
    {
      struct
      {
        u8 l, h;
      };

      u16 hl = 0;
    };

    u16 sp = 0, pc = 0;
  } regs;

private:
  void UpdateF(bool z, bool n, bool h, bool c);
  bool Cond(u8 opcode);

  template <int group>
  u16 ReadR16(u8 bits);
  template <int group>
  void WriteR16(u8 bits, u16 val);

  u8 ReadR8(u8 bits);
  void WriteR8(u8 bits, u8 value);

  u8 Execute(u8 opcode);
  void Push(u16 val);
  u16 Pop();
  FILE* log;
  int tima_cycles = 0;
  int div_cycles = 0;

  bool ime = false;
  bool ei = false;
};
}  // namespace natsukashii::core