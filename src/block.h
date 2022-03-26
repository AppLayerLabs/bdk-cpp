#ifndef BLOCK_H
#define BLOCK_H

#include <include/web3cpp/devcore/Common.h>
#include <include/web3cpp/ethcore/TransactionBase.h>

class Block {
    private:

        dev::u256 _blockHash;
        dev::u256 _prevBlockHash;
        dev::u256 _timestamp;
        dev::u256 _txCount;
        dev::u256 _merkleRootHash;
        dev::u256 _nHeight;
        // Data should be at max 32 bytes.
        dev::u256 _blockData;

    public:

        dev::u256 blockHash()       { return _blockHash; };
        dev::u256 prevBlockHash()   { return _prevBlockHash; };
        dev::u256 timestamp()       { return _timestamp; };
        dev::u256 txCount()         { return _txCount; };
        dev::u256 merkleRootHash()  { return _merkleRootHash; };
        dev::u256 nHeight()         { return _nHeight; };
        dev::u256 blockData()       { return _blockData; };

        // Serialize to protobuf string.
        std::string serializeToString();
        // Serialize from protobufstring.
        bool serializeFromString(std::string blockBytes);

        bool addTx(dev::eth::TransactionSkeleton tx);

        Block(std::string blockBytes);
};



#endif // BLOCK_H