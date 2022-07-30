#ifndef BLOCK_H
#define BLOCK_H

#include <array>
#include <stdint.h>
#include <vector>

#include "utils.h"
#include "transaction.h"

/**
 * The Block class only contains the structure of blocks and some utilities
 * to deal with them. It does **NOT** check transaction logic/signature.
 * It is meant to be a fast way to parse from/to the network and disk.
 *
 * BLOCK STRUCTURE:
 *
 * 32 BYTES - PREV BLOCK HASH
 * 8 BYTES - TIMESTAMP
 * 8 BYTES - NHEIGHT
 * 4 BYTES - TX_COUNT
 * 4 BYTES - TX ARRAY SIZE
 * [
 *   4 BYTES - TX SIZE
 *   X BYTES - TX
 *   ,
 *   4 BYTES - TX SIZE
 *   X BYTES - TX
 *   ,
 *   ...
 * ]
 *
 * EXAMPLE (in hex):
 *
 * prevBlockHash:                   5c 37 d5 04 e9 41 5c 3b 75 af aa 3a d2 44 84 38 22 74 bb a3 1f 10 dc d2 68 e5 54 78 5d 5b 80 75
 * timestamp:                       00 00 01 81 81 0E B6 50
 * nHeight:                         7a 8b 54 df bf e9 f2 1d
 * txCount:                         00 00 00 01
 * [
 *   TxSize: 350 chars (175 bytes)  00 00 00 af
 *   TxRaw:                         f8ad81be850c92a69c0082e18c94d586e7f844cea2f87f50152665bcbc2c279d8d7080b844a9059cbb00000000000000000000000026548521f99d0709f615aa0f766a7df60f99250b00000000000000000000000000000000000000000000002086ac351052600000830150f7a07e16328b7f3823abeb13d0cab11cdffaf967c9b2eaf3757c42606d6f2ecd8ce6a040684c94b289cdda22822e5cb374ea374d7a3ba581a9014faf35b19e5345ab92
 * ]
 *
 * Raw Block = prevBlockHash + timestamp + nHeight + txCount + [ txSize, tx, ... ]
 * 5c37d504e9415c3b75afaa3ad24484382274bba31f10dcd268e554785d5b8075
 * 00000181810EB650
 * 7a8b54dfbfe9f21d
 * 00000001
 * 000000af
 * f8ad81be850c92a69c0082e18c94d586e7f844cea2f87f50152665bcbc2c279d8d7080b844a9059cbb00000000000000000000000026548521f99d0709f615aa0f766a7df60f99250b00000000000000000000000000000000000000000000002086ac351052600000830150f7a07e16328b7f3823abeb13d0cab11cdffaf967c9b2eaf3757c42606d6f2ecd8ce6a040684c94b289cdda22822e5cb374ea374d7a3ba581a9014faf35b19e5345ab92
 *
 * rawBlock: 5c37d504e9415c3b75afaa3ad24484382274bba31f10dcd268e554785d5b807500000181810EB6507a8b54dfbfe9f21d00000001000000aff8ad81be850c92a69c0082e18c94d586e7f844cea2f87f50152665bcbc2c279d8d7080b844a9059cbb00000000000000000000000026548521f99d0709f615aa0f766a7df60f99250b00000000000000000000000000000000000000000000002086ac351052600000830150f7a07e16328b7f3823abeb13d0cab11cdffaf967c9b2eaf3757c42606d6f2ecd8ce6a040684c94b289cdda22822e5cb374ea374d7a3ba581a9014faf35b19e5345ab92
 */
class Block {
  private:
    // TODO: Add creator Header (might require writing a secp256k1 wrapper)
    uint256_t _prevBlockHash;
    uint64_t _timestamp;
    uint64_t _nHeight;
    uint32_t _txCount;
    std::vector<Tx::Base> _transactions;
    bool finalized = false;
    bool inChain = false;

  public:
    Block(const std::string &blockData);  // Constructor from network/rpc.

    // Constructor from creation.
    Block(
      const uint256_t &_prevBlockHashC,
      const uint64_t &_timestampC,
      const uint64_t &_nHeightC
    ) : _prevBlockHash(_prevBlockHashC),
        _timestamp(_timestampC),
        _nHeight(_nHeightC),
        _txCount(0)
    {};

    // Getters.
    const std::string prevBlockHash() { return Utils::uint256ToBytes(this->_prevBlockHash); };
    const uint64_t& timestamp() { return this->_timestamp; };
    const uint64_t timestampInSeconds() { return this->_timestamp / 1000000000; };
    const uint64_t& nHeight() { return this->_nHeight; };
    const uint32_t& txCount() { return this->_txCount; };
    const std::vector<Tx::Base>& transactions() { return this->_transactions; };
    const uint64_t blockSize();
    std::string getBlockHash(); // Hash (in bytes)
    std::string serializeToBytes();

    // When transactions are indexed. the block is considered to be on the chain.
    void indexTxs();
    bool appendTx(Tx::Base &tx);
    bool finalizeBlock();
};

#endif  // BLOCK_H
