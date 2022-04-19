#include "validation.h"
#include "block.h"

Block genesis("0000000000000000000000000000000000000000000000000000000000000000",
  1648317800,
  0, // 0 tx's
  0,
  ""
);

Block Validation::initialize() {

    blocksDb.setAndOpenDB(nodeID + "-blocks");
    accountsDb.setAndOpenDB(nodeID + "-balances");
    confirmedTxs.setAndOpenDB(nodeID + "-txs");
    nonceDb.setAndOpenDB(nodeID + "-nonces");
    txToBlock.setAndOpenDB(nodeID + "-txToBlocks");
    if (blocksDb.isEmpty()) {
      blocksDb.putKeyValue(genesis.blockHash(), genesis.serializeToString());
      blocksDb.putKeyValue(boost::lexical_cast<std::string>(genesis.nHeight()), genesis.serializeToString());
      blocksDb.putKeyValue("latest", genesis.serializeToString());
    }
    if(accountsDb.isEmpty()) {
      accountsDb.putKeyValue("0xcc95a9aad79c390167cd59b951d3e43d959bf2c4", "10000000000000000000000");
    }
    Block bestBlock(blocksDb.getKeyValue("latest"));

    return bestBlock;
}

void Validation::cleanAndClose() {
    blocksDb.cleanCloseDB();
    confirmedTxs.cleanCloseDB();
    txToBlock.cleanCloseDB();
    accountsDb.cleanCloseDB();
    nonceDb.cleanCloseDB();
}