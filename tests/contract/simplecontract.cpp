#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/erc20.h"
#include "../../src/contract/abi.h"
#include "../../src/utils/db.h"
#include "../../src/utils/options.h"
#include "../../src/contract/contractmanager.h"
#include "../../src/core/rdpos.h"

#include <filesystem>

/// Forward Declaration.
ethCallInfo buildCallInfo(const Address& addressToCall, const std::string& dataToCall);

void initialize(std::unique_ptr<Options>& options,
                std::unique_ptr<DB>& db,
                std::unique_ptr<ContractManager> &contractManager,
                const std::string& dbName,
                const PrivKey& ownerPrivKey,
                const std::string& name,
                const uint256_t& value,
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
    ABI::Encoder::EncVar createNewSimpleContractVars;
    createNewSimpleContractVars.push_back(name);
    createNewSimpleContractVars.push_back(value);
    ABI::Encoder createNewSimpleContractEncoder(createNewSimpleContractVars);
    std::string createNewSimpleContractData = Hex::toBytes("0x6de23252") + createNewSimpleContractEncoder.getRaw();

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
    PrivKey ownerPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
    Address owner = Secp256k1::toAddress(Secp256k1::toUPub(ownerPrivKey));
    SECTION("SimpleContract creation") {
      Address contractAddress;
      {
        std::unique_ptr<Options> options;
        std::unique_ptr<DB> db;
        std::unique_ptr<ContractManager> contractManager;
        initialize(options, db, contractManager, "SimpleContractCreationTest", ownerPrivKey, "TestName", 19283187581);

        /// Get the contract address.
        contractAddress = contractManager->getContracts()[0].second;

        ABI::Encoder getNameEncoder({}, "getName()");
        ABI::Encoder getValueEncoder({}, "getValue()");

        std::string nameData = contractManager->callContract(buildCallInfo(contractAddress, getNameEncoder.getRaw()));
        std::string valueData = contractManager->callContract(buildCallInfo(contractAddress, getValueEncoder.getRaw()));

        ABI::Decoder nameDecoder({ABI::Types::string}, nameData);
        ABI::Decoder valueDecoder({ABI::Types::uint256}, valueData);

        REQUIRE(nameDecoder.getData<std::string>(0) == "TestName");
        REQUIRE(valueDecoder.getData<uint256_t>(0) == 19283187581);
      }

      std::unique_ptr<Options> options;
      std::unique_ptr<DB> db;
      std::unique_ptr<ContractManager> contractManager;
      initialize(options, db, contractManager, "SimpleContractCreationTest", ownerPrivKey, "TestName", 19283187581, false);


      REQUIRE(contractAddress == contractManager->getContracts()[0].second);

      ABI::Encoder getNameEncoder({}, "getName()");
      ABI::Encoder getValueEncoder({}, "getValue()");

      std::string nameData = contractManager->callContract(buildCallInfo(contractAddress, getNameEncoder.getRaw()));
      std::string valueData = contractManager->callContract(buildCallInfo(contractAddress, getValueEncoder.getRaw()));

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
        initialize(options, db, contractManager, "SimpleContractSetNameAndSetValue", ownerPrivKey, "TestName", 19283187581);

        /// Get the contract address.
        contractAddress = contractManager->getContracts()[0].second;

        ABI::Encoder getNameEncoder({}, "getName()");
        ABI::Encoder getValueEncoder({}, "getValue()");

        std::string nameData = contractManager->callContract(buildCallInfo(contractAddress, getNameEncoder.getRaw()));
        std::string valueData = contractManager->callContract(buildCallInfo(contractAddress, getValueEncoder.getRaw()));

        ABI::Decoder nameDecoder({ABI::Types::string}, nameData);
        ABI::Decoder valueDecoder({ABI::Types::uint256}, valueData);

        ABI::Encoder setNameEncoder({"TryThisName"}, "setName(string)");
        ABI::Encoder setValueEncoder({uint256_t("918258172319061203818967178162134821351")}, "setValue(uint256)");

        TxBlock setNameTx(
          contractAddress,
          owner,
          setNameEncoder.getRaw(),
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
          setValueEncoder.getRaw(),
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

        nameData = contractManager->callContract(buildCallInfo(contractAddress, getNameEncoder.getRaw()));
        valueData = contractManager->callContract(buildCallInfo(contractAddress, getValueEncoder.getRaw()));

        nameDecoder = ABI::Decoder({ABI::Types::string}, nameData);
        valueDecoder = ABI::Decoder({ABI::Types::uint256}, valueData);

        REQUIRE(nameDecoder.getData<std::string>(0) == "TryThisName");
        REQUIRE(valueDecoder.getData<uint256_t>(0) == uint256_t("918258172319061203818967178162134821351"));
      }

      std::unique_ptr<Options> options;
      std::unique_ptr<DB> db;
      std::unique_ptr<ContractManager> contractManager;
      initialize(options, db, contractManager, "SimpleContractSetNameAndSetValue", ownerPrivKey, "TestName", 19283187581, false);


      REQUIRE(contractAddress == contractManager->getContracts()[0].second);

      ABI::Encoder getNameEncoder({}, "getName()");
      ABI::Encoder getValueEncoder({}, "getValue()");

      std::string nameData = contractManager->callContract(buildCallInfo(contractAddress, getNameEncoder.getRaw()));
      std::string valueData = contractManager->callContract(buildCallInfo(contractAddress, getValueEncoder.getRaw()));

      ABI::Decoder nameDecoder({ABI::Types::string}, nameData);
      ABI::Decoder valueDecoder({ABI::Types::uint256}, valueData);

      REQUIRE(nameDecoder.getData<std::string>(0) == "TryThisName");
      REQUIRE(valueDecoder.getData<uint256_t>(0) == uint256_t("918258172319061203818967178162134821351"));
    }
  }
}