/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

// TODO: this is commented out on CMakeLists.txt, revise later

#include <filesystem>

#include "../../src/libs/catch2/catch_amalgamated.hpp"

#include "../../src/core/blockchain.h"

#include "../sdktestsuite.hpp"

// FIXME: use or remove
// Forward declaration from tests/net/http/httpjsonrpc.cpp, for usage within sending transactions
std::string makeHTTPRequest(
  const std::string& reqBody, const std::string& host, const std::string& port,
  const std::string& target, const std::string& requestType, const std::string& contentType
);

/**
 * Tests for the new Blockchain class.
 * Here we are using and testing the Blockchain class itself, not the SDKTestSuite test helper subclass.
 */
namespace TBlockchain {
  auto createTestDumpPath = SDKTestSuite::createTestDumpPath;
  TEST_CASE("Blockchain Class", "[core][blockchain]") {
    std::string testDumpPath = Utils::getTestDumpPath();

    // TODO: various Blockchain class RPC tests

    // Simple Blockchain start/stop test
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

    // Perform various account/nonce tests (repeat nonce, nonce sequence w/ same account in same block, ...)
    SECTION("BlockchainAccountNonceTest") {
      std::string testDumpPath = createTestDumpPath("BlockchainAccountNonceTest");

      int p2p_port = SDKTestSuite::getTestPort();
      int rpc_port = SDKTestSuite::getTestPort();
      const Options options = SDKTestSuite::getOptionsForTest(testDumpPath, false, "", p2p_port, rpc_port, 0, 1, {}, 0);

      Blockchain blockchain(options, testDumpPath);

      // For this test, we will fool Blockchain/State and just inject
      //  some TxBlock and FinalizedBlock objects we create here, which
      //  is faster, doesn't involve networking, and allows us greater
      //  control of what's going on.
      // We can write a networked test as well which takes transactions
      //  via the BDK RPC or the CometBFT RPC port (blockchain.comet().rpcSyncCall('tx',... )
      //  but the BDK RPC will validate the tx and prevent us from sending invalid txs to
      //  the mempool.

      // next block:

      // give infinite money to account A
      // tx A --> AA nonce 0 10 token
      // tx A --> AA nonce 1 10 token
      // tx A --> AA nonce 2 10 token
      // tx A --> AA nonce 3 10 token
      // verify all included in block, balance of AA is now 40 token

      TestAccount accA = TestAccount::newRandomAccount();

      // We need to inject a fake comet validator whose key will be the coinbase.
      Bytes accApubKeyBytes = Secp256k1::toPub(accA.privKey).asBytes();
      blockchain.setValidators({{accApubKeyBytes, 10}}); // second arg is voting power (irrelevant)
      Bytes accAcometAddress = Comet::getCometAddressFromPubKey(accApubKeyBytes);

      TestAccount accAA = TestAccount::newRandomAccount();

      blockchain.state().addBalance(accA.address);

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

      // Fake an ABCI block here
      CometBlock cometBlock;
      cometBlock.height = 1;
      cometBlock.timeNanos = 0;
      // Here we have to create the CometBFT address that corresponds to the Eth address that we want the coinbase to be set to.
      // Unfortunately this has to be valid otherwise the coinbase set step will blow up.
      cometBlock.proposerAddr = accAcometAddress;
      // Put our transactions in there
      cometBlock.txs.push_back(tx_A_AA_0.rlpSerialize(true));
      cometBlock.txs.push_back(tx_A_AA_1.rlpSerialize(true));
      cometBlock.txs.push_back(tx_A_AA_2.rlpSerialize(true));
      cometBlock.txs.push_back(tx_A_AA_3.rlpSerialize(true));
      cometBlock.hash.resize(32); // The block hash can be whatever, it's not checked.
      cometBlock.prevHash.resize(32); // The prev block hash can be whatever, it's not checked.

      std::vector<bool> succeeded;
      std::vector<uint64_t> gasUsed;
      FinalizedBlock finBlock = FinalizedBlock::fromCometBlock(cometBlock);
      blockchain.state().processBlock(finBlock, succeeded, gasUsed);

      REQUIRE(succeeded.size() == 4);
      REQUIRE(succeeded[0] == true);
      REQUIRE(succeeded[1] == true);
      REQUIRE(succeeded[2] == true);
      REQUIRE(succeeded[3] == true);

      // FIXME/TODO: check that the account balance of accAA is the expected one.

      // ***********************************
      // FIXME/TODO: expand this test:
      // ***********************************

      // For the next test, we will go even deeper and pretend we are the ABCI, and call
      // Blockchain::buildBlockProposal directly with a bunch of weird transactions.
      // This is easier than torturing CometBFT to stuff "bad" transactions in the mempool
      // (which would require us to actually bypass checks that are in the Blockchain class)
      // Afterwards, we manually check for what the block builder did.

      // Build the transactions

      // Fake CheckTx calls (not really necessary, but mimics what would happen more closely)


      // Fake the CometBlock that is the pre-proposal (just stuff all the txs there)

      // Call buildBlockProposal

      // Check the returned (filtered) proposal, check that it is what we expect

      // Fake the ABCI block and send it to processBlock() for good measure.


      // Check that the transactions picked by the block builder each have the expected outcome.


      // ADD MORE TESTS
      // - REPEAT NONCE, PICK UP THE GREATEST **FEE** NOT COST
      // - NONCE IN PAST
      // - NONCES IN FUTURE (WITH A HOLE)
      // - HOLE BECAUSE TX TOO EXPENSIVE IN THE MIDDLE
      // FIXME TODO


      // next block:

      // tx A --> AA nonce 4 20 token , but small gas limit
      // tx A --> AA nonce 4 10 token , but large gas limit
      // verify only second tx is included (no repeat nonce in block) and balance of AA is now 50 token

      // next block:

      // tx A --> AA nonce 3 10 token
      // tx A --> AA nonce 5 10 token
      // tx A --> AA nonce 6 10 token
      // tx A --> AA nonce 8 10 token
      // tx A --> AA nonce 9 10 token
      // verify only nonce 5 and nonce 6 txs go in the block, the other txs are dropped eventually
      // we need to be able to lie to CometBFT and get txs injected in the mempool directly
      // call comet() send transaction with bad nonces 3, 8, 9, it will go through because it's
      //  not going via the BDK RPC which does check the TxBlock

      //blockchain.stop();
    }

    /*
      // Reference for HTTP/RPC request code
      // FIXME/TODO: write tests with something like this and remove


      PrivKey privKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
      Address me = Secp256k1::toAddress(Secp256k1::toUPub(privKey));
      Address targetOfTransactions = Address(Utils::randBytes(20));
      uint256_t targetBalance = 0;
      uint256_t myBalance("1000000000000000000000");

        (...)

        uint64_t blocks = 0;
        while (blocks < 10) {
          TxBlock tx(
              targetOfTransactions,
              me,
              Bytes(),
              8080,
              blockchainValidator1->getState()->getNativeNonce(me),
              1000000000000000000,
              21000,
              1000000000,
              1000000000,
              privKey
          );

          (...)

          myBalance -= tx.getValue() + (tx.getMaxFeePerGas() * tx.getGasLimit());
          targetBalance += tx.getValue();
          /// Send the transactions through HTTP
          auto sendRawTxJson = json({
                                    {"jsonrpc", "2.0"},
                                    {"id", 1},
                                    {"method", "eth_sendRawTransaction"},
                                    {"params", json::array({Hex::fromBytes(tx.rlpSerialize(), true).forRPC()})}});

          /// Send the transaction to the first validator.
          auto sendRawTxResponse = json::parse(makeHTTPRequest(sendRawTxJson.dump(),
                                               "127.0.0.1",
                                               std::to_string(8101),
                                               "/",
                                               "POST",
                                               "application/json"));


          REQUIRE(sendRawTxResponse["result"].get<std::string>() == tx.hash().hex(true).get());

  */
  }
}

