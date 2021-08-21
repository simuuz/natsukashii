#include "ppu.h"
#include "ini.h"
#include <algorithm>

namespace natsukashii::core
{
Ppu::Ppu(bool skip) : skip(skip)
{
  mINI::INIFile file{"config.ini"};
  mINI::INIStructure ini;

  io.scx = 0;
  io.scy = 0;
  io.lyc = 0;
  io.wx = 0;
  io.wy = 0;
  io.ly = 0;

  if (skip)
  {
    io.lcdc.raw = 0x91;
    io.bgp = 0xfc;
    io.obp0 = 0xff;
    io.obp1 = 0xff;
  }
  else
  {
    io.lcdc.raw = 0;
    io.bgp = 0;
    io.obp0 = 0;
    io.obp1 = 0;
  }
}

void Ppu::SaveState(std::ofstream& savestate) {
  savestate.write((char*)vram, VRAM_SZ);
  savestate.write((char*)oam, OAM_SZ);
}

void Ppu::LoadState(std::ifstream& loadstate) {
  loadstate.read((char*)vram, VRAM_SZ);
  loadstate.read((char*)oam, OAM_SZ);
}

void Ppu::Reset()
{
  fbIndex = 0;
  mode = OAM;

  memset(pixels, colors[3], FBSIZE);
  memset(vram, 0, VRAM_SZ);
  memset(oam, 0, OAM_SZ);

  io.scx = 0;
  io.scy = 0;
  io.lyc = 0;
  io.wx = 0;
  io.wy = 0;
  io.ly = 0;

  if (skip)
  {
    io.lcdc.raw = 0x91;
    io.bgp = 0xfc;
    io.obp0 = 0xff;
    io.obp1 = 0xff;
  }
  else
  {
    io.lcdc.raw = 0;
    io.bgp = 0;
    io.obp0 = 0;
    io.obp1 = 0;
  }
}

void Ppu::CompareLYC(u8 &intf)
{
  io.stat.lyceq = io.lyc == io.ly;
  if (io.lyc == io.ly && io.stat.lyceq_int)
  {
    intf |= 2;
  }
}

void Ppu::DispatchEvents(u64 time, Scheduler& scheduler, u8 &intf)
{
  if (!io.lcdc.enabled)
  {
    return;
  }

  switch (mode)
  {
  case HBlank:
    io.ly++;
    CompareLYC(intf);

    if (io.ly == 0x90)
    {
      ChangeMode(time, scheduler, VBlank, intf);
    }
    else
    {
      ChangeMode(time, scheduler, OAM, intf);
    }
    break;
  case VBlank:
    io.ly++;
    CompareLYC(intf);
    if (io.ly == 154) {
      io.ly = 0;
      window_internal_counter = 0;
      ChangeMode(time, scheduler, OAM, intf);
    } else {
      scheduler.push(Entry(time +  456, Event::PPU));
    }
    break;
  case OAM:
    ChangeMode(time, scheduler, LCDTransfer, intf);
    break;
  case LCDTransfer:
    ChangeMode(time, scheduler, HBlank, intf);
    break;
  }
}

void Ppu::ChangeMode(u64 time, Scheduler& scheduler, Mode m, u8 &intf)
{
  mode = m;
  switch (m)
  {
  case HBlank:
    scheduler.push(Entry(time + 204, Event::PPU));
    if (io.stat.hblank_int)
    {
      intf |= 2;
    }
    break;
  case VBlank:
    scheduler.push(Entry(time +  456, Event::PPU));
    intf |= 1;
    if (io.stat.vblank_int)
    {
      intf |= 2;
    }
    break;
  case OAM:
    scheduler.push(Entry(time +  80, Event::PPU));
    if (io.stat.oam_int)
    {
      intf |= 2;
    }
    break;
  case LCDTransfer:
    scheduler.push(Entry(time + 172, Event::PPU));
    render = true;
    Scanline();
    break;
  }
}

u8 Ppu::ReadIO(u16 addr)
{
  switch (addr & 0xff)
  {
  case 0x40:
    return io.lcdc.raw;
  case 0x41:
    return io.stat.read();
  case 0x42:
    return io.scy;
  case 0x43:
    return io.scx;
  case 0x44:
    return io.ly;
  case 0x45:
    return io.lyc;
  case 0x47:
    return io.bgp;
  case 0x48:
    return io.obp0;
  case 0x49:
    return io.obp1;
  case 0x4a:
    return io.wy;
  case 0x4b:
    return io.wx;
  default:
    return 0xff;
  }
}

void Ppu::WriteIO(Mem &mem, u16 addr, u8 val, u8 &intf)
{
  switch (addr & 0xff)
  {
  case 0x40:
    io.old_lcdc.raw = io.lcdc.raw;
    io.lcdc.raw = val;
    if (!io.old_lcdc.enabled && io.lcdc.enabled)
    {
      intf |= 2;
    }
    break;
  case 0x41:
    io.stat.write(val);
    break;
  case 0x42:
    io.scy = val;
    break;
  case 0x43:
    io.scx = val;
    break;
  case 0x45:
    io.lyc = val;
    break;
  case 0x46: // OAM DMA very simple implementation
  {
    u16 start = (u16)val << 8;
    for (u8 i = 0; i < 0xa0; i++)
    {
      oam[i] = mem.Read(start | i);
    }
  }
  break;
  case 0x47:
    io.bgp = val;
    break;
  case 0x48:
    io.obp0 = val;
    break;
  case 0x49:
    io.obp1 = val;
    break;
  case 0x4a:
    io.wy = val;
    break;
  case 0x4b:
    io.wx = val;
    break;
  default:
    break;
  }
}

void Ppu::Scanline()
{
  RenderBGs();
  RenderSprites();
}

void Ppu::RenderBGs()
{
  fbIndex = io.ly * WIDTH;
  u16 bg_tilemap = io.lcdc.bg_tilemap_area == 1 ? 0x9C00 : 0x9800;
  u16 window_tilemap = io.lcdc.window_tilemap_area == 1 ? 0x9C00 : 0x9800;
  u16 tiledata = io.lcdc.bgwin_tiledata_area == 1 ? 0x8000 : 0x8800;

  bool render_window = (io.wy <= io.ly && io.lcdc.window_enable);

  for (int x = 0; x < WIDTH; x++)
  {
    u16 tileline = 0;
    u8 scrolled_x = io.scx + x;
    u8 scrolled_y = io.scy + io.ly;

    if (io.lcdc.bgwin_priority)
    {
      u8 index = 0;
      u16 real_wx = (u16)io.wx - 7;
      if (render_window && real_wx <= x)
      {
        scrolled_x = x - real_wx;
        scrolled_y = window_internal_counter;

        index = vram[(window_tilemap + (((scrolled_y >> 3) << 5) & 0x3FF) + ((scrolled_x >> 3) & 31)) & 0x1fff];
      }
      else
      {
        index = vram[(bg_tilemap + (((scrolled_y >> 3) << 5) & 0x3FF) + ((scrolled_x >> 3) & 31)) & 0x1fff];
      }

      if (tiledata == 0x8000)
      {
        tileline = *(u16*)&vram[tiledata + ((u16)index << 4) + ((u16)(scrolled_y & 7) << 1)];
      }
      else
      {
        tileline = *(u16*)&vram[0x9000 + s16((s8)index) * 16 + ((u16)(scrolled_y & 7) << 1)];
      }
    }

    u8 high = (tileline >> 8);
    u8 low = (tileline & 0xff);

    u8 colorID = (bit<u8>(high, 7 - (scrolled_x & 7)) << 1) | (bit<u8>(low, 7 - (scrolled_x & 7)));
    u8 color_index = (io.bgp >> (colorID << 1)) & 3;
    colorIDbg[fbIndex] = colorID;
    pixels[fbIndex] = colors[color_index];
    fbIndex++;
  }

  if (render_window && io.ly >= io.wy && io.wx >= 0 && io.wx <= 168)
  {
    window_internal_counter++;
  }
}

void Ppu::FetchSprites()
{
  sprites.fill(Sprite());
  count = 0;
  u8 height = io.lcdc.obj_size ? 16 : 8;

  for (int i = 0; i < 0xa0 && count < 10; i += 4)
  {
    Sprite sprite(oam[i] - 16, oam[i + 1] - 8, oam[i + 2], oam[i + 3]);
    if (io.ly >= (s16)sprite.ypos && io.ly < ((s16)sprite.ypos + height))
    {
      sprites[count] = sprite;
      count++;
    }
  }

  std::stable_sort(sprites.begin(), sprites.begin() + count, [](Sprite a, Sprite b) {
    return b.xpos <= a.xpos;
  });
}

void Ppu::RenderSprites()
{
  if (!io.lcdc.obj_enable)
    return;

  FetchSprites();

  for (int i = 0; i < count; i++)
  {
    u16 tile_y;
    if (io.lcdc.obj_size)
    {
      tile_y = (sprites[i].attribs.yflip) ? ((io.ly - sprites[i].ypos) ^ 15) : io.ly - sprites[i].ypos;
    }
    else
    {
      tile_y = (sprites[i].attribs.yflip) ? ((io.ly - sprites[i].ypos) ^ 7) & 7 : (io.ly - sprites[i].ypos) & 7;
    }

    u8 pal = (sprites[i].attribs.palnum) ? io.obp1 : io.obp0;
    fbIndex = sprites[i].xpos + WIDTH * io.ly;
    u16 tile_index = io.lcdc.obj_size ? sprites[i].tileidx & ~1 : sprites[i].tileidx;
    u16 tile = *(u16*)&vram[0x8000 | ((tile_index << 4) + (tile_y << 1))];

    for (int x = 0; x < 8; x++)
    {
      s8 tile_x = (sprites[i].attribs.xflip) ? 7 - x : x;
      u8 high = tile >> 8;
      u8 low = tile & 0xff;
      u8 colorID = (bit<u8>(high, 7 - tile_x) << 1) | bit<u8>(low, 7 - tile_x);
      u8 colorIndex = (pal >> (colorID << 1)) & 3;

      if ((sprites[i].xpos + x) < 166 && colorID != 0 && pixels[fbIndex] != colors[colorIndex])
      {
        if (sprites[i].attribs.obj_to_bg_prio)
        {
          if (colorIDbg[fbIndex] < 1)
          {
            pixels[fbIndex] = colors[colorIndex];
          }
        }
        else
        {
          pixels[fbIndex] = colors[colorIndex];
        }
      }

      fbIndex++;
    }
  }
}
} // namespace natsukashii::core