#include "merkle.h"
#include "hash.h"
#include "tx.h"

Merkle::Merkle(const std::vector<Hash>& leaves) {
  std::vector<Hash> leafs;
  for (const auto &leaf : leaves) {
    this->leafs.emplace_back(std::move(Utils::sha3(leaf.get())));
  }
  this->tree.emplace_back(this->leafs);
  while (this->tree.back().size() > 1) {
    this->tree.emplace_back(newLayer(this->tree.back()));
  }
};

Merkle::Merkle(const std::unordered_map<uint64_t, Tx, SafeHash>& txs) {
  std::vector<Hash> leafs;
  for (uint64_t i = 0; i < txs.size(); i++) {
    leafs.emplace_back(std::move(Utils::sha3(txs.find(i)->second.hash().get())));
  }

 this->tree.emplace_back(leafs);

 while (this->tree.back().size() > 1) {
    this->tree.emplace_back(newLayer(this->tree.back()));
  }
}

Merkle::Merkle(const std::unordered_map<uint64_t, Tx::Validator, SafeHash> &txs) {
  std::vector<Hash> leafs;
  for (uint64_t i = 0; i < txs.size(); ++i) {
    this->leafs.emplace_back(std::move(Utils::sha3(txs.find(i)->second.hash().get())));
  }
  this->tree.emplace_back(this->leafs);
  while (this->tree.back().size() > 1) {
    this->tree.emplace_back(newLayer(this->tree.back()));
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
  std::vector<Hash> leafs;
  if (leafIndex > this->leafs.size() - 1) return {};
  std::vector<Hash> ret;
  uint64_t pos = leafIndex;
  // Check if left (even) or right (odd) child, pick its sibling,
  // move to the next layer, repeat until root layer then skip it
  for (std::vector<Hash> layer : tree) {
    if (layer.size() == 1) break;
    pos = (pos % 2 == 0) ? pos + 1 : pos - 1;
    ret.push_back(layer[pos]);
    pos = std::ceil(pos / 2);
  }
  return ret;
}

void Patricia::addLeaf(Hash branch, std::string data) {
  PNode* tmpRoot = &this->_root;
  std::string str = branch.hex();
  for (int i = 0; i < str.length(); i++) {
    PNode* child = tmpRoot->getChild(str[i]);
    if (child == NULL) tmpRoot->addChild(str[i]);
    tmpRoot = tmpRoot->getChild(str[i]);
  }
  tmpRoot->setData(data);
}

std::string Patricia::getLeaf(Hash branch) {
  PNode* tmpRoot = &this->_root;
  std::string str = branch.hex();
  for (int i = 0; i < str.length(); i++) {
    tmpRoot = tmpRoot->getChild(str[i]);
    if (tmpRoot == NULL) return "";
  }
  return (!tmpRoot->getData().empty()) ? tmpRoot->getData() : "";
}

bool Patricia::delLeaf(Hash branch) {
  PNode* tmpRoot = &this->_root;
  std::string str = branch.hex();
  for (int i = 0; i < str.length(); i++) {
    tmpRoot = tmpRoot->getChild(str[i]);
    if (tmpRoot == NULL) return false;
  }
  if (!tmpRoot->getData().empty()) {
    tmpRoot->setData("");
    return true;
  } else return false;
}

