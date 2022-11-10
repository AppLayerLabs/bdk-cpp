#include "merkle.h"


MerkleRoot::MerkleRoot(const std::vector<Hash> &leafs) {
  for (const auto &leaf : leafs) {
    _leafs.emplace_back(std::move(Utils::sha3(leaf.get())));
  }

  _layers.emplace_back(_leafs);
  while (_layers.back().size() > 1) {
    _layers.emplace_back(hashLayer(_layers.back()));
  }
};

std::vector<Hash> MerkleRoot::hashLayer(const std::vector<Hash> &layer) {

  std::vector<Hash> newLayer;
  for (uint64_t i = 0; i < layer.size(); i += 2) {
    if (i + 1 < layer.size()) {
      newLayer.emplace_back(Utils::sha3(layer[i].get() + layer[i + 1].get()));
    } else {
      newLayer.emplace_back(layer[i]);
    }
  }
  return newLayer;
};