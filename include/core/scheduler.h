#pragma once

#include "common.h"

#define ENTRIES_MAX 32

namespace natsukashii::core {
enum class Event {
  None,
  APU,
  PPU,
  Timers,
  Panic,
};

struct Entry {
  Event event;
  u64 time;
  Entry() : time(0), event(Event::None) {}
  Entry(u64 time, Event event) : time(time), event(event) {}
};

struct Scheduler {
  std::array<Entry, ENTRIES_MAX> entries;
  int pos;

  Scheduler();

  void push(Entry entry);
  void pop(int count);
};
} // natsukashii::core