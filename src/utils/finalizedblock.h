/*
Copyright (c) [2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef FINALIZEDBLOCK_H
#define FINALIZEDBLOCK_H

/**
 * CometBFT integration note:
 *
 * FinalizedBlock is what a block looks like from the cometbft app (i.e. contract machine) side.
 *
 * Every CometBlock instance implies one corresponding (matching) FinalizedBlock instance.
 *
 * Ideally, we should remove as many fields from FinalizedBlock as possible.
 *
 * Use fromCometBlock() to create a FinalizedBlock, not fromBytes(), since now there is no
 * longer a bytes source for a block. We get blocks via the ABCI as disjoint binary transaction lists
 * and metadata scattered in named ABCI parameters (and all other info that cometbft doesn't think the
 * app should care about  like signatures, other header info, etc. can be fetched via the cometbft RPC).
 *
 * NEW: We are going to use this class to store a block query result as well
 * (block data that comes via a cometbft JSON-RPC block query response); in that
 * case we could have more fields available, such as the validator signature (for
 * whatever purpose that would serve). That is done via FinalizedBlock::fromRPC(json&).
 */

#include "merkle.h" // tx.h -> ecdsa.h -> utils.h -> strings.h, (logger.h -> future), bytes/join.h

class CometBlock;

// REVIEW/TODO: We may want to create a class to represent a standard ETH1
//       block header, that can be predictably hashed with sha3(), and that is
//       deterministically derived from a FinalizedBlock, which in turn is
//       always derived from a CometBlock (NEW: or a json block RPC query response).

/// Abstraction of a finalized block. Members are purposefully const due to the immutable nature of the structure.
class FinalizedBlock {
  private:
    // If we want to get a per-block hash that is computed on our side, then that
    // hash can only be the hash computed over the Eth1 block header structure
    // we'd be emulating for each CometBlock.
    const Hash hash_; ///< CometBFT block hash (block_id) of this block.

    const Hash prevBlockHash_; ///< CometBFT block Hash (block_id) of the previous block (optional).

    const Address proposerAddr_; ///< CometBFT Address of the block proposer (NOT the Eth Address)

    const uint64_t timestamp_; ///< Epoch timestamp of the block, in microseconds.

    const uint64_t nHeight_; ///< Height of the block in chain.

    const Hash txMerkleRoot_; ///< Merkle root for block transactions.

    // TODO: This should probably be vector<shared_ptr<TxBlock>> so that the tx objects
    // can be automatically used by all the rpc/query/cache code that uses shared_ptr<TxBlock>.
    const std::vector<TxBlock> txs_; ///< List of block transactions.

  public:
    /**
     * Move Constructor.
     * Only the move constructor is declared, simply because there is no reason
     * within BDKD to copy the arguments when creating a new block.
     * @param proposerAddr Proposer address in CometBFT format (NOT Eth Address format).
     * @param prevBlockHash Hash of the previous block.
     * @param txMerkleRoot Merkle root for the block transactions.
     * @param timestamp Epoch timestamp of the block, in microseconds.
     * @param nHeight Height of the block in chain.
     * @param txs List of block transactions.
     * @param hash Cached hash of the block.
     */
    FinalizedBlock(
      Address&& proposerAddr,
      Hash&& prevBlockHash,
      Hash&& txMerkleRoot,
      uint64_t timestamp, // Primitive types like uint64_t can (and should) be passed by value, no &&
      uint64_t nHeight, // Same for nHeight
      std::vector<TxBlock>&& txs,
      Hash&& hash
    ) :
      proposerAddr_(std::move(proposerAddr)),
      prevBlockHash_(std::move(prevBlockHash)),
      txMerkleRoot_(std::move(txMerkleRoot)),
      timestamp_(timestamp),
      nHeight_(nHeight),
      txs_(std::move(txs)),
      hash_(std::move(hash))
    {
    }

    /**
     * Move constructor.
     * @param block The FinalizedBlock to move.
     */
    FinalizedBlock(FinalizedBlock&& block) :
      proposerAddr_(std::move(block.proposerAddr_)),
      prevBlockHash_(std::move(block.prevBlockHash_)),
      txMerkleRoot_(std::move(block.txMerkleRoot_)),
      timestamp_(block.timestamp_),
      nHeight_(block.nHeight_),
      txs_(std::move(block.txs_)),
      hash_(std::move(block.hash_))
    {
    }

    /**
     * Copy constructor.
     * @param block The FinalizedBlock to copy.
     */
    FinalizedBlock(const FinalizedBlock& block) :
      proposerAddr_(block.proposerAddr_),
      prevBlockHash_(block.prevBlockHash_),
      txMerkleRoot_(block.txMerkleRoot_),
      timestamp_(block.timestamp_),
      nHeight_(block.nHeight_),
      txs_(block.txs_),
      hash_(block.hash_)
    {
    }

    /**
     * Build a FinalizedBlock from a CometBlock.
     */
    static FinalizedBlock fromCometBlock(const CometBlock& block);

    /**
     * Build a FinalizedBlock from a CometBFT block query response.
     * To get a shared_ptr for the returned object X, just use make_shared(std::move(X)).
     * FIXME/TODO
     */
    static FinalizedBlock fromRPC(const json& ret);

    ///@{
    /** Getter. */
    const Address& getProposerAddr() const { return this->proposerAddr_; }
    const Hash& getPrevBlockHash() const { return this->prevBlockHash_; }
    const Hash& getTxMerkleRoot() const { return this->txMerkleRoot_; }
    const uint64_t& getTimestamp() const { return this->timestamp_; }
    const uint64_t& getNHeight() const { return this->nHeight_; }
    const std::vector<TxBlock>& getTxs() const { return this->txs_; }
    const Hash& getHash() const { return this->hash_; }
    ///@}

    /// Equality operator. Checks the block hash.
    bool operator==(const FinalizedBlock& b) const {
      return ((this->getHash() == b.getHash()));
    }
};

#endif  // FINALIZEDBLOCK_H
