/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/templates/erc20.h"
#include "../../src/contract/contractmanager.h"
#include "../../src/utils/db.h"
#include "../../src/utils/options.h"
#include "../../src/utils/dynamicexception.h"
#include "../blockchainwrapper.hpp"
#include <filesystem>
#include <sys/types.h>

const std::vector<Hash> validatorPrivKeysContractManager {
  Hash(Hex::toBytes("0x0a0415d68a5ec2df57aab65efc2a7231b59b029bae7ff1bd2e40df9af96418c8")),
  Hash(Hex::toBytes("0xb254f12b4ca3f0120f305cabf1188fe74f0bd38e58c932a3df79c4c55df8fa66")),
  Hash(Hex::toBytes("0x8a52bb289198f0bcf141688a8a899bf1f04a02b003a8b1aa3672b193ce7930da")),
  Hash(Hex::toBytes("0x9048f5e80549e244b7899e85a4ef69512d7d68613a3dba828266736a580e7745")),
  Hash(Hex::toBytes("0x0b6f5ad26f6eb79116da8c98bed5f3ed12c020611777d4de94c3c23b9a03f739")),
  Hash(Hex::toBytes("0xa69eb3a3a679e7e4f6a49fb183fb2819b7ab62f41c341e2e2cc6288ee22fbdc7")),
  Hash(Hex::toBytes("0xd9b0613b7e4ccdb0f3a5ab0956edeb210d678db306ab6fae1e2b0c9ebca1c2c5")),
  Hash(Hex::toBytes("0x426dc06373b694d8804d634a0fd133be18e4e9bcbdde099fce0ccf3cb965492f"))
};

// Forward declaration from state.cpp
ethCallInfoAllocated buildCallInfo(const Address& addressToCall, const Functor& function, const Bytes& dataToCall);

TestBlockchainWrapper initialize(const std::vector<Hash>& validatorPrivKeys,
                                 const PrivKey& validatorKey,
                                 const uint64_t& serverPort,
                                 bool clearDb,
                                 const std::string& folderName);

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
        // We don't need rdPoS here for testing ContractManager, so we pass a nullptr instead
        auto blockchainWrapper = initialize(validatorPrivKeysContractManager, PrivKey(), 8080, true, "ContractManagerTestCreateNew");
        ContractManager contractManager(blockchainWrapper.db, blockchainWrapper.state, *(static_cast<rdPoS*>(nullptr)), blockchainWrapper.options);

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

      auto blockchainWrapper = initialize(validatorPrivKeysContractManager, PrivKey(), 8080, false, "ContractManagerTestCreateNew");
      ContractManager contractManager(blockchainWrapper.db, blockchainWrapper.state, *(static_cast<rdPoS*>(nullptr)), blockchainWrapper.options);

      const auto contractAddress = contractManager.getContracts()[0].second;

      Bytes encodedData = ABI::Encoder::encodeData(owner);
      Functor functor = ABI::FunctorEncoder::encode<Address>("balanceOf");

      Bytes getBalanceMeResult = contractManager.callContract(buildCallInfo(contractAddress, functor, encodedData));

      auto getBalanceMeDecoder = ABI::Decoder::decodeData<uint256_t>(getBalanceMeResult);
      REQUIRE(std::get<0>(getBalanceMeDecoder) == tokenSupply);

    }

    SECTION("ContractManager createNewContractERC20ContractTransferTo()") {
      if (std::filesystem::exists(testDumpPath + "/ContractManagerTestTransferTo")) {
        std::filesystem::remove_all(testDumpPath + "/ContractManagerTestTransferTo");
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
        auto blockchainWrapper = initialize(validatorPrivKeysContractManager, PrivKey(), 8080, true, "ContractManagerTestTransferTo");
        ContractManager contractManager(blockchainWrapper.db, blockchainWrapper.state, *(static_cast<rdPoS*>(nullptr)), blockchainWrapper.options);

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

      auto blockchainWrapper = initialize(validatorPrivKeysContractManager, PrivKey(), 8080, false, "ContractManagerTestTransferTo");
      ContractManager contractManager(blockchainWrapper.db, blockchainWrapper.state, *(static_cast<rdPoS*>(nullptr)), blockchainWrapper.options);


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
      if (std::filesystem::exists(testDumpPath + "/ContractManagerTestNestedCalls")) {
        std::filesystem::remove_all(testDumpPath + "/ContractManagerTestNestedCalls");
      }
      if (!std::filesystem::exists(testDumpPath)) { // Ensure the testdump folder actually exists
        std::filesystem::create_directories(testDumpPath);
      }

      PrivKey privKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
      Address owner = Secp256k1::toAddress(Secp256k1::toUPub(privKey));
      Address contractA, contractB, contractC;

      {
        auto blockchainWrapper = initialize(validatorPrivKeysContractManager, PrivKey(), 8080, true, "ContractManagerTestNestedCalls");
        ContractManager contractManager(blockchainWrapper.db, blockchainWrapper.state, *(static_cast<rdPoS*>(nullptr)), blockchainWrapper.options);

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
        Bytes setNumBytes(setNumFunctor.cbegin(), setNumFunctor.cend());
        Utils::appendBytes(setNumBytes, setNumEnc);
        TxBlock setNumTx(contractA, owner, setNumBytes, 8080, 0, 0, 0, 0, 0, privKey);
        try {
          contractManager.callContract(setNumTx);
        } catch (std::exception& e) {
          ; // Test should continue after throw
        }
      }

      // Tx should've thrown by now, check if all values are intact
      auto blockchainWrapper = initialize(validatorPrivKeysContractManager, PrivKey(), 8080, false, "ContractManagerTestNestedCalls");
      ContractManager contractManager(blockchainWrapper.db, blockchainWrapper.state, *(static_cast<rdPoS*>(nullptr)), blockchainWrapper.options);
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

    SECTION("ContractManager ethCall() throws") {
      if (std::filesystem::exists(testDumpPath + "/ContractManagerTestEthCall")) {
        std::filesystem::remove_all(testDumpPath + "/ContractManagerTestEthCall");
      }
      if (!std::filesystem::exists(testDumpPath)) { // Ensure the testdump folder actually exists
        std::filesystem::create_directories(testDumpPath);
      }

      PrivKey privKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
      Address owner = Secp256k1::toAddress(Secp256k1::toUPub(privKey));

      {
        ethCallInfo callInfo;

        auto blockchainWrapper = initialize(validatorPrivKeysContractManager, PrivKey(), 8080, true, "ContractManagerTestEthCall");
        ContractManager contractManager(blockchainWrapper.db, blockchainWrapper.state, *(static_cast<rdPoS*>(nullptr)), blockchainWrapper.options);

        try {
          contractManager.ethCall(callInfo);
          FAIL("Expected DynamicException was not thrown."); // This line will fail the test if no exception is thrown
        } catch (const DynamicException& e) {
          // Check that the exception message matches the expected message
          std::string expectedMessage = "Invalid function call with functor: 00000000";
          REQUIRE(std::string(e.what()) == expectedMessage);
        } catch (...) {
          FAIL("An unexpected exception type was thrown.");
        }
      }
    }
  }
}

