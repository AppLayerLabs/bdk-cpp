#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/erc20.h"
#include "../../src/contract/abi.h"
#include "../../src/utils/db.h"

#include <filesystem>

/// Forward Decleration.
std::tuple<Address,Address,uint64_t, uint256_t, uint256_t, std::string> txToInfo(const TxBlock& tx);

namespace TERC20 {
  TEST_CASE("ERC20 Class", "[contract][erc20]") {
    SECTION("ERC20 Class Constructor") {
      if (std::filesystem::exists("erc20ClassConstructor"))
        std::filesystem::remove_all(std::filesystem::path("erc20ClassConstructor"));
      std::unique_ptr<DB> db = std::make_unique<DB>("erc20ClassConstructor");


      Address owner(Utils::randBytes(20), true);
      Address contractAddress(Utils::randBytes(20), true);
      {
        ERC20 erc20("TestToken", "TST", 18, 1000000000000000000, contractAddress, owner, 8080, db);
        ABI::Decoder nameDecoder({ABI::Types::string}, erc20.name());
        REQUIRE(erc20.getContractName() == "ERC20");
        REQUIRE(erc20.getContractAddress() == contractAddress);
        REQUIRE(erc20.getContractCreator() == owner);
        REQUIRE(erc20.getContractChainId() == 8080);

        REQUIRE(nameDecoder.getData<std::string>(0) == "TestToken");
        ABI::Decoder symbolDecoder({ABI::Types::string}, erc20.symbol());
        REQUIRE(symbolDecoder.getData<std::string>(0) == "TST");
        ABI::Decoder decimalsDecoder({ABI::Types::uint256}, erc20.decimals());
        REQUIRE(decimalsDecoder.getData<uint256_t>(0) == 18);
        ABI::Decoder totalSupplyDecoder({ABI::Types::uint256}, erc20.totalSupply());
        REQUIRE(totalSupplyDecoder.getData<uint256_t>(0) == 1000000000000000000);
        ABI::Decoder balanceOfDecoder({ABI::Types::uint256}, erc20.balanceOf(owner));
        REQUIRE(balanceOfDecoder.getData<uint256_t>(0) == 1000000000000000000);
        ABI::Decoder allowanceOfDecoder({ABI::Types::uint256}, erc20.allowance(owner, owner));
        REQUIRE(allowanceOfDecoder.getData<uint256_t>(0) == 0);
      }

      ERC20 erc20(contractAddress, db);
      ABI::Decoder nameDecoder({ABI::Types::string}, erc20.name());
      REQUIRE(nameDecoder.getData<std::string>(0) == "TestToken");
      ABI::Decoder symbolDecoder({ABI::Types::string}, erc20.symbol());
      REQUIRE(symbolDecoder.getData<std::string>(0) == "TST");
      ABI::Decoder decimalsDecoder({ABI::Types::uint256}, erc20.decimals());
      REQUIRE(decimalsDecoder.getData<uint256_t>(0) == 18);
      ABI::Decoder totalSupplyDecoder({ABI::Types::uint256}, erc20.totalSupply());
      REQUIRE(totalSupplyDecoder.getData<uint256_t>(0) == 1000000000000000000);
      ABI::Decoder balanceOfDecoder({ABI::Types::uint256}, erc20.balanceOf(owner));
      REQUIRE(balanceOfDecoder.getData<uint256_t>(0) == 1000000000000000000);
    }

    SECTION("ERC20 transfer()") {
      if (std::filesystem::exists("erc20ClassTransfer"))
        std::filesystem::remove_all(std::filesystem::path("erc20ClassTransfer"));
      std::unique_ptr<DB> db = std::make_unique<DB>("erc20ClassTransfer");

      PrivKey ownerPrivKey(Utils::randBytes(32));
      Address owner(Secp256k1::toAddress(Secp256k1::toUPub(ownerPrivKey)));
      Address contractAddress(Utils::randBytes(20), true);
      Address destinationOfTransactions(Utils::randBytes(20), true);
      {
        ERC20 erc20("TestToken", "TST", 18, 1000000000000000000, contractAddress, owner, 8080, db);

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
        TxBlock transferTx(
          contractAddress,
          owner,
          Hex::toBytes("0xa9059cbb") + transferEncoder.getRaw(),
          8080,
          0,
          0,
          0,
          0,
          0,
          ownerPrivKey
        );

        auto randomPrivKey = PrivKey(Utils::randBytes(64));
        TxBlock transferTxThrows = TxBlock(
          contractAddress,
          Secp256k1::toAddress(Secp256k1::toUPub(randomPrivKey)),
          Hex::toBytes("0xa9059cbb") + transferEncoder.getRaw(),
          8080,
          0,
          0,
          0,
          0,
          0,
          randomPrivKey
        );

        REQUIRE_THROWS(erc20.ethCall(txToInfo(transferTxThrows)));
        erc20.ethCall(txToInfo(transferTx));

        std::string getBalanceMeResult = erc20.ethCall(getBalanceMeEncoder.getRaw());
        ABI::Decoder getBalanceMeDecoder({ABI::Types::uint256}, getBalanceMeResult);
        REQUIRE(getBalanceMeDecoder.getData<uint256_t>(0) == 1000000000000000000);

        std::string getBalanceDestinationResult = erc20.ethCall(getBalanceDestinationEncoder.getRaw());
        ABI::Decoder getBalanceDestinationDecoder({ABI::Types::uint256}, getBalanceDestinationResult);
        REQUIRE(getBalanceDestinationDecoder.getData<uint256_t>(0) == 0);

        erc20.ethCall(transferTx);

        getBalanceMeResult = erc20.ethCall(getBalanceMeEncoder.getRaw());
        getBalanceMeDecoder = ABI::Decoder({ABI::Types::uint256}, getBalanceMeResult);
        REQUIRE(getBalanceMeDecoder.getData<uint256_t>(0) == 500000000000000000);

        getBalanceDestinationResult = erc20.ethCall(getBalanceDestinationEncoder.getRaw());
        getBalanceDestinationDecoder = ABI::Decoder({ABI::Types::uint256}, getBalanceDestinationResult);
        REQUIRE(getBalanceDestinationDecoder.getData<uint256_t>(0) == 500000000000000000);

      }

      ERC20 erc20(contractAddress, db);

      ABI::Encoder::EncVar getBalanceMeVars;
      getBalanceMeVars.push_back(owner);
      ABI::Encoder getBalanceMeEncoder(getBalanceMeVars, "balanceOf(address)");
      std::string getBalanceMeResult = erc20.ethCall(getBalanceMeEncoder.getRaw());
      ABI::Decoder getBalanceMeDecoder({ABI::Types::uint256}, getBalanceMeResult);
      REQUIRE(getBalanceMeDecoder.getData<uint256_t>(0) == 500000000000000000);

      ABI::Encoder::EncVar getBalanceDestinationVars;
      getBalanceDestinationVars.push_back(destinationOfTransactions);
      ABI::Encoder getBalanceDestinationEncoder(getBalanceDestinationVars, "balanceOf(address)");
      std::string getBalanceDestinationResult = erc20.ethCall(getBalanceDestinationEncoder.getRaw());
      ABI::Decoder getBalanceDestinationDecoder({ABI::Types::uint256}, getBalanceDestinationResult);
      REQUIRE(getBalanceDestinationDecoder.getData<uint256_t>(0) == 500000000000000000);
    }

    SECTION("ERC20 approve") {
      if (std::filesystem::exists("erc20ClassApprove"))
        std::filesystem::remove_all(std::filesystem::path("erc20ClassApprove"));
      std::unique_ptr<DB> db = std::make_unique<DB>("erc20ClassApprove");

      PrivKey ownerPrivKey(Utils::randBytes(32));
      Address owner(Secp256k1::toAddress(Secp256k1::toUPub(ownerPrivKey)));
      Address contractAddress(Utils::randBytes(20), true);
      Address destinationOfApproval(Utils::randBytes(20), true);
      {
        ERC20 erc20("TestToken", "TST", 18, 1000000000000000000, contractAddress, owner, 8080, db);

        ABI::Encoder::EncVar getAllowanceVars;
        getAllowanceVars.push_back(owner);
        getAllowanceVars.push_back(destinationOfApproval);
        ABI::Encoder getAllowanceEncoder(getAllowanceVars, "allowance(address,address)");

        ABI::Encoder::EncVar approveVars;
        approveVars.push_back(destinationOfApproval);
        approveVars.push_back(500000000000000000);
        ABI::Encoder approveEncoder(approveVars);
        TxBlock approveTx(
          contractAddress,
          owner,
          Hex::toBytes("0x095ea7b3") + approveEncoder.getRaw(),
          8080,
          0,
          0,
          0,
          0,
          0,
          ownerPrivKey
        );

        erc20.ethCall(txToInfo(approveTx));

        std::string getAllowanceResult = erc20.ethCall(getAllowanceEncoder.getRaw());
        ABI::Decoder getAllowanceDecoder({ABI::Types::uint256}, getAllowanceResult);
        REQUIRE(getAllowanceDecoder.getData<uint256_t>(0) == 0);

        erc20.ethCall(approveTx);

        getAllowanceResult = erc20.ethCall(getAllowanceEncoder.getRaw());
        getAllowanceDecoder = ABI::Decoder({ABI::Types::uint256}, getAllowanceResult);
        REQUIRE(getAllowanceDecoder.getData<uint256_t>(0) == 500000000000000000);
      }

      ERC20 erc20(contractAddress, db);

      ABI::Encoder::EncVar getAllowanceVars;
      getAllowanceVars.push_back(owner);
      getAllowanceVars.push_back(destinationOfApproval);
      ABI::Encoder getAllowanceEncoder(getAllowanceVars, "allowance(address,address)");
      std::string getAllowanceResult = erc20.ethCall(getAllowanceEncoder.getRaw());
      ABI::Decoder getAllowanceDecoder({ABI::Types::uint256}, getAllowanceResult);
      REQUIRE(getAllowanceDecoder.getData<uint256_t>(0) == 500000000000000000);

    }

    SECTION("ERC20 transferFrom") {
      if (std::filesystem::exists("erc20ClassTransferFrom"))
        std::filesystem::remove_all(std::filesystem::path("erc20ClassTransferFrom"));
      std::unique_ptr<DB> db = std::make_unique<DB>("erc20ClassTransferFrom");

      PrivKey ownerPrivKey(Utils::randBytes(32));
      Address owner(Secp256k1::toAddress(Secp256k1::toUPub(ownerPrivKey)));
      Address contractAddress(Utils::randBytes(20), true);
      PrivKey destinationOfApprovalPrivKey(Utils::randBytes(32));
      Address destinationOfApproval(Secp256k1::toAddress(Secp256k1::toUPub(destinationOfApprovalPrivKey)));
      {
        ERC20 erc20("TestToken", "TST", 18, 1000000000000000000, contractAddress, owner, 8080, db);

        ABI::Encoder::EncVar approveVars;
        approveVars.push_back(destinationOfApproval);
        approveVars.push_back(500000000000000000);
        ABI::Encoder approveEncoder(approveVars);
        TxBlock approveTx(
          contractAddress,
          owner,
          Hex::toBytes("0x095ea7b3") + approveEncoder.getRaw(),
          8080,
          0,
          0,
          0,
          0,
          0,
          ownerPrivKey
        );

        erc20.ethCall(approveTx);

        ABI::Encoder::EncVar transferFromVars;
        transferFromVars.push_back(owner);
        transferFromVars.push_back(destinationOfApproval);
        transferFromVars.push_back(500000000000000000);
        ABI::Encoder transferFromEncoder(transferFromVars);
        TxBlock transferFromTx(
          contractAddress,
          destinationOfApproval,
          Hex::toBytes("0x23b872dd") + transferFromEncoder.getRaw(),
          8080,
          0,
          0,
          0,
          0,
          0,
          destinationOfApprovalPrivKey
        );

        auto randomPrivKey = PrivKey(Utils::randBytes(64));
        TxBlock transferFromTxThrows = TxBlock(
          contractAddress,
          Secp256k1::toAddress(Secp256k1::toUPub(randomPrivKey)),
          Hex::toBytes("0x23b872dd") + transferFromEncoder.getRaw(),
          8080,
          0,
          0,
          0,
          0,
          0,
          randomPrivKey
        );

        REQUIRE_THROWS(erc20.ethCall(txToInfo(transferFromTxThrows)));
        erc20.ethCall(txToInfo(transferFromTx));

        ABI::Encoder::EncVar getBalanceMeVars;
        getBalanceMeVars.push_back(owner);
        ABI::Encoder getBalanceMeEncoder(getBalanceMeVars, "balanceOf(address)");
        std::string getBalanceMeResult = erc20.ethCall(getBalanceMeEncoder.getRaw());
        ABI::Decoder getBalanceMeDecoder({ABI::Types::uint256}, getBalanceMeResult);
        REQUIRE(getBalanceMeDecoder.getData<uint256_t>(0) == 1000000000000000000);

        ABI::Encoder::EncVar getBalanceDestinationVars;
        getBalanceDestinationVars.push_back(destinationOfApproval);
        ABI::Encoder getBalanceDestinationEncoder(getBalanceDestinationVars, "balanceOf(address)");
        std::string getBalanceDestinationResult = erc20.ethCall(getBalanceDestinationEncoder.getRaw());
        ABI::Decoder getBalanceDestinationDecoder({ABI::Types::uint256}, getBalanceDestinationResult);
        REQUIRE(getBalanceDestinationDecoder.getData<uint256_t>(0) == 0);

        erc20.ethCall(transferFromTx);

        getBalanceMeResult = erc20.ethCall(getBalanceMeEncoder.getRaw());
        getBalanceMeDecoder = ABI::Decoder({ABI::Types::uint256}, getBalanceMeResult);
        REQUIRE(getBalanceMeDecoder.getData<uint256_t>(0) == 500000000000000000);

        getBalanceDestinationResult = erc20.ethCall(getBalanceDestinationEncoder.getRaw());
        getBalanceDestinationDecoder = ABI::Decoder({ABI::Types::uint256}, getBalanceDestinationResult);
        REQUIRE(getBalanceDestinationDecoder.getData<uint256_t>(0) == 500000000000000000);

      }

      ERC20 erc20(contractAddress, db);

      ABI::Encoder::EncVar getBalanceMeVars;
      getBalanceMeVars.push_back(owner);
      ABI::Encoder getBalanceMeEncoder(getBalanceMeVars, "balanceOf(address)");
      std::string getBalanceMeResult = erc20.ethCall(getBalanceMeEncoder.getRaw());
      ABI::Decoder getBalanceMeDecoder({ABI::Types::uint256}, getBalanceMeResult);
      REQUIRE(getBalanceMeDecoder.getData<uint256_t>(0) == 500000000000000000);

      ABI::Encoder::EncVar getBalanceDestinationVars;
      getBalanceDestinationVars.push_back(destinationOfApproval);
      ABI::Encoder getBalanceDestinationEncoder(getBalanceDestinationVars, "balanceOf(address)");
      std::string getBalanceDestinationResult = erc20.ethCall(getBalanceDestinationEncoder.getRaw());
      ABI::Decoder getBalanceDestinationDecoder({ABI::Types::uint256}, getBalanceDestinationResult);
      REQUIRE(getBalanceDestinationDecoder.getData<uint256_t>(0) == 500000000000000000);
    }
  }
}