#ifndef MERKLE_H
#define MERKLE_H

#include "utils.h"

/**
 * Custom implementation of a Merkle Patricia Tree. Adapted from:
 * https://medium.com/coinmonks/implementing-merkle-tree-and-patricia-tree-b8badd6d9591
 * https://lab.miguelmota.com/merkletreejs/example/
 */

class Merkle {
  private:
    Hash _root;
    std::vector<std::vector<Hash>> _layers; // Top down (leafs -> root)
    std::vector<Hash> _leafs;

    std::vector<Hash> newLayer(const std::vector<Hash>& layer);

  public:
    Merkle(const std::vector<Hash>& leafs);

    const Hash& root() const { return this->_root; }
    const std::vector<std::vector<Hash>>& layers() const { return this->_layers; }
    const std::vector<Hash>& leafs() const { return this->_leafs; }

    const std::vector<Hash> getProof(const uint64_t& leafIndex) const;
};

#endif  // MERKLE_H
