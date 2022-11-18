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

Merkle::Merkle(const std::unordered_map<uint64_t, Tx::Base, SafeHash> &transactions) {
  for (uint64_t i = 0; i < transactions.size(); ++i) {
    this->_leafs.emplace_back(std::move(Utils::sha3(transactions.find(i)->second.hash().get())));
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

void Patricia::add(Hash hash, std::string data) {
  PNode* tmpRoot = &this->_root;
  std::string str = hash.hex();
  for (int i = 0; i < str.length(); i++) {
    PNode* child = tmpRoot->getChild(str[i]);
    if (child == NULL) tmpRoot->addChild(str[i]);
    tmpRoot = tmpRoot->getChild(str[i]);
  }
  tmpRoot->setData(data);
}

std::string Patricia::get(Hash hash) {
  PNode* tmpRoot = &this->_root;
  std::string str = hash.hex();
  for (int i = 0; i < str.length(); i++) {
    tmpRoot = tmpRoot->getChild(str[i]);
    if (tmpRoot == NULL) return "";
  }
  return (!tmpRoot->getData().empty()) ? tmpRoot->getData() : "";
}

bool Patricia::remove(Hash hash) {
  PNode* tmpRoot = &this->_root;
  std::string str = hash.hex();
  for (int i = 0; i < str.length(); i++) {
    tmpRoot = tmpRoot->getChild(str[i]);
    if (tmpRoot == NULL) return false;
  }
  if (!tmpRoot->getData().empty()) {
    tmpRoot->setData("");
    return true;
  } else return false;
}

