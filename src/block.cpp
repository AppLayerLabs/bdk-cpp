#include "block.h"



bool Block::addTx(dev::eth::TransactionBase tx) {
    if (!tx.hasSignature()) {
        logToFile("Block::addTx error: submitted transaction has no signature");
        return false;
    }

    if (tx.safeSender() == NULL) {
        logToFile("Block::addTx error: invalid Tx!");
        return false;
    }

    this->transactions.push_back(tx);
    this->txCount = this->txCount + 1;
    return true;
}

std::string Block::serializeToString() {
    std::string ret;
    size_t blockSize = 0;
    uint64_t strIndex = 0;
    blockSize = blockSize + 192; // 192 bytes from blockheader.

    // Load transactions to string.
    // Before each transaction, there is a 4 bytes value telling how many bytes a given tranasction has.
    blockSize = blockSize + (this->txCount * 4);

    std::vector<dev::bytes> transactionsRLPs;

    for (auto tx : this->transactions) {
        transactionsRLPs.push_back(tx.rlp());
        blockSize = blockSize + tx.rlp().size();
    }

    // Resize string to fit block
    ret.resize(blockSize);
    // Serialize block header to string.
    auto prevBlockHashBytes = Utils::u256toBytes(_prevBlockHash);
    auto timestampBytes = Utils::u256toBytes(_timestamp);
    auto txCountBytes = Utils::u256toBytes(_txCount);
    auto nHeightBytes = Utils::u256toBytes(_nHeight);
    auto blockDataBytes = Utils::u256toBytes(_blockData);
    
    for (auto byte : prevBlockHashBytes) {
        ret[strIndex] = byte;
        ++strIndex;
    }
    
    for (auto byte : timestampBytes) {
        ret[strIndex] = byte;
        ++strIndex;
    }

    for (auto byte : txCountBytes) {
        ret[strIndex] = byte;
        ++strIndex;
    }

    for (auto byte : nHeightBytes) {
        ret[strIndex] = byte;
        ++strIndex;
    }

    for (auto byte : blockDataBytes) {
        ret[strIndex] = byte;
        ++strIndex;
    }

    // Serialize transactions to string.
    for (auto tx : transactionsRLPs) {
        uint32_t transactionSize = tx.size(); // 4 Bytes
        for (uint i = 0; i < 4; ++i) {
            ret[strIndex] = (transactionSize >> (i * 8));
            ++strIndex;
        }

        for (auto byte : tx) {
            ret[strIndex] = byte;
            ++strIndex;
        }
    }

    return ret;s
}