#pragma once
#include <mem.h>
#include <scheduler.h>

constexpr int VRAM_SZ = 0x2000;
constexpr int OAM_SZ = 0xa0;
constexpr int WIDTH = 160;
constexpr int HEIGHT = 144;
constexpr int FBSIZE = WIDTH * HEIGHT;
constexpr u32 colors[4] = { 0xFED018FF, 0xD35600FF, 0x5E1210FF, 0x0D0405FF };

namespace natsukashii::core
{
struct Sprite
{
private:
  struct Attributes
  {
    union
    {
      struct
      {
        unsigned:4;
        unsigned palnum:1;
        unsigned xflip:1;
        unsigned yflip:1;
        unsigned obj_to_bg_prio:1;
      };

      u8 raw;
    };

    explicit Attributes(u8 val) : raw(val) { }
  };
public:
  explicit Sprite(u8 ypos = 0, u8 xpos = 0, u8 tileidx = 0, u8 attribs = 0)
        : ypos(ypos), xpos(xpos), tileidx(tileidx), attribs(attribs) { }
  u8 ypos, xpos, tileidx;
  Attributes attribs;
};

class Ppu
{
public:
  explicit Ppu(bool skip);
  void Reset();
  void SaveState(std::ofstream& savestate);
  void LoadState(std::ifstream& loadstate);

  u32 pixels[FBSIZE]{0x0D0405FF};
  u8 vram[VRAM_SZ]{0};
  u8 oam[OAM_SZ]{0};

  friend class Bus;
  bool render = false;

  void DispatchEvents(u64 time, Scheduler& scheduler, u8& intf);

private:
  bool oam_lock = false;
  bool vram_lock = false;
  bool latch = false;
  bool reset = false;
  bool skip = false;

  enum Mode
  {
    HBlank,
    VBlank,
    OAM,
    LCDTransfer
  };
  
  Mode mode = OAM;
  
  struct LCDC
  {
    union
    {
      struct
      {
        unsigned bgwin_priority:1;
        unsigned obj_enable:1;
        unsigned obj_size:1;
        unsigned bg_tilemap_area:1;
        unsigned bgwin_tiledata_area:1;
        unsigned window_enable:1;
        unsigned window_tilemap_area:1;
        unsigned enabled:1;
      };
      
      u8 raw;
    };

    LCDC() : raw(0) {}
  };

  struct STAT
  {
    union
    {
      struct
      {
        unsigned mode:2;
        unsigned lyceq:1;
        unsigned hblank_int:1;
        unsigned vblank_int:1;
        unsigned oam_int:1;
        unsigned lyceq_int:1;
        unsigned:1;
      };

      u8 raw;
    };

    STAT() : raw(0) {}

    constexpr void write(u8 value)
    {
      raw = value;
      raw |= 0x80;
    }

    constexpr u8 read()
    {
      return (1 << 7) | (raw & 0x7f);
    }
  };
  
  struct IO
  {
    u8 bgp = 0, scy = 0, scx = 0;
    LCDC lcdc, old_lcdc;
    u8 wx = 0, wy = 0, obp0 = 0, obp1 = 0;
    u8 lyc = 0, ly = 0;
    STAT stat;
  } io;

  u8 window_internal_counter = 0;
  u32 fbIndex = 0;

  u8 colorIDbg[FBSIZE]{0};

  u64 curr_cycles = 0;
  
  std::array<Sprite, 10> sprites;
  u8 count = 0;

  void WriteIO(Mem& mem, u16 addr, u8 val, u8& intf);
  u8 ReadIO(u16 addr);

  void ChangeMode(u64 time, Scheduler& scheduler, Mode m, u8& intf);
  void FetchSprites();
  void RenderSprites();
  void RenderBGs();
  void Scanline();
  void CompareLYC(u8& intf);
};
}  // namespace natsukashii::core