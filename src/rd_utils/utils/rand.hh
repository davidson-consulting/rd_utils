#pragma once

#include <stdint.h>

namespace rd_utils::utils::rand {

  void setSeed (uint64_t);

  uint64_t uniform (uint64_t low, uint64_t high);

  double uniformf (double low, double high);


}
