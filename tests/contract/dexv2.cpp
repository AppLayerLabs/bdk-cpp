#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/contractmanager.h"
#include "../../src/contract/dexv2/dexv2pair.h"
#include "../../src/contract/dexv2/dexv2factory.h"
#include "../../src/contract/dexv2/dexv2router02.h"
#include "../../src/contract/abi.h"
#include "../../src/utils/db.h"
#include "../../src/utils/options.h"
#include "../../src/core/rdpos.h"
#include "../../src/core/state.h"

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

/// Create a new transaction based on to, value, function (with arguments) and arguments...
template <typename... Args>
TxBlock createNewTransaction(const PrivKey& privKey,
                             const Address& to,
                             const uint256_t& value,
                             const std::unique_ptr<State>& state,
                             const std::unique_ptr<Options>& options,
                             const Bytes& functor,
                             Args&&... args) {
  ABI::Encoder::EncVar vars = {std::forward<Args>(args)...};
  ABI::Encoder encoder(vars);

  Bytes txData = functor;
  Utils::appendBytes(txData, encoder.getData());

  Address from = Secp256k1::toAddress(Secp256k1::toUPub(privKey));

  TxBlock tx(to,
             from,
             txData,
             options->getChainID(),
             state->getNativeNonce(from),
             value,
             1000000000, // 1 GWEI
             1000000000, // 1 GWEI
             21000,
             privKey);

  return tx;
}

Address createNewERC20(std::unique_ptr<State>& state, std::unique_ptr<rdPoS>& rdpos, std::unique_ptr<Storage>& storage, std::unique_ptr<Options>& options,
                       const std::string& tokenName, const std::string& tokenSymbol, uint8_t decimals, const uint256_t& mintValue) {
  PrivKey chainOwnerPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
  auto createNewERC20Tx = createNewTransaction(
    chainOwnerPrivKey,
    ProtocolContractAddresses.at("ContractManager"),
    0,
    state,
    options,
    Hex::toBytes("0xb74e5ed5"),
    tokenName,
    tokenSymbol,
    decimals,
    mintValue
  );
  auto prevContractList = state->getContracts();

  auto newBlock = createValidBlock(rdpos, storage, {createNewERC20Tx});

  if (!state->validateNextBlock(newBlock)) {
    throw std::runtime_error("createNewERC20: Failed to validate block");
  }

  try {
    state->processNextBlock(std::move(newBlock));
  } catch (std::exception &e) {
    throw std::runtime_error("createNewERC20: Failed to process block: " + std::string(e.what()));
  }

  auto newContractList = state->getContracts();
  Address newContractAddress;
  /// Find the new contract address
  for (const auto& contract : newContractList) {
    if (std::find(prevContractList.begin(), prevContractList.end(), contract) == prevContractList.end()) {
      newContractAddress = contract.second;
      break;
    }
  }
  return newContractAddress;
}

Address createNewFactory(std::unique_ptr<State>& state, std::unique_ptr<rdPoS>& rdpos, std::unique_ptr<Storage>& storage, std::unique_ptr<Options>& options,
                         const Address& feeToSetter) {
  PrivKey chainOwnerPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
  auto createNewFactoryTx = createNewTransaction(
    chainOwnerPrivKey,
    ProtocolContractAddresses.at("ContractManager"),
    0,
    state,
    options,
    Hex::toBytes("0xf02da5d2"),
    feeToSetter
  );

  auto prevContractList = state->getContracts();

  auto newBlock = createValidBlock(rdpos, storage, {createNewFactoryTx});

  if (!state->validateNextBlock(newBlock)) {
    throw std::runtime_error("createNewERC20: Failed to validate block");
  }

  try {
    state->processNextBlock(std::move(newBlock));
  } catch (std::exception &e) {
    throw std::runtime_error("createNewERC20: Failed to process block: " + std::string(e.what()));
  }

  auto newContractList = state->getContracts();
  Address newContractAddress;
  /// Find the new contract address
  for (const auto& contract : newContractList) {
    if (std::find(prevContractList.begin(), prevContractList.end(), contract) == prevContractList.end()) {
      newContractAddress = contract.second;
      break;
    }
  }
  return newContractAddress;

}

