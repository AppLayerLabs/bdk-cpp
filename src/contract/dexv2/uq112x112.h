#ifndef UQ112X112_H
#define UQ112X112_H

#include "../variables/safeuint224_t.h"


namespace UQ112x112 {
  static const uint224_t Q112("5192296858534827628530496329220096");

  static uint224_t encode(const uint112_t& x) {
    return (uint224_t(x) * Q112);
  }

  static uint224_t uqdiv(const uint224_t& x, const uint112_t& y) {
    return (x / uint224_t(y));
  }
};

#endif // UQ112X112_H