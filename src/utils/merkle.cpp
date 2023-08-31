/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "merkle.h"

std::vector<Hash> Merkle::newLayer(const std::vector<Hash>& layer) const {
  std::vector<Hash> ret;
  for (uint64_t i = 0; i < layer.size(); i += 2) ret.emplace_back((
    (i + 1 < layer.size())
      ? Utils::sha3(
        // Lambda to concatenate
        Bytes([&]() -> Bytes {
          Bytes bytes;
          bytes.reserve(64);
          Utils::appendBytes(bytes, std::min(layer[i], layer[i + 1]));
          Utils::appendBytes(bytes, std::max(layer[i], layer[i + 1]));
          return bytes;
        }()
      ))
      : layer[i]
  ));
  return ret;
}

Merkle::Merkle(const std::vector<Hash>& leaves) {
  // Mount the base leaves
  std::vector<Hash> tmp;
  for (const Hash& leaf : leaves) tmp.emplace_back(std::move(Utils::sha3(leaf.get())));
  this->tree_.emplace_back(tmp);
  // Make the layers up to root
  while (this->tree_.back().size() > 1) this->tree_.emplace_back(newLayer(this->tree_.back()));
}

const std::vector<Hash> Merkle::getProof(const uint64_t leafIndex) const {
  if (leafIndex > this->tree_.front().size() - 1) return {};
  std::vector<Hash> ret;
  uint64_t pos = leafIndex;
  // Check if left (even) or right (odd) child, pick its sibling,
  // move to the next layer, repeat until root layer then skip it
  for (std::vector<Hash> layer : this->tree_) {
    if (layer.size() == 1) break;
    pos = (pos % 2 == 0) ? pos + 1 : pos - 1;
    ret.push_back(layer[pos]);
    pos = std::ceil(pos / 2);
  }
  return ret;
}

bool Merkle::verify(const std::vector<Hash>& proof, const Hash& leaf, const Hash& root) {
  Hash computedHash = leaf;
  for (const Hash& hash : proof) computedHash = Utils::sha3(
    // Lambda to concatenate
    Bytes([&]() -> Bytes {
      Bytes bytes;
      bytes.reserve(64);
      Utils::appendBytes(bytes, std::min(computedHash, hash));
      Utils::appendBytes(bytes, std::max(computedHash, hash));
      return bytes;
    }())
  );
  return computedHash == root;
}

