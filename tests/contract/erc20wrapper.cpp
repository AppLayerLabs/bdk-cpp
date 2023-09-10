/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/templates/erc20.h"
#include "../../src/contract/abi.h"
#include "../../src/utils/db.h"
#include "../../src/utils/options.h"
#include "../../src/contract/contractmanager.h"
#include "../../src/core/rdpos.h"

#include <filesystem>
#include <string>

// Forward declaration.
ethCallInfoAllocated buildCallInfo(const Address& addressToCall, const Functor& function, const Bytes& dataToCall);

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

    Bytes createNewERC20ContractEncoder = ABI::Encoder::encodeData(std::string("TestToken"), std::string("TST"), 18, 1000000000000000000);
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

namespace TERC20Wrapper {
  std::string testDumpPath = Utils::getTestDumpPath();
  TEST_CASE("ERC20Wrapper Class", "[contract][erc20wrapper]") {
    PrivKey ownerPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
    Address owner = Secp256k1::toAddress(Secp256k1::toUPub(ownerPrivKey));
    SECTION("ERC20Wrapper Constructor Test") {
      Address erc20Address;
      Address wrapperAddress;
      {
        std::unique_ptr<Options> options;
        std::unique_ptr<DB> db;
        std::unique_ptr<ContractManager> contractManager;
        std::string dbName = testDumpPath + "/erc20wrapperDb";
        initialize(options, db, contractManager, dbName, ownerPrivKey);
        for (const auto& [name, address] : contractManager->getContracts()) {
          if (name == "ERC20") {
            erc20Address = address;
          }
          if (name == "ERC20Wrapper") {
            wrapperAddress = address;
          }
        }
      }
      std::unique_ptr<Options> options;
      std::unique_ptr<DB> db;
      std::unique_ptr<ContractManager> contractManager;
      std::string dbName = testDumpPath + "/erc20wrapperDb";
      initialize(options, db, contractManager, dbName, ownerPrivKey, false);

      for (const auto& [name, address] : contractManager->getContracts()) {
        if (name == "ERC20") {
          REQUIRE(erc20Address == address);
        }
        if (name == "ERC20Wrapper") {
          REQUIRE(wrapperAddress == address);
        }
      }
    }

    SECTION("ERC20Wrapper Deposit") {
      Address erc20Address;
      Address wrapperAddress;
      {
        std::unique_ptr<Options> options;
        std::unique_ptr<DB> db;
        std::unique_ptr<ContractManager> contractManager;
        std::string dbName = testDumpPath + "/erc20wrapperDb";
        initialize(options, db, contractManager, dbName, ownerPrivKey);
        for (const auto &[name, address]: contractManager->getContracts()) {
          if (name == "ERC20") {
            erc20Address = address;
          }
          if (name == "ERC20Wrapper") {
            wrapperAddress = address;
          }
        }

        Bytes getAllowanceEncoder = ABI::Encoder::encodeData(owner, wrapperAddress);
        Functor getAllowanceFunctor = ABI::Encoder::encodeFunction("allowance(address,address)");

        Bytes depositEncoder = ABI::Encoder::encodeData(erc20Address, static_cast<uint256_t>(500000000000000000));
        Bytes depositData = Hex::toBytes("0x47e7ef24");
        Utils::appendBytes(depositData, depositEncoder);
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

        // Try to deposit without approving first.
        REQUIRE_THROWS(contractManager->callContract(depositTx));

        Bytes approveEncoder = ABI::Encoder::encodeData(wrapperAddress, static_cast<uint256_t>(500000000000000000));
        Bytes approveData = Hex::toBytes("0x095ea7b3");
        Utils::appendBytes(approveData, approveEncoder);
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
          buildCallInfo(erc20Address, getAllowanceFunctor, getAllowanceEncoder));

        auto getAllowanceDecoder = ABI::Decoder::decodeData<uint256_t>(getAllowanceResult);
        REQUIRE(std::get<0>(getAllowanceDecoder) == 500000000000000000);

        contractManager->callContract(depositTx);
        Bytes getAllowanceResult2 = contractManager->callContract(
          buildCallInfo(erc20Address, getAllowanceFunctor, getAllowanceEncoder));

        auto getAllowanceDecoder2 = ABI::Decoder::decodeData<uint256_t>(getAllowanceResult2);
        REQUIRE(std::get<0>(getAllowanceDecoder2) == 0);

        Bytes getContractBalanceEncoder = ABI::Encoder::encodeData(erc20Address);
        Functor getContractBalanceFunctor = ABI::Encoder::encodeFunction("getContractBalance(address)");

        Bytes getContractBalanceResult = contractManager->callContract(
          buildCallInfo(wrapperAddress, getContractBalanceFunctor, getContractBalanceEncoder));

        auto getContractBalanceDecoder = ABI::Decoder::decodeData<uint256_t>(getContractBalanceResult);
        REQUIRE(std::get<0>(getContractBalanceDecoder) == 500000000000000000);

        Bytes getUserBalanceEncoder = ABI::Encoder::encodeData(erc20Address, owner);
        Functor getUserBalanceFunctor = ABI::Encoder::encodeFunction("getUserBalance(address,address)");

        Bytes getUserBalanceResult = contractManager->callContract(
          buildCallInfo(wrapperAddress, getUserBalanceFunctor, getUserBalanceEncoder));

        auto getUserBalanceDecoder = ABI::Decoder::decodeData<uint256_t>(getUserBalanceResult);
        REQUIRE(std::get<0>(getUserBalanceDecoder) == 500000000000000000);

        Bytes getBalanceEncoder = ABI::Encoder::encodeData(owner);
        Functor getBalanceFunctor = ABI::Encoder::encodeFunction("balanceOf(address)");

        Bytes getBalanceResult = contractManager->callContract(
          buildCallInfo(erc20Address, getBalanceFunctor, getBalanceEncoder));

        auto getBalanceDecoder = ABI::Decoder::decodeData<uint256_t>(getBalanceResult);
        REQUIRE(std::get<0>(getBalanceDecoder) == 500000000000000000);

        Bytes getBalanceWrapperEncoder = ABI::Encoder::encodeData(wrapperAddress);
        Functor getBalanceWrapperFunctor = ABI::Encoder::encodeFunction("balanceOf(address)");

        Bytes getBalanceWrapperResult = contractManager->callContract(
          buildCallInfo(erc20Address, getBalanceWrapperFunctor, getBalanceWrapperEncoder));

        auto getBalanceWrapperDecoder = ABI::Decoder::decodeData<uint256_t>(getBalanceWrapperResult);
        REQUIRE(std::get<0>(getBalanceWrapperDecoder) == 500000000000000000);

      }
      std::unique_ptr<Options> options;
      std::unique_ptr<DB> db;
      std::unique_ptr<ContractManager> contractManager;
      std::string dbName = testDumpPath + "/erc20wrapperDb";
      initialize(options, db, contractManager, dbName, ownerPrivKey, false);

      Bytes getAllowanceEncoder = ABI::Encoder::encodeData(owner, wrapperAddress);
      Functor getAllowanceFunctor = ABI::Encoder::encodeFunction("allowance(address,address)");

      Bytes getAllowanceResult = contractManager->callContract(buildCallInfo(erc20Address, getAllowanceFunctor, getAllowanceEncoder));

      auto getAllowanceDecoder = ABI::Decoder::decodeData<uint256_t>(getAllowanceResult);
      REQUIRE(std::get<0>(getAllowanceDecoder) == 0);

      Bytes getContractBalanceEncoder = ABI::Encoder::encodeData(erc20Address);
      Functor getContractBalanceFunctor = ABI::Encoder::encodeFunction("getContractBalance(address)");

      Bytes getContractBalanceResult = contractManager->callContract(buildCallInfo(wrapperAddress, getContractBalanceFunctor, getContractBalanceEncoder));

      auto getContractBalanceDecoder = ABI::Decoder::decodeData<uint256_t>(getContractBalanceResult);
      REQUIRE(std::get<0>(getContractBalanceDecoder) == 500000000000000000);

      Bytes getUserBalanceEncoder = ABI::Encoder::encodeData(erc20Address, owner);
      Functor getUserBalanceFunctor = ABI::Encoder::encodeFunction("getUserBalance(address,address)");

      Bytes getUserBalanceResult = contractManager->callContract(buildCallInfo(wrapperAddress, getUserBalanceFunctor, getUserBalanceEncoder));

      auto getUserBalanceDecoder = ABI::Decoder::decodeData<uint256_t>(getUserBalanceResult);
      REQUIRE(std::get<0>(getUserBalanceDecoder) == 500000000000000000);

      Bytes getBalanceEncoder = ABI::Encoder::encodeData(owner);
      Functor getBalanceFunctor = ABI::Encoder::encodeFunction("balanceOf(address)");

      Bytes getBalanceResult = contractManager->callContract(buildCallInfo(erc20Address, getBalanceFunctor, getBalanceEncoder));

      auto getBalanceDecoder = ABI::Decoder::decodeData<uint256_t>(getBalanceResult);
      REQUIRE(std::get<0>(getBalanceDecoder) == 500000000000000000);

      Bytes getBalanceWrapperEncoder = ABI::Encoder::encodeData(wrapperAddress);
      Functor getBalanceWrapperFunctor = ABI::Encoder::encodeFunction("balanceOf(address)");

      Bytes getBalanceWrapperResult = contractManager->callContract(buildCallInfo(erc20Address, getBalanceWrapperFunctor, getBalanceWrapperEncoder));

      auto getBalanceWrapperDecoder = ABI::Decoder::decodeData<uint256_t>(getBalanceWrapperResult);
      REQUIRE(std::get<0>(getBalanceWrapperDecoder) == 500000000000000000);
    }

