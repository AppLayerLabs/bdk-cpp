/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/templates/erc20.h"
#include "../../src/contract/contractmanager.h"
#include "../../src/core/rdpos.h"
#include "../../src/utils/db.h"
#include "../../src/utils/options.h"
#include <filesystem>
#include <sys/types.h>

// Forward declaration.
ethCallInfoAllocated buildCallInfo(const Address& addressToCall, const Functor& function, const Bytes& dataToCall);

namespace TContractManager {
  std::string testDumpPath = Utils::getTestDumpPath();
  // TODO: Add more testcases for ContractManager once it's integrated with State.
  TEST_CASE("ContractManager class", "[contract][contractmanager]") {
    SECTION("ContractManager createNewContractERC20Contract()") {
      if (std::filesystem::exists(testDumpPath + "/ContractManagerTestCreateNew")) {
        std::filesystem::remove_all(testDumpPath + "/ContractManagerTestCreateNew");
      }
      if (!std::filesystem::exists(testDumpPath)) { // Ensure the testdump folder actually exists
        std::filesystem::create_directories(testDumpPath);
      }

      PrivKey privKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
      Address owner = Secp256k1::toAddress(Secp256k1::toUPub(privKey));
      std::string tokenName = "TestToken";
      std::string tokenSymbol = "TT";
      uint256_t tokenDecimals = 18;
      uint256_t tokenSupply = 1000000000000000000;

      {
        std::unique_ptr options = std::make_unique<Options>(Options::fromFile(testDumpPath + "/ContractManagerTestCreateNew"));
        std::unique_ptr db = std::make_unique<DB>(testDumpPath + "/ContractManagerTestCreateNew");
        std::unique_ptr<rdPoS> rdpos;
        ContractManager contractManager(nullptr, db, rdpos, options, nullptr);

        Bytes createNewERC20ContractEncoder = ABI::Encoder::encodeData(tokenName, tokenSymbol, tokenDecimals, tokenSupply);
        Bytes createNewERC20ContractData = Hex::toBytes("0xb74e5ed5");  // createNewERC20Contract(string,string,uint8,uint256)
        Utils::appendBytes(createNewERC20ContractData, createNewERC20ContractEncoder);

        TxBlock createNewERC2OTx = TxBlock(
          ProtocolContractAddresses.at("ContractManager"),
          owner,
          createNewERC20ContractData,
          8080,
          0,
          0,
          0,
          0,
          0,
          privKey
        );

        auto randomPrivKey = PrivKey(Utils::randBytes(32));
        TxBlock createNewERC2OTxThrow = TxBlock(
          ProtocolContractAddresses.at("ContractManager"),
          Secp256k1::toAddress(Secp256k1::toUPub(randomPrivKey)),
          createNewERC20ContractData,
          8080,
          0,
          0,
          0,
          0,
          0,
          randomPrivKey
        );

        REQUIRE_THROWS(contractManager.validateCallContractWithTx(createNewERC2OTxThrow.txToCallInfo()));

        REQUIRE(contractManager.getContracts().size() == 0);

        contractManager.callContract(createNewERC2OTx);

        REQUIRE(contractManager.getContracts().size() == 1);

        const auto contractAddress = contractManager.getContracts()[0].second;

        Bytes encodedData = ABI::Encoder::encodeData(owner);
        Functor functor = ABI::FunctorEncoder::encode<Address>("balanceOf");
        Bytes getBalanceMeResult = contractManager.callContract(buildCallInfo(contractAddress, functor, encodedData));

        auto getBalanceMeDecoder = ABI::Decoder::decodeData<uint256_t>(getBalanceMeResult);
        REQUIRE(std::get<0>(getBalanceMeDecoder) == tokenSupply);
      }

      std::unique_ptr options = std::make_unique<Options>(Options::fromFile(testDumpPath + "/ContractManagerTestCreateNew"));
      std::unique_ptr db = std::make_unique<DB>(testDumpPath + "/ContractManagerTestCreateNew");
      std::unique_ptr<rdPoS> rdpos;
      ContractManager contractManager(nullptr, db, rdpos, options, nullptr);

      const auto contractAddress = contractManager.getContracts()[0].second;

      Bytes encodedData = ABI::Encoder::encodeData(owner);
      Functor functor = ABI::FunctorEncoder::encode<Address>("balanceOf");

      Bytes getBalanceMeResult = contractManager.callContract(buildCallInfo(contractAddress, functor, encodedData));

      auto getBalanceMeDecoder = ABI::Decoder::decodeData<uint256_t>(getBalanceMeResult);
      REQUIRE(std::get<0>(getBalanceMeDecoder) == tokenSupply);

    }

    SECTION("ContractManager createNewContractERC20ContractTransferTo()") {
      if (std::filesystem::exists(testDumpPath + "/ContractManagerTestCreateNew")) {
        std::filesystem::remove_all(testDumpPath + "/ContractManagerTestCreateNew");
      }
      if (!std::filesystem::exists(testDumpPath)) { // Ensure the testdump folder actually exists
        std::filesystem::create_directories(testDumpPath);
      }

      PrivKey privKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
      Address owner = Secp256k1::toAddress(Secp256k1::toUPub(privKey));
      Address destinationOfTransfer = Address(Utils::randBytes(20));
      std::string tokenName = "TestToken";
      std::string tokenSymbol = "TT";
      uint256_t tokenDecimals = 18;
      uint256_t tokenSupply = 1000000000000000000;

      {
        std::unique_ptr options = std::make_unique<Options>(Options::fromFile(testDumpPath + "/ContractManagerTestCreateNew"));
        std::unique_ptr db = std::make_unique<DB>(testDumpPath + "/ContractManagerTestCreateNew");
        std::unique_ptr<rdPoS> rdpos;
        ContractManager contractManager(nullptr, db, rdpos, options, nullptr);

        Bytes createNewERC20ContractEncoder = ABI::Encoder::encodeData(tokenName, tokenSymbol, tokenDecimals, tokenSupply);

        Bytes createNewERC20ContractData = Hex::toBytes("0xb74e5ed5");  // createNewERC20Contract(string,string,uint8,uint256)
        Utils::appendBytes(createNewERC20ContractData, createNewERC20ContractEncoder);

        TxBlock createNewERC2OTx = TxBlock(
          ProtocolContractAddresses.at("ContractManager"),
          owner,
          createNewERC20ContractData,
          8080,
          0,
          0,
          0,
          0,
          0,
          privKey
        );

        contractManager.callContract(createNewERC2OTx);

        const auto contractAddress = contractManager.getContracts()[0].second;

        Bytes encodedData = ABI::Encoder::encodeData(owner);
        Functor functor = ABI::FunctorEncoder::encode<Address>("balanceOf");

        Bytes getBalanceMeResult = contractManager.callContract(buildCallInfo(contractAddress, functor, encodedData));

        auto getBalanceMeDecoder = ABI::Decoder::decodeData<uint256_t>(getBalanceMeResult);
        REQUIRE(std::get<0>(getBalanceMeDecoder) == tokenSupply);

        Bytes transferEncoder = ABI::Encoder::encodeData(destinationOfTransfer, static_cast<uint256_t>(500000000000000000));
        Bytes transferData = Hex::toBytes("0xa9059cbb");
        Utils::appendBytes(transferData, transferEncoder);
        TxBlock transferTx(
          contractAddress,
          owner,
          transferData,
          8080,
          0,
          0,
          0,
          0,
          0,
          privKey
        );

        contractManager.callContract(transferTx);

        getBalanceMeResult = contractManager.callContract(buildCallInfo(contractAddress, functor, encodedData));

        auto getBalanceMeDecoder2 = ABI::Decoder::decodeData<uint256_t>(getBalanceMeResult);
        REQUIRE(std::get<0>(getBalanceMeDecoder2) == 500000000000000000);

        Bytes getBalanceDestinationEncoder = ABI::Encoder::encodeData(destinationOfTransfer);
        Functor getBalanceDestinationFunctor = ABI::FunctorEncoder::encode<Address>("balanceOf");

        Bytes getBalanceDestinationResult = contractManager.callContract(buildCallInfo(contractAddress, getBalanceDestinationFunctor, getBalanceDestinationEncoder));

        auto getBalanceDestinationDecoder = ABI::Decoder::decodeData<uint256_t>(getBalanceDestinationResult);
        REQUIRE(std::get<0>(getBalanceDestinationDecoder) == 500000000000000000);
      }

      std::unique_ptr options = std::make_unique<Options>(Options::fromFile(testDumpPath + "/ContractManagerTestCreateNew"));
      std::unique_ptr db = std::make_unique<DB>(testDumpPath + "/ContractManagerTestCreateNew");
      std::unique_ptr<rdPoS> rdpos;
      ContractManager contractManager(nullptr, db, rdpos, options, nullptr);

      const auto contractAddress = contractManager.getContracts()[0].second;

      Bytes getBalanceMeEncoder = ABI::Encoder::encodeData(owner);
      Functor getBalanceMeFunctor = ABI::FunctorEncoder::encode<Address>("balanceOf");

      Bytes getBalanceMeResult = contractManager.callContract(buildCallInfo(contractAddress, getBalanceMeFunctor, getBalanceMeEncoder));

      auto getBalanceMeDecoder = ABI::Decoder::decodeData<uint256_t>(getBalanceMeResult);
      REQUIRE(std::get<0>(getBalanceMeDecoder) == 500000000000000000);

      Bytes getBalanceDestinationEncoder = ABI::Encoder::encodeData(destinationOfTransfer);
      Functor getBalanceDestinationFunctor = ABI::FunctorEncoder::encode<Address>("balanceOf");

      Bytes getBalanceDestinationResult = contractManager.callContract(buildCallInfo(contractAddress, getBalanceDestinationFunctor, getBalanceDestinationEncoder));

      auto getBalanceDestinationDecoder = ABI::Decoder::decodeData<uint256_t>(getBalanceDestinationResult);
      REQUIRE(std::get<0>(getBalanceDestinationDecoder) == 500000000000000000);
    }

    SECTION("ContractManager testNestedCalls") {
      if (std::filesystem::exists(testDumpPath + "/ContractManagerTestCreateNew")) {
        std::filesystem::remove_all(testDumpPath + "/ContractManagerTestCreateNew");
      }
      if (!std::filesystem::exists(testDumpPath)) { // Ensure the testdump folder actually exists
        std::filesystem::create_directories(testDumpPath);
      }

      PrivKey privKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
      Address owner = Secp256k1::toAddress(Secp256k1::toUPub(privKey));
      Address contractA, contractB, contractC;

      {
        std::unique_ptr options = std::make_unique<Options>(Options::fromFile(testDumpPath + "/ContractManagerTestCreateNew"));
        std::unique_ptr db = std::make_unique<DB>(testDumpPath + "/ContractManagerTestCreateNew");
        std::unique_ptr<rdPoS> rdpos;
        ContractManager contractManager(nullptr, db, rdpos, options, nullptr);

        // Create the contracts
        TxBlock createNewTestThrowATx = TxBlock(
          ProtocolContractAddresses.at("ContractManager"), owner, Hex::toBytes("0x6a025712"), // createNewThrowTestAContract()
          8080, 0, 0, 0, 0, 0, privKey
        );
        TxBlock createNewTestThrowBTx = TxBlock(
          ProtocolContractAddresses.at("ContractManager"), owner, Hex::toBytes("0xd0f59623"), // createNewThrowTestBContract()
          8080, 0, 0, 0, 0, 0, privKey
        );
        TxBlock createNewTestThrowCTx = TxBlock(
          ProtocolContractAddresses.at("ContractManager"), owner, Hex::toBytes("0x022367af"), // createNewThrowTestCContract()
          8080, 0, 0, 0, 0, 0, privKey
        );
        contractManager.callContract(createNewTestThrowATx);
        contractManager.callContract(createNewTestThrowBTx);
        contractManager.callContract(createNewTestThrowCTx);
        for (auto contract : contractManager.getContracts()) {
          if (contract.first == "ThrowTestA") contractA = contract.second;
          if (contract.first == "ThrowTestB") contractB = contract.second;
          if (contract.first == "ThrowTestC") contractC = contract.second;
        }

        // Create the transaction that will nest call setNum
        // Remember that uint256_t encodes and decodes all other uints

        Bytes setNumEnc = ABI::Encoder::encodeData(200, contractB, 100, contractC, 3);
        Functor setNumFunctor = ABI::FunctorEncoder::encode<uint8_t, Address, uint8_t, Address, uint8_t>("setNumA");
        Bytes setNumBytes;
        Utils::appendBytes(setNumBytes, setNumFunctor);
        Utils::appendBytes(setNumBytes, setNumEnc);
        TxBlock setNumTx(contractA, owner, setNumBytes, 8080, 0, 0, 0, 0, 0, privKey);
        try {
          contractManager.callContract(setNumTx);
        } catch (std::exception& e) {
          ; // Test should continue after throw
        }
      }

      // Tx should've thrown by now, check if all values are intact
      std::unique_ptr options = std::make_unique<Options>(Options::fromFile(testDumpPath + "/ContractManagerTestCreateNew"));
      std::unique_ptr db = std::make_unique<DB>(testDumpPath + "/ContractManagerTestCreateNew");
      std::unique_ptr<rdPoS> rdpos;
      ContractManager contractManager(nullptr, db, rdpos, options, nullptr);
      Bytes getNumEncA = Bytes(32, 0);
      Bytes getNumEncB = Bytes(32, 0);
      Bytes getNumEncC = Bytes(32, 0);
      Functor getNumFunctorA = ABI::FunctorEncoder::encode<void>("getNumA");
      Functor getNumFunctorB = ABI::FunctorEncoder::encode<void>("getNumB");
      Functor getNumFunctorC = ABI::FunctorEncoder::encode<void>("getNumC");
      Bytes dataA = contractManager.callContract(buildCallInfo(contractA, getNumFunctorA, getNumEncA));
      Bytes dataB = contractManager.callContract(buildCallInfo(contractB, getNumFunctorB, getNumEncB));
      Bytes dataC = contractManager.callContract(buildCallInfo(contractC, getNumFunctorC, getNumEncC));
      auto decA = ABI::Decoder::decodeData<uint256_t>(dataA);
      auto decB = ABI::Decoder::decodeData<uint256_t>(dataB);
      auto decC = ABI::Decoder::decodeData<uint256_t>(dataC);
      REQUIRE(std::get<0>(decA) == 0);
      REQUIRE(std::get<0>(decB) == 0);
      REQUIRE(std::get<0>(decC) == 0);
    }
  }
}

