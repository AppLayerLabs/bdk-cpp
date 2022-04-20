#include "validation.h"
#include "block.h"

Block genesis("0000000000000000000000000000000000000000000000000000000000000000",
  1648317800,
  0, // 0 tx's
  0,
  ""
);

Block Validation::initialize() {

    Utils::logToFile("validation nodeID: " + nodeID);
    blocksDb.setAndOpenDB(nodeID + "-blocks");
    accountsDb.setAndOpenDB(nodeID + "-balances");
    confirmedTxs.setAndOpenDB(nodeID + "-txs");
    nonceDb.setAndOpenDB(nodeID + "-nonces");
    txToBlock.setAndOpenDB(nodeID + "-txToBlocks");
    tokenDB.setAndOpenDB(nodeID + "-tokens");
    Utils::logToFile("DB created");
    if (blocksDb.isEmpty()) {
      blocksDb.putKeyValue(genesis.blockHash(), genesis.serializeToString());
      blocksDb.putKeyValue(boost::lexical_cast<std::string>(genesis.nHeight()), genesis.serializeToString());
      blocksDb.putKeyValue("latest", genesis.serializeToString());
    }
    if(accountsDb.isEmpty()) {
      accountsDb.putKeyValue("0xe6a2d1ef7d7129d2a422af0a725629a0a1fbdec4", "10000000000000000000000");
    }
    Block bestBlock(blocksDb.getKeyValue("latest"));
    Utils::logToFile("I think it is here...");
    ERC20::loadAllERC20(tokenDB, tokens);
    Uniswap::loadUniswap(uniswap, uniswapDB, tokens);
    Utils::logToFile("Not here?");

    return bestBlock;
}

void Validation::cleanAndClose() {
    Uniswap::saveUniswap(uniswap, uniswapDB);
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
      std::string fromBalanceStr = accountsDb.getKeyValue(from);
      std::string toBalanceStr = accountsDb.getKeyValue(to);
      dev::u256 fromBalance = 0;
      dev::u256 toBalance = 0;
      if (fromBalanceStr != "") {
        fromBalance = boost::lexical_cast<dev::u256>(fromBalanceStr);
      }
      if (toBalanceStr != "") {
        toBalance = boost::lexical_cast<dev::u256>(toBalanceStr);
      }
      dev::u256 transactionValue = tx.second.value();


      // Normal Transfer
      if (transactionValue != 0) {
        fromBalance = fromBalance - transactionValue;
        toBalance = toBalance + transactionValue;
  
        accountsDb.putKeyValue(from, boost::lexical_cast<std::string>(fromBalance));
        accountsDb.putKeyValue(to, boost::lexical_cast<std::string>(toBalance));
      }
      // ERC20 Transfer!
      if (tokens.count(to)) {
        std::string data = dev::toHex(tx.second.data());
        std::string abiSelector = data.substr(0,8);
        std::string abiStr = data.substr(abiSelector.size(),data.size());
        Utils::logToFile(std::string("ERC20 Transfer: ") + abiStr);
        std::vector<std::string> abi = Utils::parseHex(abiStr, {"address", "uint"});
        tokens[to]->transfer(from, abi[0], boost::lexical_cast<dev::u256>(abi[1]), true);
      }

      auto nonce = nonceDb.getKeyValue(from);
      if (nonce == "") {
        nonceDb.putKeyValue(from, boost::lexical_cast<std::string>(1));
      } else {
        std::string newNonce = boost::lexical_cast<std::string>(uint64_t(1 + boost::lexical_cast<uint64_t>(nonce)));
        nonceDb.putKeyValue(from, newNonce);
      }

      newBestBlock.addTx(tx.second);
      confirmedTxs.putKeyValue(tx.first, dev::toHex(tx.second.rlp()));
      transactionHexes.push_back(tx.first);
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
  std::string to = std::string("0x") + tx.to().hex();
  dev::u256 userBalance = boost::lexical_cast<dev::u256>(accountsDb.getKeyValue(from));
  auto txValue = tx.value();
  if (txValue > userBalance) {
    Utils::logToFile("validateTransaction: insuficient balance");
    return false;
  }

  if (tokens.count(to)) { // ERC20 Transfer!
    std::string data = dev::toHex(tx.data());
    std::string abiSelector = data.substr(0,8);
    std::string abiStr = data.substr(abiSelector.size(),data.size());
    Utils::logToFile(std::string("ERC20 Transfer Check: ") + abiStr);
    std::vector<std::string> abi = Utils::parseHex(abiStr, {"address", "uint"});
    bool result = tokens[to]->transfer(from, abi[0], boost::lexical_cast<dev::u256>(abi[1]), false);
    return result;
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

void Validation::createNewERC20(json &methods) {
  tokens[methods["address"].get<std::string>()] = std::make_shared<ERC20>(methods);
  return;
}