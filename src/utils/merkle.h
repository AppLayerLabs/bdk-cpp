#ifndef MERKLE_H
#define MERKLE_H

#include "utils.h"

class MerkleRoot {

  private:
    std::vector<Hash> _leafs;
    std::vector<std::vector<Hash>> _layers;
    Hash _root;


    std::vector<Hash> hashLayer(const std::vector<Hash> &layer);
  public:

    MerkleRoot(const std::vector<Hash> &leafs);

    const Hash& root() const { return _root; }
    const std::vector<Hash>& leafs() const { return _leafs; }
    const std::vector<std::vector<Hash>>& layers() const { return _layers; }
    const std::vector<Hash> getHexProof(const uint64_t &index) const;


};



#endif  // MERKLE_H

