#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/erc20.h"
#include "../../src/contract/contractmanager.h"
#include "../../src/core/rdpos.h"
#include "../../src/utils/db.h"
#include "../../src/utils/options.h"
#include <filesystem>


namespace TContractManager {
  /// TODO: Add more testcases for ContractManager once it's integrated with State.
  TEST_CASE("ContractManager class", "[contract][contractmanager]") {
    SECTION("ContractManager createNewContractERC20Contract()") {
      if (std::filesystem::exists("ContractManagerTestCreateNew")) {
        std::filesystem::remove_all("ContractManagerTestCreateNew");
      }

      PrivKey privKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
      Address owner = Secp256k1::toAddress(Secp256k1::toUPub(privKey));
      std::string tokenName = "TestToken";
      std::string tokenSymbol = "TT";
      uint256_t tokenDecimals = 18;
      uint256_t tokenSupply = 1000000000000000000;

      {
        std::unique_ptr options = std::make_unique<Options>(Options::fromFile("ContractManagerTestCreateNew"));
        std::unique_ptr db = std::make_unique<DB>("ContractManagerTestCreateNew");
        std::unique_ptr<rdPoS> rdpos;
        ContractManager contractManager(db, rdpos, options);

        ABI::Encoder::EncVar createNewERC20ContractVars;
        createNewERC20ContractVars.push_back(tokenName);
        createNewERC20ContractVars.push_back(tokenSymbol);
        createNewERC20ContractVars.push_back(tokenDecimals);
        createNewERC20ContractVars.push_back(tokenSupply);
        ABI::Encoder createNewERC20ContractEncoder(createNewERC20ContractVars);
        std::string createNewERC20ContractData = Hex::toBytes("0xb74e5ed5") + createNewERC20ContractEncoder.getRaw();

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

        contractManager.validateCallContract(createNewERC2OTx);
        REQUIRE(contractManager.getContracts().size() == 0);

        contractManager.callContract(createNewERC2OTx);

        const auto contractAddress = contractManager.getContracts()[0].second;
        ABI::Encoder::EncVar getBalanceMeVars;
        getBalanceMeVars.push_back(owner);
        ABI::Encoder getBalanceMeEncoder(getBalanceMeVars, "balanceOf(address)");
        std::string getBalanceMeResult = contractManager.callContract(contractAddress, getBalanceMeEncoder.getRaw());
        ABI::Decoder getBalanceMeDecoder({ABI::Types::uint256}, getBalanceMeResult);
        REQUIRE(getBalanceMeDecoder.getData<uint256_t>(0) == tokenSupply);
      }

      std::unique_ptr options = std::make_unique<Options>(Options::fromFile("ContractManagerTestCreateNew"));
      std::unique_ptr db = std::make_unique<DB>("ContractManagerTestCreateNew");
      std::unique_ptr<rdPoS> rdpos;
      ContractManager contractManager(db, rdpos, options);

      const auto contractAddress = contractManager.getContracts()[0].second;
      ABI::Encoder::EncVar getBalanceMeVars;
      getBalanceMeVars.push_back(owner);
      ABI::Encoder getBalanceMeEncoder(getBalanceMeVars, "balanceOf(address)");
      std::string getBalanceMeResult = contractManager.callContract(contractAddress, getBalanceMeEncoder.getRaw());
      ABI::Decoder getBalanceMeDecoder({ABI::Types::uint256}, getBalanceMeResult);
      REQUIRE(getBalanceMeDecoder.getData<uint256_t>(0) == tokenSupply);

    }

    SECTION("ContractManager createNewContractERC20ContractTransferTo()") {
      if (std::filesystem::exists("ContractManagerTestCreateNew")) {
        std::filesystem::remove_all("ContractManagerTestCreateNew");
      }

      PrivKey privKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
      Address owner = Secp256k1::toAddress(Secp256k1::toUPub(privKey));
      Address destinationOfTransfer = Address(Utils::randBytes(20), true);
      std::string tokenName = "TestToken";
      std::string tokenSymbol = "TT";
      uint256_t tokenDecimals = 18;
      uint256_t tokenSupply = 1000000000000000000;

      {
        std::unique_ptr options = std::make_unique<Options>(Options::fromFile("ContractManagerTestCreateNew"));
        std::unique_ptr db = std::make_unique<DB>("ContractManagerTestCreateNew");
        std::unique_ptr<rdPoS> rdpos;
        ContractManager contractManager(db, rdpos, options);

        ABI::Encoder::EncVar createNewERC20ContractVars;
        createNewERC20ContractVars.push_back(tokenName);
        createNewERC20ContractVars.push_back(tokenSymbol);
        createNewERC20ContractVars.push_back(tokenDecimals);
        createNewERC20ContractVars.push_back(tokenSupply);
        ABI::Encoder createNewERC20ContractEncoder(createNewERC20ContractVars);
        std::string createNewERC20ContractData = Hex::toBytes("0xb74e5ed5") + createNewERC20ContractEncoder.getRaw();

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
        std::string getBalanceMeResult = contractManager.callContract(contractAddress, getBalanceMeEncoder.getRaw());
        ABI::Decoder getBalanceMeDecoder({ABI::Types::uint256}, getBalanceMeResult);
        REQUIRE(getBalanceMeDecoder.getData<uint256_t>(0) == tokenSupply);

        ABI::Encoder::EncVar transferVars;
        transferVars.push_back(destinationOfTransfer);
        transferVars.push_back(500000000000000000);
        ABI::Encoder transferEncoder(transferVars);
        std::string transferData = Hex::toBytes("0xa9059cbb") + transferEncoder.getRaw();
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

        getBalanceMeResult = contractManager.callContract(contractAddress, getBalanceMeEncoder.getRaw());
        getBalanceMeDecoder = ABI::Decoder({ABI::Types::uint256}, getBalanceMeResult);
        REQUIRE(getBalanceMeDecoder.getData<uint256_t>(0) == 500000000000000000);

        ABI::Encoder::EncVar getBalanceDestinationVars;
        getBalanceDestinationVars.push_back(destinationOfTransfer);
        ABI::Encoder getBalanceDestinationEncoder(getBalanceDestinationVars, "balanceOf(address)");
        std::string getBalanceDestinationResult = contractManager.callContract(contractAddress, getBalanceDestinationEncoder.getRaw());
        ABI::Decoder getBalanceDestinationDecoder({ABI::Types::uint256}, getBalanceDestinationResult);
        REQUIRE(getBalanceDestinationDecoder.getData<uint256_t>(0) == 500000000000000000);
      }

      std::unique_ptr options = std::make_unique<Options>(Options::fromFile("ContractManagerTestCreateNew"));
      std::unique_ptr db = std::make_unique<DB>("ContractManagerTestCreateNew");
      std::unique_ptr<rdPoS> rdpos;
      ContractManager contractManager(db, rdpos, options);

      const auto contractAddress = contractManager.getContracts()[0].second;
      ABI::Encoder::EncVar getBalanceMeVars;
      getBalanceMeVars.push_back(owner);
      ABI::Encoder getBalanceMeEncoder(getBalanceMeVars, "balanceOf(address)");
      std::string getBalanceMeResult = contractManager.callContract(contractAddress, getBalanceMeEncoder.getRaw());
      ABI::Decoder getBalanceMeDecoder({ABI::Types::uint256}, getBalanceMeResult);
      REQUIRE(getBalanceMeDecoder.getData<uint256_t>(0) == 500000000000000000);

      ABI::Encoder::EncVar getBalanceDestinationVars;
      getBalanceDestinationVars.push_back(destinationOfTransfer);
      ABI::Encoder getBalanceDestinationEncoder(getBalanceDestinationVars, "balanceOf(address)");
      std::string getBalanceDestinationResult = contractManager.callContract(contractAddress, getBalanceDestinationEncoder.getRaw());
      ABI::Decoder getBalanceDestinationDecoder({ABI::Types::uint256}, getBalanceDestinationResult);
      REQUIRE(getBalanceDestinationDecoder.getData<uint256_t>(0) == 500000000000000000);
    }
  }
}