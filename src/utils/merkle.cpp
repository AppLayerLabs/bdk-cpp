#include "merkle.h"

MerkleTree::MerkleTree(std::vector<std::shared_ptr<MerkleNode>> blocks) {
  std::vector<std::shared_ptr<MerkleNode>> nodes;
  while (blocks.size() != 1) {
    // Uncomment to print node hashes
    //for (unsigned int i = 0; i < blocks.size(); i++) {
    //  std::cout << blocks[i]->hash << std::cout;
    //}
    for (unsigned int i = 0, j = 0; i < blocks.size(); i+=2, j++) {
      // Check for adjacent block and assign children
      if (i != blocks.size() - 1) {
        nodes.push_back(std::make_shared<MerkleNode>(
          Utils::sha3(blocks[i]->hash.get() + blocks[i+1]->hash.get()))
        );
        nodes[j]->left = blocks[i];
        nodes[j]->right = blocks[i+1];
      } else {
        nodes.push_back(blocks[i]);
      }
    }
    blocks = nodes;
    nodes.clear();
  }
  this->root = blocks[0];
}

void MerkleTree::printTree(std::shared_ptr<MerkleNode> n, int indent) {
  if (n) {
    if (n->left) printTree(n->left, indent + 4);
    if (n->right) printTree(n->right, indent + 4);
    if (indent) std::cout << std::setw(indent) << " ";
    std::cout << n->hash[0] << std::endl;
  }
}

void MerkleTree::deleteTree(std::shared_ptr<MerkleNode> n) {
  if (n) {
    deleteTree(n->left);
    deleteTree(n->right);
    n = NULL;
    delete &n;
  }
}