    SECTION("ERC20Wrapper Withdraw") {
      Address erc20Address;
      Address wrapperAddress;
      {
        std::unique_ptr<Options> options;
        std::unique_ptr<DB> db;
        std::unique_ptr<ContractManager> contractManager;
        std::string dbName = testDumpPath + "/erc20wrapperDb";
        initialize(options, db, contractManager, dbName, ownerPrivKey);
        for (const auto &[name, address]: contractManager->getContracts()) {
          if (name == "ERC20") {
            erc20Address = address;
          }
          if (name == "ERC20Wrapper") {
            wrapperAddress = address;
          }
        }

        Bytes getAllowanceEncoder = ABI::Encoder::encodeData(owner, wrapperAddress);
        Functor getAllowanceFunctor = ABI::Encoder::encodeFunction("allowance(address,address)");

        Bytes depositData = Hex::toBytes("0x47e7ef24");
        Bytes depositEncoder = ABI::Encoder::encodeData(erc20Address, static_cast<uint256_t>(500000000000000000));
        Utils::appendBytes(depositData, depositEncoder);
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

        REQUIRE_THROWS(contractManager->callContract(depositTx));

        Bytes approveEncoder = ABI::Encoder::encodeData(wrapperAddress, static_cast<uint256_t>(500000000000000000));
        Bytes approveData = Hex::toBytes("0x095ea7b3");
        Utils::appendBytes(approveData, approveEncoder);
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
          buildCallInfo(erc20Address, getAllowanceFunctor, getAllowanceEncoder));

        auto getAllowanceDecoder = ABI::Decoder::decodeData<uint256_t>(getAllowanceResult);
        REQUIRE(std::get<0>(getAllowanceDecoder) == 500000000000000000);

        contractManager->callContract(depositTx);
        Bytes getAllowanceResult2 = contractManager->callContract(
          buildCallInfo(erc20Address, getAllowanceFunctor, getAllowanceEncoder));

        auto getAllowanceDecoder2 = ABI::Decoder::decodeData<uint256_t>(getAllowanceResult2);
        REQUIRE(std::get<0>(getAllowanceDecoder2) == 0);

        Bytes getContractBalanceEncoder = ABI::Encoder::encodeData(erc20Address);
        Functor getContractBalanceFunctor = ABI::Encoder::encodeFunction("getContractBalance(address)");

        Bytes getContractBalanceResult = contractManager->callContract(
          buildCallInfo(wrapperAddress, getContractBalanceFunctor, getContractBalanceEncoder));

        auto getContractBalanceDecoder = ABI::Decoder::decodeData<uint256_t>(getContractBalanceResult);
        REQUIRE(std::get<0>(getContractBalanceDecoder) == 500000000000000000);

        Bytes getUserBalanceEncoder = ABI::Encoder::encodeData(erc20Address, owner);
        Functor getUserBalanceFunctor = ABI::Encoder::encodeFunction("getUserBalance(address,address)");

        Bytes getUserBalanceResult = contractManager->callContract(
          buildCallInfo(wrapperAddress, getUserBalanceFunctor, getUserBalanceEncoder));

        auto getUserBalanceDecoder= ABI::Decoder::decodeData<uint256_t>(getUserBalanceResult);
        REQUIRE(std::get<0>(getUserBalanceDecoder) == 500000000000000000);

        Bytes getBalanceEncoder = ABI::Encoder::encodeData(owner);
        Functor getBalanceFunctor = ABI::Encoder::encodeFunction("balanceOf(address)");
        Bytes getBalanceResult = contractManager->callContract(
          buildCallInfo(erc20Address, getBalanceFunctor, getBalanceEncoder));

        auto getBalanceDecoder  = ABI::Decoder::decodeData<uint256_t>(getBalanceResult);
        REQUIRE(std::get<0>(getBalanceDecoder) == 500000000000000000);

        Bytes getBalanceWrapperEncoder = ABI::Encoder::encodeData(wrapperAddress);
        Functor getBalanceWrapperFunctor = ABI::Encoder::encodeFunction("balanceOf(address)");

        Bytes getBalanceWrapperResult = contractManager->callContract(
          buildCallInfo(erc20Address, getBalanceWrapperFunctor, getBalanceWrapperEncoder));

        auto getBalanceWrapperDecoder = ABI::Decoder::decodeData<uint256_t>(getBalanceWrapperResult);
        REQUIRE(std::get<0>(getBalanceWrapperDecoder) == 500000000000000000);

        Bytes withdrawEncoder = ABI::Encoder::encodeData(erc20Address, static_cast<uint256_t>(250000000000000000));
        Bytes withdrawData = Hex::toBytes("0xf3fef3a3");
        Utils::appendBytes(withdrawData, withdrawEncoder);
        TxBlock withdrawTx(
          wrapperAddress,
          owner,
          withdrawData,
          8080,
          0,
          0,
          0,
          0,
          0,
          ownerPrivKey
        );

        contractManager->callContract(withdrawTx);

        getContractBalanceResult = contractManager->callContract(
          buildCallInfo(wrapperAddress, getContractBalanceFunctor, getContractBalanceEncoder));

        getContractBalanceDecoder = ABI::Decoder::decodeData<uint256_t>(getContractBalanceResult);
        REQUIRE(std::get<0>(getContractBalanceDecoder) == 250000000000000000);

        getUserBalanceResult = contractManager->callContract(
          buildCallInfo(wrapperAddress, getUserBalanceFunctor, getUserBalanceEncoder));
        getUserBalanceDecoder = ABI::Decoder::decodeData<uint256_t>(getUserBalanceResult);
        REQUIRE(std::get<0>(getUserBalanceDecoder) == 250000000000000000);

        getBalanceResult = contractManager->callContract(
          buildCallInfo(erc20Address, getBalanceFunctor, getBalanceEncoder));
        getBalanceDecoder = ABI::Decoder::decodeData<uint256_t>(getBalanceResult);
        REQUIRE(std::get<0>(getBalanceDecoder) == 750000000000000000);

        getBalanceWrapperResult = contractManager->callContract(
          buildCallInfo(erc20Address, getBalanceWrapperFunctor, getBalanceWrapperEncoder));
        getBalanceWrapperDecoder = ABI::Decoder::decodeData<uint256_t>(getBalanceWrapperResult);
        REQUIRE(std::get<0>(getBalanceWrapperDecoder) == 250000000000000000);
      }
      std::unique_ptr<Options> options;
      std::unique_ptr<DB> db;
      std::unique_ptr<ContractManager> contractManager;
      std::string dbName = testDumpPath + "/erc20wrapperDb";
      initialize(options, db, contractManager, dbName, ownerPrivKey, false);

