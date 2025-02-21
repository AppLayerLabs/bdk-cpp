/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "libs/catch2/catch_amalgamated.hpp"
#include "libs/base64.hpp"

#include "core/blockchain.h"
#include "contract/templates/systemcontract.h"

#include "../../sdktestsuite.hpp"

#include <filesystem>

/**
 * Create a test transaction that calls a CPP contract with some arguments.
 */
template <typename ReturnType, typename TContract, typename ...Args>
void blockchainCallCpp(
  Blockchain& blockchain,
  const TestAccount& callerTestAccount,
  const Address& contractAddress,
  const uint256_t& value,
  ReturnType(TContract::*func)(const Args&...),
  const Args&... args
) {
  TContract::registerContract();
  Functor txFunctor = ABI::FunctorEncoder::encode<Args...>(
    ContractReflectionInterface::getFunctionName(func)
  );
  Bytes txData;
  Utils::appendBytes(txData, UintConv::uint32ToBytes(txFunctor.value));
  if constexpr (sizeof...(Args) > 0) {
    Utils::appendBytes(
      txData, ABI::Encoder::encodeData<Args...>(std::forward<decltype(args)>(args)...)
    );
  }
  Gas gas(1'000'000'000);

  //
  // FIXME: I don't know why the estimateGas() function is not reverting.
  //        It actually applies e.g. the stake or delegation, and it shows in logs.
  //
  //const uint64_t gasUsed = 10'000 + blockchain.state().estimateGas(
  //  EncodedCallMessage(callerTestAccount.address, contractAddress, gas, value, txData)
  //);
  //const uint64_t gasUsed = 10'000 + std::invoke([&] () {
  //  return blockchain.state().estimateGas(EncodedCallMessage(callerTestAccount.address, contractAddress, gas, value, txData));
  //});
  const uint64_t gasUsed = 1'000'000;  // Just use a big number for now

  Bytes txBytes = TxBlock(
    contractAddress,
    callerTestAccount.address,
    txData,
    blockchain.opt().getChainID(),
    blockchain.state().getNativeNonce(callerTestAccount.address),
    value,
    1000000000,
    1000000000,
    gasUsed,
    callerTestAccount.privKey
  ).rlpSerialize();
  // Comet::sendTransaction() is fully asynchronous.
  // If it returns 0, we know it failed, but if it returns > 0, we don't know if
  //  the transaction will be accepted in the mempool, then included in a block,
  //  and then executed successfully. The only way to know is to monitor the
  //  blockchain's machine state for the expected transaction effects given a
  //  certain timeout, or we would have to add some extra facilities to
  //  Blockchain to make testing contracts easier.
  REQUIRE(blockchain.comet().sendTransaction(txBytes) > 0);
}

void blockchainSendNativeTokens(
  Blockchain& blockchain,
  const TestAccount& fromAccount,
  const Address& toAddress,
  const uint256_t& value
) {
  Bytes txData;
  Gas gas(1'000'000'000);
  // TODO/REVIEW: is this "estimateGas" reverting or does it have the same problem
  //              as blockchainCallCpp() above?
  const uint64_t gasUsed = 10'000 + blockchain.state().estimateGas(
    EncodedCallMessage(fromAccount.address, toAddress, gas, value, txData)
  );
  Bytes txBytes = TxBlock(
    toAddress,
    fromAccount.address,
    txData,
    blockchain.opt().getChainID(),
    blockchain.state().getNativeNonce(fromAccount.address),
    value,
    1000000000,
    1000000000,
    gasUsed,
    fromAccount.privKey
  ).rlpSerialize();
  REQUIRE(blockchain.comet().sendTransaction(txBytes) > 0);
}

/**
 * Wait for a deposit to clear on a blockchain, or error out on timeout.
 */
bool blockchainCheckDeposit(
  Blockchain& blockchain,
  const Address& accountAddress,
  const uint256_t& value
) {
  int tries = 1000; // func waitbal
  while (--tries > 0) {
    uint256_t bal = blockchain.state().getNativeBalance(accountAddress);
    if (bal == value) { break; }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  return tries > 0;
}

/**
 * Tests for the new Blockchain class.
 * Here we are using and testing the Blockchain class itself, not the SDKTestSuite test helper subclass.
 */
namespace TBlockchain {
  auto createTestDumpPath = SDKTestSuite::createTestDumpPath;
  TEST_CASE("Blockchain Class", "[integration][core][blockchain]") {
    std::string testDumpPath = Utils::getTestDumpPath();

    // Check that we can't deploy SystemContract twice in Blockchain
    // (SDKTestSuite is a Blockchain subclass)
    SECTION("BlockchainSystemContractSingletonTest") {
      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("BlockchainSystemContractSingletonTest");
      // The constructor arguments can be whatever that won't make the actual class ctor itself throw.
      std::vector<std::string> initialValidatorPubKeys = {"030000000000000000000000000000000000000000000000000000000000000000"};
      uint64_t initialNumSlots = 12;
      uint64_t maxSlots = 34;
      // If `SystemContract` is added to the `ContractTypes` tuple (in `customcontracts.h`) then this does NOT throw.
      // Which means that not including `SystemContract` in `ContractTypes` is sufficient to render it undeployable by transactions.
      GLOGDEBUG("TEST: trying to spawn a SystemContract");
      REQUIRE_THROWS(
        sdk.deployContract<SystemContract>(initialValidatorPubKeys, initialNumSlots, maxSlots)
      );
      GLOGDEBUG("TEST: finished trying to spawn a SystemContract");
    }

    // SystemContract integration test with Blockchain
    SECTION("BlockchainValidatorSetTest") {
      const int numNodes = 6;
      const int numNonValidators = 2;
      const int numValidators = numNodes - numNonValidators;

      auto ports = SDKTestSuite::generateCometTestPorts(numNodes);

      // Unfortunately, BDK HTTP ports were a late addition to getOptionsForTest()
      std::vector<int> httpPorts;
      for (int i = 0; i < numNodes; ++i) { httpPorts.push_back(SDKTestSuite::getTestPort()); }

      std::vector<Options> options;
      for (int i = 0; i < numNodes; ++i) {
        options.emplace_back(
          SDKTestSuite::getOptionsForTest(
            createTestDumpPath("BlockchainValidatorSetTest_" + std::to_string(i)), false, "",
            ports[i].p2p, ports[i].rpc, i, numNodes, ports, numNonValidators, 0,
            "500ms", "500ms", httpPorts[i]
          )
        );
      }

      GLOGDEBUG("TEST: Starting Blockchain instances one by one; this will take a while...");

      std::vector<std::unique_ptr<Blockchain>> blockchains;
      for (int i = 0; i < numNodes; ++i) {
        blockchains.emplace_back(std::make_unique<Blockchain>(options[i], std::to_string(i)));
        // NOTE: Blockchain::start() waits for CometState::RUNNING, so it blocks for a while.
        //       This is fine (the test still works, nodes eventually manage to dial each other) but
        //       this could be parallelized so that the test would finish faster.
        blockchains.back()->start();
      }

      GLOGDEBUG("TEST: Started all Blockchain instances");

      std::vector<CometValidatorUpdate> origValidatorSet;
      uint64_t origValidatorSetHeight = 0;

      // Ensure all nodes see numValidator validators in the currently active validator set,
      // which is the genesis set so it is immediately active.
      for (int i = 0; i < numNodes; ++i) {
        blockchains[i]->state().getValidatorSet(origValidatorSet, origValidatorSetHeight);
        REQUIRE(origValidatorSet.size() == numValidators);
      }

      // From now on we are making some calls, so we need TestAccount for
      // the chain owner and all the validators, which will be calling the
      // SystemContract, staking tokens, etc.

      // Create a TestAccount for each node based on its validator privkey
      std::vector<TestAccount> nodeAccs;
      for (int i = 0; i < numNodes; ++i) {
        // We know that SDKTestSuite::getOptionsForTest() uses cometTestKeys
        std::string privKeyStrBase64 = cometTestKeys[i].priv_key;
        Bytes privBytes = base64::decode_into<Bytes>(privKeyStrBase64);
        PrivKey privKey(privBytes);
        nodeAccs.emplace_back(TestAccount(privKey));
      }

      Address systemContractAddr = ProtocolContractAddresses.at("SystemContract");
      uint256_t aThousandNativeTokens = uint256_t("1000000000000000000000"); // 1'000 eth

      for (int i = 0; i < numNodes; ++i) {
        // Chain owner gives 5,000 tokens to each test node...
        GLOGDEBUG("TEST: Send native tokens: " + std::to_string(i));
        blockchainSendNativeTokens(
          *blockchains[0], // use node 0 to call (could be any node)
          SDKTestSuite::chainOwnerAccount(), // from: chain owner
          nodeAccs[i].address, // to: each node's address (controlled by its validator private key)
          aThousandNativeTokens * 5 // 5,000 eth
        );
        // ...and all nodes agree on this...
        for (int j = 0; j < numNodes; ++j) {
          REQUIRE(blockchainCheckDeposit(*blockchains[j], nodeAccs[i].address, aThousandNativeTokens * 5));
        }
        // ...and each test node stakes 1,000 tokens in the chain governance contract, so
        //    each node can actually "register" (delegate to themselves).
        blockchainCallCpp(
          *blockchains[0], // use node 0 to call (could be any node)
          nodeAccs[i], // from: node i
          systemContractAddr, // to: SystemContract
          aThousandNativeTokens, // node i is depositing (staking) this much in the SystemContract
          &SystemContract::stake // calls SystemContract::stake() to deposit tokens from node i
        );
        // Balance in SystemContract account must grow by 1,000 eth for each validator account,
        // and all running nodes agree on this.
        for (int j = 0; j < numNodes; ++j) {
          REQUIRE(blockchainCheckDeposit(*blockchains[j], systemContractAddr, aThousandNativeTokens * (i + 1)));
        }
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(3000)); // this should be more than enough
      GLOGDEBUG("Test: start first delegations");

      // Nodes #0 .. #4 delegate 5, 4, 3, 2, 1 for themselves, respectively
      // This results in nodes #0, #1, #2, #3 elected, since there are 4 validator slots (set on genesis)
      // These are the same 4 validator keys that were set on genesis
      // Node #4 only gets 1 vote (from self) so it loses the election to the other 4 nodes
      for (int i = 0; i < 5; ++i) {
        blockchainCallCpp(
          *blockchains[0], // use node 0 to call (could be any node)
          nodeAccs[i], // node i is delegating
          systemContractAddr, // to: SystemContract
          0, // delegate does not send native tokens
          &SystemContract::delegate, // calls SystemContract::delegate() to stake tokens on specific validator
          Hex::fromBytes(nodeAccs[i].pubKey).get(), // the delegation is to node i (self)
          uint256_t("1000000000000000000") * (5 - i) // node 0 delegates 5 eth, node 1 delegates 4 eth ... node 4 delegates 1 eth
        );
      }

      // After all transactions went through, the validator set with 4 slots should contain the same validators,
      //   but possibly (and probably) in a different order, because the voting power has changed from 10,10,10,10
      //   to 5 eth, 4 eth, 3 th and 2 eth. The original sorting order was arbitrarily determined by the genesis set.
      // NOTE: just wait a while and then read the blockchain's validator set, as the blockchain's validator set
      //       is modified (or not) by the system contract when it receives delegate calls.
      //       it could be that the system contract changed but the blockchain validator set wasn't notified, but
      //       this check won't catch that kind of bug here.
      GLOGDEBUG("Test: end first delegations");
      std::this_thread::sleep_for(std::chrono::milliseconds(5'000)); // we actually need to wait for H+2 activation!
      GLOGDEBUG("Test: checking that the exact same validator keys are still elected (order and voting power is OK to change)");
      for (int i = 0; i < 5; ++i) {
        std::vector<CometValidatorUpdate> validatorSet;
        uint64_t validatorSetHeight = 0;
        blockchains[i]->state().getValidatorSet(validatorSet, validatorSetHeight);
        REQUIRE(origValidatorSet.size() == validatorSet.size()); // == numValidators, already asserted above
        std::set<PubKey> keysToMatch;
        for (int j = 0; j < validatorSet.size(); ++j) {
          keysToMatch.insert(PubKey(origValidatorSet[j].publicKey));
        }
        for (int j = 0; j < validatorSet.size(); ++j) {
          keysToMatch.erase(PubKey(validatorSet[j].publicKey));
        }
        REQUIRE(keysToMatch.empty()); // orig and current were same size and had the exact same keys (in whatever order, doesn't matter)
        REQUIRE(origValidatorSetHeight != validatorSetHeight); // they should be different, obviously (orig is 0, current is > 0)
      }

      // Delegates 10 tokens for node #5
      GLOGDEBUG("Test: delegating to node 5, should push node 3 out and be (5, 0, 1, 2)");
      blockchainCallCpp(
        *blockchains[0], // use node 0 to call (could be any node)
        nodeAccs[5],
        systemContractAddr, // to: SystemContract
        0, // delegate does not send native tokens
        &SystemContract::delegate, // calls SystemContract::delegate() to stake tokens on specific validator
        Hex::fromBytes(nodeAccs[5].pubKey).get(), // self delegation (node is "registering" itself -- first delegation must be from/to self)
        uint256_t("1000000000000000000") * 10 // node 5 delegates 10 eth to itself
      );

      // After tx goes through, resulting validator set should be #5, #0, #1, #2 for the 4 slots (node #3 gets pushed out)
      std::this_thread::sleep_for(std::chrono::milliseconds(5'000)); // we actually need to wait for H+2 activation!
      GLOGDEBUG("Test: checking node 5 pushes node 3 out (5, 0, 1, 2)");
      for (int i = 0; i < 5; ++i) {
        std::vector<CometValidatorUpdate> validatorSet;
        uint64_t validatorSetHeight = 0;
        blockchains[i]->state().getValidatorSet(validatorSet, validatorSetHeight);
        REQUIRE(validatorSet.size() == numValidators); // still 4 slots
        std::set<PubKey> keysToMatch;
        keysToMatch.insert(nodeAccs[5].pubKey);
        keysToMatch.insert(nodeAccs[0].pubKey);
        keysToMatch.insert(nodeAccs[1].pubKey);
        keysToMatch.insert(nodeAccs[2].pubKey);
        for (int j = 0; j < validatorSet.size(); ++j) {
          keysToMatch.erase(PubKey(validatorSet[j].publicKey));
        }
        REQUIRE(keysToMatch.empty());
      }

      // Node #3 is no longer elected, so it cannot vote for slots
      GLOGDEBUG("Test: node #3 votes for 4 slots (vote should not be accepted since node #3 is no longer elected)");
      blockchainCallCpp(
        *blockchains[0], // use node 0 to call (could be any node)
        nodeAccs[3],
        systemContractAddr, // to: SystemContract
        0,
        &SystemContract::voteSlots,
        Hex::fromBytes(nodeAccs[3].pubKey).get(), // caller key
        4UL // or static_cast<uint64_t>(4)
      );

      // Validators #5, #0 vote to change number of slots to 5, 5
      GLOGDEBUG("Test: nodes #5 and #0 vote for 5 slots");
      blockchainCallCpp(
        *blockchains[0], // use node 0 to call (could be any node)
        nodeAccs[5],
        systemContractAddr, // to: SystemContract
        0,
        &SystemContract::voteSlots,
        Hex::fromBytes(nodeAccs[5].pubKey).get(), // caller key
        5UL
      );
      blockchainCallCpp(
        *blockchains[0], // use node 0 to call (could be any node)
        nodeAccs[0],
        systemContractAddr, // to: SystemContract
        0,
        &SystemContract::voteSlots,
        Hex::fromBytes(nodeAccs[0].pubKey).get(), // caller key
        5UL
      );

      // If node #3's vote is considered (which is an error), it will change to 4 after all three votes go through since there are 4 slots and 3 votes (>2/3)
      // So we expect the number of slots to not change.
      std::this_thread::sleep_for(std::chrono::milliseconds(5'000)); // we actually need to wait for H+2 activation!
      GLOGDEBUG("Test: checking number of slots is unchanged across the entire network");
      for (int i = 0; i < 5; ++i) {
        std::vector<CometValidatorUpdate> validatorSet;
        uint64_t validatorSetHeight = 0;
        blockchains[i]->state().getValidatorSet(validatorSet, validatorSetHeight);
        REQUIRE(validatorSet.size() == numValidators); // still 4 slots
      }

      // Node #1 vote to change the number of slots to 6
      GLOGDEBUG("Test: nodes #1 votes for 6 slots");
      blockchainCallCpp(
        *blockchains[0], // use node 0 to call (could be any node)
        nodeAccs[1],
        systemContractAddr, // to: SystemContract
        0,
        &SystemContract::voteSlots,
        Hex::fromBytes(nodeAccs[1].pubKey).get(), // caller key
        6UL // valid votes should now be 5, 5, 6 (75% of elected validators want to change upwards), and 5 is the greatest increase that 2/3 agree with
      );

      // After tx goes through, number of slots should change to 5, making the validator set be: #5, #0, #1, #2, #3
      std::this_thread::sleep_for(std::chrono::milliseconds(5'000)); // we actually need to wait for H+2 activation!
      GLOGDEBUG("Test: checking number of slots changed to 5 and elected validators are 5, 0, 1, 2, 3 (in order)");
      for (int i = 0; i < 5; ++i) {
        std::vector<CometValidatorUpdate> validatorSet;
        uint64_t validatorSetHeight = 0;
        blockchains[i]->state().getValidatorSet(validatorSet, validatorSetHeight);
        REQUIRE(validatorSet.size() == 5); // slot increase
        REQUIRE(PubKey(validatorSet[0].publicKey) == nodeAccs[5].pubKey);
        REQUIRE(PubKey(validatorSet[1].publicKey) == nodeAccs[0].pubKey);
        REQUIRE(PubKey(validatorSet[2].publicKey) == nodeAccs[1].pubKey);
        REQUIRE(PubKey(validatorSet[3].publicKey) == nodeAccs[2].pubKey);
        REQUIRE(PubKey(validatorSet[4].publicKey) == nodeAccs[3].pubKey);
      }

      // Validator #1 gets fully undelegated (-4 tokens).
      GLOGDEBUG("Test: fully undelegating from node 1, should push node 1 out and be (5, 0, 2, 3, 4)");
      blockchainCallCpp(
        *blockchains[0], // use node 0 to call (could be any node)
        nodeAccs[1],
        systemContractAddr, // to: SystemContract
        0,
        &SystemContract::undelegate, // calls SystemContract::undelegate() to unvote validator
        Hex::fromBytes(nodeAccs[1].pubKey).get(), // self undelegation
        uint256_t("1000000000000000000") * 4 // node 1 undelegates 4 eth from itself, now 0 votes total, node 4 gets elected in its place
      );

      // After tx goes through, validator set should be #5, #0, #2, #3, #4
      std::this_thread::sleep_for(std::chrono::milliseconds(5'000)); // we actually need to wait for H+2 activation!
      GLOGDEBUG("Test: checking elected validators are 5, 0, 2, 3, 4 (in order)");
      for (int i = 0; i < 5; ++i) {
        std::vector<CometValidatorUpdate> validatorSet;
        uint64_t validatorSetHeight = 0;
        blockchains[i]->state().getValidatorSet(validatorSet, validatorSetHeight);
        REQUIRE(validatorSet.size() == 5); // same number of slots and we still have at least 5 validators to fill in these 5 slots
                                           // (if node 4 had also fully undelegated, we'd see only 4 validators elected for the 5 slots)
        REQUIRE(PubKey(validatorSet[0].publicKey) == nodeAccs[5].pubKey);
        REQUIRE(PubKey(validatorSet[1].publicKey) == nodeAccs[0].pubKey);
        REQUIRE(PubKey(validatorSet[2].publicKey) == nodeAccs[2].pubKey);
        REQUIRE(PubKey(validatorSet[3].publicKey) == nodeAccs[3].pubKey);
        REQUIRE(PubKey(validatorSet[4].publicKey) == nodeAccs[4].pubKey);
      }

      // for the next test, we need to lock block finalization (incomingBlock() on the entire network, which will essentially stall
      // block proposal and thus mempool tx selection for blocks). we need this because we need all CPP calls below to go in the same block.
      GLOGDEBUG("TEST: locking block processing across entire network");
      for (int i = 0; i < numNodes; ++i) {
        blockchains[i]->lockBlockProcessing();
      }

      // set decrease numslot votes: 1, 1, 2, 3, 4 by sending 5 voteSlots txs
      GLOGDEBUG("Test: elected validators will vote for numslots decrease: 1,1,2,3,4 (doesn't matter which validator votes what)");
      blockchainCallCpp(
        *blockchains[0], // use node 0 to call (could be any node)
        nodeAccs[0],
        systemContractAddr, // to: SystemContract
        0,
        &SystemContract::voteSlots,
        Hex::fromBytes(nodeAccs[0].pubKey).get(), // caller key
        1UL
      );
      blockchainCallCpp(
        *blockchains[0], // use node 0 to call (could be any node)
        nodeAccs[2],
        systemContractAddr, // to: SystemContract
        0,
        &SystemContract::voteSlots,
        Hex::fromBytes(nodeAccs[2].pubKey).get(), // caller key
        1UL
      );
      blockchainCallCpp(
        *blockchains[0], // use node 0 to call (could be any node)
        nodeAccs[3],
        systemContractAddr, // to: SystemContract
        0,
        &SystemContract::voteSlots,
        Hex::fromBytes(nodeAccs[3].pubKey).get(), // caller key
        2UL
      );
      blockchainCallCpp(
        *blockchains[0], // use node 0 to call (could be any node)
        nodeAccs[4],
        systemContractAddr, // to: SystemContract
        0,
        &SystemContract::voteSlots,
        Hex::fromBytes(nodeAccs[4].pubKey).get(), // caller key
        3UL
      );
      blockchainCallCpp(
        *blockchains[0], // use node 0 to call (could be any node)
        nodeAccs[5],
        systemContractAddr, // to: SystemContract
        0,
        &SystemContract::voteSlots,
        Hex::fromBytes(nodeAccs[5].pubKey).get(), // caller key
        4UL
      );

      // wait a little bit to ensure all 5 voteSlots txs above can enter all mempools
      GLOGDEBUG("TEST: waiting for all 5 txs to enter mempools across the entire network");
      int decvoteTxsTimeout = 11;
      while (--decvoteTxsTimeout > 0) { // 10 iterations = 10s
        int ok = 0;
        for (int i = 0; i < numNodes; ++i) {
          if (blockchains[i]->getNumUnconfirmedTxs() >= 5) {
            ++ok;
          }
        }
        if (ok == numNodes) {
          break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1'000));
      }
      REQUIRE(decvoteTxsTimeout > 0);

      // now that the txs are in all mempools, unlock block processing so all the 5 txs can go to the same prepareproposal (block)
      // by whatever validator gets picked as the next proposer.
      // this works because holding up the FinalizeBlock ABCI callback across the entire network will prevent the next proposer
      // from advancing to the PrepareProposal step, which includes flushing the mempool into the next block (since it's just a
      // tiny amount of tx data and blocks tolerate megabytes of data, it's 100% odds that all of them go in the next block).
      GLOGDEBUG("TEST: unlocking block processing across entire network");
      for (int i = 0; i < numNodes; ++i) {
        blockchains[i]->unlockBlockProcessing();
      }

      // should reduce to 3, not 4, since 2/3 threshold is met by 3 (the vote on 4 is skipped)
      std::this_thread::sleep_for(std::chrono::milliseconds(5'000)); // we actually need to wait for H+2 activation!
      GLOGDEBUG("Test: checking elected validators are 5, 0, 2 (in order)");
      for (int i = 0; i < 5; ++i) {
        std::vector<CometValidatorUpdate> validatorSet;
        uint64_t validatorSetHeight = 0;
        blockchains[i]->state().getValidatorSet(validatorSet, validatorSetHeight);
        REQUIRE(validatorSet.size() == 3); // decrease slots vote should have evaluated to 3
        REQUIRE(PubKey(validatorSet[0].publicKey) == nodeAccs[5].pubKey);
        REQUIRE(PubKey(validatorSet[1].publicKey) == nodeAccs[0].pubKey);
        REQUIRE(PubKey(validatorSet[2].publicKey) == nodeAccs[2].pubKey);
      }
      GLOGDEBUG("TEST: Validator set test finished.");
    }

    SECTION("BlockchainBootTest") {
      std::string testDumpPath = createTestDumpPath("BlockchainBootTest");

      GLOGDEBUG("TEST: creating Options for Blockchain");

      // Standard single node test setup
      int p2p_port = SDKTestSuite::getTestPort();
      int rpc_port = SDKTestSuite::getTestPort();
      const Options options = SDKTestSuite::getOptionsForTest(testDumpPath, false, "", p2p_port, rpc_port, 0, 1, {}, 0);

      GLOGDEBUG("TEST: creating Blockchain");

      // Create a blockchain node
      Blockchain blockchain(options, testDumpPath);

      GLOGDEBUG("TEST: starting Blockchain");

      // Start the blockchain node
      // This waits until it reaches CometState::RUNNING, then starts the RPC endpoint.
      blockchain.start();

      GLOGDEBUG("TEST: stopping Blockchain");

      // Then just check that we can stop it without any problems.
      blockchain.stop();

      GLOGDEBUG("TEST: done");
    }

    SECTION("BlockchainAccountNonceTest") {
      // Perform various account/nonce tests (repeat nonce, nonce sequence w/ same account in same block, ...)
      std::string testDumpPath = createTestDumpPath("BlockchainAccountNonceTest");

      int p2p_port = SDKTestSuite::getTestPort();
      int rpc_port = SDKTestSuite::getTestPort();
      const Options options = SDKTestSuite::getOptionsForTest(testDumpPath, false, "", p2p_port, rpc_port, 0, 1, {}, 0);

      Blockchain blockchain(options, testDumpPath);

      // Here we have to create the CometBFT address that corresponds to the Eth address that we want the coinbase to be set to.
      // Unfortunately this has to be valid otherwise the coinbase processing step in State::processBlock() will blow up.
      // Fortunately, we know getOptionsForTest() will use cometTestKeys[0] for our first and only validator.
      Bytes accValidatorPrivKeyBytes = base64::decode_into<Bytes>(cometTestKeys[0].priv_key);
      PrivKey accValidatorPrivKey(accValidatorPrivKeyBytes);
      Bytes accValidatorPubKeyBytes = Secp256k1::toPub(accValidatorPrivKey).asBytes();
      Bytes accValidatorCometAddress = Comet::getCometAddressFromPubKey(accValidatorPubKeyBytes);
      TestAccount accValidator(accValidatorPrivKey);

      // Need to emulate initChain() call to force initialization of the validator set.
      Bytes dummyBytes;
      std::string chainIdStr = std::to_string(options.getChainID());
      blockchain.initChain(0, chainIdStr, dummyBytes, 1, {{accValidatorPubKeyBytes, 10}}, dummyBytes);

      // For this test, we will not do blockchain start() and stop().
      // Instead, we will just fool Blockchain/State and inject some
      //  TxBlock and FinalizedBlock objects we create here, which is
      //  faster, doesn't involve networking, and allows better control.
      // We can write a networked test as well which takes transactions
      //  via the BDK RPC or the CometBFT RPC port (blockchain.comet().rpcSyncCall('tx',... )
      //  but the BDK RPC will validate the tx and prevent us from sending invalid txs to
      //  the mempool.

      // give lots of tokens to account A
      // tx A --> AA nonce 0 X token
      // tx A --> AA nonce 1 X token
      // tx A --> AA nonce 2 X token
      // tx A --> AA nonce 3 X token
      // verify all included in block, balance of AA is now 4*X token

      TestAccount accA = TestAccount::newRandomAccount();
      TestAccount accAA = TestAccount::newRandomAccount();

      blockchain.state().setBalance(accA.address, uint256_t("1000000000000000000000")); // +1,000 eth
      blockchain.state().setBalance(accAA.address, uint256_t("1000000000000000000000")); // +1,000 eth

      uint256_t accAbal0 = blockchain.state().getNativeBalance(accA.address);
      uint256_t accAAbal0 = blockchain.state().getNativeBalance(accAA.address);
      GLOGDEBUG("TEST: accA starting balance: " + accAbal0.str());
      GLOGDEBUG("TEST: accAA starting balance: " + accAAbal0.str());
      REQUIRE(accAbal0 == uint256_t("1000000000000000000000"));
      REQUIRE(accAAbal0 == uint256_t("1000000000000000000000"));

      TxBlock tx_A_AA_0 (
        accAA.address, // to
        accA.address, // from
        Bytes(), // data
        options.getChainID(), // chainId
        0, // nonce
        1000000000000000000, // value
        21000, // maxPriorityFeePerGas
        1000000000, // maxFeePerGas
        1000000000, // gasLimit
        accA.privKey // privKey
      );

      TxBlock tx_A_AA_1 (
        accAA.address, // to
        accA.address, // from
        Bytes(), // data
        options.getChainID(), // chainId
        1, // nonce
        1000000000000000000, // value
        21000, // maxPriorityFeePerGas
        1000000000, // maxFeePerGas
        1000000000, // gasLimit
        accA.privKey // privKey
      );

      TxBlock tx_A_AA_2 (
        accAA.address, // to
        accA.address, // from
        Bytes(), // data
        options.getChainID(), // chainId
        2, // nonce
        1000000000000000000, // value
        21000, // maxPriorityFeePerGas
        1000000000, // maxFeePerGas
        1000000000, // gasLimit
        accA.privKey // privKey
      );

      TxBlock tx_A_AA_3 (
        accAA.address, // to
        accA.address, // from
        Bytes(), // data
        options.getChainID(), // chainId
        3, // nonce
        1000000000000000000, // value
        21000, // maxPriorityFeePerGas
        1000000000, // maxFeePerGas
        1000000000, // gasLimit
        accA.privKey // privKey
      );

      REQUIRE(blockchain.state().validateTransaction(tx_A_AA_0, true) == true);
      REQUIRE(blockchain.state().validateTransaction(tx_A_AA_1, true) == true);
      REQUIRE(blockchain.state().validateTransaction(tx_A_AA_2, true) == true);
      REQUIRE(blockchain.state().validateTransaction(tx_A_AA_3, true) == true);

      // Fake an ABCI block here with the transactions
      CometBlock cometBlock;
      cometBlock.height = 1;
      cometBlock.timeNanos = 1;
      cometBlock.proposerAddr = accValidatorCometAddress;
      cometBlock.txs.push_back(tx_A_AA_0.rlpSerialize(true));
      cometBlock.txs.push_back(tx_A_AA_1.rlpSerialize(true));
      cometBlock.txs.push_back(tx_A_AA_2.rlpSerialize(true));
      cometBlock.txs.push_back(tx_A_AA_3.rlpSerialize(true));
      cometBlock.txs.push_back(Utils::randBytes(32)); // append a randomHash non-tx tx (required by our protocol / FinalizedBlock::fromCometBlock())
      cometBlock.hash.resize(32); // The block hash can be whatever, it's not checked.
      cometBlock.prevHash.resize(32); // The prev block hash can be whatever, it's not checked.

      // Create a BDK FinalizedBlock from the fake ABCI block and send it to the machine state
      std::vector<bool> succeeded;
      std::vector<uint64_t> gasUsed;
      std::vector<CometValidatorUpdate> validatorUpdates;
      FinalizedBlock finBlock1 = FinalizedBlock::fromCometBlock(cometBlock);
      blockchain.state().processBlock(finBlock1, succeeded, gasUsed, validatorUpdates);

      REQUIRE(succeeded.size() == 4);
      REQUIRE(succeeded[0] == true);
      REQUIRE(succeeded[1] == true);
      REQUIRE(succeeded[2] == true);
      REQUIRE(succeeded[3] == true);
      REQUIRE(gasUsed[0] == 21000);
      REQUIRE(gasUsed[1] == 21000);
      REQUIRE(gasUsed[2] == 21000);
      REQUIRE(gasUsed[3] == 21000);

      uint256_t accNonce1 = blockchain.state().getNativeNonce(accA.address);
      REQUIRE(accNonce1 == 4);

      uint256_t accAbal1 = blockchain.state().getNativeBalance(accA.address);
      uint256_t accAAbal1 = blockchain.state().getNativeBalance(accAA.address);
      GLOGDEBUG("TEST: accA block 1 balance: " + accAbal1.str());
      GLOGDEBUG("TEST: accAA block 1 balance: " + accAAbal1.str());
      REQUIRE(accAbal1 == uint256_t("995999916000000000000")); // REVIEW: 21 * 4 = 84; 100 - 84 = 16 (21K * 1M = gasPrice for each tx)
      REQUIRE(accAAbal1 == uint256_t("1004000000000000000000"));

      // For the next tests, we will go even deeper and pretend we are the ABCI, and call
      // Blockchain::buildBlockProposal directly with a bunch of weird transactions.
      // This is easier than torturing CometBFT to stuff "bad" transactions in the mempool
      // (which would require us to actually bypass checks that are in the Blockchain class)
      // Afterwards, we manually check for what the block builder did.

      // Test nonce in past, nonce in future OK, and nonce in future with a hole (fails)

      // Build the transactions

      TxBlock tx_A_AA_3_PastNonce (
        accAA.address, // to
        accA.address, // from
        Bytes(), // data
        options.getChainID(), // chainId
        3, // nonce
        1000000000000000000, // value
        21000, // maxPriorityFeePerGas
        1000000000, // maxFeePerGas
        1000000000, // gasLimit
        accA.privKey // privKey
      );

      TxBlock tx_A_AA_4_PresentNonce (
        accAA.address, // to
        accA.address, // from
        Bytes(), // data
        options.getChainID(), // chainId
        4, // nonce
        1000000000000000000, // value
        21000, // maxPriorityFeePerGas
        1000000000, // maxFeePerGas
        1000000000, // gasLimit
        accA.privKey // privKey
      );

      TxBlock tx_A_AA_5_FutureNonce (
        accAA.address, // to
        accA.address, // from
        Bytes(), // data
        options.getChainID(), // chainId
        5, // nonce
        1000000000000000000, // value
        21000, // maxPriorityFeePerGas
        1000000000, // maxFeePerGas
        1000000000, // gasLimit
        accA.privKey // privKey
      );

      TxBlock tx_A_AA_6_TooExpensive (
        accAA.address, // to
        accA.address, // from
        Bytes(), // data
        options.getChainID(), // chainId
        6, // nonce
        uint256_t("10000000000000000000000"), // value 10,000 eth: can't pay so will reject
        21000, // maxPriorityFeePerGas
        1000000000, // maxFeePerGas
        1000000000, // gasLimit
        accA.privKey // privKey
      );

      // Nonce 6 is missing because the tx with nonce 6 is too expensive so it was never added to the mempool
      TxBlock tx_A_AA_7_MissingNonce6 (
        accAA.address, // to
        accA.address, // from
        Bytes(), // data
        options.getChainID(), // chainId
        7, // nonce
        1000000000000000000, // value
        21000, // maxPriorityFeePerGas
        1000000000, // maxFeePerGas
        1000000000, // gasLimit
        accA.privKey // privKey
      );

      // Fake CheckTx calls (not really necessary, but mimics what would happen more closely)
      REQUIRE(blockchain.state().validateTransaction(tx_A_AA_3_PastNonce, true) == false); // expected to fail
      REQUIRE(blockchain.state().validateTransaction(tx_A_AA_4_PresentNonce, true) == true);
      REQUIRE(blockchain.state().validateTransaction(tx_A_AA_5_FutureNonce, true) == true);
      REQUIRE(blockchain.state().validateTransaction(tx_A_AA_6_TooExpensive, true) == false); // expected to fail
      REQUIRE(blockchain.state().validateTransaction(tx_A_AA_7_MissingNonce6, true) == false); // expected to fail

      // Create a more profitable alternative for nonce 4
      TxBlock tx_A_AA_4_PresentNonce_MoreProfitable (
        accAA.address, // to
        accA.address, // from
        Bytes(), // data
        options.getChainID(), // chainId
        4, // nonce
        100000000000000000, // value (10x smaller than value of tx_A_AA_4_PresentNonce)
        21000, // maxPriorityFeePerGas
        2000000000, // maxFeePerGas
        2000000000, // gasLimit
        accA.privKey // privKey
      );
      REQUIRE(blockchain.state().validateTransaction(tx_A_AA_4_PresentNonce_MoreProfitable, true) == true);

      // Fake the CometBlock that is the pre-proposal (just stuff all the txs there)
      cometBlock.height = 2;
      cometBlock.timeNanos = 2;
      cometBlock.txs.clear();
      cometBlock.txs.push_back(tx_A_AA_3_PastNonce.rlpSerialize(true)); // should be excluded by block builder
      cometBlock.txs.push_back(tx_A_AA_4_PresentNonce.rlpSerialize(true)); // should be excluded by block builder
      cometBlock.txs.push_back(tx_A_AA_4_PresentNonce_MoreProfitable.rlpSerialize(true));
      cometBlock.txs.push_back(tx_A_AA_5_FutureNonce.rlpSerialize(true));
      cometBlock.txs.push_back(tx_A_AA_7_MissingNonce6.rlpSerialize(true)); // should be excluded by block builder

      // Call buildBlockProposal
      GLOGDEBUG("TEST: calling buildBlockProposal");
      bool noChange;
      std::vector<size_t> txIds;
      std::vector<Bytes> injectTxs;
      blockchain.buildBlockProposal(100'000'000, cometBlock, noChange, txIds, injectTxs);
      for (const size_t txId : txIds) {
        GLOGDEBUG("TEST: proposal has included txId: " + std::to_string(txId));
      }
      REQUIRE(txIds.size() == 2);
      REQUIRE(txIds[0] == 2); // tx_A_AA_4_PresentNonce_MoreProfitable
      REQUIRE(txIds[1] == 3); // tx_A_AA_5_FutureNonce

      // Fix the block according to the block builder
      std::vector<Bytes> newTxs;
      for (const size_t txId : txIds) {
        newTxs.push_back(cometBlock.txs[txId]);
      }
      cometBlock.txs = newTxs;
      REQUIRE(cometBlock.txs.size() == 2);

      // Send it to processBlock() for good measure.
      succeeded.clear();
      gasUsed.clear();
      cometBlock.txs.push_back(Utils::randBytes(32)); // append a randomHash non-tx tx (required by our protocol / FinalizedBlock::fromCometBlock())
      FinalizedBlock finBlock2 = FinalizedBlock::fromCometBlock(cometBlock);
      blockchain.state().processBlock(finBlock2, succeeded, gasUsed, validatorUpdates);

      // Check that the transactions picked by the block builder each have the expected outcome.
      uint256_t accAbal2 = blockchain.state().getNativeBalance(accA.address);
      uint256_t accAAbal2 = blockchain.state().getNativeBalance(accAA.address);
      GLOGDEBUG("TEST: accA block 2 balance: " + accAbal2.str());
      GLOGDEBUG("TEST: accAA block 2 balance: " + accAAbal2.str());
      REQUIRE(accAbal2 == uint256_t("994899853000000000000"));
      REQUIRE(accAAbal2 == uint256_t("1005100000000000000000"));

      // Test eject tx from memory model then build a nonce sequence that depends on it.

      // Massively expensive tx
      TxBlock tx_A_AA_6 (
        accAA.address, // to
        accA.address, // from
        Bytes(), // data
        options.getChainID(), // chainId
        6, // nonce
        uint256_t("900000000000000000000"), // value is 900 ETH, account A has 994.89... ETH right now
        21000, // maxPriorityFeePerGas
        1000000000, // maxFeePerGas
        1000000000, // gasLimit
        accA.privKey // privKey
      );

      // Regular tx
      TxBlock tx_A_AA_7 (
        accAA.address, // to
        accA.address, // from
        Bytes(), // data
        options.getChainID(), // chainId
        7, // nonce
        1000000000000000000, // value
        21000, // maxPriorityFeePerGas
        1000000000, // maxFeePerGas
        1000000000, // gasLimit
        accA.privKey // privKey
      );

      // For now, we can afford both, emulate CheckTx
      REQUIRE(blockchain.state().validateTransaction(tx_A_AA_6, true) == true);
      REQUIRE(blockchain.state().validateTransaction(tx_A_AA_7, true) == true);

      // Take 900 ETH out of A, so now it can't afford tx 6, though it can afford tx 7
      uint256_t preHackBalance = accAbal2;
      uint256_t hackedBalance = accAbal2 - uint256_t("900000000000000000000");
      blockchain.state().setBalance(accA.address, hackedBalance); // -900 eth
      uint256_t accAbal2hacked = blockchain.state().getNativeBalance(accA.address);
      GLOGDEBUG("TEST: accA block 2 balance (after -900 eth hack): " + accAbal2hacked.str());
      REQUIRE(accAbal2hacked == uint256_t("94899853000000000000"));

      // Do a recheck for tx 6, but turn off affectsMempool so it will flag it as
      //   ejected in the State's mempoolModel_ instead of just removing it.
      GLOGDEBUG("TEST: forcing flag tx nonce=6 as ejected in the State's mempool model due to insufficient balance");
      REQUIRE(blockchain.state().validateTransaction(tx_A_AA_6, false) == false);

      // Give the money back to A, so now it can afford tx 6 again.
      // But it should no longer matter, as tx 6 has already been ejected from the mempool
      //   according to the State's mempool model.
      GLOGDEBUG("TEST: restoring accA block 2 balance");
      blockchain.state().setBalance(accA.address, preHackBalance); // +900 eth

      // Now send both txs to the block builder, both should be excluded
      cometBlock.height = 2;
      cometBlock.timeNanos = 2;
      cometBlock.txs.clear();
      cometBlock.txs.push_back(tx_A_AA_6.rlpSerialize(true)); // should be excluded by block builder: flagged as ejected
      cometBlock.txs.push_back(tx_A_AA_7.rlpSerialize(true)); // should be excluded by block builder: nonce path deleted
      txIds.clear();
      blockchain.buildBlockProposal(100'000'000, cometBlock, noChange, txIds, injectTxs);
      REQUIRE(txIds.size() == 0);
    }

    SECTION("BlockchainStateDumpTriggerTest") {
      std::string testDumpPath = createTestDumpPath("BlockchainStateDumpTriggerTest");

      GLOGDEBUG("TEST: creating Options for Blockchain");

      int snapshotCount = 5;
      int stateDumpTrigger = 4; // # blocks between auto snapshot save
      int p2p_port = SDKTestSuite::getTestPort();
      int rpc_port = SDKTestSuite::getTestPort();
      const Options options = SDKTestSuite::getOptionsForTest(
        testDumpPath, false, "", p2p_port, rpc_port, 0, 1, {}, 0, stateDumpTrigger, "100ms"
      );

      GLOGDEBUG("TEST: creating Blockchain with stateDumpTrigger = " + std::to_string(stateDumpTrigger));

      Blockchain blockchain(options, testDumpPath);

      GLOGDEBUG("TEST: starting Blockchain");

      blockchain.start();

      GLOGDEBUG("TEST: starting Blockchain");

      // Wait until a minimum height is reached
      while (blockchain.state().getHeight() < stateDumpTrigger * snapshotCount) {
        GLOGDEBUG("TEST: Blockchain height = " + std::to_string(blockchain.state().getHeight()));
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      }

      GLOGDEBUG("TEST: stopping Blockchain");

      blockchain.stop();

      // Check we have created snapshotCount snapshot directories
      std::filesystem::path snapshotsDir = std::filesystem::path(testDumpPath) / "snapshots";
      REQUIRE(std::filesystem::exists(snapshotsDir));
      REQUIRE(std::filesystem::is_directory(snapshotsDir));
      for (int i = 1; i <= snapshotCount; ++i) {
        int expectedHeight = i * stateDumpTrigger;
        std::filesystem::path expectedSnapshotDir = snapshotsDir / std::to_string(expectedHeight);
        GLOGDEBUG("TEST: Checking existence of snapshot directory: " + expectedSnapshotDir.string());
        REQUIRE(std::filesystem::exists(expectedSnapshotDir));
        REQUIRE(std::filesystem::is_directory(expectedSnapshotDir));
      }

      GLOGDEBUG("TEST: done");
    }
  }
}