Address createNewRouter(std::unique_ptr<State>& state, std::unique_ptr<rdPoS>& rdpos, std::unique_ptr<Storage>& storage, std::unique_ptr<Options>& options,
                        const Address& factory, const Address& nativeWrapper) {
  PrivKey chainOwnerPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
  auto createNewRouterTx = createNewTransaction(
    chainOwnerPrivKey,
    ProtocolContractAddresses.at("ContractManager"),
    0,
    state,
    options,
    Hex::toBytes("0x5d0ba0d6"),
    factory,
    nativeWrapper
  );

  auto prevContractList = state->getContracts();

  auto newBlock = createValidBlock(rdpos, storage, {createNewRouterTx});

  if (!state->validateNextBlock(newBlock)) {
    throw std::runtime_error("createNewERC20: Failed to validate block");
  }

  try {
    state->processNextBlock(std::move(newBlock));
  } catch (std::exception &e) {
    throw std::runtime_error("createNewERC20: Failed to process block: " + std::string(e.what()));
  }

  auto newContractList = state->getContracts();
  Address newContractAddress;
  /// Find the new contract address
  for (const auto& contract : newContractList) {
    if (std::find(prevContractList.begin(), prevContractList.end(), contract) == prevContractList.end()) {
      newContractAddress = contract.second;
      break;
    }
  }
  return newContractAddress;
}

Address createNewNative(std::unique_ptr<State>& state, std::unique_ptr<rdPoS>& rdpos, std::unique_ptr<Storage>& storage, std::unique_ptr<Options>& options,
                        const std::string& tokenName, const std::string& tokenSymbol, uint8_t tokenDecimal) {
  PrivKey chainOwnerPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
  auto createNewNativeTx = createNewTransaction(
    chainOwnerPrivKey,
    ProtocolContractAddresses.at("ContractManager"),
    0,
    state,
    options,
    Hex::toBytes("0xb296fad4"),
    tokenName,
    tokenSymbol,
    tokenDecimal
  );

  auto prevContractList = state->getContracts();

  auto newBlock = createValidBlock(rdpos, storage, {createNewNativeTx});

  if (!state->validateNextBlock(newBlock)) {
    throw std::runtime_error("createNewERC20: Failed to validate block");
  }

  try {
    state->processNextBlock(std::move(newBlock));
  } catch (std::exception &e) {
    throw std::runtime_error("createNewERC20: Failed to process block: " + std::string(e.what()));
  }

  auto newContractList = state->getContracts();
  Address newContractAddress;
  /// Find the new contract address
  for (const auto& contract : newContractList) {
    if (std::find(prevContractList.begin(), prevContractList.end(), contract) == prevContractList.end()) {
      newContractAddress = contract.second;
      break;
    }
  }
  return newContractAddress;
}

TxBlock createApproveTx(std::unique_ptr<State>& state, std::unique_ptr<Options>& options, const PrivKey& privKey, const Address& erc20, const Address& spender, const uint256_t& value) {
  auto approveTx = createNewTransaction(
    privKey,
    erc20,
    0,
    state,
    options,
    Hex::toBytes("0x095ea7b3"),
    spender,
    value
  );
  return approveTx;
}

