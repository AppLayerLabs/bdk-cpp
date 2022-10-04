#ifndef MERKLE_H
#define MERKLE_H

#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "utils.h"

/**
 * Custom implementation of a Merkle Patricia Trie tree.
 * Adapted from https://github.com/sputnik1458/merkle-tree
 * Mental notes for reference:
 * - "Trie" = radix tree (as in "heap sort")
 * - "Patricia" = binary choice at each node (this is modified in Ethereum)
 * - "Merkle" = root node is a fingerprint of the whole tree
 * Docs for reference:
 * https://ethereum.stackexchange.com/questions/6415/eli5-how-does-a-merkle-patricia-trie-tree-work
 * https://github.com/PIVX-project/PIVX/blob/3.0/src/primitives/block.cpp#L26
 */

class MerkleNode {
  public:
    Hash hash;
    std::shared_ptr<MerkleNode> left;
    std::shared_ptr<MerkleNode> right;
    MerkleNode(Hash data) { this->hash = data; }
};

class MerkleTree {
  public:
    std::shared_ptr<MerkleNode> root;
    MerkleTree(std::vector<std::shared_ptr<MerkleNode>> blocks);
    ~MerkleTree() { this->deleteTree(this->root); }
    void printTree(std::shared_ptr<MerkleNode> n, int indent);
    void deleteTree(std::shared_ptr<MerkleNode> n);
};

#endif  // MERKLE_H
