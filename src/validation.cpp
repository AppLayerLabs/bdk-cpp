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
    tokenDB.setAndOpenDB(nodeID + "-tokens");
    if (blocksDb.isEmpty()) {
      blocksDb.putKeyValue(genesis.blockHash(), genesis.serializeToString());
      blocksDb.putKeyValue(boost::lexical_cast<std::string>(genesis.nHeight()), genesis.serializeToString());
      blocksDb.putKeyValue("latest", genesis.serializeToString());
    }
    if(accountsDb.isEmpty()) {
      accountsDb.putKeyValue("0xcc95a9aad79c390167cd59b951d3e43d959bf2c4", "10000000000000000000000");
    }
    Block bestBlock(blocksDb.getKeyValue("latest"));

    this->tokens = ERC20::loadAllERC20(tokenDB);

    return bestBlock;
}

void Validation::cleanAndClose() {
    ERC20::saveAllERC20(tokens, tokenDB);
    blocksDb.cleanCloseDB();
    confirmedTxs.cleanCloseDB();
    txToBlock.cleanCloseDB();
    accountsDb.cleanCloseDB();
    nonceDb.cleanCloseDB();
    tokenDB.cleanCloseDB();
}

Block Validation::createNewBlock() {
Block pastBlock(blocksDb.getKeyValue("latest"));
    const auto p1 = std::chrono::system_clock::now();
    uint64_t now = std::chrono::duration_cast<std::chrono::seconds>(p1.time_since_epoch()).count();
    auto nextHeight = pastBlock.nHeight() + 1;
    Block newBestBlock(pastBlock.blockHash(),
                       boost::lexical_cast<dev::u256>(now),
                       0,
                       nextHeight,
                       "");
    std::vector<std::string> transactionHexes;
    lock.lock();
    for (auto tx : mempool) {
      // Save changes to DB.
      std::string from = std::string("0x") + tx.second.from().hex();
      std::string to   = std::string("0x") + tx.second.to().hex();
      dev::u256 fromBalance = boost::lexical_cast<dev::u256>(accountsDb.getKeyValue(from));
      dev::u256 toBalance = boost::lexical_cast<dev::u256>(accountsDb.getKeyValue(to));
      dev::u256 transactionValue = tx.second.value();
      fromBalance = fromBalance - transactionValue;
      toBalance = toBalance + transactionValue;

      accountsDb.putKeyValue(from, boost::lexical_cast<std::string>(fromBalance));
      accountsDb.putKeyValue(to, boost::lexical_cast<std::string>(toBalance));

      newBestBlock.addTx(tx.second);
      confirmedTxs.putKeyValue(std::string("0x") + tx.first, dev::toHex(tx.second.rlp()));
      transactionHexes.push_back(std::string("0x") + tx.first);
    }
    // Hash blockheader again
    newBestBlock.serializeToString();
    
    for (auto tx : transactionHexes) {
      txToBlock.putKeyValue(tx, std::string("0x") + newBestBlock.blockHash());
    }

    // All transaction added, clear mempool.
    mempool.clear();

    blocksDb.deleteKeyValue("latest");
    blocksDb.putKeyValue(std::string("0x") + newBestBlock.blockHash(), newBestBlock.serializeToString());
    blocksDb.putKeyValue(boost::lexical_cast<std::string>(newBestBlock.nHeight()), newBestBlock.serializeToString());
    blocksDb.putKeyValue("latest", newBestBlock.serializeToString());
    lock.unlock();
    return newBestBlock;
}

Block Validation::getBlock(std::string blockKey) {
  return Block(blocksDb.getKeyValue(blockKey));
}

Block Validation::getLatestBlock() {
  return Block(blocksDb.getKeyValue("latest"));
}

std::string Validation::getAccountBalanceFromDB(std::string address) {
  return accountsDb.getKeyValue(address);
}

std::string Validation::getAccountNonce(std::string address) {
  return nonceDb.getKeyValue(address);  
}

std::string Validation::getConfirmedTx(std::string txHash) {
  return confirmedTxs.getKeyValue(txHash);
}

std::string Validation::getTxToBlock(std::string txHash) {
  return txToBlock.getKeyValue(txHash);
}

void Validation::faucet(std::string address) {
  std::string prevBal = accountsDb.getKeyValue(address);
  dev::u256 bal = 1000000000000000000;
  if (prevBal != "") {
    bal = bal + boost::lexical_cast<dev::u256>(prevBal);
  }
  accountsDb.putKeyValue(address, boost::lexical_cast<std::string>(bal));
}

bool Validation::validateTransaction(dev::eth::TransactionBase tx) {
  std::string from = std::string("0x") + tx.from().hex();
  dev::u256 userBalance = boost::lexical_cast<dev::u256>(accountsDb.getKeyValue(from));
  auto txValue = tx.value();
  if (txValue > userBalance) {
    Utils::logToFile("validateTransaction: insuficient balance");
    return false;
  }

  return true;
}

bool Validation::addTxToMempool(dev::eth::TransactionBase tx) {
  if (!validateTransaction(tx)) {
    return false;
  }
  std::string txHex = std::string("0x") + tx.sha3().hex();

  if (mempool.count(txHex)) {
    Utils::logToFile("Transaction already exists");
    return false;
  }

  lock.lock();
  mempool[txHex] = tx;
  lock.unlock();

  return true;
}