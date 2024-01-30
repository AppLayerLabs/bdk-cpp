/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include <iostream>
#include <filesystem>

#include "src/core/blockchain.h"

std::unique_ptr<Blockchain> blockchain = nullptr;

void signalHandler(int signum) {
  Logger::logToDebug(LogType::INFO, "MAIN", "MAIN", "Received signal " + std::to_string(signum) + ". Stopping the blockchain.");
  blockchain->stop();
  blockchain = nullptr; // Destroy the blockchain object, calling the destructor of every module and dumping to DB.
  Utils::safePrint("Exiting...");
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  exit(signum);
}

void initialize(std::unique_ptr<Options>& options,
                std::unique_ptr<DB>& db,
                std::unique_ptr<ContractManager> &contractManager,
                const std::string& dbName,
                const PrivKey& ownerPrivKey,
                const std::string& tokenName,
                const std::string& tokenSymbol,
                const uint8_t& tokenDecimals,
                const uint256_t& tokenSupply,
                bool deleteDB = true) {
  if (deleteDB) {
    if (std::filesystem::exists(dbName)) {
      std::filesystem::remove_all(dbName);
    }
  }

  options = std::make_unique<Options>(Options::fromFile(dbName));
  db = std::make_unique<DB>(dbName);
  std::unique_ptr<rdPoS> rdpos;
  contractManager = std::make_unique<ContractManager>(nullptr, db, rdpos, options);

  if (deleteDB) {
    /// Create the contract.

    Bytes createNewERC20ContractEncoder = ABI::Encoder::encodeData(tokenName, tokenSymbol, tokenDecimals, tokenSupply);
    Bytes createNewERC20ContractData = Hex::toBytes("0xb74e5ed5");
    Utils::appendBytes(createNewERC20ContractData, createNewERC20ContractEncoder);

    TxBlock createNewERC2OTx = TxBlock(
      ProtocolContractAddresses.at("ContractManager"),
      Secp256k1::toAddress(Secp256k1::toUPub(ownerPrivKey)),
      createNewERC20ContractData,
      8080,
      0,
      0,
      0,
      0,
      0,
      ownerPrivKey
    );

    contractManager->callContract(createNewERC2OTx);
  }
}



int main() {
//
  //uint256_t i = 300;
  //auto iInBytes = Utils::uint256ToBytes(i);
  //auto iFromBytes = Utils::bytesToUint256(iInBytes);
//
  //std::cout << "i: " << i << std::endl;
  //std::cout << "iInBytes: " << Hex::fromBytes(iInBytes) << std::endl;
  //std::cout << "iFromBytes: " << iFromBytes << std::endl;
  //std::cout << "Bytes: " << Hex::fromBytes(Utils::uint256ToBytes(iFromBytes)) << std::endl;
//
//
  //uint256_t hugeI("54851124499604551173541613414445985997851774274368406963454445803903751445765");
//
  //auto hugeIInBytes = Utils::uint256ToBytes(hugeI);
  //auto hugeIFromBytes = Utils::bytesToUint256(hugeIInBytes);
//
  //std::cout << "hugeI: " << hugeI << std::endl;
  //std::cout << "hugeIInBytes: " << Hex::fromBytes(hugeIInBytes) << std::endl;
  //std::cout << "hugeIFromBytes: " << hugeIFromBytes << std::endl;
  //std::cout << "Bytes: " << Hex::fromBytes(Utils::uint256ToBytes(hugeIFromBytes)) << std::endl;
//
  //std::cout << "limbs size: " << hugeI.backend().size() << std::endl;
//
//
  //return 0;
  std::cout << "Uh" << std::endl;
  std::string testDumpPath = Utils::getTestDumpPath();
  PrivKey ownerPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
  Address owner = Secp256k1::toAddress(Secp256k1::toUPub(ownerPrivKey));
  Address erc20Address;
  Address destinationOfTransactions(Utils::randBytes(20));

  std::unique_ptr<Options> options;
  std::unique_ptr<DB> db;
  std::unique_ptr<ContractManager> contractManager;
  std::string dbName = testDumpPath + "/erc20ClassTransfer";
  std::string tokenName = "TestToken";
  std::string tokenSymbol = "TST";
  uint8_t tokenDecimals = 18;
  uint256_t tokenSupply = 1000000000000000000;
  initialize(options, db, contractManager, dbName, ownerPrivKey, tokenName, tokenSymbol, tokenDecimals,
             tokenSupply);

  erc20Address = contractManager->getContracts()[0].second;

  Bytes transferData = Hex::toBytes("0xa9059cbb");

  Bytes transferEncoder = ABI::Encoder::encodeData(destinationOfTransactions, static_cast<uint256_t>(500));
  Utils::appendBytes(transferData, transferEncoder);
  TxBlock transferTx(
    erc20Address,
    owner,
    transferData,
    8080,
    0,
    0,
    0,
    0,
    0,
    ownerPrivKey
  );

  uint64_t iterations = 10000000;
  auto start = std::chrono::high_resolution_clock::now();

  for (uint64_t i = 0; i < iterations; i++) {
    contractManager->callContract(transferTx);
  }

  auto end = std::chrono::high_resolution_clock::now();
  double durationInMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  double microsecondsPerIterations = durationInMicroseconds / iterations;
  std::cout << "Duration: " << durationInMicroseconds << " microseconds" << std::endl;
  std::cout << "Iterations: " << iterations << std::endl;
  std::cout << "Microseconds per iteration: " << microsecondsPerIterations << std::endl;



  return 0;
  Utils::logToCout = true;
  std::string blockchainPath = std::filesystem::current_path().string() + std::string("/blockchain");
  blockchain = std::make_unique<Blockchain>(blockchainPath);
  // Start the blockchain syncing engine.
  std::signal(SIGINT, signalHandler);
  std::signal(SIGHUP, signalHandler);
  blockchain->start();
  std::this_thread::sleep_until(std::chrono::system_clock::now() + std::chrono::hours(std::numeric_limits<int>::max()));
  return 0;
}

