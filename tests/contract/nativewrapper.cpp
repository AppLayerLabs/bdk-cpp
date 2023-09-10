/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/templates/erc20.h"
#include "../../src/contract/abi.h"
#include "../../src/utils/db.h"
#include "../../src/utils/options.h"
#include "../../src/contract/contractmanager.h"
#include "../../src/core/storage.h"
#include "../../src/core/state.h"
#include "../../src/core/rdpos.h"

// Forward declaration.
ethCallInfoAllocated buildCallInfo(const Address& addressToCall, const Functor& function, const Bytes& dataToCall);

void initialize(std::unique_ptr<DB>& db,
                std::unique_ptr<Storage>& storage,
                std::unique_ptr<P2P::ManagerNormal>& p2p,
                std::unique_ptr<rdPoS>& rdpos,
                std::unique_ptr<State>& state,
                std::unique_ptr<Options>& options,
                PrivKey validatorKey,
                uint64_t serverPort,
                bool clearDb,
                std::string folderName);

Block createValidBlock(std::unique_ptr<rdPoS>& rdpos, std::unique_ptr<Storage>& storage, const std::vector<TxBlock>& txs = {});

const std::vector<Hash> validatorPrivKeys {
  Hash(Hex::toBytes("0x0a0415d68a5ec2df57aab65efc2a7231b59b029bae7ff1bd2e40df9af96418c8")),
  Hash(Hex::toBytes("0xb254f12b4ca3f0120f305cabf1188fe74f0bd38e58c932a3df79c4c55df8fa66")),
  Hash(Hex::toBytes("0x8a52bb289198f0bcf141688a8a899bf1f04a02b003a8b1aa3672b193ce7930da")),
  Hash(Hex::toBytes("0x9048f5e80549e244b7899e85a4ef69512d7d68613a3dba828266736a580e7745")),
  Hash(Hex::toBytes("0x0b6f5ad26f6eb79116da8c98bed5f3ed12c020611777d4de94c3c23b9a03f739")),
  Hash(Hex::toBytes("0xa69eb3a3a679e7e4f6a49fb183fb2819b7ab62f41c341e2e2cc6288ee22fbdc7")),
  Hash(Hex::toBytes("0xd9b0613b7e4ccdb0f3a5ab0956edeb210d678db306ab6fae1e2b0c9ebca1c2c5")),
  Hash(Hex::toBytes("0x426dc06373b694d8804d634a0fd133be18e4e9bcbdde099fce0ccf3cb965492f"))
};

