#ifndef COUNTERS_H
#define COUNTERS_H

#include "safeint.h"

/// Counters for contracts, roughly based on OpenZeppelin implementation.
class Counter {
  private:
    SafeInt_t<64> value_;

  public:
    Counter(DynamicContract* contract) : value_(contract) {
        value_ = 0;
    }

    uint64_t current() const {
        return value_.get();
    }

    void increment() {
        value_ += 1;
    }

    void decrement() {
        value_ -= 1;
    }

    void reset() {
        value_ = 0;
    }
};

#endif // COUNTERS_H