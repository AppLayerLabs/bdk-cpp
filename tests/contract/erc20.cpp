#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/erc20.h"
#include "../../src/contract/abi.h"
#include "../../src/utils/db.h"
#include "../../src/utils/options.h"
#include "../../src/contract/contractmanager.h"
#include "../../src/core/rdpos.h"

#include <filesystem>

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
    ABI::Encoder::EncVar createNewERC20ContractVars;
    createNewERC20ContractVars.push_back(tokenName);
    createNewERC20ContractVars.push_back(tokenSymbol);
    createNewERC20ContractVars.push_back(tokenDecimals);
    createNewERC20ContractVars.push_back(tokenSupply);
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
  }
}

namespace TERC20 {
  TEST_CASE("ERC2O Class", "[contract][erc20]") {
    PrivKey ownerPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
    Address owner = Secp256k1::toAddress(Secp256k1::toUPub(ownerPrivKey));
    SECTION("ERC20 Class Constructor") {
      Address erc20Address;
      {
        std::unique_ptr<Options> options;
        std::unique_ptr<DB> db;
        std::unique_ptr<ContractManager> contractManager;
        std::string dbName = "erc20ClassConstructor";
        std::string tokenName = "TestToken";
        std::string tokenSymbol = "TST";
        uint8_t tokenDecimals = 18;
        uint256_t tokenSupply = 1000000000000000000;
        initialize(options, db, contractManager, dbName, ownerPrivKey, tokenName, tokenSymbol, tokenDecimals, tokenSupply);

        erc20Address = contractManager->getContracts()[0].second;
        ABI::Encoder nameEncoder({}, "name()");
        ABI::Encoder symbolEncoder({}, "symbol()");
        ABI::Encoder decimalsEncoder({}, "decimals()");
        ABI::Encoder totalSupplyEncoder({}, "totalSupply()");

        Bytes nameData = contractManager->callContract(buildCallInfo(erc20Address, nameEncoder.getFunctor(), nameEncoder.getData()));
        ABI::Decoder nameDecoder({ABI::Types::string}, nameData);
        REQUIRE(nameDecoder.getData<std::string>(0) == tokenName);

        Bytes symbolData = contractManager->callContract(buildCallInfo(erc20Address, symbolEncoder.getFunctor(), symbolEncoder.getData()));
        ABI::Decoder symbolDecoder({ABI::Types::string}, symbolData);
        REQUIRE(symbolDecoder.getData<std::string>(0) == tokenSymbol);

        Bytes decimalsData = contractManager->callContract(buildCallInfo(erc20Address, decimalsEncoder.getFunctor(), decimalsEncoder.getData()));
        ABI::Decoder decimalsDecoder({ABI::Types::uint256}, decimalsData);
        REQUIRE(decimalsDecoder.getData<uint256_t>(0) == 18);

        Bytes totalSupplyData = contractManager->callContract(buildCallInfo(erc20Address, totalSupplyEncoder.getFunctor(), totalSupplyEncoder.getData()));
        ABI::Decoder totalSupplyDecoder({ABI::Types::uint256}, totalSupplyData);
        REQUIRE(totalSupplyDecoder.getData<uint256_t>(0) == tokenSupply);

        ABI::Encoder::EncVar balanceOfVars;
        balanceOfVars.push_back(owner);
        ABI::Encoder balanceOfEncoder(balanceOfVars, "balanceOf(address)");
        Bytes balanceOfData = contractManager->callContract(buildCallInfo(erc20Address, totalSupplyEncoder.getFunctor(), totalSupplyEncoder.getData()));
        ABI::Decoder balanceOfDecoder({ABI::Types::uint256}, balanceOfData);
        REQUIRE(balanceOfDecoder.getData<uint256_t>(0) == tokenSupply);
      }

      std::unique_ptr<Options> options;
      std::unique_ptr<DB> db;
      std::unique_ptr<ContractManager> contractManager;
      std::string dbName = "erc20ClassConstructor";
      std::string tokenName = "TestToken";
      std::string tokenSymbol = "TST";
      uint8_t tokenDecimals = 18;
      uint256_t tokenSupply = 1000000000000000000;
      initialize(options, db, contractManager, dbName, ownerPrivKey, tokenName, tokenSymbol, tokenDecimals, tokenSupply, false);

      REQUIRE(erc20Address == contractManager->getContracts()[0].second);
      ABI::Encoder nameEncoder({}, "name()");
      ABI::Encoder symbolEncoder({}, "symbol()");
      ABI::Encoder decimalsEncoder({}, "decimals()");
      ABI::Encoder totalSupplyEncoder({}, "totalSupply()");

      Bytes nameData = contractManager->callContract(buildCallInfo(erc20Address, nameEncoder.getFunctor(), nameEncoder.getData()));
      ABI::Decoder nameDecoder({ABI::Types::string}, nameData);
      REQUIRE(nameDecoder.getData<std::string>(0) == tokenName);

      Bytes symbolData = contractManager->callContract(buildCallInfo(erc20Address, symbolEncoder.getFunctor(), symbolEncoder.getData()));
      ABI::Decoder symbolDecoder({ABI::Types::string}, symbolData);
      REQUIRE(symbolDecoder.getData<std::string>(0) == tokenSymbol);

      Bytes decimalsData = contractManager->callContract(buildCallInfo(erc20Address, decimalsEncoder.getFunctor(), decimalsEncoder.getData()));
      ABI::Decoder decimalsDecoder({ABI::Types::uint256}, decimalsData);
      REQUIRE(decimalsDecoder.getData<uint256_t>(0) == 18);

      Bytes totalSupplyData = contractManager->callContract(buildCallInfo(erc20Address, totalSupplyEncoder.getFunctor(), totalSupplyEncoder.getData()));
      ABI::Decoder totalSupplyDecoder({ABI::Types::uint256}, totalSupplyData);
      REQUIRE(totalSupplyDecoder.getData<uint256_t>(0) == tokenSupply);

      ABI::Encoder::EncVar balanceOfVars;
      balanceOfVars.push_back(owner);
      ABI::Encoder balanceOfEncoder(balanceOfVars, "balanceOf(address)");
      Bytes balanceOfData = contractManager->callContract(buildCallInfo(erc20Address, totalSupplyEncoder.getFunctor(), totalSupplyEncoder.getData()));
      ABI::Decoder balanceOfDecoder({ABI::Types::uint256}, balanceOfData);
      REQUIRE(balanceOfDecoder.getData<uint256_t>(0) == tokenSupply);
    }

    SECTION("ERC20 transfer()") {
      Address erc20Address;
      Address destinationOfTransactions(Utils::randBytes(20));

      {
        std::unique_ptr<Options> options;
        std::unique_ptr<DB> db;
        std::unique_ptr<ContractManager> contractManager;
        std::string dbName = "erc20ClassTransfer";
        std::string tokenName = "TestToken";
        std::string tokenSymbol = "TST";
        uint8_t tokenDecimals = 18;
        uint256_t tokenSupply = 1000000000000000000;
        initialize(options, db, contractManager, dbName, ownerPrivKey, tokenName, tokenSymbol, tokenDecimals,
                   tokenSupply);

        erc20Address = contractManager->getContracts()[0].second;

        ABI::Encoder::EncVar getBalanceMeVars;
        getBalanceMeVars.push_back(owner);
        ABI::Encoder getBalanceMeEncoder(getBalanceMeVars, "balanceOf(address)");

        ABI::Encoder::EncVar getBalanceDestinationVars;
        getBalanceDestinationVars.push_back(destinationOfTransactions);
        ABI::Encoder getBalanceDestinationEncoder(getBalanceDestinationVars, "balanceOf(address)");

        ABI::Encoder::EncVar transferVars;
        transferVars.push_back(destinationOfTransactions);
        transferVars.push_back(500000000000000000);
        ABI::Encoder transferEncoder(transferVars);
        Bytes transferData = Hex::toBytes("0xa9059cbb");
        Utils::appendBytes(transferData, transferEncoder.getData());
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

        auto randomPrivKey = PrivKey(Utils::randBytes(32));
        TxBlock transferTxThrows = TxBlock(
          erc20Address,
          Secp256k1::toAddress(Secp256k1::toUPub(randomPrivKey)),
          transferData,
          8080,
          0,
          0,
          0,
          0,
          0,
          randomPrivKey
        );

        REQUIRE_THROWS(contractManager->validateCallContractWithTx(transferTxThrows.txToCallInfo()));
        contractManager->validateCallContractWithTx(transferTx.txToCallInfo());

        Bytes getBalanceMeResult = contractManager->callContract(buildCallInfo(erc20Address, getBalanceMeEncoder.getFunctor(), getBalanceMeEncoder.getData()));
        ABI::Decoder getBalanceMeDecoder({ABI::Types::uint256}, getBalanceMeResult);
        REQUIRE(getBalanceMeDecoder.getData<uint256_t>(0) == 1000000000000000000);

        Bytes getBalanceDestinationResult = contractManager->callContract(buildCallInfo(erc20Address, getBalanceDestinationEncoder.getFunctor(), getBalanceDestinationEncoder.getData()));
        ABI::Decoder getBalanceDestinationDecoder({ABI::Types::uint256}, getBalanceDestinationResult);
        REQUIRE(getBalanceDestinationDecoder.getData<uint256_t>(0) == 0);

        REQUIRE_THROWS(contractManager->callContract(transferTxThrows));
        contractManager->callContract(transferTx);

        getBalanceMeResult = contractManager->callContract(buildCallInfo(erc20Address, getBalanceMeEncoder.getFunctor(), getBalanceMeEncoder.getData()));
        getBalanceMeDecoder = ABI::Decoder({ABI::Types::uint256}, getBalanceMeResult);
        REQUIRE(getBalanceMeDecoder.getData<uint256_t>(0) == 500000000000000000);

        getBalanceDestinationResult = contractManager->callContract(buildCallInfo(erc20Address, getBalanceDestinationEncoder.getFunctor(), getBalanceDestinationEncoder.getData()));
        getBalanceDestinationDecoder = ABI::Decoder({ABI::Types::uint256}, getBalanceDestinationResult);
        REQUIRE(getBalanceDestinationDecoder.getData<uint256_t>(0) == 500000000000000000);

      }
      std::unique_ptr<Options> options;
      std::unique_ptr<DB> db;
      std::unique_ptr<ContractManager> contractManager;
      std::string dbName = "erc20ClassTransfer";
      std::string tokenName = "TestToken";
      std::string tokenSymbol = "TST";
      uint8_t tokenDecimals = 18;
      uint256_t tokenSupply = 1000000000000000000;
      initialize(options, db, contractManager, dbName, ownerPrivKey, tokenName, tokenSymbol, tokenDecimals,
                   tokenSupply, false);

      REQUIRE(erc20Address == contractManager->getContracts()[0].second);

      ABI::Encoder::EncVar getBalanceMeVars;
      getBalanceMeVars.push_back(owner);
      ABI::Encoder getBalanceMeEncoder(getBalanceMeVars, "balanceOf(address)");

      ABI::Encoder::EncVar getBalanceDestinationVars;
      getBalanceDestinationVars.push_back(destinationOfTransactions);
      ABI::Encoder getBalanceDestinationEncoder(getBalanceDestinationVars, "balanceOf(address)");

      Bytes getBalanceMeResult = contractManager->callContract(buildCallInfo(erc20Address, getBalanceMeEncoder.getFunctor(), getBalanceMeEncoder.getData()));
      ABI::Decoder getBalanceMeDecoder({ABI::Types::uint256}, getBalanceMeResult);
      REQUIRE(getBalanceMeDecoder.getData<uint256_t>(0) == 500000000000000000);

      Bytes getBalanceDestinationResult = contractManager->callContract(buildCallInfo(erc20Address, getBalanceDestinationEncoder.getFunctor(), getBalanceDestinationEncoder.getData()));
      ABI::Decoder getBalanceDestinationDecoder({ABI::Types::uint256}, getBalanceDestinationResult);
      REQUIRE(getBalanceDestinationDecoder.getData<uint256_t>(0) == 500000000000000000);
    }

    SECTION("ERC20 Approve") {
      Address erc20Address;
      Address destinationOfApproval(Utils::randBytes(20));

      {
        std::unique_ptr<Options> options;
        std::unique_ptr<DB> db;
        std::unique_ptr<ContractManager> contractManager;
        std::string dbName = "erc20ClassApprove";
        std::string tokenName = "TestToken";
        std::string tokenSymbol = "TST";
        uint8_t tokenDecimals = 18;
        uint256_t tokenSupply = 1000000000000000000;
        initialize(options, db, contractManager, dbName, ownerPrivKey, tokenName, tokenSymbol, tokenDecimals,
                   tokenSupply);

        erc20Address = contractManager->getContracts()[0].second;
        ABI::Encoder::EncVar getAllowanceVars;
        getAllowanceVars.push_back(owner);
        getAllowanceVars.push_back(destinationOfApproval);
        ABI::Encoder getAllowanceEncoder(getAllowanceVars, "allowance(address,address)");

        ABI::Encoder::EncVar approveVars;
        approveVars.push_back(destinationOfApproval);
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

        contractManager->validateCallContractWithTx(approveTx.txToCallInfo());

        Bytes getAllowanceResult = contractManager->callContract(buildCallInfo(erc20Address, getAllowanceEncoder.getFunctor(), getAllowanceEncoder.getData()));
        ABI::Decoder getAllowanceDecoder({ABI::Types::uint256}, getAllowanceResult);
        REQUIRE(getAllowanceDecoder.getData<uint256_t>(0) == 0);

        contractManager->callContract(approveTx);

        getAllowanceResult = contractManager->callContract(buildCallInfo(erc20Address, getAllowanceEncoder.getFunctor(), getAllowanceEncoder.getData()));
        getAllowanceDecoder = ABI::Decoder({ABI::Types::uint256}, getAllowanceResult);
        REQUIRE(getAllowanceDecoder.getData<uint256_t>(0) == 500000000000000000);

      }

      std::unique_ptr<Options> options;
      std::unique_ptr<DB> db;
      std::unique_ptr<ContractManager> contractManager;
      std::string dbName = "erc20ClassApprove";
      std::string tokenName = "TestToken";
      std::string tokenSymbol = "TST";
      uint8_t tokenDecimals = 18;
      uint256_t tokenSupply = 1000000000000000000;
      initialize(options, db, contractManager, dbName, ownerPrivKey, tokenName, tokenSymbol, tokenDecimals,
                 tokenSupply, false);

      REQUIRE(erc20Address == contractManager->getContracts()[0].second);

      ABI::Encoder::EncVar getAllowanceVars;
      getAllowanceVars.push_back(owner);
      getAllowanceVars.push_back(destinationOfApproval);
      ABI::Encoder getAllowanceEncoder(getAllowanceVars, "allowance(address,address)");

      Bytes getAllowanceResult = contractManager->callContract(buildCallInfo(erc20Address, getAllowanceEncoder.getFunctor(), getAllowanceEncoder.getData()));
      ABI::Decoder getAllowanceDecoder({ABI::Types::uint256}, getAllowanceResult);
      REQUIRE(getAllowanceDecoder.getData<uint256_t>(0) == 500000000000000000);
    }

    SECTION("ERC20 transferFrom") {
      PrivKey destinationOfApprovalPrivKey(Utils::randBytes(32));
      Address destinationOfApproval(Secp256k1::toAddress(Secp256k1::toUPub(destinationOfApprovalPrivKey)));
      Address erc20Address;
      {
        std::unique_ptr<Options> options;
        std::unique_ptr<DB> db;
        std::unique_ptr<ContractManager> contractManager;
        std::string dbName = "erc20ClassTransferFrom";
        std::string tokenName = "TestToken";
        std::string tokenSymbol = "TST";
        uint8_t tokenDecimals = 18;
        uint256_t tokenSupply = 1000000000000000000;
        initialize(options, db, contractManager, dbName, ownerPrivKey, tokenName, tokenSymbol, tokenDecimals,
                   tokenSupply);

        erc20Address = contractManager->getContracts()[0].second;
        ABI::Encoder::EncVar approveVars;
        approveVars.push_back(destinationOfApproval);
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

        ABI::Encoder::EncVar transferFromVars;
        transferFromVars.push_back(owner);
        transferFromVars.push_back(destinationOfApproval);
        transferFromVars.push_back(500000000000000000);
        ABI::Encoder transferFromEncoder(transferFromVars);
        Bytes transferFromBytes = Hex::toBytes("0x23b872dd");
        Utils::appendBytes(transferFromBytes, transferFromEncoder.getData());
        TxBlock transferFromTx(
          erc20Address,
          destinationOfApproval,
          transferFromBytes,
          8080,
          0,
          0,
          0,
          0,
          0,
          destinationOfApprovalPrivKey
        );

        auto randomPrivKey = PrivKey(Utils::randBytes(32));
        TxBlock transferFromTxThrows = TxBlock(
          erc20Address,
          Secp256k1::toAddress(Secp256k1::toUPub(randomPrivKey)),
          transferFromBytes,
          8080,
          0,
          0,
          0,
          0,
          0,
          randomPrivKey
        );

        REQUIRE_THROWS(contractManager->validateCallContractWithTx(transferFromTxThrows.txToCallInfo()));
        contractManager->validateCallContractWithTx(transferFromTx.txToCallInfo());

        ABI::Encoder::EncVar getBalanceMeVars;
        getBalanceMeVars.push_back(owner);
        ABI::Encoder getBalanceMeEncoder(getBalanceMeVars, "balanceOf(address)");
        Bytes getBalanceMeResult = contractManager->callContract(buildCallInfo(erc20Address, getBalanceMeEncoder.getFunctor(), getBalanceMeEncoder.getData()));
        ABI::Decoder getBalanceMeDecoder({ABI::Types::uint256}, getBalanceMeResult);
        REQUIRE(getBalanceMeDecoder.getData<uint256_t>(0) == 1000000000000000000);

        ABI::Encoder::EncVar getBalanceDestinationVars;
        getBalanceDestinationVars.push_back(destinationOfApproval);
        ABI::Encoder getBalanceDestinationEncoder(getBalanceDestinationVars, "balanceOf(address)");
        Bytes getBalanceDestinationResult = contractManager->callContract(buildCallInfo(erc20Address,
                                                                                              getBalanceDestinationEncoder.getFunctor(),
                                                                                              getBalanceDestinationEncoder.getData()));
        ABI::Decoder getBalanceDestinationDecoder({ABI::Types::uint256}, getBalanceDestinationResult);
        REQUIRE(getBalanceDestinationDecoder.getData<uint256_t>(0) == 0);

        REQUIRE_THROWS(contractManager->callContract(transferFromTxThrows));
        contractManager->callContract(transferFromTx);

        getBalanceMeResult = contractManager->callContract(buildCallInfo(erc20Address, getBalanceMeEncoder.getFunctor(), getBalanceMeEncoder.getData()));
        getBalanceMeDecoder = ABI::Decoder({ABI::Types::uint256}, getBalanceMeResult);
        REQUIRE(getBalanceMeDecoder.getData<uint256_t>(0) == 500000000000000000);

        getBalanceDestinationResult = contractManager->callContract(buildCallInfo(erc20Address,
                                                                                  getBalanceDestinationEncoder.getFunctor(),
                                                                                  getBalanceDestinationEncoder.getData()));
        getBalanceDestinationDecoder = ABI::Decoder({ABI::Types::uint256}, getBalanceDestinationResult);
        REQUIRE(getBalanceDestinationDecoder.getData<uint256_t>(0) == 500000000000000000);
      }

      std::unique_ptr<Options> options;
      std::unique_ptr<DB> db;
      std::unique_ptr<ContractManager> contractManager;
      std::string dbName = "erc20ClassTransferFrom";
      std::string tokenName = "TestToken";
      std::string tokenSymbol = "TST";
      uint8_t tokenDecimals = 18;
      uint256_t tokenSupply = 1000000000000000000;
      initialize(options, db, contractManager, dbName, ownerPrivKey, tokenName, tokenSymbol, tokenDecimals,
                 tokenSupply, false);

      ABI::Encoder::EncVar getBalanceMeVars;
      getBalanceMeVars.push_back(owner);
      ABI::Encoder getBalanceMeEncoder(getBalanceMeVars, "balanceOf(address)");
      Bytes getBalanceMeResult = contractManager->callContract(buildCallInfo(erc20Address, getBalanceMeEncoder.getFunctor(), getBalanceMeEncoder.getData()));
      ABI::Decoder getBalanceMeDecoder({ABI::Types::uint256}, getBalanceMeResult);
      REQUIRE(getBalanceMeDecoder.getData<uint256_t>(0) == 500000000000000000);

      ABI::Encoder::EncVar getBalanceDestinationVars;
      getBalanceDestinationVars.push_back(destinationOfApproval);
      ABI::Encoder getBalanceDestinationEncoder(getBalanceDestinationVars, "balanceOf(address)");
      Bytes getBalanceDestinationResult = contractManager->callContract(buildCallInfo(erc20Address,
                                                                                            getBalanceDestinationEncoder.getFunctor(),
                                                                                            getBalanceDestinationEncoder.getData()));
      ABI::Decoder getBalanceDestinationDecoder({ABI::Types::uint256}, getBalanceDestinationResult);
      REQUIRE(getBalanceDestinationDecoder.getData<uint256_t>(0) == 500000000000000000);
    }
  }
}