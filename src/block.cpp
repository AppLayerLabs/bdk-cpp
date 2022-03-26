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
    blockSize = blockSize + 224; // 224 bytes from blockheader.

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
    auto timestampBytes = Utils::u256toBytes(_timestamp);
    auto txCountBytes = Utils::u256toBytes(_txCount);
    auto nHeightBytes = Utils::u256toBytes(_nHeight);
    auto blockDataBytes = Utils::u256toBytes(_blockData);
    
    for (auto byte : this->_prevBlockHash) {
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
        unsigned char txSizeBytes[4];
        std::memcpy(&txSizeBytes, &transactionSize, sizeof(transactionSize));

        ret[strIndex] = txSizeBytes[0];
        ret[strIndex+1] = txSizeBytes[1];
        ret[strIndex+2] = txSizeBytes[2];
        ret[strIndex+3] = txSizeBytes[3];
        strIndex = strIndex + 4;
        for (auto byte : tx) {
            ret[strIndex] = byte;
            ++strIndex;
        }
    }

    this->_blockHash = dev::toHex(dev::sha3(ret));
    return ret;
}


bool Block::serializeFromString(std::string blockBytes) {
    // First 224 bytes == Block Header.
    // Remaining == txCount * (4 + txSize);

    size_t strIndex;

    unsigned char timestampBytes[48];
    unsigned char txCountBytes[48];
    unsigned char nHeightBytes[48];
    unsigned char blockDataBytes[48];

    // Read block headers

    for (uint64_t i = 0; i < 32; ++i) {
        _prevBlockHash[i] = blockBytes[strIndex];
        ++strIndex;
    }

    for (uint64_t i = 0; i < 48; ++i) {
        timestampBytes[i] = blockBytes[strIndex];
        ++strIndex;
    }
    std::memcpy(&this->_timestamp, &timestampBytes, sizeof(timestampBytes));

    for (uint64_t i = 0; i < 48; ++i) {
        txCountBytes[i] = blockBytes[strIndex];
        ++strIndex;
    }
    std::memcpy(&this->_txCount, &txCountBytes, sizeof(txCountBytes));

    for (uint64_t i = 0; i < 48; ++i) {
        nHeightBytes[i] = blockBytes[strIndex];
        ++strIndex;
    }
    std::memcpy(&this->_nHeight, &nHeightBytes, sizeof(nHeightBytes));

    for (uint64_t i = 0; i < 48; ++i) {
        blockDataBytes[i] = blockBytes[strIndex];
        ++strIndex;
    }
    std::memcpy(&this->_blockData, &blockDataBytes, sizeof(blockDataBytes));


    // Read transactions.
    while (true) {
        if (strIndex == blockBytes.size()) { break; };
        // Read the first 4 bytes (tx size).
        uint32_t txSize = 0;
        
        unsigned char txSizeBytes[sizeof(txSize)];

        txSizeBytes[0] = blockBytes[strIndex];
        txSizeBytes[1] = blockBytes[strIndex+1];
        txSizeBytes[2] = blockBytes[strIndex+2];
        txSizeBytes[3] = blockBytes[strIndex+3];

        std::memcpy(&txSize, &txSizeBytes, sizeof(txSizeBytes));
        strIndex = strIndex + 4;
        dev::bytes txRLP;
        for (auto i = 0; i < txSize; ++i) {
            txRLP.push_back(blockBytes[strIndex]);
            ++strIndex;
        }
        dev::eth::TransactionBase tx(txRLP, dev::eth::CheckTransaction::Everything);
        _transactions.push_back(tx);
    }

    this->_blockHash = dev::toHex(dev::sha3(blockBytes));

    return true;
}