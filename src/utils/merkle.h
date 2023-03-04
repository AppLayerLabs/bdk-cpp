#ifndef MERKLE_H
#define MERKLE_H

#include <string>
#include <vector>

#include "safehash.h"
#include "strings.h"

#include "tx.h"
#include "utils.h"

/**
 * Custom implementation of a Merkle tree.
 * Adapted from:
 * https://medium.com/coinmonks/implementing-merkle-tree-and-patricia-tree-b8badd6d9591
 * https://lab.miguelmota.com/merkletreejs/example/
 */
class Merkle {
  private:
    std::vector<std::vector<Hash>> tree;  ///< The Merkle tree itself.

    /**
     * Insert a new layer in the Merkle tree.
     * @param layer The list of hashes to convert into a layer.
     * @return The newly created layer.
     */
    std::vector<Hash> newLayer(const std::vector<Hash>& layer) const;
  public:
    /**
     * Constructor.
     * @param leaves The list of leaves to create the Merkle tree from.
     */
    Merkle(const std::vector<Hash>& leaves);

    /**
     * Constructor for %Block transactions.
     * @param txs The list of transactions to create the Merkle tree from.
     */
    template <typename TxType>
    Merkle(const std::vector<TxType>& txs) {
      // Mount the base leaves
      std::vector<Hash> tmp;
      for (auto tx : txs) {
        tmp.emplace_back(std::move(Utils::sha3(tx.hash().get())));
      }
      this->tree.emplace_back(tmp);
      // Make the layers up to root
      while (this->tree.back().size() > 1) {
        this->tree.emplace_back(newLayer(this->tree.back()));
      }
    }

    /// Getter for `tree`.
    inline const std::vector<std::vector<Hash>>& getTree() const { return this->tree; }

    /// Get the root of the Merkle tree.
    inline const Hash& getRoot() const { return this->tree.back().front(); }

    /// Get the leaves of the Merkle tree.
    inline const std::vector<Hash>& getLeaves() const { return this->tree.front(); }

    /**
     * Get the proof for a given leaf in the Merkle tree.
     * @param leafIndex The index of the leaf.
     *                  Considers only the leaf layer (e.g. {A, B, C, D, E, F},
     *                  `getProof(2)` would get the proof for leaf C).
     * @return A list of proofs for the leaf.
     */
    const std::vector<Hash> getProof(const uint64_t leafIndex) const;

    static bool verify(const std::vector<Hash> &proof, const Hash& leaf, const Hash& root);
};

/**
 * Abstraction of a Patricia tree node.
 * Used internally by the %Patricia class.
 */
class PNode {
  private:
    char _id;  ///< ID of the node. `/` implies it's the root node.
    std::string data; ///< Data of the node. Non-empty implies it's the end node.
    std::vector<PNode> children;  ///< List of children nodes.
  public:
    /**
     * Constructor.
     * @param id The ID of the node.
     */
    PNode(char id) : _id(id) {};

    /// Getter for `id`.
    inline const char& getId() const { return this->_id; }

    /// Getter for `data`.
    inline const std::string& getData() const { return this->data; }

    /// Setter for `data`.
    inline void setData(std::string content) { this->data = content; }

    /**
     * Check if the node has any children.
     * @returns `true` if the node has children, `false` otherwise.
     */
    inline bool hasChildren() const { return this->children.size() > 0; }

    /**
     * Add a child to the node.
     * @param id The ID of the child node.
     */
    inline void addChild(char id) { this->children.emplace_back(PNode(id)); }

    /**
     * Get a child from the node.
     * @param id The ID of the child node.
     * @return A pointer to the child node, or a NULL pointer if not found.
     */
    PNode* getChild(char id) const;
};

/**
 * Custom implementation of a Patricia tree.
 * Adapted from:
 * https://medium.com/coinmonks/implementing-merkle-tree-and-patricia-tree-b8badd6d9591
 * https://lab.miguelmota.com/merkletreejs/example/
 */
class Patricia {
  private:
    PNode root; ///< Root node.
  public:
    Patricia() : root('/') {}; ///< Constructor. Sets the ID of the root node to `/`.

    /**
     * Create a new branch in the tree and add a leaf node with data.
     * @param branch The hash string to use as a base for creating the branch.
     * @param data The data string to add to the leaf node.
     */
    void addLeaf(Hash branch, std::string data) const;

    /**
     * Get data from a leaf node in a given branch.
     * @param branch The hash string to use as a base for searching the branch.
     * @return The data string contained in the leaf node.
     */
    std::string getLeaf(Hash branch) const;

    /**
     * Remove data from a leaf node in a given branch.
     * Does not remove the branch itself.
     * @param branch The hash string to use as a base for removing data from the branch.
     * @return `true` if the removal was successful, `false` otherwise.
     */
    bool delLeaf(Hash branch) const;
};

#endif  // MERKLE_H
