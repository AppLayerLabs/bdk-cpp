/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/utils/db.h"
#include "../../src/utils/options.h"
#include "../../src/core/rdpos.h"
#include "../../src/contract/abi.h"
#include "../../src/contract/contractmanager.h"
#include "../../src/contract/templates/simplecontract.h"

#include <filesystem>

// Forward Declaration.
ethCallInfoAllocated buildCallInfo(const Address& addressToCall, const Functor& function, const Bytes& dataToCall);

void initialize(
  std::unique_ptr<Options>& options,
  std::unique_ptr<DB>& db,
  std::unique_ptr<ContractManager> &contractManager,
  const std::string& dbName,
  const PrivKey& ownerPrivKey,
  const std::string& name,
  const uint256_t& value,
  bool deleteDB = true
) {
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
    // Create the contract.
    Bytes createNewSimpleContractEncoder = ABI::Encoder::encodeData(name, value);
    Bytes createNewSimpleContractData = Hex::toBytes("0x6de23252"); // createNewSimpleContractContract(string,uint256)
    Utils::appendBytes(createNewSimpleContractData, createNewSimpleContractEncoder);

    TxBlock createNewSimpleContractTx = TxBlock(
      ProtocolContractAddresses.at("ContractManager"),
      Secp256k1::toAddress(Secp256k1::toUPub(ownerPrivKey)),
      createNewSimpleContractData,
      8080,
      0,
      0,
      0,
      0,
      0,
      ownerPrivKey
    );

    contractManager->callContract(createNewSimpleContractTx);
  }
}