      Bytes getAllowanceEncoder = ABI::Encoder::encodeData(owner, wrapperAddress);
      Functor getAllowanceFunctor = ABI::Encoder::encodeFunction("allowance(address,address)");

      Bytes getAllowanceResult = contractManager->callContract(buildCallInfo(erc20Address, getAllowanceFunctor, getAllowanceEncoder));

      auto getAllowanceDecoder = ABI::Decoder::decodeData<uint256_t>(getAllowanceResult);
      REQUIRE(std::get<0>(getAllowanceDecoder) == 0);

      Bytes getContractBalanceEncoder = ABI::Encoder::encodeData(erc20Address);
      Functor getContractBalanceFunctor = ABI::Encoder::encodeFunction("getContractBalance(address)");
      Bytes getContractBalanceResult = contractManager->callContract(buildCallInfo(wrapperAddress, getContractBalanceFunctor, getContractBalanceEncoder));

      auto getContractBalanceDecoder = ABI::Decoder::decodeData<uint256_t>(getContractBalanceResult);
      REQUIRE(std::get<0>(getContractBalanceDecoder) == 250000000000000000);

      Bytes getUserBalanceEncoder = ABI::Encoder::encodeData(erc20Address, owner);
      Functor getUserBalanceFunctor = ABI::Encoder::encodeFunction("getUserBalance(address,address)");
      Bytes getUserBalanceResult = contractManager->callContract(buildCallInfo(wrapperAddress, getUserBalanceFunctor, getUserBalanceEncoder));

