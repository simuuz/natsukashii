#include "scheduler.h"
#include <string.h>

namespace natsukashii::core {
Scheduler::Scheduler() {
  entries[0].time = UINT64_MAX;
  entries[0].event = Event::Panic;
  pos = 1;
}

void Scheduler::push(Entry entry) {
  if(pos < ENTRIES_MAX) {
    for(int i = 0; i < pos; i++) {
      if(entries[i].time > entry.time) {
        memmove(&entries.data()[i + 1], &entries.data()[i], (pos - i) * sizeof(Entry));
        entries[i] = entry;
        break;
      }
    }

    pos++;
  }
}

void Scheduler::pop(int count) {
  memmove(&entries.data()[0], &entries.data()[count], sizeof(Entry) * (pos - count));
  pos -= count;
}
} // natsukashii::core