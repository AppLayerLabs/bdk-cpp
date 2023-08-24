#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/templates/erc20.h"
#include "../../src/contract/abi.h"
#include "../../src/utils/db.h"
#include "../../src/utils/options.h"
#include "../../src/contract/contractmanager.h"
#include "../../src/core/rdpos.h"

#include <filesystem>

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
    ABI::Encoder::EncVar createNewERC20ContractVars;
    createNewERC20ContractVars.push_back("TestToken");
    createNewERC20ContractVars.push_back("TST");
    createNewERC20ContractVars.push_back(static_cast<uint256_t>(18));
    createNewERC20ContractVars.push_back(static_cast<uint256_t>(1000000000000000000));
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

        ABI::Encoder::EncVar getAllowanceVars;
        getAllowanceVars.push_back(owner);
        getAllowanceVars.push_back(wrapperAddress);
        ABI::Encoder getAllowanceEncoder(getAllowanceVars, "allowance(address,address)");

        ABI::Encoder::EncVar depositVars;
        depositVars.push_back(erc20Address);
        depositVars.push_back(static_cast<uint256_t>(500000000000000000));
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

        // Try to deposit without approving first.
        REQUIRE_THROWS(contractManager->callContract(depositTx));

        ABI::Encoder::EncVar approveVars;
        approveVars.push_back(wrapperAddress);
        approveVars.push_back(static_cast<uint256_t>(500000000000000000));
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
        REQUIRE(getAllowanceDecoder.getData<uint256_t>(0) == 500000000000000000);

        contractManager->callContract(depositTx);
        Bytes getAllowanceResult2 = contractManager->callContract(
          buildCallInfo(erc20Address, getAllowanceEncoder.getFunctor(), getAllowanceEncoder.getData()));
        ABI::Decoder getAllowanceDecoder2({ABI::Types::uint256}, getAllowanceResult2);
        REQUIRE(getAllowanceDecoder2.getData<uint256_t>(0) == 0);


        ABI::Encoder getContractBalanceEncoder({erc20Address}, "getContractBalance(address)");
        Bytes getContractBalanceResult = contractManager->callContract(
          buildCallInfo(wrapperAddress, getContractBalanceEncoder.getFunctor(), getContractBalanceEncoder.getData()));
        ABI::Decoder getContractBalanceDecoder({ABI::Types::uint256}, getContractBalanceResult);
        REQUIRE(getContractBalanceDecoder.getData<uint256_t>(0) == 500000000000000000);

        ABI::Encoder getUserBalanceEncoder({erc20Address, owner}, "getUserBalance(address,address)");
        Bytes getUserBalanceResult = contractManager->callContract(
          buildCallInfo(wrapperAddress, getUserBalanceEncoder.getFunctor(), getUserBalanceEncoder.getData()));
        ABI::Decoder getUserBalanceDecoder({ABI::Types::uint256}, getUserBalanceResult);
        REQUIRE(getUserBalanceDecoder.getData<uint256_t>(0) == 500000000000000000);

        ABI::Encoder getBalanceEncoder({owner}, "balanceOf(address)");
        Bytes getBalanceResult = contractManager->callContract(
          buildCallInfo(erc20Address, getBalanceEncoder.getFunctor(), getBalanceEncoder.getData()));
        ABI::Decoder getBalanceDecoder({ABI::Types::uint256}, getBalanceResult);
        REQUIRE(getBalanceDecoder.getData<uint256_t>(0) == 500000000000000000);

        ABI::Encoder getBalanceWrapperEncoder({wrapperAddress}, "balanceOf(address)");
        Bytes getBalanceWrapperResult = contractManager->callContract(
          buildCallInfo(erc20Address, getBalanceWrapperEncoder.getFunctor(), getBalanceWrapperEncoder.getData()));
        ABI::Decoder getBalanceWrapperDecoder({ABI::Types::uint256}, getBalanceWrapperResult);
        REQUIRE(getBalanceWrapperDecoder.getData<uint256_t>(0) == 500000000000000000);

      }
      std::unique_ptr<Options> options;
      std::unique_ptr<DB> db;
      std::unique_ptr<ContractManager> contractManager;
      std::string dbName = testDumpPath + "/erc20wrapperDb";
      initialize(options, db, contractManager, dbName, ownerPrivKey, false);

      ABI::Encoder::EncVar getAllowanceVars;
      getAllowanceVars.push_back(owner);
      getAllowanceVars.push_back(wrapperAddress);
      ABI::Encoder getAllowanceEncoder(getAllowanceVars, "allowance(address,address)");

      Bytes getAllowanceResult = contractManager->callContract(buildCallInfo(erc20Address, getAllowanceEncoder.getFunctor(), getAllowanceEncoder.getData()));
      ABI::Decoder getAllowanceDecoder({ABI::Types::uint256}, getAllowanceResult);
      REQUIRE(getAllowanceDecoder.getData<uint256_t>(0) == 0);

      ABI::Encoder getContractBalanceEncoder({erc20Address}, "getContractBalance(address)");
      Bytes getContractBalanceResult = contractManager->callContract(buildCallInfo(wrapperAddress, getContractBalanceEncoder.getFunctor(), getContractBalanceEncoder.getData()));
      ABI::Decoder getContractBalanceDecoder({ABI::Types::uint256}, getContractBalanceResult);
      REQUIRE(getContractBalanceDecoder.getData<uint256_t>(0) == 500000000000000000);

      ABI::Encoder getUserBalanceEncoder({erc20Address, owner}, "getUserBalance(address,address)");
      Bytes getUserBalanceResult = contractManager->callContract(buildCallInfo(wrapperAddress, getUserBalanceEncoder.getFunctor(), getUserBalanceEncoder.getData()));
      ABI::Decoder getUserBalanceDecoder({ABI::Types::uint256}, getUserBalanceResult);
      REQUIRE(getUserBalanceDecoder.getData<uint256_t>(0) == 500000000000000000);

      ABI::Encoder getBalanceEncoder({owner}, "balanceOf(address)");
      Bytes getBalanceResult = contractManager->callContract(buildCallInfo(erc20Address, getBalanceEncoder.getFunctor(), getBalanceEncoder.getData()));
      ABI::Decoder getBalanceDecoder({ABI::Types::uint256}, getBalanceResult);
      REQUIRE(getBalanceDecoder.getData<uint256_t>(0) == 500000000000000000);

      ABI::Encoder getBalanceWrapperEncoder({wrapperAddress}, "balanceOf(address)");
      Bytes getBalanceWrapperResult = contractManager->callContract(buildCallInfo(erc20Address, getBalanceWrapperEncoder.getFunctor(), getBalanceWrapperEncoder.getData()));
      ABI::Decoder getBalanceWrapperDecoder({ABI::Types::uint256}, getBalanceWrapperResult);
      REQUIRE(getBalanceWrapperDecoder.getData<uint256_t>(0) == 500000000000000000);
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

        ABI::Encoder::EncVar getAllowanceVars;
        getAllowanceVars.push_back(owner);
        getAllowanceVars.push_back(wrapperAddress);
        ABI::Encoder getAllowanceEncoder(getAllowanceVars, "allowance(address,address)");

        ABI::Encoder::EncVar depositVars;
        depositVars.push_back(erc20Address);
        depositVars.push_back(static_cast<uint256_t>(500000000000000000));
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

        REQUIRE_THROWS(contractManager->callContract(depositTx));

        ABI::Encoder::EncVar approveVars;
        approveVars.push_back(wrapperAddress);
        approveVars.push_back(static_cast<uint256_t>(500000000000000000));
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
        REQUIRE(getAllowanceDecoder.getData<uint256_t>(0) == 500000000000000000);

        contractManager->callContract(depositTx);
        Bytes getAllowanceResult2 = contractManager->callContract(
          buildCallInfo(erc20Address, getAllowanceEncoder.getFunctor(), getAllowanceEncoder.getData()));
        ABI::Decoder getAllowanceDecoder2({ABI::Types::uint256}, getAllowanceResult2);
        REQUIRE(getAllowanceDecoder2.getData<uint256_t>(0) == 0);


        ABI::Encoder getContractBalanceEncoder({erc20Address}, "getContractBalance(address)");
        Bytes getContractBalanceResult = contractManager->callContract(
          buildCallInfo(wrapperAddress, getContractBalanceEncoder.getFunctor(), getContractBalanceEncoder.getData()));
        ABI::Decoder getContractBalanceDecoder({ABI::Types::uint256}, getContractBalanceResult);
        REQUIRE(getContractBalanceDecoder.getData<uint256_t>(0) == 500000000000000000);

        ABI::Encoder getUserBalanceEncoder({erc20Address, owner}, "getUserBalance(address,address)");
        Bytes getUserBalanceResult = contractManager->callContract(
          buildCallInfo(wrapperAddress, getUserBalanceEncoder.getFunctor(), getUserBalanceEncoder.getData()));
        ABI::Decoder getUserBalanceDecoder({ABI::Types::uint256}, getUserBalanceResult);
        REQUIRE(getUserBalanceDecoder.getData<uint256_t>(0) == 500000000000000000);

        ABI::Encoder getBalanceEncoder({owner}, "balanceOf(address)");
        Bytes getBalanceResult = contractManager->callContract(
          buildCallInfo(erc20Address, getBalanceEncoder.getFunctor(), getBalanceEncoder.getData()));
        ABI::Decoder getBalanceDecoder({ABI::Types::uint256}, getBalanceResult);
        REQUIRE(getBalanceDecoder.getData<uint256_t>(0) == 500000000000000000);

        ABI::Encoder getBalanceWrapperEncoder({wrapperAddress}, "balanceOf(address)");
        Bytes getBalanceWrapperResult = contractManager->callContract(
          buildCallInfo(erc20Address, getBalanceWrapperEncoder.getFunctor(), getBalanceWrapperEncoder.getData()));
        ABI::Decoder getBalanceWrapperDecoder({ABI::Types::uint256}, getBalanceWrapperResult);
        REQUIRE(getBalanceWrapperDecoder.getData<uint256_t>(0) == 500000000000000000);

        ABI::Encoder::EncVar withdrawVars;
        withdrawVars.push_back(erc20Address);
        withdrawVars.push_back(static_cast<uint256_t>(250000000000000000));
        ABI::Encoder withdrawEncoder(withdrawVars);
        Bytes withdrawData = Hex::toBytes("0xf3fef3a3");
        Utils::appendBytes(withdrawData, withdrawEncoder.getData());
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
          buildCallInfo(wrapperAddress, getContractBalanceEncoder.getFunctor(), getContractBalanceEncoder.getData()));
        getContractBalanceDecoder = ABI::Decoder({ABI::Types::uint256}, getContractBalanceResult);
        REQUIRE(getContractBalanceDecoder.getData<uint256_t>(0) == 250000000000000000);

        getUserBalanceResult = contractManager->callContract(
          buildCallInfo(wrapperAddress, getUserBalanceEncoder.getFunctor(), getUserBalanceEncoder.getData()));
        getUserBalanceDecoder = ABI::Decoder({ABI::Types::uint256}, getUserBalanceResult);
        REQUIRE(getUserBalanceDecoder.getData<uint256_t>(0) == 250000000000000000);

        getBalanceResult = contractManager->callContract(
          buildCallInfo(erc20Address, getBalanceEncoder.getFunctor(), getBalanceEncoder.getData()));
        getBalanceDecoder = ABI::Decoder({ABI::Types::uint256}, getBalanceResult);
        REQUIRE(getBalanceDecoder.getData<uint256_t>(0) == 750000000000000000);

        getBalanceWrapperResult = contractManager->callContract(
          buildCallInfo(erc20Address, getBalanceWrapperEncoder.getFunctor(), getBalanceWrapperEncoder.getData()));
        getBalanceWrapperDecoder = ABI::Decoder({ABI::Types::uint256}, getBalanceWrapperResult);
        REQUIRE(getBalanceWrapperDecoder.getData<uint256_t>(0) == 250000000000000000);
      }
      std::unique_ptr<Options> options;
      std::unique_ptr<DB> db;
      std::unique_ptr<ContractManager> contractManager;
      std::string dbName = testDumpPath + "/erc20wrapperDb";
      initialize(options, db, contractManager, dbName, ownerPrivKey, false);

      ABI::Encoder::EncVar getAllowanceVars;
      getAllowanceVars.push_back(owner);
      getAllowanceVars.push_back(wrapperAddress);
      ABI::Encoder getAllowanceEncoder(getAllowanceVars, "allowance(address,address)");

      Bytes getAllowanceResult = contractManager->callContract(buildCallInfo(erc20Address, getAllowanceEncoder.getFunctor(), getAllowanceEncoder.getData()));
      ABI::Decoder getAllowanceDecoder({ABI::Types::uint256}, getAllowanceResult);
      REQUIRE(getAllowanceDecoder.getData<uint256_t>(0) == 0);

      ABI::Encoder getContractBalanceEncoder({erc20Address}, "getContractBalance(address)");
      Bytes getContractBalanceResult = contractManager->callContract(buildCallInfo(wrapperAddress, getContractBalanceEncoder.getFunctor(), getContractBalanceEncoder.getData()));
      ABI::Decoder getContractBalanceDecoder({ABI::Types::uint256}, getContractBalanceResult);
      REQUIRE(getContractBalanceDecoder.getData<uint256_t>(0) == 250000000000000000);

      ABI::Encoder getUserBalanceEncoder({erc20Address, owner}, "getUserBalance(address,address)");
      Bytes getUserBalanceResult = contractManager->callContract(buildCallInfo(wrapperAddress, getUserBalanceEncoder.getFunctor(), getUserBalanceEncoder.getData()));
      ABI::Decoder getUserBalanceDecoder({ABI::Types::uint256}, getUserBalanceResult);
      REQUIRE(getUserBalanceDecoder.getData<uint256_t>(0) == 250000000000000000);

      ABI::Encoder getBalanceEncoder({owner}, "balanceOf(address)");
      Bytes getBalanceResult = contractManager->callContract(buildCallInfo(erc20Address, getBalanceEncoder.getFunctor(), getBalanceEncoder.getData()));
      ABI::Decoder getBalanceDecoder({ABI::Types::uint256}, getBalanceResult);
      REQUIRE(getBalanceDecoder.getData<uint256_t>(0) == 750000000000000000);

      ABI::Encoder getBalanceWrapperEncoder({wrapperAddress}, "balanceOf(address)");
      Bytes getBalanceWrapperResult = contractManager->callContract(buildCallInfo(erc20Address, getBalanceWrapperEncoder.getFunctor(), getBalanceWrapperEncoder.getData()));
      ABI::Decoder getBalanceWrapperDecoder({ABI::Types::uint256}, getBalanceWrapperResult);
      REQUIRE(getBalanceWrapperDecoder.getData<uint256_t>(0) == 250000000000000000);
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

        ABI::Encoder::EncVar getAllowanceVars;
        getAllowanceVars.push_back(owner);
        getAllowanceVars.push_back(wrapperAddress);
        ABI::Encoder getAllowanceEncoder(getAllowanceVars, "allowance(address,address)");

        ABI::Encoder::EncVar depositVars;
        depositVars.push_back(erc20Address);
        depositVars.push_back(static_cast<uint256_t>(500000000000000000));
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

        REQUIRE_THROWS(contractManager->callContract(depositTx));

        ABI::Encoder::EncVar approveVars;
        approveVars.push_back(wrapperAddress);
        approveVars.push_back(static_cast<uint256_t>(500000000000000000));
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
        REQUIRE(getAllowanceDecoder.getData<uint256_t>(0) == 500000000000000000);

        contractManager->callContract(depositTx);
        Bytes getAllowanceResult2 = contractManager->callContract(
          buildCallInfo(erc20Address, getAllowanceEncoder.getFunctor(), getAllowanceEncoder.getData()));
        ABI::Decoder getAllowanceDecoder2({ABI::Types::uint256}, getAllowanceResult2);
        REQUIRE(getAllowanceDecoder2.getData<uint256_t>(0) == 0);


        ABI::Encoder getContractBalanceEncoder({erc20Address}, "getContractBalance(address)");
        Bytes getContractBalanceResult = contractManager->callContract(
          buildCallInfo(wrapperAddress, getContractBalanceEncoder.getFunctor(), getContractBalanceEncoder.getData()));
        ABI::Decoder getContractBalanceDecoder({ABI::Types::uint256}, getContractBalanceResult);
        REQUIRE(getContractBalanceDecoder.getData<uint256_t>(0) == 500000000000000000);

        ABI::Encoder getUserBalanceEncoder({erc20Address, owner}, "getUserBalance(address,address)");
        Bytes getUserBalanceResult = contractManager->callContract(
          buildCallInfo(wrapperAddress, getUserBalanceEncoder.getFunctor(), getUserBalanceEncoder.getData()));
        ABI::Decoder getUserBalanceDecoder({ABI::Types::uint256}, getUserBalanceResult);
        REQUIRE(getUserBalanceDecoder.getData<uint256_t>(0) == 500000000000000000);

        ABI::Encoder getBalanceEncoder({owner}, "balanceOf(address)");
        Bytes getBalanceResult = contractManager->callContract(
          buildCallInfo(erc20Address, getBalanceEncoder.getFunctor(), getBalanceEncoder.getData()));
        ABI::Decoder getBalanceDecoder({ABI::Types::uint256}, getBalanceResult);
        REQUIRE(getBalanceDecoder.getData<uint256_t>(0) == 500000000000000000);

        ABI::Encoder getBalanceWrapperEncoder({wrapperAddress}, "balanceOf(address)");
        Bytes getBalanceWrapperResult = contractManager->callContract(
          buildCallInfo(erc20Address, getBalanceWrapperEncoder.getFunctor(), getBalanceWrapperEncoder.getData()));
        ABI::Decoder getBalanceWrapperDecoder({ABI::Types::uint256}, getBalanceWrapperResult);
        REQUIRE(getBalanceWrapperDecoder.getData<uint256_t>(0) == 500000000000000000);

        ABI::Encoder::EncVar transferToVars;
        transferToVars.push_back(erc20Address);
        transferToVars.push_back(destinationOfTransfers);
        transferToVars.push_back(static_cast<uint256_t>(250000000000000000));
        ABI::Encoder transferToEncoder(transferToVars);
        Bytes transferToData = Hex::toBytes("0xa5f2a152");
        Utils::appendBytes(transferToData, transferToEncoder.getData());
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
          buildCallInfo(wrapperAddress, getContractBalanceEncoder.getFunctor(), getContractBalanceEncoder.getData()));
        getContractBalanceDecoder = ABI::Decoder({ABI::Types::uint256}, getContractBalanceResult);
        REQUIRE(getContractBalanceDecoder.getData<uint256_t>(0) == 250000000000000000);

        getUserBalanceResult = contractManager->callContract(
          buildCallInfo(wrapperAddress, getUserBalanceEncoder.getFunctor(), getUserBalanceEncoder.getData()));
        getUserBalanceDecoder = ABI::Decoder({ABI::Types::uint256}, getUserBalanceResult);
        REQUIRE(getUserBalanceDecoder.getData<uint256_t>(0) == 250000000000000000);

        getBalanceResult = contractManager->callContract(
          buildCallInfo(erc20Address, getBalanceEncoder.getFunctor(), getBalanceEncoder.getData()));
        getBalanceDecoder = ABI::Decoder({ABI::Types::uint256}, getBalanceResult);
        REQUIRE(getBalanceDecoder.getData<uint256_t>(0) == 500000000000000000);

        getBalanceWrapperResult = contractManager->callContract(
          buildCallInfo(erc20Address, getBalanceWrapperEncoder.getFunctor(), getBalanceWrapperEncoder.getData()));
        getBalanceWrapperDecoder = ABI::Decoder({ABI::Types::uint256}, getBalanceWrapperResult);
        REQUIRE(getBalanceWrapperDecoder.getData<uint256_t>(0) == 250000000000000000);

        ABI::Encoder getBalanceDestinationEncoder({destinationOfTransfers}, "balanceOf(address)");
        Bytes getBalanceDestinationResult = contractManager->callContract(
          buildCallInfo(erc20Address, getBalanceDestinationEncoder.getFunctor(), getBalanceDestinationEncoder.getData()));
        ABI::Decoder getBalanceDestinationDecoder({ABI::Types::uint256}, getBalanceDestinationResult);
        REQUIRE(getBalanceDestinationDecoder.getData<uint256_t>(0) == 250000000000000000);
      }
      std::unique_ptr<Options> options;
      std::unique_ptr<DB> db;
      std::unique_ptr<ContractManager> contractManager;
      std::string dbName = testDumpPath + "/erc20wrapperDb";
      initialize(options, db, contractManager, dbName, ownerPrivKey, false);

      ABI::Encoder::EncVar getAllowanceVars;
      getAllowanceVars.push_back(owner);
      getAllowanceVars.push_back(wrapperAddress);
      ABI::Encoder getAllowanceEncoder(getAllowanceVars, "allowance(address,address)");

      Bytes getAllowanceResult = contractManager->callContract(buildCallInfo(erc20Address, getAllowanceEncoder.getFunctor(), getAllowanceEncoder.getData()));
      ABI::Decoder getAllowanceDecoder({ABI::Types::uint256}, getAllowanceResult);
      REQUIRE(getAllowanceDecoder.getData<uint256_t>(0) == 0);

      ABI::Encoder getContractBalanceEncoder({erc20Address}, "getContractBalance(address)");
      Bytes getContractBalanceResult = contractManager->callContract(buildCallInfo(wrapperAddress, getContractBalanceEncoder.getFunctor(), getContractBalanceEncoder.getData()));
      ABI::Decoder getContractBalanceDecoder({ABI::Types::uint256}, getContractBalanceResult);
      REQUIRE(getContractBalanceDecoder.getData<uint256_t>(0) == 250000000000000000);

      ABI::Encoder getUserBalanceEncoder({erc20Address, owner}, "getUserBalance(address,address)");
      Bytes getUserBalanceResult = contractManager->callContract(buildCallInfo(wrapperAddress, getUserBalanceEncoder.getFunctor(), getUserBalanceEncoder.getData()));
      ABI::Decoder getUserBalanceDecoder({ABI::Types::uint256}, getUserBalanceResult);
      REQUIRE(getUserBalanceDecoder.getData<uint256_t>(0) == 250000000000000000);

      ABI::Encoder getBalanceEncoder({owner}, "balanceOf(address)");
      Bytes getBalanceResult = contractManager->callContract(buildCallInfo(erc20Address, getBalanceEncoder.getFunctor(), getBalanceEncoder.getData()));
      ABI::Decoder getBalanceDecoder({ABI::Types::uint256}, getBalanceResult);
      REQUIRE(getBalanceDecoder.getData<uint256_t>(0) == 500000000000000000);

      ABI::Encoder getBalanceWrapperEncoder({wrapperAddress}, "balanceOf(address)");
      Bytes getBalanceWrapperResult = contractManager->callContract(buildCallInfo(erc20Address, getBalanceWrapperEncoder.getFunctor(), getBalanceWrapperEncoder.getData()));
      ABI::Decoder getBalanceWrapperDecoder({ABI::Types::uint256}, getBalanceWrapperResult);
      REQUIRE(getBalanceWrapperDecoder.getData<uint256_t>(0) == 250000000000000000);

      ABI::Encoder getBalanceDestinationEncoder({destinationOfTransfers}, "balanceOf(address)");
      Bytes getBalanceDestinationResult = contractManager->callContract(
        buildCallInfo(erc20Address, getBalanceDestinationEncoder.getFunctor(), getBalanceDestinationEncoder.getData()));
      ABI::Decoder getBalanceDestinationDecoder({ABI::Types::uint256}, getBalanceDestinationResult);
      REQUIRE(getBalanceDestinationDecoder.getData<uint256_t>(0) == 250000000000000000);
    }
  }
}
