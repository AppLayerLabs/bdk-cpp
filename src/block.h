#ifndef BLOCK_H
#define BLOCK_H

#include <include/web3cpp/devcore/Common.h>
#include <include/web3cpp/ethcore/TransactionBase.h>
#include "utils.h"

class Block {
    private:

        // Block header == 224 Bytes.
        // _blockHash itself is not incuded in header
        std::string _blockHash;
        // HEADER
        std::string _prevBlockHash; // Size == 32
        dev::u256 _timestamp;       // Size == 48
        dev::u256 _txCount;         // Size == 48
        dev::u256 _nHeight;         // Size == 48
        // Data should be at max 32 bytes.
        dev::u256 _blockData;       // Size == 48
        // HEADER END
        // Block contents.
        std::vector<dev::eth::TransactionBase> _transactions;

    public:

        std::string blockHash()       { return _blockHash; };
        std::string prevBlockHash()   { return _prevBlockHash; };
        dev::u256 timestamp()       { return _timestamp; };
        dev::u256 txCount()         { return _txCount; };
        dev::u256 nHeight()         { return _nHeight; };
        dev::u256 blockData()       { return _blockData; };
        std::vector<dev::eth::TransactionBase> transactions() { return _transactions; };

        // Serialize to protobuf string.
        // serializeToString() also creates the blockhash! so we call it for both purposes.
        std::string serializeToString();
        // Serialize from protobufstring.
        bool serializeFromString(std::string blockBytes);

        bool addTx(dev::eth::TransactionBase tx);

        void submitBlock();

        Block(std::string blockBytes);

        Block(std::string __prevBlockHash, dev::u256 __timestamp, dev::u256 __txCount, dev::u256 __nHeight, dev::u256 __blockData);
};



#endif // BLOCK_H