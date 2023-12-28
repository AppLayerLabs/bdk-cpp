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
  std::unique_ptr<Options>& options, std::unique_ptr<DB>& db,
  std::unique_ptr<ContractManager>& contractManager, std::unique_ptr<EventManager>& eventManager,
  const std::string& dbName,
  const PrivKey& ownerPrivKey,
  const std::string& name,
  const uint256_t& value,
  bool deleteDB = true
) {
  // Initialize parameters
  if (deleteDB && std::filesystem::exists(dbName)) std::filesystem::remove_all(dbName);
  options = std::make_unique<Options>(Options::fromFile(dbName));
  db = std::make_unique<DB>(dbName);
  std::unique_ptr<rdPoS> rdpos;
  eventManager = std::make_unique<EventManager>(db);
  contractManager = std::make_unique<ContractManager>(nullptr, db, rdpos, options, eventManager);
  // Create the contract
  if (deleteDB) {
    ABI::Encoder::EncVar createNewSimpleContractVars;
    createNewSimpleContractVars.push_back(name);
    createNewSimpleContractVars.push_back(value);
    ABI::Encoder createNewSimpleContractEncoder(createNewSimpleContractVars);
    Bytes createNewSimpleContractData = Hex::toBytes("0x6de23252"); // createNewSimpleContractContract(string,uint256)
    Utils::appendBytes(createNewSimpleContractData, createNewSimpleContractEncoder.getData());
    TxBlock createNewSimpleContractTx = TxBlock(
      ProtocolContractAddresses.at("ContractManager"),
      Secp256k1::toAddress(Secp256k1::toUPub(ownerPrivKey)),
      createNewSimpleContractData, 8080, 0, 0, 0, 0, 0, ownerPrivKey
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
        std::unique_ptr<EventManager> eventManager;
        initialize(options, db, contractManager, eventManager,
          testDumpPath + "/SimpleContractCreationTest", ownerPrivKey, "TestName", 19283187581
        );

        // Get the contract address.
        contractAddress = contractManager->getContracts()[0].second;

        ABI::Encoder getNameEncoder({}, "getName()");
        ABI::Encoder getValueEncoder({}, "getValue()");

        Bytes nameData = contractManager->callContract(buildCallInfo(contractAddress, getNameEncoder.getFunctor(), getNameEncoder.getData()));
        Bytes valueData = contractManager->callContract(buildCallInfo(contractAddress, getValueEncoder.getFunctor(), getValueEncoder.getData()));

        ABI::Decoder nameDecoder({ABI::Types::string}, nameData);
        ABI::Decoder valueDecoder({ABI::Types::uint256}, valueData);

        REQUIRE(nameDecoder.getData<std::string>(0) == "TestName");
        REQUIRE(valueDecoder.getData<uint256_t>(0) == 19283187581);
      }

      std::unique_ptr<Options> options;
      std::unique_ptr<DB> db;
      std::unique_ptr<ContractManager> contractManager;
      std::unique_ptr<EventManager> eventManager;
      initialize(options, db, contractManager, eventManager,
        testDumpPath + "/SimpleContractCreationTest", ownerPrivKey, "TestName", 19283187581, false
      );

      REQUIRE(contractAddress == contractManager->getContracts()[0].second);

      ABI::Encoder getNameEncoder({}, "getName()");
      ABI::Encoder getValueEncoder({}, "getValue()");

      Bytes nameData = contractManager->callContract(buildCallInfo(contractAddress, getNameEncoder.getFunctor(), getNameEncoder.getData()));
      Bytes valueData = contractManager->callContract(buildCallInfo(contractAddress, getValueEncoder.getFunctor(), getValueEncoder.getData()));

      ABI::Decoder nameDecoder({ABI::Types::string}, nameData);
      ABI::Decoder valueDecoder({ABI::Types::uint256}, valueData);

      REQUIRE(nameDecoder.getData<std::string>(0) == "TestName");
      REQUIRE(valueDecoder.getData<uint256_t>(0) == 19283187581);
    }

    SECTION("SimpleContract setName and setValue") {
      Address contractAddress;
      {
        std::unique_ptr<Options> options;
        std::unique_ptr<DB> db;
        std::unique_ptr<ContractManager> contractManager;
        std::unique_ptr<EventManager> eventManager;
        initialize(options, db, contractManager, eventManager,
          testDumpPath + "/SimpleContractSetNameAndSetValue", ownerPrivKey, "TestName", 19283187581
        );

        // Get the contract address.
        contractAddress = contractManager->getContracts()[0].second;

        ABI::Encoder getNameEncoder({}, "getName()");
        ABI::Encoder getValueEncoder({}, "getValue()");

        Bytes nameData = contractManager->callContract(buildCallInfo(contractAddress, getNameEncoder.getFunctor(), getNameEncoder.getData()));
        Bytes valueData = contractManager->callContract(buildCallInfo(contractAddress, getValueEncoder.getFunctor(), getValueEncoder.getData()));

        ABI::Decoder nameDecoder({ABI::Types::string}, nameData);
        ABI::Decoder valueDecoder({ABI::Types::uint256}, valueData);

        ABI::Encoder setNameEncoder({"TryThisName"}, "setName(string)");
        ABI::Encoder setValueEncoder({uint256_t("918258172319061203818967178162134821351")}, "setValue(uint256)");

        Bytes setNameBytes;
        Utils::appendBytes(setNameBytes, setNameEncoder.getFunctor().get());
        Utils::appendBytes(setNameBytes, setNameEncoder.getData());

        Bytes setValueBytes;
        Utils::appendBytes(setValueBytes, setValueEncoder.getFunctor().get());
        Utils::appendBytes(setValueBytes, setValueEncoder.getData());

        TxBlock setNameTx(contractAddress, owner, setNameBytes, 8080, 0, 0, 0, 0, 0, ownerPrivKey);
        TxBlock setValueTx(contractAddress, owner, setValueBytes, 8080, 0, 0, 0, 0, 0, ownerPrivKey);

        // Simulating a block so events can have separated keys (block height + tx index + log index).
        Hash randomBlockHash = Hash::random();
        contractManager->callContract(setNameTx, randomBlockHash, 0);
        contractManager->callContract(setValueTx, randomBlockHash, 1);

        nameData = contractManager->callContract(buildCallInfo(contractAddress, getNameEncoder.getFunctor(), getNameEncoder.getData()));
        valueData = contractManager->callContract(buildCallInfo(contractAddress, getValueEncoder.getFunctor(), getValueEncoder.getData()));

        nameDecoder = ABI::Decoder({ABI::Types::string}, nameData);
        valueDecoder = ABI::Decoder({ABI::Types::uint256}, valueData);

        REQUIRE(nameDecoder.getData<std::string>(0) == "TryThisName");
        REQUIRE(valueDecoder.getData<uint256_t>(0) == uint256_t("918258172319061203818967178162134821351"));

        Event nameEvent = eventManager->getEvents(
          0, 1, contractAddress, { Utils::sha3(Utils::stringToBytes("TryThisName")).asBytes() }
        ).at(0);

        REQUIRE(nameEvent.getName() == "nameChanged");
        REQUIRE(nameEvent.getLogIndex() == 0);
        REQUIRE(nameEvent.getTxHash() == setNameTx.hash());
        REQUIRE(nameEvent.getTxIndex() == 0);
        REQUIRE(nameEvent.getBlockIndex() == 0);
        REQUIRE(nameEvent.getAddress() == contractAddress);
        REQUIRE(nameEvent.getData() == Bytes());
        REQUIRE(nameEvent.getTopics().at(1) == Utils::sha3(Utils::stringToBytes("TryThisName"))); // at(0) = functor (non-anonymous)
        REQUIRE(!nameEvent.isAnonymous());

        Event valueEvent = eventManager->getEvents(
          0, 1, contractAddress, { Utils::padLeftBytes(Utils::uintToBytes(uint256_t("918258172319061203818967178162134821351")), 32) }
        ).at(0);

        REQUIRE(valueEvent.getName() == "valueChanged");
        REQUIRE(valueEvent.getLogIndex() == 0);
        REQUIRE(valueEvent.getTxHash() == setValueTx.hash());
        REQUIRE(valueEvent.getTxIndex() == 1);
        REQUIRE(valueEvent.getBlockIndex() == 0);
        REQUIRE(valueEvent.getAddress() == contractAddress);
        REQUIRE(valueEvent.getData() == Bytes());
        REQUIRE(valueEvent.getTopics().at(1) == Utils::padLeftBytes(Utils::uintToBytes(uint256_t("918258172319061203818967178162134821351")), 32)); // at(0) = functor (non-anonymous)
        REQUIRE(!valueEvent.isAnonymous());
      }

      std::unique_ptr<Options> options;
      std::unique_ptr<DB> db;
      std::unique_ptr<ContractManager> contractManager;
      std::unique_ptr<EventManager> eventManager;
      initialize(options, db, contractManager, eventManager,
        testDumpPath + "/SimpleContractSetNameAndSetValue", ownerPrivKey, "TestName", 19283187581, false
      );

      REQUIRE(contractAddress == contractManager->getContracts()[0].second);

      ABI::Encoder getNameEncoder({}, "getName()");
      ABI::Encoder getValueEncoder({}, "getValue()");

      Bytes nameData = contractManager->callContract(buildCallInfo(contractAddress, getNameEncoder.getFunctor(), getNameEncoder.getData()));
      Bytes valueData = contractManager->callContract(buildCallInfo(contractAddress, getValueEncoder.getFunctor(), getValueEncoder.getData()));

      ABI::Decoder nameDecoder({ABI::Types::string}, nameData);
      ABI::Decoder valueDecoder({ABI::Types::uint256}, valueData);

      REQUIRE(nameDecoder.getData<std::string>(0) == "TryThisName");
      REQUIRE(valueDecoder.getData<uint256_t>(0) == uint256_t("918258172319061203818967178162134821351"));

      // No tx hash here but it was already tested before
      Event nameEvent = eventManager->getEvents(
        0, 1, contractAddress, { Utils::sha3(Utils::stringToBytes("TryThisName")).asBytes() }
      ).at(0);

      REQUIRE(nameEvent.getName() == "nameChanged");
      REQUIRE(nameEvent.getLogIndex() == 0);
      REQUIRE(nameEvent.getTxIndex() == 0);
      REQUIRE(nameEvent.getBlockIndex() == 0);
      REQUIRE(nameEvent.getAddress() == contractAddress);
      REQUIRE(nameEvent.getData() == Bytes());
      REQUIRE(nameEvent.getTopics().at(1) == Utils::sha3(Utils::stringToBytes("TryThisName"))); // at(0) = functor (non-anonymous)
      REQUIRE(!nameEvent.isAnonymous());

      Event valueEvent = eventManager->getEvents(
        0, 1, contractAddress, { Utils::padLeftBytes(Utils::uintToBytes(uint256_t("918258172319061203818967178162134821351")), 32) }
      ).at(0);

      REQUIRE(valueEvent.getName() == "valueChanged");
      REQUIRE(valueEvent.getLogIndex() == 0);
      REQUIRE(valueEvent.getTxIndex() == 1);
      REQUIRE(valueEvent.getBlockIndex() == 0);
      REQUIRE(valueEvent.getAddress() == contractAddress);
      REQUIRE(valueEvent.getData() == Bytes());
      REQUIRE(valueEvent.getTopics().at(1) == Utils::padLeftBytes(Utils::uintToBytes(uint256_t("918258172319061203818967178162134821351")), 32)); // at(0) = functor (non-anonymous)
      REQUIRE(!valueEvent.isAnonymous());
    }
  }
}

