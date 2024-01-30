/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef MERKLE_H
#define MERKLE_H

#include <string>
#include <vector>

#include "safehash.h"
#include "strings.h"

#include "tx.h"
#include "utils.h"

/**
 * Custom implementation of a %Merkle tree. Adapted from:
 *
 * https://medium.com/coinmonks/implementing-merkle-tree-and-patricia-tree-b8badd6d9591
 *
 * https://lab.miguelmota.com/merkletreejs/example/
 */
class Merkle {
  private:
    std::vector<std::vector<Hash>> tree_;  ///< The %Merkle tree itself.

    /**
     * Insert a new layer in the %Merkle tree.
     * @param layer The list of hashes to convert into a layer.
     * @return The newly created layer.
     */
    std::vector<Hash> newLayer(const std::vector<Hash>& layer) const;

  public:
    /**
     * Constructor.
     * @param leaves The list of leaves to create the %Merkle tree from.
     */
    Merkle(const std::vector<Hash>& leaves);

    /**
     * Constructor for block transactions.
     * TxType would be one of the enum types described in rdPoS.
     * @param txs The list of transactions to create the %Merkle tree from.
     */
    template <typename TxType> Merkle(const std::vector<TxType>& txs) {
      // Mount the base leaves
      std::vector<Hash> tmp;
      for (auto tx : txs) tmp.emplace_back(std::move(Utils::sha3(tx.hash().get())));
      this->tree_.emplace_back(tmp);
      // Make the layers up to root
      while (this->tree_.back().size() > 1) this->tree_.emplace_back(newLayer(this->tree_.back()));
    }

    /// Getter for `tree`.
    inline const std::vector<std::vector<Hash>>& getTree() const { return this->tree_; }

    /// Getter for `tree`, but returns only the root.
    inline const Hash getRoot() const {
      if (this->tree_.back().size() == 0) return Hash();
      return this->tree_.back().front();
    }

    /// Getter for `tree`, but returns only the leaves.
    inline const std::vector<Hash>& getLeaves() const { return this->tree_.front(); }

    /**
     * Get the proof for a given leaf in the %Merkle tree.
     * @param leafIndex The index of the leaf to get the proof from.
     *                  Considers only the leaf layer (e.g. {A, B, C, D, E, F},
     *                  `getProof(2)` would get the proof for leaf C).
     * @return A list of proofs for the leaf.
     */
    const std::vector<Hash> getProof(const uint64_t leafIndex) const;

    /**
     * Verify a leaf node's data integrity against its proof and the root hash.
     * @param proof The leaf's proof list as per getProof().
     * @param leaf The leaf to verify.
     * @param root The tree's root.
     * @return `true` if the leaf node is valid, `false` otherwise.
     */
    static bool verify(const std::vector<Hash>& proof, const Hash& leaf, const Hash& root);
};

#endif  // MERKLE_H
