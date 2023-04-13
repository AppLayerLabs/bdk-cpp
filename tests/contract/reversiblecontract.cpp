#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/contract.h"
#include "../../src/contract/variables/safeunorderedmap.h"
#include "../../src/contract/abi.h"

class ReversibleContract : public Contract {
  private:
    SafeUnorderedMap<Address, uint256_t> balances;

  public:
    ReversibleContract() : Contract("ReversibleContract",
                                    Address(Hex::toBytes("0x0000000000000000000000000000000000000000"), true),
                                    Address(Hex::toBytes("0x0000000000000000000000000000000000000000"), true),
                                    8080, nullptr),
                                    balances(this) {
      this->registerFunction(Hex::toBytes("0xae639329"), [this](const TxBlock &txBlock) {
        Address from = txBlock.getFrom();
        std::vector<ABI::Types> types = { ABI::Types::address, ABI::Types::uint256 };
        ABI::Decoder decoder(types, txBlock.getData().substr(4));
        Address to = decoder.getData<Address>(0);
        uint256_t amount = decoder.getData<uint256_t>(1);
        this->sendTo(from, to, amount);
      });

      this->registerFunction(Hex::toBytes("0x5b86f599"), [this](const TxBlock &txBlock) {
        Address from = txBlock.getFrom();
        std::vector<ABI::Types> types = { ABI::Types::uint256 };
        ABI::Decoder decoder(types, txBlock.getData().substr(4));
        uint256_t amount = decoder.getData<uint256_t>(0);
        this->increaseBalance(from, amount);
      });

      this->registerViewFunction(Hex::toBytes("0xf8b2cb4f"), [this](const std::string &str) {
        std::vector<ABI::Types> types = { ABI::Types::address };
        ABI::Decoder decoder(types, str.substr(4));
        return this->getBalance(decoder.getData<Address>(0));
      });
    }


    /// ABI: 0xae639329
    void sendTo(const Address& from, const Address& to, uint256_t amount) {
      balances[from] -= amount;
      balances[to] += amount;
    }

    /// 0x5b86f599
    void increaseBalance(const Address& to, uint256_t amount) {
      balances[to] += amount;
    }

    /// 0xf8b2cb4f
    std::string getBalance(const Address& from) const {
      auto it = balances.find(from);
      if (it == balances.end()) {
        return ABI::Encoder({0}).getRaw();
      } else {
        return ABI::Encoder({it->second}).getRaw();
      }
    }
};

