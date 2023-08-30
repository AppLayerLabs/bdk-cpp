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
        ContractManager contractManager(nullptr, db, rdpos, options);

        ABI::Encoder::EncVar createNewERC20ContractVars;
        createNewERC20ContractVars.push_back(tokenName);
        createNewERC20ContractVars.push_back(tokenSymbol);
        createNewERC20ContractVars.push_back(tokenDecimals);
        createNewERC20ContractVars.push_back(tokenSupply);
        ABI::Encoder createNewERC20ContractEncoder(createNewERC20ContractVars);
        Bytes createNewERC20ContractData = Hex::toBytes("0xb74e5ed5");  // createNewERC20Contract(string,string,uint8,uint256)
        Utils::appendBytes(createNewERC20ContractData, createNewERC20ContractEncoder.getData());

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
        ABI::Encoder::EncVar getBalanceMeVars;
        getBalanceMeVars.push_back(owner);
        ABI::Encoder getBalanceMeEncoder(getBalanceMeVars, "balanceOf(address)");
        Bytes getBalanceMeResult = contractManager.callContract(buildCallInfo(contractAddress, getBalanceMeEncoder.getFunctor(), getBalanceMeEncoder.getData()));
        ABI::Decoder getBalanceMeDecoder({ABI::Types::uint256}, getBalanceMeResult);
        REQUIRE(getBalanceMeDecoder.getData<uint256_t>(0) == tokenSupply);
      }

      std::unique_ptr options = std::make_unique<Options>(Options::fromFile(testDumpPath + "/ContractManagerTestCreateNew"));
      std::unique_ptr db = std::make_unique<DB>(testDumpPath + "/ContractManagerTestCreateNew");
      std::unique_ptr<rdPoS> rdpos;
      ContractManager contractManager(nullptr, db, rdpos, options);

      const auto contractAddress = contractManager.getContracts()[0].second;
      ABI::Encoder::EncVar getBalanceMeVars;
      getBalanceMeVars.push_back(owner);
      ABI::Encoder getBalanceMeEncoder(getBalanceMeVars, "balanceOf(address)");
      Bytes getBalanceMeResult = contractManager.callContract(buildCallInfo(contractAddress, getBalanceMeEncoder.getFunctor(), getBalanceMeEncoder.getData()));
      ABI::Decoder getBalanceMeDecoder({ABI::Types::uint256}, getBalanceMeResult);
      REQUIRE(getBalanceMeDecoder.getData<uint256_t>(0) == tokenSupply);

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
        ContractManager contractManager(nullptr, db, rdpos, options);

        ABI::Encoder::EncVar createNewERC20ContractVars;
        createNewERC20ContractVars.push_back(tokenName);
        createNewERC20ContractVars.push_back(tokenSymbol);
        createNewERC20ContractVars.push_back(tokenDecimals);
        createNewERC20ContractVars.push_back(tokenSupply);
        ABI::Encoder createNewERC20ContractEncoder(createNewERC20ContractVars);
        Bytes createNewERC20ContractData = Hex::toBytes("0xb74e5ed5");  // createNewERC20Contract(string,string,uint8,uint256)
        Utils::appendBytes(createNewERC20ContractData, createNewERC20ContractEncoder.getData());

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
        ABI::Encoder::EncVar getBalanceMeVars;
        getBalanceMeVars.push_back(owner);
        ABI::Encoder getBalanceMeEncoder(getBalanceMeVars, "balanceOf(address)");
        Bytes getBalanceMeResult = contractManager.callContract(buildCallInfo(contractAddress, getBalanceMeEncoder.getFunctor(), getBalanceMeEncoder.getData()));
        ABI::Decoder getBalanceMeDecoder({ABI::Types::uint256}, getBalanceMeResult);
        REQUIRE(getBalanceMeDecoder.getData<uint256_t>(0) == tokenSupply);

        ABI::Encoder::EncVar transferVars;
        transferVars.push_back(destinationOfTransfer);
        transferVars.push_back(static_cast<uint256_t>(500000000000000000));
        ABI::Encoder transferEncoder(transferVars);
        Bytes transferData = Hex::toBytes("0xa9059cbb");
        Utils::appendBytes(transferData, transferEncoder.getData());
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

        getBalanceMeResult = contractManager.callContract(buildCallInfo(contractAddress, getBalanceMeEncoder.getFunctor(), getBalanceMeEncoder.getData()));
        getBalanceMeDecoder = ABI::Decoder({ABI::Types::uint256}, getBalanceMeResult);
        REQUIRE(getBalanceMeDecoder.getData<uint256_t>(0) == 500000000000000000);

        ABI::Encoder::EncVar getBalanceDestinationVars;
        getBalanceDestinationVars.push_back(destinationOfTransfer);
        ABI::Encoder getBalanceDestinationEncoder(getBalanceDestinationVars, "balanceOf(address)");
        Bytes getBalanceDestinationResult = contractManager.callContract(buildCallInfo(contractAddress, getBalanceDestinationEncoder.getFunctor(), getBalanceDestinationEncoder.getData()));
        ABI::Decoder getBalanceDestinationDecoder({ABI::Types::uint256}, getBalanceDestinationResult);
        REQUIRE(getBalanceDestinationDecoder.getData<uint256_t>(0) == 500000000000000000);
      }

      std::unique_ptr options = std::make_unique<Options>(Options::fromFile(testDumpPath + "/ContractManagerTestCreateNew"));
      std::unique_ptr db = std::make_unique<DB>(testDumpPath + "/ContractManagerTestCreateNew");
      std::unique_ptr<rdPoS> rdpos;
      ContractManager contractManager(nullptr, db, rdpos, options);

      const auto contractAddress = contractManager.getContracts()[0].second;
      ABI::Encoder::EncVar getBalanceMeVars;
      getBalanceMeVars.push_back(owner);
      ABI::Encoder getBalanceMeEncoder(getBalanceMeVars, "balanceOf(address)");
      Bytes getBalanceMeResult = contractManager.callContract(buildCallInfo(contractAddress, getBalanceMeEncoder.getFunctor(), getBalanceMeEncoder.getData()));
      ABI::Decoder getBalanceMeDecoder({ABI::Types::uint256}, getBalanceMeResult);
      REQUIRE(getBalanceMeDecoder.getData<uint256_t>(0) == 500000000000000000);

      ABI::Encoder::EncVar getBalanceDestinationVars;
      getBalanceDestinationVars.push_back(destinationOfTransfer);
      ABI::Encoder getBalanceDestinationEncoder(getBalanceDestinationVars, "balanceOf(address)");
      Bytes getBalanceDestinationResult = contractManager.callContract(buildCallInfo(contractAddress, getBalanceDestinationEncoder.getFunctor(), getBalanceDestinationEncoder.getData()));
      ABI::Decoder getBalanceDestinationDecoder({ABI::Types::uint256}, getBalanceDestinationResult);
      REQUIRE(getBalanceDestinationDecoder.getData<uint256_t>(0) == 500000000000000000);
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
        ContractManager contractManager(nullptr, db, rdpos, options);

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
        ABI::Encoder::EncVar setNumEncVar;
        setNumEncVar.push_back(uint256_t(200));
        setNumEncVar.push_back(contractB);
        setNumEncVar.push_back(uint256_t(100));
        setNumEncVar.push_back(contractC);
        setNumEncVar.push_back(uint256_t(3));
        ABI::Encoder setNumEnc(setNumEncVar, "setNumA(uint8,address,uint8,address,uint8)");
        Bytes setNumBytes;
        Utils::appendBytes(setNumBytes, setNumEnc.getFunctor());
        Utils::appendBytes(setNumBytes, setNumEnc.getData());
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
      ContractManager contractManager(nullptr, db, rdpos, options);
      ABI::Encoder getNumEncA({}, "getNumA()");
      ABI::Encoder getNumEncB({}, "getNumB()");
      ABI::Encoder getNumEncC({}, "getNumC()");
      Bytes dataA = contractManager.callContract(buildCallInfo(contractA, getNumEncA.getFunctor(), getNumEncA.getData()));
      Bytes dataB = contractManager.callContract(buildCallInfo(contractB, getNumEncB.getFunctor(), getNumEncB.getData()));
      Bytes dataC = contractManager.callContract(buildCallInfo(contractC, getNumEncC.getFunctor(), getNumEncC.getData()));
      ABI::Decoder decA({ABI::Types::uint8}, dataA);
      ABI::Decoder decB({ABI::Types::uint8}, dataB);
      ABI::Decoder decC({ABI::Types::uint8}, dataC);
      REQUIRE(decA.getData<uint256_t>(0) == 0);
      REQUIRE(decB.getData<uint256_t>(0) == 0);
      REQUIRE(decC.getData<uint256_t>(0) == 0);
    }
  }
}