namespace TSimpleContract {
  TEST_CASE("SimpleContract class", "[contract][simplecontract]") {
    std::string testDumpPath = Utils::getTestDumpPath();
    PrivKey ownerPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
    Address owner = Secp256k1::toAddress(Secp256k1::toUPub(ownerPrivKey));
    SECTION("SimpleContract creation") {
      Address contractAddress;
      {
        std::unique_ptr<Options> options;
        std::unique_ptr<DB> db;
        std::unique_ptr<ContractManager> contractManager;
        initialize(options, db, contractManager, testDumpPath + "/SimpleContractCreationTest", ownerPrivKey, "TestName", 19283187581);

        // Get the contract address.
        contractAddress = contractManager->getContracts()[0].second;

        Bytes getNameEncoder = Bytes(32, 0);
        Bytes getValueEncoder = Bytes(32, 0);

        Functor getNameFunctor = ABI::Encoder::encodeFunction("getName()");
        Functor getValueFunctor = ABI::Encoder::encodeFunction("getValue()");

        Bytes nameData = contractManager->callContract(buildCallInfo(contractAddress, getNameFunctor, getNameEncoder));
        Bytes valueData = contractManager->callContract(buildCallInfo(contractAddress, getValueFunctor, getValueEncoder));

        auto nameDecoder = ABI::Decoder::decodeData<std::string>(nameData);
        auto valueDecoder = ABI::Decoder::decodeData<uint256_t>(valueData);

        REQUIRE(std::get<0>(nameDecoder) == "TestName");
        REQUIRE(std::get<0>(valueDecoder) == 19283187581);
      }

      std::unique_ptr<Options> options;
      std::unique_ptr<DB> db;
      std::unique_ptr<ContractManager> contractManager;
      initialize(options, db, contractManager,  testDumpPath + "/SimpleContractCreationTest", ownerPrivKey, "TestName", 19283187581, false);

      REQUIRE(contractAddress == contractManager->getContracts()[0].second);

      Bytes getNameEncoder = Bytes(32, 0);
      Bytes getValueEncoder = Bytes(32, 0);

      Functor getNameFunctor = ABI::Encoder::encodeFunction("getName()");
      Functor getValueFunctor = ABI::Encoder::encodeFunction("getValue()");

      Bytes nameData = contractManager->callContract(buildCallInfo(contractAddress, getNameFunctor, getNameEncoder));
      Bytes valueData = contractManager->callContract(buildCallInfo(contractAddress, getValueFunctor, getValueEncoder));

      auto nameDecoder = ABI::Decoder::decodeData<std::string>(nameData);
      auto valueDecoder = ABI::Decoder::decodeData<uint256_t>(valueData);

      REQUIRE(std::get<0>(nameDecoder) == "TestName");
      REQUIRE(std::get<0>(valueDecoder) == 19283187581);
    }

    SECTION("SimpleContract setName and setValue") {
      Address contractAddress;
      {
        std::unique_ptr<Options> options;
        std::unique_ptr<DB> db;
        std::unique_ptr<ContractManager> contractManager;
        initialize(options, db, contractManager, testDumpPath + "/SimpleContractSetNameAndSetValue", ownerPrivKey, "TestName", 19283187581);

        // Get the contract address.
        contractAddress = contractManager->getContracts()[0].second;

        Bytes getNameEncoder = Bytes(32, 0);
        Bytes getValueEncoder = Bytes(32, 0);

        Functor getNameFunctor = ABI::Encoder::encodeFunction("getName()");
        Functor getValueFunctor = ABI::Encoder::encodeFunction("getValue()");

        Bytes nameData = contractManager->callContract(buildCallInfo(contractAddress, getNameFunctor, getNameEncoder));
        Bytes valueData = contractManager->callContract(buildCallInfo(contractAddress, getValueFunctor, getValueEncoder));

        auto nameDecoder = ABI::Decoder::decodeData<std::string>(nameData);
        auto valueDecoder = ABI::Decoder::decodeData<uint256_t>(valueData);

        Bytes setNameEncoder = ABI::Encoder::encodeData(std::string("TryThisName"));
        Functor setNameFunctor = ABI::Encoder::encodeFunction("setName(string)");
        Bytes setValueEncoder = ABI::Encoder::encodeData(uint256_t("918258172319061203818967178162134821351"));
        Functor setValueFunctor = ABI::Encoder::encodeFunction("setValue(uint256)");

        Bytes setNameBytes;
        Utils::appendBytes(setNameBytes, setNameFunctor);
        Utils::appendBytes(setNameBytes, setNameEncoder);

        Bytes setValueBytes;
        Utils::appendBytes(setValueBytes, setValueFunctor);
        Utils::appendBytes(setValueBytes, setValueEncoder);

        TxBlock setNameTx(
          contractAddress,
          owner,
          setNameBytes,
          8080,
          0,
          0,
          0,
          0,
          0,
          ownerPrivKey
        );

        TxBlock setValueTx(
          contractAddress,
          owner,
          setValueBytes,
          8080,
          0,
          0,
          0,
          0,
          0,
          ownerPrivKey
        );

        contractManager->callContract(setNameTx);
        contractManager->callContract(setValueTx);

        nameData = contractManager->callContract(buildCallInfo(contractAddress, getNameFunctor, getNameEncoder));
        valueData = contractManager->callContract(buildCallInfo(contractAddress, getValueFunctor, getValueEncoder));

        nameDecoder = ABI::Decoder::decodeData<std::string>(nameData);
        valueDecoder = ABI::Decoder::decodeData<uint256_t>(valueData);

        REQUIRE(std::get<0>(nameDecoder) == "TryThisName");
        REQUIRE(std::get<0>(valueDecoder) == uint256_t("918258172319061203818967178162134821351"));
      }

      std::unique_ptr<Options> options;
      std::unique_ptr<DB> db;
      std::unique_ptr<ContractManager> contractManager;
      initialize(options, db, contractManager, testDumpPath + "/SimpleContractSetNameAndSetValue", ownerPrivKey, "TestName", 19283187581, false);

      REQUIRE(contractAddress == contractManager->getContracts()[0].second);

      Bytes getNameEncoder = Bytes(32, 0);
      Bytes getValueEncoder = Bytes(32, 0);

      Functor getNameFunctor = ABI::Encoder::encodeFunction("getName()");
      Functor getValueFunctor = ABI::Encoder::encodeFunction("getValue()");

      Bytes nameData = contractManager->callContract(buildCallInfo(contractAddress, getNameFunctor, getNameEncoder));
      Bytes valueData = contractManager->callContract(buildCallInfo(contractAddress, getValueFunctor, getValueEncoder));

      auto nameDecoder = ABI::Decoder::decodeData<std::string>(nameData);
      auto valueDecoder = ABI::Decoder::decodeData<uint256_t>(valueData);

      REQUIRE(std::get<0>(nameDecoder) == "TryThisName");
      REQUIRE(std::get<0>(valueDecoder) == uint256_t("918258172319061203818967178162134821351"));
    }
  }
}

