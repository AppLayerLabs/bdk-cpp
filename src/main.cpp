#include <iostream>
#include "src/core/blockchain.h"
#include <filesystem>

/// Forward Decleration.
ethCallInfoAllocated buildCallInfo(const Address& addressToCall, const Functor& function, const Bytes& dataToCall) {
  ethCallInfoAllocated callInfo;
  auto& [from, to, gasLimit, gasPrice, value, functor, data] = callInfo;
  to = addressToCall;
  functor = function;
  data = dataToCall;
  return callInfo;
}

void initialize(std::unique_ptr<Options>& options,
                std::unique_ptr<DB>& db,
                std::unique_ptr<ContractManager> &contractManager,
                const std::string& dbName,
                const PrivKey& ownerPrivKey,
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
    ABI::Encoder::EncVar createNewERC20ContractVars;
    createNewERC20ContractVars.push_back("TestToken");
    createNewERC20ContractVars.push_back("TST");
    createNewERC20ContractVars.push_back(18);
    createNewERC20ContractVars.push_back(1000000000000000000);
    ABI::Encoder createNewERC20ContractEncoder(createNewERC20ContractVars);
    Bytes createNewERC20ContractData = Hex::toBytes("0xb74e5ed5");
    Utils::appendBytes(createNewERC20ContractData, createNewERC20ContractEncoder.getData());

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

    TxBlock createNewERC20Wrapper = TxBlock(
      ProtocolContractAddresses.at("ContractManager"),
      Secp256k1::toAddress(Secp256k1::toUPub(ownerPrivKey)),
      Hex::toBytes("0x97aa51a3"),
      8080,
      0,
      0,
      0,
      0,
      0,
      ownerPrivKey
    );

    contractManager->callContract(createNewERC20Wrapper);
  }
}