namespace TReversibleContract {
  TEST_CASE("ReversibleContract", "[contract][reversiblecontract]") {
    SECTION("ReversibleContract Contract Full Test") {
      ReversibleContract contract;
      PrivKey privKey = PrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
      Address myAddress = Secp256k1::toAddress(Secp256k1::toUPub(privKey));
      Address destinationAddress = Address(Utils::randBytes(20), true);

      /*
       *     TxBlock(
       *             const Address to, const Address from, const std::string data,
       *             const uint64_t chainId, const uint256_t nonce, const uint256_t value,
       *             const uint256_t maxPriorityFeePerGas, const uint256_t maxFeePerGas, const uint256_t gasLimit, const PrivKey privKey
       *           );
       */

      ABI::Encoder::EncVar getBalanceMeVars;
      getBalanceMeVars.push_back(myAddress);
      ABI::Encoder getBalanceEncoder(getBalanceMeVars, "getBalance(address)");
      std::string getBalanceMeStr = getBalanceEncoder.getRaw();

      ABI::Encoder::EncVar getBalanceDestinationVars;
      getBalanceDestinationVars.push_back(destinationAddress);
      ABI::Encoder getBalanceDestinationEncoder(getBalanceDestinationVars, "getBalance(address)");
      std::string getBalanceDestinationStr = getBalanceDestinationEncoder.getRaw();

      /// This transaction should fail because the address doesn't have balance.
      ABI::Encoder::EncVar sendToAddressWithoutBalanceVars;
      sendToAddressWithoutBalanceVars.push_back(destinationAddress);
      sendToAddressWithoutBalanceVars.push_back(uint256_t("1000000000000000000"));
      ABI::Encoder sendToAddressWithoutBalanceEncoder(sendToAddressWithoutBalanceVars);
      TxBlock txThrow(
        Address(Hex::toBytes("0x0000000000000000000000000000000000000000"), true),
        myAddress,
        Hex::toBytes("0xae639329") + sendToAddressWithoutBalanceEncoder.getRaw(),
        8080,
        0,
        0,
        0,
        0,
        0,
        privKey
      );

      REQUIRE_THROWS(contract.ethCall(txThrow, true));

      ABI::Encoder::EncVar increaseBalanceVars;
      increaseBalanceVars.push_back(uint256_t("1000000000000000000"));
      ABI::Encoder addBalanceEncoder(increaseBalanceVars);
      TxBlock txAddBalance(
        Address(Hex::toBytes("0x0000000000000000000000000000000000000000"), true),
        myAddress,
        Hex::toBytes("0x5b86f599") + addBalanceEncoder.getRaw(),
        8080,
        0,
        0,
        0,
        0,
        0,
        privKey
      );

      contract.ethCall(txAddBalance, false);
      std::string getBalanceMeResultStr = contract.ethCall(getBalanceMeStr);
      ABI::Decoder getBalanceMeResultDecoder({ABI::Types::uint256}, getBalanceMeResultStr);
      uint256_t getBalanceMeResult = getBalanceMeResultDecoder.getData<uint256_t>(0);
      REQUIRE(getBalanceMeResult == uint256_t("0"));

      contract.ethCall(txAddBalance, true);

      std::string getBalanceMeResultStr2 = contract.ethCall(getBalanceMeStr);
      ABI::Decoder getBalanceMeResultDecoder2({ABI::Types::uint256}, getBalanceMeResultStr2);
      uint256_t getBalanceMeResult2 = getBalanceMeResultDecoder2.getData<uint256_t>(0);
      REQUIRE(getBalanceMeResult2 == uint256_t("1000000000000000000"));


      ABI::Encoder::EncVar sendToVars;
      sendToVars.push_back(destinationAddress);
      sendToVars.push_back(uint256_t("500000000000000000"));
      ABI::Encoder sendToEncoder(sendToVars);
      TxBlock txSendTo(
        Address(Hex::toBytes("0x0000000000000000000000000000000000000000"), true),
        myAddress,
        Hex::toBytes("0xae639329") + sendToEncoder.getRaw(),
        8080,
        0,
        0,
        0,
        0,
        0,
        privKey
      );

      contract.ethCall(txSendTo, false);

      std::string getBalanceMeResultStr3 = contract.ethCall(getBalanceMeStr);
      ABI::Decoder getBalanceMeResultDecoder3({ABI::Types::uint256}, getBalanceMeResultStr3);
      uint256_t getBalanceMeResult3 = getBalanceMeResultDecoder3.getData<uint256_t>(0);
      REQUIRE(getBalanceMeResult3 == uint256_t("1000000000000000000"));

      std::string getBalanceDestinationResultStr = contract.ethCall(getBalanceDestinationStr);
      ABI::Decoder getBalanceDestinationResultDecoder({ABI::Types::uint256}, getBalanceDestinationResultStr);
      uint256_t getBalanceDestinationResult = getBalanceDestinationResultDecoder.getData<uint256_t>(0);
      REQUIRE(getBalanceDestinationResult == uint256_t("0"));

      contract.ethCall(txSendTo, true);

      std::string getBalanceMeResultStr4 = contract.ethCall(getBalanceMeStr);
      ABI::Decoder getBalanceMeResultDecoder4({ABI::Types::uint256}, getBalanceMeResultStr4);
      uint256_t getBalanceMeResult4 = getBalanceMeResultDecoder4.getData<uint256_t>(0);
      REQUIRE(getBalanceMeResult4 == uint256_t("500000000000000000"));

      std::string getBalanceDestinationResultStr2 = contract.ethCall(getBalanceDestinationStr);
      ABI::Decoder getBalanceDestinationResultDecoder2({ABI::Types::uint256}, getBalanceDestinationResultStr2);
      uint256_t getBalanceDestinationResult2 = getBalanceDestinationResultDecoder2.getData<uint256_t>(0);
      REQUIRE(getBalanceDestinationResult2 == uint256_t("500000000000000000"));

    }
  }
}