      auto getUserBalanceDecoder = ABI::Decoder::decodeData<uint256_t>(getUserBalanceResult);
      REQUIRE(std::get<0>(getUserBalanceDecoder) == 250000000000000000);

      Bytes getBalanceEncoder = ABI::Encoder::encodeData(owner);
      Functor getBalanceFunctor = ABI::Encoder::encodeFunction("balanceOf(address)");
      Bytes getBalanceResult = contractManager->callContract(buildCallInfo(erc20Address, getBalanceFunctor, getBalanceEncoder));

      auto getBalanceDecoder = ABI::Decoder::decodeData<uint256_t>(getBalanceResult);
      REQUIRE(std::get<0>(getBalanceDecoder) == 750000000000000000);

      Bytes getBalanceWrapperEncoder = ABI::Encoder::encodeData(wrapperAddress);
      Functor getBalanceWrapperFunctor = ABI::Encoder::encodeFunction("balanceOf(address)");
      Bytes getBalanceWrapperResult = contractManager->callContract(buildCallInfo(erc20Address, getBalanceWrapperFunctor, getBalanceWrapperEncoder));

      auto getBalanceWrapperDecoder = ABI::Decoder::decodeData<uint256_t>(getBalanceWrapperResult);
      REQUIRE(std::get<0>(getBalanceWrapperDecoder) == 250000000000000000);
    }

    SECTION("ERC20Wrapper transferTo") {
      Address erc20Address;
      Address wrapperAddress;
      Address destinationOfTransfers(Utils::randBytes(20));
      {
        std::unique_ptr<Options> options;
        std::unique_ptr<DB> db;
        std::unique_ptr<ContractManager> contractManager;
        std::string dbName = testDumpPath + "/erc20wrapperDb";
        initialize(options, db, contractManager, dbName, ownerPrivKey);
        for (const auto &[name, address]: contractManager->getContracts()) {
          if (name == "ERC20") {
            erc20Address = address;
          }
          if (name == "ERC20Wrapper") {
            wrapperAddress = address;
          }
        }

        Bytes getAllowanceEncoder = ABI::Encoder::encodeData(owner, wrapperAddress);
        Functor getAllowanceFunctor = ABI::Encoder::encodeFunction("allowance(address,address)");

        Bytes depositEncoder = ABI::Encoder::encodeData(erc20Address, static_cast<uint256_t>(500000000000000000));
        Bytes depositData = Hex::toBytes("0x47e7ef24");
        Utils::appendBytes(depositData, depositEncoder);
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

        REQUIRE_THROWS(contractManager->callContract(depositTx));

        Bytes approveEncoder = ABI::Encoder::encodeData(wrapperAddress, static_cast<uint256_t>(500000000000000000));
        Bytes approveData = Hex::toBytes("0x095ea7b3");
        Utils::appendBytes(approveData, approveEncoder);
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
          buildCallInfo(erc20Address, getAllowanceFunctor, getAllowanceEncoder));

        auto getAllowanceDecoder = ABI::Decoder::decodeData<uint256_t>(getAllowanceResult);
        REQUIRE(std::get<0>(getAllowanceDecoder) == 500000000000000000);

        contractManager->callContract(depositTx);
        Bytes getAllowanceResult2 = contractManager->callContract(
          buildCallInfo(erc20Address, getAllowanceFunctor, getAllowanceEncoder));

        auto getAllowanceDecoder2 = ABI::Decoder::decodeData<uint256_t>(getAllowanceResult2);
        REQUIRE(std::get<0>(getAllowanceDecoder2) == 0);

        Bytes getContractBalanceEncoder = ABI::Encoder::encodeData(erc20Address);
        Functor getContractBalanceFunctor = ABI::Encoder::encodeFunction("getContractBalance(address)");

        Bytes getContractBalanceResult = contractManager->callContract(
          buildCallInfo(wrapperAddress, getContractBalanceFunctor, getContractBalanceEncoder));

        auto getContractBalanceDecoder = ABI::Decoder::decodeData<uint256_t>(getContractBalanceResult);
        REQUIRE(std::get<0>(getContractBalanceDecoder) == 500000000000000000);

        Bytes getUserBalanceEncoder = ABI::Encoder::encodeData(erc20Address, owner);
        Functor getUserBalanceFunctor = ABI::Encoder::encodeFunction("getUserBalance(address,address)");

        Bytes getUserBalanceResult = contractManager->callContract(
          buildCallInfo(wrapperAddress, getUserBalanceFunctor, getUserBalanceEncoder));

        auto getUserBalanceDecoder = ABI::Decoder::decodeData<uint256_t>(getUserBalanceResult);
        REQUIRE(std::get<0>(getUserBalanceDecoder) == 500000000000000000);

        Bytes getBalanceEncoder = ABI::Encoder::encodeData(owner);
        Functor getBalanceFunctor = ABI::Encoder::encodeFunction("balanceOf(address)");
        Bytes getBalanceResult = contractManager->callContract(
          buildCallInfo(erc20Address, getBalanceFunctor, getBalanceEncoder));

        auto getBalanceWrapperDecoder = ABI::Decoder::decodeData<uint256_t>(getBalanceResult);
        REQUIRE(std::get<0>(getBalanceWrapperDecoder) == 500000000000000000);

        Bytes getBalanceWrapperEncoder = ABI::Encoder::encodeData(wrapperAddress);
        Functor getBalanceWrapperFunctor = ABI::Encoder::encodeFunction("balanceOf(address)");
        Bytes getBalanceWrapperResult = contractManager->callContract(
          buildCallInfo(erc20Address, getBalanceWrapperFunctor, getBalanceWrapperEncoder));

        getBalanceWrapperDecoder = ABI::Decoder::decodeData<uint256_t>(getBalanceWrapperResult);
        REQUIRE(std::get<0>(getBalanceWrapperDecoder) == 500000000000000000);

        Bytes transferToEncoder = ABI::Encoder::encodeData(erc20Address, destinationOfTransfers, static_cast<uint256_t>(250000000000000000));
        Bytes transferToData = Hex::toBytes("0xa5f2a152");
        Utils::appendBytes(transferToData, transferToEncoder);
        TxBlock transferTx(
          wrapperAddress,
          owner,
          transferToData,
          8080,
          0,
          0,
          0,
          0,
          0,
          ownerPrivKey
        );

        contractManager->callContract(transferTx);

        getContractBalanceResult = contractManager->callContract(
          buildCallInfo(wrapperAddress, getContractBalanceFunctor, getContractBalanceEncoder));
        getContractBalanceDecoder = ABI::Decoder::decodeData<uint256_t>(getContractBalanceResult);
        REQUIRE(std::get<0>(getContractBalanceDecoder) == 250000000000000000);

        getUserBalanceResult = contractManager->callContract(
          buildCallInfo(wrapperAddress, getUserBalanceFunctor, getUserBalanceEncoder));
        getUserBalanceDecoder = ABI::Decoder::decodeData<uint256_t>(getUserBalanceResult);
        REQUIRE(std::get<0>(getUserBalanceDecoder) == 250000000000000000);

        getBalanceResult = contractManager->callContract(
          buildCallInfo(erc20Address, getBalanceFunctor, getBalanceEncoder));
        auto getBalanceDecoder = ABI::Decoder::decodeData<uint256_t>(getBalanceResult);
        REQUIRE(std::get<0>(getBalanceDecoder) == 500000000000000000);

        getBalanceWrapperResult = contractManager->callContract(
          buildCallInfo(erc20Address, getBalanceWrapperFunctor, getBalanceWrapperEncoder));
        getBalanceWrapperDecoder = ABI::Decoder::decodeData<uint256_t>(getBalanceWrapperResult);
        REQUIRE(std::get<0>(getBalanceWrapperDecoder) == 250000000000000000);

        Bytes getBalanceDestinationEncoder = ABI::Encoder::encodeData(destinationOfTransfers);
        Functor getBalanceDestinationFunctor = ABI::Encoder::encodeFunction("balanceOf(address)");
        Bytes getBalanceDestinationResult = contractManager->callContract(
          buildCallInfo(erc20Address, getBalanceDestinationFunctor, getBalanceDestinationEncoder));

        auto getBalanceDestinationDecoder = ABI::Decoder::decodeData<uint256_t>(getBalanceDestinationResult);
        REQUIRE(std::get<0>(getBalanceDestinationDecoder) == 250000000000000000);
      }
      std::unique_ptr<Options> options;
      std::unique_ptr<DB> db;
      std::unique_ptr<ContractManager> contractManager;
      std::string dbName = testDumpPath + "/erc20wrapperDb";
      initialize(options, db, contractManager, dbName, ownerPrivKey, false);

      Bytes getAllowanceEncoder = ABI::Encoder::encodeData(owner, wrapperAddress);
      Functor getAllowanceFunctor = ABI::Encoder::encodeFunction("allowance(address,address)");

      Bytes getAllowanceResult = contractManager->callContract(buildCallInfo(erc20Address, getAllowanceFunctor, getAllowanceEncoder));

      auto getAllowanceDecoder = ABI::Decoder::decodeData<uint256_t>(getAllowanceResult);
      REQUIRE(std::get<0>(getAllowanceDecoder) == 0);

      Bytes getContractBalanceEncoder = ABI::Encoder::encodeData(erc20Address);
      Functor getContractBalanceFunctor = ABI::Encoder::encodeFunction("getContractBalance(address)");
      Bytes getContractBalanceResult = contractManager->callContract(buildCallInfo(wrapperAddress, getContractBalanceFunctor, getContractBalanceEncoder));

      auto getContractBalanceDecoder = ABI::Decoder::decodeData<uint256_t>(getContractBalanceResult);
      REQUIRE(std::get<0>(getContractBalanceDecoder) == 250000000000000000);

      Bytes getUserBalanceEncoder = ABI::Encoder::encodeData(erc20Address, owner);
      Functor getUserBalanceFunctor = ABI::Encoder::encodeFunction("getUserBalance(address,address)");
      Bytes getUserBalanceResult = contractManager->callContract(buildCallInfo(wrapperAddress, getUserBalanceFunctor, getUserBalanceEncoder));

      auto getUserBalanceDecoder = ABI::Decoder::decodeData<uint256_t>(getUserBalanceResult);
      REQUIRE(std::get<0>(getUserBalanceDecoder) == 250000000000000000);

      Bytes getBalanceEncoder = ABI::Encoder::encodeData(owner);
      Functor getBalanceFunctor = ABI::Encoder::encodeFunction("balanceOf(address)");
      Bytes getBalanceResult = contractManager->callContract(buildCallInfo(erc20Address, getBalanceFunctor, getBalanceEncoder));

      auto getBalanceDecoder = ABI::Decoder::decodeData<uint256_t>(getBalanceResult);
      REQUIRE(std::get<0>(getBalanceDecoder) == 500000000000000000);

      Bytes getBalanceWrapperEncoder = ABI::Encoder::encodeData(wrapperAddress);
      Functor getBalanceWrapperFunctor = ABI::Encoder::encodeFunction("balanceOf(address)");

      Bytes getBalanceWrapperResult = contractManager->callContract(buildCallInfo(erc20Address, getBalanceWrapperFunctor, getBalanceWrapperEncoder));

      auto getBalanceWrapperDecoder = ABI::Decoder::decodeData<uint256_t>(getBalanceWrapperResult);
      REQUIRE(std::get<0>(getBalanceWrapperDecoder) == 250000000000000000);

      Bytes getBalanceDestinationEncoder = ABI::Encoder::encodeData(destinationOfTransfers);
      Functor getBalanceDestinationFunctor = ABI::Encoder::encodeFunction("balanceOf(address)");
      Bytes getBalanceDestinationResult = contractManager->callContract(
        buildCallInfo(erc20Address, getBalanceDestinationFunctor, getBalanceDestinationEncoder));

      auto getBalanceDestinationDecoder = ABI::Decoder::decodeData<uint256_t>(getBalanceDestinationResult);
      REQUIRE(std::get<0>(getBalanceDestinationDecoder) == 250000000000000000);
    }
  }
}
