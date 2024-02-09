#include "rand.hh"
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <iostream>

namespace rd_utils::utils::rand {

  bool __INIT__ = false;

  void setSeed (uint64_t seed) {
    __INIT__ = true;
    srand (seed);
  }

  uint64_t uniform (uint64_t low, uint64_t high) {
    if (!__INIT__) {
      srand (time (NULL));
      __INIT__ = true;
    }

    double x = (double)(::rand ()) / (double) (2147483648);

    uint64_t range = high - low + 1;
    uint64_t scaled = (x * range) + low;

    return scaled;
  }

  double uniformf (double low, double high) {
    if (!__INIT__) {
      srand (time (NULL));
      __INIT__ = true;
    }

    double x = (double)(::rand ()) / (double) (2147483648);

    double range = high - low;
    double scaled = (x * range) + low;

    return scaled;
  }

}
