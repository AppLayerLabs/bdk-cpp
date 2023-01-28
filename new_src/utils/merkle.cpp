#include "merkle.h"

std::vector<Hash> Merkle::newLayer(const std::vector<Hash>& layer) {
  std::vector<Hash> ret;
  for (uint64_t i = 0; i < layer.size(); i += 2) ret.emplace_back(
    ((i + 1 < layer.size()) ? Utils::sha3(layer[i].get() + layer[i + 1].get()) : layer[i])
  );
  return ret;
};

Merkle::Merkle(const std::vector<Hash>& leaves) {
  // Mount the base leaves
  std::vector<Hash> tmp;
  for (const Hash& leaf : leaves) {
    tmp.emplace_back(std::move(Utils::sha3(leaf.get())));
  }
  this->tree.emplace_back(tmp);
  // Make the layers up to root
  while (this->tree.back().size() > 1) {
    this->tree.emplace_back(newLayer(this->tree.back()));
  }
};

Merkle::Merkle(const std::unordered_map<uint64_t, TxBlock, SafeHash>& txs) {
  // Mount the base leaves
  std::vector<Hash> tmp;
  for (uint64_t i = 0; i < txs.size(); i++) {
    tmp.emplace_back(std::move(Utils::sha3(txs.find(i)->second.hash().get())));
  }
  this->tree.emplace_back(tmp);
  // Make the layers up to root
  while (this->tree.back().size() > 1) {
    this->tree.emplace_back(newLayer(this->tree.back()));
  }
}

Merkle::Merkle(const std::unordered_map<uint64_t, TxValidator, SafeHash>& txs) {
  // Mount the base leaves
  std::vector<Hash> tmp;
  for (uint64_t i = 0; i < txs.size(); ++i) {
    tmp.emplace_back(std::move(Utils::sha3(txs.find(i)->second.hash().get())));
  }
  this->tree.emplace_back(tmp);
  // Make the layers up to root
  while (this->tree.back().size() > 1) {
    this->tree.emplace_back(newLayer(this->tree.back()));
  }
};

const std::vector<Hash> Merkle::getProof(const uint64_t leafIndex) const {
  if (leafIndex > this->tree.front().size() - 1) return {};
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

PNode* PNode::getChild(char id) {
  auto it = std::find_if(
  std::begin(this->children), std::end(this->children),
    [&id](const PNode& node){ return node.id == id; }
  );
  return (it != this->children.end()) ? &*it : NULL;
}

void Patricia::addLeaf(Hash branch, std::string data) {
  PNode* tmpRoot = &this->root;
  std::string str = branch.hex();
  for (int i = 0; i < str.length(); i++) {
    PNode* child = tmpRoot->getChild(str[i]);
    if (child == NULL) tmpRoot->addChild(str[i]);
    tmpRoot = tmpRoot->getChild(str[i]);
  }
  tmpRoot->setData(data);
}

std::string Patricia::getLeaf(Hash branch) {
  PNode* tmpRoot = &this->root;
  std::string str = branch.hex();
  for (int i = 0; i < str.length(); i++) {
    tmpRoot = tmpRoot->getChild(str[i]);
    if (tmpRoot == NULL) return "";
  }
  return (!tmpRoot->getData().empty()) ? tmpRoot->getData() : "";
}

bool Patricia::delLeaf(Hash branch) {
  PNode* tmpRoot = &this->root;
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

