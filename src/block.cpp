#include "block.h"



bool Block::addTx(dev::eth::TransactionBase tx) {
    if (!tx.hasSignature()) {
        Utils::logToFile("Block::addTx error: submitted transaction has no signature");
        return false;
    }

    if (tx.safeSender() == dev::ZeroAddress) {
        Utils::logToFile("Block::addTx error: invalid Tx!");
        return false;
    }

    this->_transactions.push_back(tx);
    this->_txCount = this->_txCount + 1;
    return true;
}

std::string Block::serializeToString() {
    std::string ret;
    size_t blockSize = 0;
    uint64_t strIndex = 0;
    blockSize = blockSize + 192; // 192 bytes from blockheader.

    // Load transactions to string.
    // Before each transaction, there is a 4 bytes value telling how many bytes a given tranasction has.
    blockSize = blockSize + (boost::lexical_cast<uint32_t>(this->_txCount) * 4);

    std::vector<dev::bytes> transactionsRLPs;

    for (auto tx : this->_transactions) {
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

    return ret;
}


bool Block::serializeFromString(std::string blockBytes) {
    // First 192 bytes == Block Header.
    // Remaining == txCount * (4 + txSize);

    size_t strIndex;


    // Read block header;
    for (uint i = 0; i < 32; ++i) {
        _prevBlockHash >> (8 * i) = blockBytes[strIndex];
        ++strIndex;
    }

    for (uint i = 0; i < 32; ++i) {
        _timestamp >> (8 * i) = blockBytes[strIndex];
        ++strIndex;
    }

    for (uint i = 0; i < 32; ++i) {
        _nHeight >> (8 * i) = blockBytes[strIndex];
        ++strIndex;
    }

    for (uint i = 0; i < 32; ++i) {
        _blockData >> (8 * i) = blockBytes[strIndex];
        ++strIndex;
    }

    // Read transactions.
    while (true) {
        if (strIndex == blockBytes.size()) { break; };
        // Read the first 4 bytes (tx size).
        uint32_t txSize = 0;
        txSize = uint32_t(
            (unsigned char)(blockBytes[strIndex])   << 24 |
            (unsigned char)(blockBytes[strIndex+1]) << 16 |
            (unsigned char)(blockBytes[strIndex+2]) << 8  |
            (unsigned char)(blockBytes[strIndex+3])
        );


        strIndex = strIndex + 4;
        dev::bytes txRLP;
        for (auto i = 0; i < txSize; ++i) {
            txRLP.push_back(blockBytes[strIndex]);
            ++strIndex;
        }
        dev::eth::TransactionBase tx(txRLP, dev::eth::CheckTransaction::Everything);
        _transactions.push_back(tx);
    }

    auto hash = dev::sha3(blockBytes.data());

    this->_blockHash = Utils::bytesTou256(hash.asBytes());

    return true;
}