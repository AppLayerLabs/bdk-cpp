#ifndef MERKLE_H
#define MERKLE_H

#include "utils.h"
#include "transaction.h"

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
    Merkle(const std::unordered_map<uint64_t, Tx::Base, SafeHash> &transactions);

    const Hash& root() const { return this->_root; }
    const std::vector<std::vector<Hash>>& layers() const { return this->_layers; }
    const std::vector<Hash>& leafs() const { return this->_leafs; }

    const std::vector<Hash> getProof(const uint64_t& leafIndex) const;
};

class Patricia {
  private:
    class PNode {
      private:
        char _id;
        std::string _data;
        std::vector<PNode> _children;

      public:
        PNode(char id) : _id(id) {}
        void addChild(char id) { this->_children.emplace_back(PNode(id)); }
        PNode* getChild(char id) {
          auto it = std::find_if(
            std::begin(this->_children), std::end(this->_children),
            [&id](const PNode& node){ return node._id == id; }
          );
          return (it != this->_children.end()) ? &*it : NULL;
        }
        void setData(std::string data) { this->_data = data; }
        std::string getData() { return this->_data; }
    };
    PNode _root;

  public:
    Patricia() : _root('/') {}
    void add(Hash hash, std::string data);
    std::string get(Hash hash);
    bool remove(Hash hash);
};

#endif  // MERKLE_H
