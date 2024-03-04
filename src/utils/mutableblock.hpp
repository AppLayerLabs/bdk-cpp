/*
Copyright (c) [2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef MUTABLEBLOCK_H
#define MUTABLEBLOCK_H

#include <future>
#include <thread>

#include "utils.h"
#include "tx.h"
#include "strings.h"
#include "merkle.h"
#include "ecdsa.h"

class MutableBlock {
public:
    // Constructor to initialize a MutableBlock from raw bytes and a chain ID
    MutableBlock(const BytesArrView bytes, const uint64_t& requiredChainId);

    // Append a transaction to the block
    bool appendTx(const TxBlock &tx);

    // Append a validator transaction to the block
    bool appendTxValidator(const TxValidator &tx);

    // Serialize the block header to bytes
    Bytes serializeHeader() const;

    // Serialize the entire block to bytes, including transactions
    Bytes serializeBlock() const;

    // Finalize the block, preventing any further modifications
    // FinalizedBlock finalize(const PrivKey& validatorPrivKey, const uint64_t& newTimestamp);

private:
    Signature validatorSig_;
    Hash prevBlockHash_;
    Hash blockRandomness_;
    Hash validatorMerkleRoot_;
    Hash txMerkleRoot_;
    uint64_t timestamp_;
    uint64_t nHeight_;
    std::vector<TxBlock> txs_;
    std::vector<TxValidator> txValidators_;
    Hash hash_;
    PublicKey validatorPubKey_;

    // Helper methods for deserialization and other internal logic
    void deserialize(const BytesArrView bytes, const uint64_t& requiredChainId);
};

#endif // MUTABLEBLOCK_H