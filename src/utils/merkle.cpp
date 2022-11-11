#include "merkle.h"

Merkle::Merkle(const std::vector<Hash>& leafs) {
  for (const auto &leaf : leafs) {
    this->_leafs.emplace_back(std::move(Utils::sha3(leaf.get())));
  }
  this->_layers.emplace_back(this->_leafs);
  while (this->_layers.back().size() > 1) {
    this->_layers.emplace_back(newLayer(this->_layers.back()));
  }
};

std::vector<Hash> Merkle::newLayer(const std::vector<Hash>& layer) {
  std::vector<Hash> ret;
  for (uint64_t i = 0; i < layer.size(); i += 2) ret.emplace_back(
    ((i+1 < layer.size()) ? Utils::sha3(layer[i].get() + layer[i+1].get()) : layer[i])
  );
  return ret;
};

const std::vector<Hash> Merkle::getProof(const uint64_t& leafIndex) const {
  if (leafIndex > this->_leafs.size() - 1) return {};
  std::vector<Hash> ret;
  uint64_t pos = leafIndex;
  // Check if left (even) or right (odd) child, pick its sibling,
  // move to the next layer, repeat until root layer then skip it
  for (std::vector<Hash> layer : _layers) {
    if (layer.size() == 1) break;
    pos = (pos % 2 == 0) ? pos + 1 : pos - 1;
    ret.push_back(layer[pos]);
    pos = std::ceil(pos / 2);
  }
  return ret;
}

