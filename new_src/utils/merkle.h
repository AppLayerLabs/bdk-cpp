#ifndef MERKLE_H
#define MERKLE_H

#include <string>
#include <vector>

class Merkle {
  private:
    std::vector<std::vector<Hash>> tree;
    std::vector<Hash> newLayer(const std::vector<Hash> layer);
  public:
    Merkle(const std::vector<Hash> leaves);
    Merkle(const std::unordered_map<uint64_t, Tx, SafeHash> txs);
    inline const Hash getRoot() { return this->tree.back().front(); }
    inline const std::vector<std::vector<Hash>> getTree() { return this->tree; }
    const std::vector<Hash> getProof(const uint64_t leafIndex);
};

class PNode {
  private:
    char id;
    std::string data;
    std::vector<PNode> children;
  public:
    PNode(char id) : id(id) {};
    inline char getId() { return this->id; }
    inline std::string getData() { return this->data; }
    inline void setData(std::string data) { this->data = data; }
    inline bool hasChildren() { return this->children.size() > 0; }
    inline void addChild(char id) { this->children.emplace_back(PNode(id)); }
    PNode* getChild(char id);
};

class Patricia {
  private:
    PNode root;
  public:
    Patricia() : root('/') {};
    void addLeaf(Hash branch, std::string data);
    std::string getLeaf(Hash branch);
    bool delLeaf(Hash branch);
};

#endif  // MERKLE_H
