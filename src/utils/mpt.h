#ifndef MPT_H
#define MPT_H

#include <algorithm>
#include <cmath>
#include <vector>

#include "utils.h"

/**
 * Custom implementation of a Merkle Patricia Tree. Adapted from:
 * https://medium.com/coinmonks/implementing-merkle-tree-and-patricia-tree-b8badd6d9591
 */

class MPT {
  private:
    std::vector<std::string> merkle;

  public:
    MPT(std::vector<std::string> list);
    std::vector<std::string> get() { return this->merkle; }
    std::string root() { return this->merkle[0]; }
    bool verify(std::string data);
};

#endif  // MPT_H