namespace TNativeWrapper {
  TEST_CASE("NativeWrapper tests", "[contract][nativewrapper]") {
    std::string testDumpPath = Utils::getTestDumpPath();
    SECTION("NativeWrapper Constructor") {
      PrivKey ownerPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
      Address owner = Secp256k1::toAddress(Secp256k1::toUPub(ownerPrivKey));
      Address contractAddress;
      std::string tokenName = "WrappedToken";
      std::string tokenSymbol = "WTKN";
      uint256_t tokenDecimals = 18;
      {
        std::unique_ptr<DB> db;
        std::unique_ptr<Storage> storage;
        std::unique_ptr<P2P::ManagerNormal> p2p;
        std::unique_ptr<rdPoS> rdpos;
        std::unique_ptr<State> state;
        std::unique_ptr<Options> options;
        initialize(db, storage, p2p, rdpos, state, options, validatorPrivKeys[0], 8080, true,
                    testDumpPath + "/NativeWrapperNewContractTest");

        // Create a new Contract
        Bytes createNewNativeWrapperContractEncoder = ABI::Encoder::encodeData(tokenName, tokenSymbol, tokenDecimals);
        Bytes createNativeWrapperContractData = Hex::toBytes("0xb296fad4");
        Utils::appendBytes(createNativeWrapperContractData, createNewNativeWrapperContractEncoder);

        TxBlock createNewNativeWrapperTx = TxBlock(
          ProtocolContractAddresses.at("ContractManager"),
          owner,
          createNativeWrapperContractData,
          8080,
          0,
          0,
          0,
          0,
          0,
          ownerPrivKey
        );

        auto newBlock = createValidBlock(rdpos, storage, {createNewNativeWrapperTx});
        state->processNextBlock(std::move(newBlock));

        // Get the address of the new contract
        contractAddress = state->getContracts()[0].second;
        Bytes nameEncoder = Bytes(32, 0);
        Bytes symbolEncoder = Bytes(32, 0);
        Bytes decimalsEncoder = Bytes(32, 0);
        Bytes totalSupplyEncoder = Bytes(32, 0);

        Functor nameFunctor = ABI::Encoder::encodeFunction("name()");
        Functor symbolFunctor = ABI::Encoder::encodeFunction("symbol()");
        Functor decimalsFunctor = ABI::Encoder::encodeFunction("decimals()");
        Functor totalSupplyFunctor = ABI::Encoder::encodeFunction("totalSupply()");

        Bytes nameData = state->ethCall(buildCallInfo(contractAddress, nameFunctor, nameEncoder));

        auto nameDecoder = ABI::Decoder::decodeData<std::string>(nameData);
        REQUIRE(std::get<0>(nameDecoder) == tokenName);

        Bytes symbolData = state->ethCall(buildCallInfo(contractAddress, symbolFunctor, symbolEncoder));

        auto symbolDecoder = ABI::Decoder::decodeData<std::string>(symbolData);
        REQUIRE(std::get<0>(symbolDecoder) == tokenSymbol);

        Bytes decimalsData = state->ethCall(buildCallInfo(contractAddress, decimalsFunctor, decimalsEncoder));

        auto decimalsDecode = ABI::Decoder::decodeData<uint256_t>(decimalsData);
        REQUIRE(std::get<0>(decimalsDecode) == 18);

        Bytes totalSupplyData = state->ethCall(buildCallInfo(contractAddress, totalSupplyFunctor, totalSupplyEncoder));

        auto totalSupplyDecoder = ABI::Decoder::decodeData<uint256_t>(totalSupplyData);
        REQUIRE(std::get<0>(totalSupplyDecoder) == 0);
      }
      std::unique_ptr<DB> db;
      std::unique_ptr<Storage> storage;
      std::unique_ptr<P2P::ManagerNormal> p2p;
      std::unique_ptr<rdPoS> rdpos;
      std::unique_ptr<State> state;
      std::unique_ptr<Options> options;
      initialize(db, storage, p2p, rdpos, state, options, validatorPrivKeys[0], 8080, false,
                  testDumpPath + "/NativeWrapperNewContractTest");

      REQUIRE(contractAddress == state->getContracts()[0].second);
      Bytes nameEncoder = Bytes(32, 0);
      Bytes symbolEncoder = Bytes(32, 0);
      Bytes decimalsEncoder = Bytes(32, 0);
      Bytes totalSupplyEncoder = Bytes(32, 0);

      Functor nameFunctor = ABI::Encoder::encodeFunction("name()");
      Functor symbolFunctor = ABI::Encoder::encodeFunction("symbol()");
      Functor decimalsFunctor = ABI::Encoder::encodeFunction("decimals()");
      Functor totalSupplyFunctor = ABI::Encoder::encodeFunction("totalSupply()");

      Bytes nameData = state->ethCall(buildCallInfo(contractAddress, nameFunctor, nameEncoder));

      auto nameDecoder = ABI::Decoder::decodeData<std::string>(nameData);
      REQUIRE(std::get<0>(nameDecoder) == tokenName);

      Bytes symbolData = state->ethCall(buildCallInfo(contractAddress, symbolFunctor, symbolEncoder));

      auto symbolDecoder = ABI::Decoder::decodeData<std::string>(symbolData);
      REQUIRE(std::get<0>(symbolDecoder) == tokenSymbol);

      Bytes decimalsData = state->ethCall(buildCallInfo(contractAddress, decimalsFunctor, decimalsEncoder));

      auto decimalsDecoder = ABI::Decoder::decodeData<uint256_t>(decimalsData);
      REQUIRE(std::get<0>(decimalsDecoder) == 18);

      Bytes totalSupplyData = state->ethCall(buildCallInfo(contractAddress, totalSupplyFunctor, totalSupplyEncoder));

      auto totalSupplyDecoder = ABI::Decoder::decodeData<uint256_t>(totalSupplyData);
      REQUIRE(std::get<0>(totalSupplyDecoder) == 0);

    }

    SECTION("NativeWrapper Deposit and Transfer") {
      PrivKey ownerPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
      Address owner = Secp256k1::toAddress(Secp256k1::toUPub(ownerPrivKey));
      Address contractAddress;
      std::string tokenName = "WrappedToken";
      std::string tokenSymbol = "WTKN";
      uint256_t tokenDecimals = 18;
      uint64_t currentNonce = 0;
      uint256_t amountToTransfer = uint256_t("192838158112259");
      {
        std::unique_ptr<DB> db;
        std::unique_ptr<Storage> storage;
        std::unique_ptr<P2P::ManagerNormal> p2p;
        std::unique_ptr<rdPoS> rdpos;
        std::unique_ptr<State> state;
        std::unique_ptr<Options> options;
        initialize(db, storage, p2p, rdpos, state, options, validatorPrivKeys[0], 8080, true,
                   testDumpPath + "/NativeWrapperDepositTest");

        // Create a new Contract
        Bytes createNewNativeWrapperContractEncoder = ABI::Encoder::encodeData(tokenName, tokenSymbol, tokenDecimals);
        Bytes createNativeWrapperContractData = Hex::toBytes("0xb296fad4");
        Utils::appendBytes(createNativeWrapperContractData, createNewNativeWrapperContractEncoder);

        TxBlock createNewNativeWrapperTx = TxBlock(
          ProtocolContractAddresses.at("ContractManager"),
          owner,
          createNativeWrapperContractData,
          8080,
          currentNonce++,
          0,
          0,
          0,
          0,
          ownerPrivKey
        );

        auto newBlock = createValidBlock(rdpos, storage, {createNewNativeWrapperTx});
        state->processNextBlock(std::move(newBlock));

        // Get the address of the new contract
        contractAddress = state->getContracts()[0].second;

        TxBlock depositTx = TxBlock(
          contractAddress,
          Secp256k1::toAddress(Secp256k1::toUPub(ownerPrivKey)),
          Hex::toBytes("0xd0e30db0"),
          8080,
          currentNonce++,
          amountToTransfer,
          1000000000,
          1000000000,
          21000,
          ownerPrivKey
        );

        newBlock = createValidBlock(rdpos, storage, {depositTx});
        state->processNextBlock(std::move(newBlock));

        REQUIRE(state->getNativeBalance(contractAddress) == amountToTransfer);
        REQUIRE(state->getNativeBalance(owner) == uint256_t("1000000000000000000000") - amountToTransfer - (uint256_t(1000000000) * 21000));

        Bytes balanceOfEncoder = ABI::Encoder::encodeData(owner);
        Functor balanceOfFunctor = ABI::Encoder::encodeFunction("balanceOf(address)");
        Bytes balanceOfData = state->ethCall(buildCallInfo(contractAddress, balanceOfFunctor, balanceOfEncoder));

        auto balanceOfDecoder = ABI::Decoder::decodeData<uint256_t>(balanceOfData);
        REQUIRE(std::get<0>(balanceOfDecoder) == amountToTransfer);

      }
      std::unique_ptr<DB> db;
      std::unique_ptr<Storage> storage;
      std::unique_ptr<P2P::ManagerNormal> p2p;
      std::unique_ptr<rdPoS> rdpos;
      std::unique_ptr<State> state;
      std::unique_ptr<Options> options;
      initialize(db, storage, p2p, rdpos, state, options, validatorPrivKeys[0], 8080, false,
                  testDumpPath + "/NativeWrapperDepositTest");

      REQUIRE(contractAddress == state->getContracts()[0].second);
      REQUIRE(state->getNativeBalance(contractAddress) == amountToTransfer);
      REQUIRE(state->getNativeBalance(owner) == uint256_t("1000000000000000000000") - amountToTransfer - (uint256_t(1000000000) * 21000));

      Bytes balanceOfEncoder = ABI::Encoder::encodeData(owner);
      Functor balanceOfFunctor = ABI::Encoder::encodeFunction("balanceOf(address)");
      Bytes balanceOfData = state->ethCall(buildCallInfo(contractAddress, balanceOfFunctor, balanceOfEncoder));

      auto balanceOfDecoder = ABI::Decoder::decodeData<uint256_t>(balanceOfData);
      REQUIRE(std::get<0>(balanceOfDecoder) == amountToTransfer);
    }

    SECTION("NativeWrapper Withdraw()") {
      PrivKey ownerPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
      Address owner = Secp256k1::toAddress(Secp256k1::toUPub(ownerPrivKey));
      Address contractAddress;
      std::string tokenName = "WrappedToken";
      std::string tokenSymbol = "WTKN";
      uint256_t tokenDecimals = 18;
      uint64_t currentNonce = 0;
      uint256_t amountToTransfer = uint256_t("192838158112259");
      uint256_t amountToWithdraw = amountToTransfer / 3;
      {
        std::unique_ptr<DB> db;
        std::unique_ptr<Storage> storage;
        std::unique_ptr<P2P::ManagerNormal> p2p;
        std::unique_ptr<rdPoS> rdpos;
        std::unique_ptr<State> state;
        std::unique_ptr<Options> options;
        initialize(db, storage, p2p, rdpos, state, options, validatorPrivKeys[0], 8080, true,
                    testDumpPath + "/NativeWrapperWithdrawTest");

        // Create a new Contract
        Bytes createNewNativeWrapperContractEncoder = ABI::Encoder::encodeData(tokenName, tokenSymbol, tokenDecimals);
        Bytes createNativeWrapperContractData = Hex::toBytes("0xb296fad4");
        Utils::appendBytes(createNativeWrapperContractData, createNewNativeWrapperContractEncoder);

        TxBlock createNewNativeWrapperTx = TxBlock(
          ProtocolContractAddresses.at("ContractManager"),
          owner,
          createNativeWrapperContractData,
          8080,
          currentNonce++,
          0,
          0,
          0,
          0,
          ownerPrivKey
        );

        auto newBlock = createValidBlock(rdpos, storage, {createNewNativeWrapperTx});
        state->processNextBlock(std::move(newBlock));

        // Get the address of the new contract
        contractAddress = state->getContracts()[0].second;

        TxBlock depositTx = TxBlock(
          contractAddress,
          Secp256k1::toAddress(Secp256k1::toUPub(ownerPrivKey)),
          Hex::toBytes("0xd0e30db0"),
          8080,
          currentNonce++,
          amountToTransfer,
          1000000000,
          1000000000,
          21000,
          ownerPrivKey
        );

        newBlock = createValidBlock(rdpos, storage, {depositTx});
        state->processNextBlock(std::move(newBlock));

        REQUIRE(state->getNativeBalance(contractAddress) == amountToTransfer);
        REQUIRE(state->getNativeBalance(owner) ==
                uint256_t("1000000000000000000000") - amountToTransfer - (uint256_t(1000000000) * 21000));

        Bytes balanceOfEncoder = ABI::Encoder::encodeData(owner);
        Functor balanceOfFunctor = ABI::Encoder::encodeFunction("balanceOf(address)");
        Bytes balanceOfData = state->ethCall(buildCallInfo(contractAddress, balanceOfFunctor, balanceOfEncoder));

        auto balanceOfDecoder = ABI::Decoder::decodeData<uint256_t>(balanceOfData);
        REQUIRE(std::get<0>(balanceOfDecoder) == amountToTransfer);

        Bytes withdrawEncoder = ABI::Encoder::encodeData(amountToWithdraw);
        Bytes withdrawData = Hex::toBytes("0x2e1a7d4d");
        Utils::appendBytes(withdrawData, withdrawEncoder);

        TxBlock withdrawTx = TxBlock(
          contractAddress,
          Secp256k1::toAddress(Secp256k1::toUPub(ownerPrivKey)),
          withdrawData,
          8080,
          currentNonce++,
          0,
          1000000000,
          1000000000,
          21000,
          ownerPrivKey
        );

        newBlock = createValidBlock(rdpos, storage, {withdrawTx});
        state->processNextBlock(std::move(newBlock));

        REQUIRE(state->getNativeBalance(contractAddress) == amountToTransfer - amountToWithdraw);
        REQUIRE(state->getNativeBalance(owner) ==
                uint256_t("1000000000000000000000") - amountToTransfer + amountToWithdraw - (uint256_t(1000000000) * 21000 * 2));

        Bytes balanceOfEncoder2 = ABI::Encoder::encodeData(owner);
        Functor balanceOfFunctor2 = ABI::Encoder::encodeFunction("balanceOf(address)");

        Bytes balanceOfData2 = state->ethCall(buildCallInfo(contractAddress, balanceOfFunctor2, balanceOfEncoder2));

        auto balanceOfDecoder2 = ABI::Decoder::decodeData<uint256_t>(balanceOfData2);
        REQUIRE(std::get<0>(balanceOfDecoder2) == amountToTransfer - amountToWithdraw);
      }

      std::unique_ptr<DB> db;
      std::unique_ptr<Storage> storage;
      std::unique_ptr<P2P::ManagerNormal> p2p;
      std::unique_ptr<rdPoS> rdpos;
      std::unique_ptr<State> state;
      std::unique_ptr<Options> options;
      initialize(db, storage, p2p, rdpos, state, options, validatorPrivKeys[0], 8080, false,
                  testDumpPath + "/NativeWrapperWithdrawTest");

      REQUIRE(contractAddress == state->getContracts()[0].second);
      REQUIRE(state->getNativeBalance(contractAddress) == amountToTransfer - amountToWithdraw);
      REQUIRE(state->getNativeBalance(owner) ==
              uint256_t("1000000000000000000000") - amountToTransfer + amountToWithdraw - (uint256_t(1000000000) * 21000 * 2));

      Bytes balanceOfEncoder = ABI::Encoder::encodeData(owner);
      Functor balanceOfFunctor = ABI::Encoder::encodeFunction("balanceOf(address)");

      Bytes balanceOfData = state->ethCall(buildCallInfo(contractAddress, balanceOfFunctor, balanceOfEncoder));

      auto balanceOfDecoder = ABI::Decoder::decodeData<uint256_t>(balanceOfData);
      REQUIRE(std::get<0>(balanceOfDecoder) == amountToTransfer - amountToWithdraw);


    }
  }
}

