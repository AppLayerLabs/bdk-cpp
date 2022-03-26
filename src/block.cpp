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
    json blockJson;
    blockJson["prevBlockHash"] = this->_prevBlockHash;
    blockJson["timestamp"] = boost::lexical_cast<std::string>(this->_timestamp);
    blockJson["txCount"] = boost::lexical_cast<std::string>(this->_txCount);
    blockJson["nHeight"] = boost::lexical_cast<std::string>(this->_nHeight);
    blockJson["blockData"] = this->_blockData;
    blockJson["transactions"] = json::array();
    for (auto tx : this->_transactions) {
        blockJson["transactions"].push_back(dev::toHex(tx.rlp()));
    }
    ret = blockJson.dump();
    this->_blockHash = dev::toHex(dev::sha3(ret));
    return ret;
}


bool Block::serializeFromString(std::string blockBytes) {
    json blockJson = json::parse(blockBytes);

    this->_prevBlockHash = blockJson["prevBlockHash"];
    this->_timestamp = boost::lexical_cast<dev::u256>(blockJson["timestamp"].get<std::string>());
    this->_txCount = boost::lexical_cast<dev::u256>(blockJson["txCount"].get<std::string>());
    this->_nHeight = boost::lexical_cast<dev::u256>(blockJson["nHeight"].get<std::string>());
    this->_blockData = blockJson["blockData"].get<std::string>();

    for (auto tx : blockJson["transactions"].items()) {
        dev::eth::TransactionBase transaction(dev::fromHex(tx.value()), dev::eth::CheckTransaction::Everything);
        this->_transactions.push_back(transaction);
    }

    this->_blockHash = dev::toHex(dev::sha3(blockJson.dump()));

    return true;
}

Block::Block(std::string blockBytes) {
    this->serializeFromString(blockBytes);
}

Block::Block(std::string __prevBlockHash, dev::u256 __timestamp, dev::u256 __txCount, dev::u256 __nHeight, std::string __blockData) {
    this->_prevBlockHash = __prevBlockHash;
    this->_timestamp = __timestamp;
    this->_txCount = __txCount;
    this->_nHeight = __nHeight;
    this->_blockData = __blockData;
    this->serializeToString();
}