int main() {
  PrivKey ownerPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
  Address owner = Secp256k1::toAddress(Secp256k1::toUPub(ownerPrivKey));
  Address erc20Address;
  Address wrapperAddress;
  {
    std::unique_ptr<Options> options;
    std::unique_ptr<DB> db;
    std::unique_ptr<ContractManager> contractManager;
    std::string dbName = "erc20wrapperDb";
    initialize(options, db, contractManager, dbName, ownerPrivKey);
    for (const auto &[name, address]: contractManager->getContracts()) {
      if (name == "ERC20") {
        erc20Address = address;
      }
      if (name == "ERC20Wrapper") {
        wrapperAddress = address;
      }
    }

    ABI::Encoder::EncVar getAllowanceVars;
    getAllowanceVars.push_back(owner);
    getAllowanceVars.push_back(wrapperAddress);
    ABI::Encoder getAllowanceEncoder(getAllowanceVars, "allowance(address,address)");

    ABI::Encoder::EncVar depositVars;
    depositVars.push_back(erc20Address);
    depositVars.push_back(500000000000000000);
    ABI::Encoder depositEncoder(depositVars);
    Bytes depositData = Hex::toBytes("0x47e7ef24");
    Utils::appendBytes(depositData, depositEncoder.getData());
    TxBlock depositTx(
      wrapperAddress,
      owner,
      depositData,
      8080,
      0,
      0,
      0,
      0,
      0,
      ownerPrivKey
    );

    // REQUIRE_THROWS(contractManager->callContract(depositTx));

    ABI::Encoder::EncVar approveVars;
    approveVars.push_back(wrapperAddress);
    approveVars.push_back(500000000000000000);
    ABI::Encoder approveEncoder(approveVars);
    Bytes approveData = Hex::toBytes("0x095ea7b3");
    Utils::appendBytes(approveData, approveEncoder.getData());
    TxBlock approveTx(
      erc20Address,
      owner,
      approveData,
      8080,
      0,
      0,
      0,
      0,
      0,
      ownerPrivKey
    );

    contractManager->callContract(approveTx);

    Bytes getAllowanceResult = contractManager->callContract(
      buildCallInfo(erc20Address, getAllowanceEncoder.getFunctor(), getAllowanceEncoder.getData()));
    ABI::Decoder getAllowanceDecoder({ABI::Types::uint256}, getAllowanceResult);
    if (getAllowanceDecoder.getData<uint256_t>(0) != 500000000000000000) { throw std::runtime_error("Allowance is not correct 1"); }

    contractManager->callContract(depositTx);
    Bytes getAllowanceResult2 = contractManager->callContract(
      buildCallInfo(erc20Address, getAllowanceEncoder.getFunctor(), getAllowanceEncoder.getData()));
    ABI::Decoder getAllowanceDecoder2({ABI::Types::uint256}, getAllowanceResult2);
    if (getAllowanceDecoder2.getData<uint256_t>(0) != 0) { throw std::runtime_error("Allowance is not correct 2") ;}


    ABI::Encoder getContractBalanceEncoder({erc20Address}, "getContractBalance(address)");
    Bytes getContractBalanceResult = contractManager->callContract(
      buildCallInfo(wrapperAddress, getContractBalanceEncoder.getFunctor(), getContractBalanceEncoder.getData()));
    ABI::Decoder getContractBalanceDecoder({ABI::Types::uint256}, getContractBalanceResult);
    if (getContractBalanceDecoder.getData<uint256_t>(0) != 500000000000000000) { throw std::runtime_error("Contract balance is not correct 1");};

    ABI::Encoder getUserBalanceEncoder({erc20Address, owner}, "getUserBalance(address,address)");
    Bytes getUserBalanceResult = contractManager->callContract(
      buildCallInfo(wrapperAddress, getUserBalanceEncoder.getFunctor(), getUserBalanceEncoder.getData()));
    ABI::Decoder getUserBalanceDecoder({ABI::Types::uint256}, getUserBalanceResult);
    if (getUserBalanceDecoder.getData<uint256_t>(0) != 500000000000000000) { throw std::runtime_error("User balance is not correct 1"); };

    ABI::Encoder getBalanceEncoder({owner}, "balanceOf(address)");
    Bytes getBalanceResult = contractManager->callContract(
      buildCallInfo(erc20Address, getBalanceEncoder.getFunctor(), getBalanceEncoder.getData()));
    ABI::Decoder getBalanceDecoder({ABI::Types::uint256}, getBalanceResult);
    if (getBalanceDecoder.getData<uint256_t>(0) != 500000000000000000) { throw std::runtime_error("Balance is not correct 1"); };

    ABI::Encoder getBalanceWrapperEncoder({wrapperAddress}, "balanceOf(address)");
    Bytes getBalanceWrapperResult = contractManager->callContract(
      buildCallInfo(erc20Address, getBalanceWrapperEncoder.getFunctor(), getBalanceWrapperEncoder.getData()));
    ABI::Decoder getBalanceWrapperDecoder({ABI::Types::uint256}, getBalanceWrapperResult);
    if (getBalanceWrapperDecoder.getData<uint256_t>(0) != 500000000000000000) { throw std::runtime_error("Balance is not correct 2"); };

  }
  std::unique_ptr<Options> options;
  std::unique_ptr<DB> db;
  std::unique_ptr<ContractManager> contractManager;
  std::string dbName = "erc20wrapperDb";
  initialize(options, db, contractManager, dbName, ownerPrivKey, false);

  ABI::Encoder::EncVar getAllowanceVars;
  getAllowanceVars.push_back(owner);
  getAllowanceVars.push_back(wrapperAddress);
  ABI::Encoder getAllowanceEncoder(getAllowanceVars, "allowance(address,address)");
  std::cout << "erc20Address: " << erc20Address.hex() << std::endl;
  std::cout << "wrapperAddress: " << wrapperAddress.hex() << std::endl;

  Bytes getAllowanceResult = contractManager->callContract(buildCallInfo(erc20Address, getAllowanceEncoder.getFunctor(), getAllowanceEncoder.getData()));
  ABI::Decoder getAllowanceDecoder({ABI::Types::uint256}, getAllowanceResult);
  if (getAllowanceDecoder.getData<uint256_t>(0) != 0) { throw std::runtime_error("Allowance is not correct 3"); };

  ABI::Encoder getContractBalanceEncoder({erc20Address}, "getContractBalance(address)");
  Bytes getContractBalanceResult = contractManager->callContract(buildCallInfo(wrapperAddress, getContractBalanceEncoder.getFunctor(), getContractBalanceEncoder.getData()));
  ABI::Decoder getContractBalanceDecoder({ABI::Types::uint256}, getContractBalanceResult);
  if (getContractBalanceDecoder.getData<uint256_t>(0) != 500000000000000000) { throw std::runtime_error("Contract balance is not correct 2"); };

  ABI::Encoder getUserBalanceEncoder({erc20Address, owner}, "getUserBalance(address,address)");
  Bytes getUserBalanceResult = contractManager->callContract(buildCallInfo(wrapperAddress, getUserBalanceEncoder.getFunctor(), getUserBalanceEncoder.getData()));
  ABI::Decoder getUserBalanceDecoder({ABI::Types::uint256}, getUserBalanceResult);
  if (getUserBalanceDecoder.getData<uint256_t>(0) != 500000000000000000) { throw std::runtime_error("User balance is not correct 2"); };

  ABI::Encoder getBalanceEncoder({owner}, "balanceOf(address)");
  Bytes getBalanceResult = contractManager->callContract(buildCallInfo(erc20Address, getBalanceEncoder.getFunctor(), getBalanceEncoder.getData()));
  ABI::Decoder getBalanceDecoder({ABI::Types::uint256}, getBalanceResult);
  if(getBalanceDecoder.getData<uint256_t>(0) != 500000000000000000) { throw std::runtime_error("Balance is not correct 3"); };

  ABI::Encoder getBalanceWrapperEncoder({wrapperAddress}, "balanceOf(address)");
  Bytes getBalanceWrapperResult = contractManager->callContract(buildCallInfo(erc20Address, getBalanceWrapperEncoder.getFunctor(), getBalanceWrapperEncoder.getData()));
  ABI::Decoder getBalanceWrapperDecoder({ABI::Types::uint256}, getBalanceWrapperResult);
  if (getBalanceWrapperDecoder.getData<uint256_t>(0) != 500000000000000000) throw std::runtime_error("Balance is not correct 4");
  return 0;
  Utils::logToCout = true;
  std::string blockchainPath = std::filesystem::current_path().string() + std::string("/blockchain");
  Blockchain blockchain(blockchainPath);
  /// Start the blockchain syncing engine.
  blockchain.start();
  std::this_thread::sleep_for(std::chrono::hours(1000000));
  return 0;
}