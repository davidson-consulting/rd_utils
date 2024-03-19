#include "bitonicsort.hh"


namespace rd_utils::memory::cache::algorithm {
  uint32_t flp2 (uint32_t x) {
    x = x | (x >> 1);
    x = x | (x >> 2);
    x = x | (x >> 4);
    x = x | (x >> 8);
    x = x | (x >> 16);
    return x - (x >> 1);
  }

}