namespace TDEXV2 {
  std::string testDumpPath = Utils::getTestDumpPath();
  TEST_CASE("DEXV2 Test", "[contract][dexv2]") {

    SECTION("Deploy DEXV2Router/Factory") {
      PrivKey ownerPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
      Address owner = Secp256k1::toAddress(Secp256k1::toUPub(ownerPrivKey));
      Address factory;
      Address router;
      Address wrapped;
      {
        std::unique_ptr<DB> db;
        std::unique_ptr<Storage> storage;
        std::unique_ptr<P2P::ManagerNormal> p2p;
        std::unique_ptr<rdPoS> rdpos;
        std::unique_ptr<State> state;
        std::unique_ptr<Options> options;
        initialize(db, storage, p2p, rdpos, state, options, validatorPrivKeys[0], 8080, true, testDumpPath + "/DEXV2NewContractsTest");
        wrapped = createNewNative(
          state, rdpos, storage, options,
          "WSPARQ", "WSPARQ", 18
          );

        factory = createNewFactory(
          state, rdpos, storage, options,
          Address()
          );

        router = createNewRouter(
          state, rdpos, storage, options,
          factory, wrapped
          );
      }

      std::unique_ptr<DB> db;
      std::unique_ptr<Storage> storage;
      std::unique_ptr<P2P::ManagerNormal> p2p;
      std::unique_ptr<rdPoS> rdpos;
      std::unique_ptr<State> state;
      std::unique_ptr<Options> options;
      initialize(db, storage, p2p, rdpos, state, options, validatorPrivKeys[0], 8080, false, testDumpPath + "/DEXV2NewContractsTest");

      auto contracts = state->getContracts();

      for (const auto& contract : contracts) {
        if (contract.first == "DEXV2Factory") {
          REQUIRE(contract.second == factory);
        }
        if (contract.first == "DEXV2Router02") {
          REQUIRE(contract.second == router);
        }
        if (contract.first == "NativeWrapper") {
          REQUIRE(contract.second == wrapped);
        }
      }
    }

    SECTION("Deploy DEXV2 and add liquidity to token/token pair") {
      PrivKey ownerPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
      Address owner = Secp256k1::toAddress(Secp256k1::toUPub(ownerPrivKey));
      Address factory;
      Address router;
      Address wrapped;
      Address tokenA;
      Address tokenB;
      {
        std::unique_ptr<DB> db;
        std::unique_ptr<Storage> storage;
        std::unique_ptr<P2P::ManagerNormal> p2p;
        std::unique_ptr<rdPoS> rdpos;
        std::unique_ptr<State> state;
        std::unique_ptr<Options> options;
        initialize(db, storage, p2p, rdpos, state, options, validatorPrivKeys[0], 8080, true, testDumpPath + "/DEXV2NewContractsTest");

        std::cout << "Creating native contract" << std::endl;

        wrapped = createNewNative(
          state, rdpos, storage, options,
          "WSPARQ", "WSPARQ", 18
        );

        std::cout << "Creating factory contract" << std::endl;

        factory = createNewFactory(
          state, rdpos, storage, options,
          Address()
        );

        std::cout << "Creating router contract" << std::endl;

        router = createNewRouter(
          state, rdpos, storage, options,
          factory, wrapped
        );

        std::cout << "Creating both ERC20 Tokens" << std::endl;

        tokenA = createNewERC20(
          state, rdpos, storage, options,
          "TokenA", "TKNA", 18, uint256_t("10000000000000000000000")
          );

        tokenB = createNewERC20(
          state, rdpos, storage, options,
          "tokenB", "TKNB", 18, uint256_t("10000000000000000000000")
          );

        std::cout << "Approving liquidity" << std::endl;
        auto approveATx = createApproveTx(
          state, options, ownerPrivKey, tokenA, router, uint256_t("10000000000000000000000")
        );

        auto approveBTx = createApproveTx(
          state, options, ownerPrivKey, tokenB, router, uint256_t("10000000000000000000000")
        );

        auto newBlock = createValidBlock(rdpos, storage, {approveATx, approveBTx});
        REQUIRE(state->validateNextBlock(newBlock));
        state->processNextBlock(std::move(newBlock));

        uint256_t unixtimestamp = std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now().time_since_epoch()
        ).count() + 600;

        std::cout << "Add liquidity..." << std::endl;

        auto addLiquidityTx = createNewTransaction(
          ownerPrivKey,
          router,
          0,
          state,
          options,
          Hex::toBytes("0xe8e33700"),
          tokenA,                                                       // tokenA
          tokenB,                                                       // tokenB
          uint256_t("100000000000000000000"),                           // amountADesired
          uint256_t("250000000000000000000"),                           // amountBDesired
          0,                                                            // amountAMin
          0,                                                            // amountBMin
          Secp256k1::toAddress(Secp256k1::toUPub(ownerPrivKey)),        // to
          unixtimestamp                                                 // deadline
        );

        auto newBlock2 = createValidBlock(rdpos, storage, {addLiquidityTx});

        REQUIRE(state->validateNextBlock(newBlock2));

        state->processNextBlock(std::move(newBlock2));

        std::cout << "Liquidity added... listing contracts..." << std::endl;

        for (const auto& contracts : state->getContracts()) {
          std::cout << "Contract type " << contracts.first << " is deployed at Address: " << contracts.second.hex() << std::endl;
        }




      }
    }

   //SECTION("Deploy DEXV2 With a single pair and swap") {

   //}
  }
}