#ifndef BLOCK_H
#define BLOCK_H

#include <include/web3cpp/devcore/Common.h>
#include <include/web3cpp/ethcore/TransactionBase.h>
#include "utils.h"
#include "json.hpp"

// the json is used to encode the block to a string of bytes
// Order is extremely important to make this work!!!

using json = nlohmann::ordered_json;

class Block {
    private:
        std::string _blockHash;
        std::string _prevBlockHash;
        dev::u256 _timestamp;      
        dev::u256 _txCount;        
        dev::u256 _nHeight;        
        std::string _blockData;
        std::vector<dev::eth::TransactionBase> _transactions;

    public:

        std::string blockHash()       { return _blockHash; };
        std::string prevBlockHash()   { return _prevBlockHash; };
        dev::u256 timestamp()       { return _timestamp; };
        dev::u256 txCount()         { return _txCount; };
        dev::u256 nHeight()         { return _nHeight; };
        std::string blockData()       { return _blockData; };
        std::vector<dev::eth::TransactionBase> transactions() { return _transactions; };

        // Serialize to protobuf string.
        // Our block information is actually a json which contains the following:
        // {
        //    prevBlockHash : ""
        //    timestamp : ""
        //    txCount : ""
        //    nHeight : ""
        //    blockData : ""
        //    transactions : [
        //       "rlphash"
        //      }
        //    }
        // serializeToString() also creates the blockhash! so we call it for both purposes.
        std::string serializeToString();
        // Serialize from protobufstring.
        bool serializeFromString(std::string blockBytes);

        bool addTx(dev::eth::TransactionBase tx);

        void submitBlock();

        Block(std::string blockBytes);

        Block(std::string __prevBlockHash, dev::u256 __timestamp, dev::u256 __txCount, dev::u256 __nHeight, std::string __blockData);
};



#endif // BLOCK_H