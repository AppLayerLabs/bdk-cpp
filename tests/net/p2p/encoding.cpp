/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"

#include "../../src/net/p2p/encoding.cpp" // .cpp has extra functions that .h doesn't have (???)
#include "../../src/utils/strconv.h"

namespace TP2PEncoding {
  TEST_CASE("P2P Encoding (Helpers)", "[p2p][encoding]") {
    SECTION("nodesToMessage <-> nodesFromMessage") {
      Bytes msg;
      P2P::NodeID node1{boost::asio::ip::address_v4::from_string("127.0.0.1"), 8000};
      P2P::NodeID node2{boost::asio::ip::address_v6::from_string("::1"), 8001};
      const boost::unordered_flat_map<P2P::NodeID, P2P::NodeType, SafeHash> nodes = {
        {node1, P2P::NodeType::NORMAL_NODE}, {node2, P2P::NodeType::DISCOVERY_NODE}
      };

      P2P::nodesToMessage(msg, nodes);
      REQUIRE(Hex::fromBytes(msg).get() == "00007f0000011f400101000000000000000000000000000000011f41");
      boost::unordered_flat_map<P2P::NodeID, P2P::NodeType, SafeHash> conv = P2P::nodesFromMessage(msg);
      REQUIRE(conv.contains(node1));
      REQUIRE(conv[node1] == P2P::NodeType::NORMAL_NODE);
      REQUIRE(conv.contains(node2));
      REQUIRE(conv[node2] == P2P::NodeType::DISCOVERY_NODE);

      // Throws for coverage
      Bytes msgSmall{0x00};
      Bytes msgSmallV4{0x00, 0x00, 0x7f, 0x00}; // missing 0001
      Bytes msgSmallV6{0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // missing 0000000000000001
      Bytes msgInvalidIp{0x00, 0x02, 0x7f, 0x00, 0x00, 0x01}; // 02 is not 00 (v4) or 01 (v6)
      Bytes msgNoPort{0x00, 0x00, 0x7f, 0x00, 0x00, 0x01}; // missing 14f0 (8000)
      REQUIRE_THROWS(P2P::nodesFromMessage(msgSmall));
      REQUIRE_THROWS(P2P::nodesFromMessage(msgSmallV4));
      REQUIRE_THROWS(P2P::nodesFromMessage(msgSmallV6));
      REQUIRE_THROWS(P2P::nodesFromMessage(msgInvalidIp));
      REQUIRE_THROWS(P2P::nodesFromMessage(msgNoPort));
    }

    SECTION("nodeInfoToMessage <-> nodeInfoFromMessage") {
      Bytes msg;
      P2P::NodeID node1{boost::asio::ip::address_v4::from_string("127.0.0.1"), 8000};
      P2P::NodeID node2{boost::asio::ip::address_v6::from_string("::1"), 8001};
      const boost::unordered_flat_map<P2P::NodeID, P2P::NodeType, SafeHash> nodes = {
        {node1, P2P::NodeType::NORMAL_NODE}, {node2, P2P::NodeType::DISCOVERY_NODE}
      };
      Options opts = Options::binaryDefaultOptions("options.json");

      // Block w/ no txs, copied from block.cpp
      PrivKey validatorPrivKey(Hex::toBytes("0x4d5db4107d237df6a3d58ee5f70ae63d73d765d8a1214214d8a13340d0f2750d"));
      Hash nPrevBlockHash(Hex::toBytes("22143e16db549af9ccfd3b746ea4a74421847fa0fe7e0e278626a4e7307ac0f6"));
      uint64_t timestamp = 1678400201859;
      uint64_t nHeight = 92137812;
      FinalizedBlock newBlock = FinalizedBlock::createNewValidBlock({},{}, nPrevBlockHash, timestamp, nHeight, validatorPrivKey);
      std::shared_ptr<const FinalizedBlock> latestBlock = std::make_shared<const FinalizedBlock>(newBlock);

      P2P::nodeInfoToMessage(msg, latestBlock, nodes, opts);
      Hex msgNodeVersion = Hex::fromBytes(Utils::create_view_span(msg).subspan(0, 8));
      Hex hexNodeVersion = Hex::fromBytes(UintConv::uint64ToBytes(opts.getVersion()));
      Hex msgNHeight = Hex::fromBytes(Utils::create_view_span(msg).subspan(16, 8));
      Hex hexNHeight = Hex::fromBytes(UintConv::uint64ToBytes(nHeight));
      Hex msgHash = Hex::fromBytes(Utils::create_view_span(msg).subspan(24, 32));
      Hex hexHash = Hex::fromBytes(latestBlock->getHash().asBytes());
      Hex msgPeers = Hex::fromBytes(Utils::create_view_span(msg).subspan(56));
      Bytes bytesPeers;
      P2P::nodesToMessage(bytesPeers, nodes);
      Hex hexPeers = Hex::fromBytes(bytesPeers);
      REQUIRE(msgNodeVersion.get() == hexNodeVersion.get());
      REQUIRE(msgNHeight.get() == hexNHeight.get());
      REQUIRE(msgHash.get() == hexHash.get());
      REQUIRE(msgPeers.get() == hexPeers.get());

      P2P::NodeInfo conv = P2P::nodeInfoFromMessage(msg);
      REQUIRE(conv.nodeVersion() == opts.getVersion());
      REQUIRE(conv.latestBlockHeight() == latestBlock->getNHeight());
      REQUIRE(conv.latestBlockHash() == latestBlock->getHash());
      REQUIRE(conv.peers().size() == 2);
      REQUIRE(conv.peers()[0] == node1);
      REQUIRE(conv.peers()[1] == node2);
    }

    SECTION("blocksToMessage <-> blocksFromMessage") {
      // Data copied from block.cpp, five equal txs for three "different" blocks
      Bytes msg;
      PrivKey validatorPrivKey(Hex::toBytes("0x4d5db4107d237df6a3d58ee5f70ae63d73d765d8a1214214d8a13340d0f2750d"));
      Hash nPrevBlockHash(Hex::toBytes("97a5ebd9bbb5e330b0b3c74b9816d595ffb7a04d4a29fb117ea93f8a333b43be"));
      uint64_t timestamp = 1678400843316;
      uint64_t nHeight = 100;
      TxBlock tx(Hex::toBytes("0x02f874821f9080849502f900849502f900825208942e951aa58c8b9b504a97f597bbb2765c011a8802880de0b6b3a764000080c001a0f56fe87778b4420d3b0f8eba91d28093abfdbea281a188b8516dd8411dc223d7a05c2d2d71ad3473571ff637907d72e6ac399fe4804641dbd9e2d863586c57717d"), 1);
      std::vector<TxBlock> txs1;
      std::vector<TxBlock> txs2;
      std::vector<TxBlock> txs3;
      for (uint64_t i = 0; i < 5; i++) {
        txs1.emplace_back(tx);
        txs2.emplace_back(tx);
        txs3.emplace_back(tx);
      }
      FinalizedBlock block1 = FinalizedBlock::createNewValidBlock(std::move(txs1), {}, nPrevBlockHash, timestamp, nHeight, validatorPrivKey);
      FinalizedBlock block2 = FinalizedBlock::createNewValidBlock(std::move(txs1), {}, nPrevBlockHash, timestamp, nHeight, validatorPrivKey);
      FinalizedBlock block3 = FinalizedBlock::createNewValidBlock(std::move(txs1), {}, nPrevBlockHash, timestamp, nHeight, validatorPrivKey);
      std::vector<std::shared_ptr<const FinalizedBlock>> blocks = {
        std::make_shared<const FinalizedBlock>(block1),
        std::make_shared<const FinalizedBlock>(block2),
        std::make_shared<const FinalizedBlock>(block3)
      };
      P2P::blocksToMessage(msg, blocks);
      std::vector<FinalizedBlock> conv = P2P::blocksFromMessage(Utils::create_view_span(msg), 0);

      // For coverage purposes only, don't care enough to test this through
      Bytes dataSmall = Hex::toBytes("0x02f87482");
      Bytes blockSmall = Hex::toBytes("0x02f874821f9080849502f900849502f9");
      REQUIRE_THROWS(P2P::blocksFromMessage(dataSmall, 0));
      REQUIRE_THROWS(P2P::blocksFromMessage(blockSmall, 0));
    }

    SECTION("txsToMessage <-> txsFromMessage") {
      // 3 simple transactions copied from block.cpp
      Bytes msg1;
      Bytes msg2;
      Bytes rawTx1 = Hex::toBytes("0x02f87301808405f5e100850b5b977f998252089495944f9d42e181d76bb2c7e428410533aa3fed4a88012386f1806fe51080c080a0102fc0316ef07a9be233a270cdeb692e1666710bbdb8be67bf7d896fa96c6bafa038b6cbfdeb433911da6958a9dd3ac24d4ff39f11d1b985efca6d6d79a96a62ce");
      Bytes rawTx2 = Hex::toBytes("0x02f87501820cfd8405f5e100850b67b98b6b8252089480c67432656d59144ceff962e8faf8926599bcf8880de27d72f9c7632e80c001a057180e5af9ecbac905b17a45b56f1d93626190f1ec8df6a4a8cbbf7c0b8704a9a0166f15ae0e192835e1bf405b82c9faaaf9c8918a9d702441daa5168514377a17");
      Bytes rawTx3 = Hex::toBytes("0x02f87501826e5c8402faf0808510b3a67cc282520894eb38eab2a8d5f448d7d47005b64697e159aa284e88078cbf1fc56d4f1080c080a0763458eaffb9745026fc6360443e7ff8d171824d0410d48fdf06c08c7d4a8306a031b3d8f1753acc4239ffe0584536f12095651f72a61c684ef221aaa97a315328");
      TxBlock tx1(rawTx1, 1);
      TxBlock tx2(rawTx2, 1);
      TxBlock tx3(rawTx3, 1);
      const boost::unordered_flat_map<Hash, TxBlock, SafeHash> txs = {
        {tx1.hash(), tx1}, {tx2.hash(), tx2}, {tx3.hash(), tx3}
      };
      std::vector<TxBlock> txsVec = {tx1, tx2, tx3};

      P2P::txsToMessage(msg1, txs);
      REQUIRE(Hex::fromBytes(Utils::create_view_span(msg1).subspan(0, 4)).get() == Hex::fromBytes(Bytes{0x00, 0x00, 0x00, 0x76}).get());
      REQUIRE(Hex::fromBytes(Utils::create_view_span(msg1).subspan(4, 118)).get() == Hex::fromBytes(rawTx1).get());
      REQUIRE(Hex::fromBytes(Utils::create_view_span(msg1).subspan(122, 4)).get() == Hex::fromBytes(Bytes{0x00, 0x00, 0x00, 0x78}).get());
      REQUIRE(Hex::fromBytes(Utils::create_view_span(msg1).subspan(126, 120)).get() == Hex::fromBytes(rawTx2).get());
      REQUIRE(Hex::fromBytes(Utils::create_view_span(msg1).subspan(246, 4)).get() == Hex::fromBytes(Bytes{0x00, 0x00, 0x00, 0x78}).get());
      REQUIRE(Hex::fromBytes(Utils::create_view_span(msg1).subspan(250, 120)).get() == Hex::fromBytes(rawTx3).get());

      P2P::txsToMessage(msg2, txsVec);
      REQUIRE(Hex::fromBytes(Utils::create_view_span(msg2).subspan(0, 4)).get() == Hex::fromBytes(Bytes{0x00, 0x00, 0x00, 0x76}).get());
      REQUIRE(Hex::fromBytes(Utils::create_view_span(msg2).subspan(4, 118)).get() == Hex::fromBytes(rawTx1).get());
      REQUIRE(Hex::fromBytes(Utils::create_view_span(msg2).subspan(122, 4)).get() == Hex::fromBytes(Bytes{0x00, 0x00, 0x00, 0x78}).get());
      REQUIRE(Hex::fromBytes(Utils::create_view_span(msg2).subspan(126, 120)).get() == Hex::fromBytes(rawTx2).get());
      REQUIRE(Hex::fromBytes(Utils::create_view_span(msg2).subspan(246, 4)).get() == Hex::fromBytes(Bytes{0x00, 0x00, 0x00, 0x78}).get());
      REQUIRE(Hex::fromBytes(Utils::create_view_span(msg2).subspan(250, 120)).get() == Hex::fromBytes(rawTx3).get());

      std::vector<TxBlock> conv = P2P::txsFromMessage<TxBlock>(Utils::create_view_span(msg1), uint64_t(1));
      REQUIRE(txs.find(conv[0].hash()) != txs.end());
      REQUIRE(txs.find(conv[1].hash()) != txs.end());
      REQUIRE(txs.find(conv[2].hash()) != txs.end());

      // For coverage
      Bytes dataSmall = Hex::toBytes("0x0000");
      Bytes txSmall = Hex::toBytes("0x0000007602f87301");
      REQUIRE_THROWS(P2P::txsFromMessage<TxBlock>(Utils::create_view_span(dataSmall), uint64_t(1)));
      REQUIRE_THROWS(P2P::txsFromMessage<TxBlock>(Utils::create_view_span(txSmall), uint64_t(1)));
    }
  }

  TEST_CASE("P2P Encoding (Miscellaneous)", "[p2p][encoding]") {
    SECTION("RequestID Conversions") {
      P2P::RequestID id(uint64_t(793228721938748925));
      REQUIRE(id.hex().get() == "0b021d51e4cbd5fd");
      REQUIRE(id.toUint64() == uint64_t(793228721938748925));
      REQUIRE(P2P::RequestID::random().size() == 8);
    }

    SECTION("getCommandType") {
      Bytes cmd{0x00, 0x00}; // Ping (0x0000)
      Bytes cmdSize{0x00};
      Bytes cmdInvalid{0xFF, 0xFF};
      REQUIRE(P2P::getCommandType(Utils::create_view_span(cmd)) == P2P::CommandType::Ping);
      REQUIRE_THROWS(P2P::getCommandType(Utils::create_view_span(cmdSize)));
      REQUIRE_THROWS(P2P::getCommandType(Utils::create_view_span(cmdInvalid)));
    }

    SECTION("getRequestType") {
      Bytes req{0x00}; // Requesting
      Bytes reqSize{0x00, 0x00};
      Bytes reqInvalid{0xFF};
      REQUIRE(P2P::getRequestType(Utils::create_view_span(req)) == P2P::RequestType::Requesting);
      REQUIRE_THROWS(P2P::getRequestType(Utils::create_view_span(reqSize)));
      REQUIRE_THROWS(P2P::getRequestType(Utils::create_view_span(reqInvalid)));
    }

    SECTION("NodeID operator<") {
      P2P::NodeID node1{boost::asio::ip::address_v4::from_string("127.0.0.1"), 8000};
      P2P::NodeID node2{boost::asio::ip::address_v4::from_string("127.0.0.2"), 8000};
      P2P::NodeID node3{boost::asio::ip::address_v4::from_string("127.0.0.2"), 8001};
      P2P::NodeID node4{boost::asio::ip::address_v4::from_string("127.0.0.2"), 8001};
      REQUIRE(node1 < node2);
      REQUIRE(node2 < node3);
      REQUIRE_FALSE(node3 < node4);
    }
  }

  TEST_CASE("P2P Encoding (Requests)", "[p2p][encoding]") {
    SECTION("Encode/Decode ping Request") {
      P2P::Message msg = P2P::RequestEncoder::ping();
      bool req = P2P::RequestDecoder::ping(msg);
      REQUIRE(req == true);
      P2P::Message msg2 = P2P::AnswerEncoder::ping(msg);
      bool req2 = P2P::AnswerDecoder::ping(msg2);
      REQUIRE(req2 == true);

      // For coverage
      P2P::Message msgWrongSize(Utils::randBytes(12)); // msg is 11 bytes
      P2P::Message msgWrongCmd(Utils::makeBytes(bytes::join(
        P2P::getRequestTypePrefix(P2P::RequestType::Requesting),
        Utils::randBytes(8),
        P2P::getCommandPrefix(P2P::CommandType::Info)
      )));
      P2P::Message msg2WrongType(Utils::makeBytes(bytes::join(
        P2P::getRequestTypePrefix(P2P::RequestType::Requesting),
        Utils::randBytes(8),
        P2P::getCommandPrefix(P2P::CommandType::Ping)
      )));
      P2P::Message msg2WrongCmd(Utils::makeBytes(bytes::join(
        P2P::getRequestTypePrefix(P2P::RequestType::Answering),
        Utils::randBytes(8),
        P2P::getCommandPrefix(P2P::CommandType::Info)
      )));
      REQUIRE_FALSE(P2P::RequestDecoder::ping(msgWrongSize));
      REQUIRE_FALSE(P2P::RequestDecoder::ping(msgWrongCmd));
      REQUIRE_FALSE(P2P::AnswerDecoder::ping(msgWrongSize));
      REQUIRE_FALSE(P2P::AnswerDecoder::ping(msg2WrongType));
      REQUIRE_FALSE(P2P::AnswerDecoder::ping(msg2WrongCmd));
    }

    SECTION("Encode/Decode info Request") {
      PrivKey genesisSigner(Hex::toBytes("0x4d48bdf34d65ef2bed2e4ee9020a7d3162b494ac31d3088153425f286f3d3c8c"));
      FinalizedBlock genesis = FinalizedBlock::createNewValidBlock({},{}, Hash(), 1656356646000000, 0, genesisSigner);
      P2P::NodeID node1{boost::asio::ip::address_v4::from_string("127.0.0.1"), 8000};
      P2P::NodeID node2{boost::asio::ip::address_v6::from_string("::1"), 8001};
      const boost::unordered_flat_map<P2P::NodeID, P2P::NodeType, SafeHash> nodes = {
        {node1, P2P::NodeType::NORMAL_NODE}, {node2, P2P::NodeType::DISCOVERY_NODE}
      };
      Options opts = Options::binaryDefaultOptions("options.json");

      P2P::Message msg = P2P::RequestEncoder::info(
        std::make_shared<FinalizedBlock>(genesis), nodes, opts
      );
      P2P::NodeInfo req = P2P::RequestDecoder::info(msg);
      REQUIRE(req.latestBlockHash() == genesis.getHash());
      std::vector<P2P::NodeID> peers = req.peers();
      for (P2P::NodeID peer : peers) REQUIRE(nodes.contains(peer));

      P2P::Message msg2 = P2P::AnswerEncoder::info(
        msg, std::make_shared<FinalizedBlock>(genesis), nodes, opts
      );
      P2P::NodeInfo req2 = P2P::AnswerDecoder::info(msg2);
      REQUIRE(req2.latestBlockHash() == genesis.getHash());
      std::vector<P2P::NodeID> peers2 = req2.peers();
      for (P2P::NodeID peer : peers2) REQUIRE(nodes.contains(peer));

      // For coverage
      P2P::Message msgWrongCmd(Utils::makeBytes(bytes::join(
        P2P::getRequestTypePrefix(P2P::RequestType::Requesting),
        Utils::randBytes(8),
        P2P::getCommandPrefix(P2P::CommandType::Ping)
      )));
      P2P::Message msg2WrongType(Utils::makeBytes(bytes::join(
        P2P::getRequestTypePrefix(P2P::RequestType::Requesting),
        Utils::randBytes(8),
        P2P::getCommandPrefix(P2P::CommandType::Info)
      )));
      P2P::Message msg2WrongCmd(Utils::makeBytes(bytes::join(
        P2P::getRequestTypePrefix(P2P::RequestType::Answering),
        Utils::randBytes(8),
        P2P::getCommandPrefix(P2P::CommandType::Ping)
      )));
      REQUIRE_THROWS(P2P::RequestDecoder::info(msgWrongCmd));
      REQUIRE_THROWS(P2P::AnswerDecoder::info(msg2WrongType));
      REQUIRE_THROWS(P2P::AnswerDecoder::info(msg2WrongCmd));
    }

    SECTION("Encode/Decode requestNodes Request") {
      P2P::Message msg = P2P::RequestEncoder::requestNodes();
      bool req = P2P::RequestDecoder::requestNodes(msg);
      REQUIRE(req == true);

      P2P::NodeID node1{boost::asio::ip::address_v4::from_string("127.0.0.1"), 8000};
      P2P::NodeID node2{boost::asio::ip::address_v6::from_string("::1"), 8001};
      const boost::unordered_flat_map<P2P::NodeID, P2P::NodeType, SafeHash> nodes = {
        {node1, P2P::NodeType::NORMAL_NODE}, {node2, P2P::NodeType::DISCOVERY_NODE}
      };
      P2P::Message msg2 = P2P::AnswerEncoder::requestNodes(msg, nodes);
      boost::unordered_flat_map<P2P::NodeID, P2P::NodeType, SafeHash> req2 = P2P::AnswerDecoder::requestNodes(msg2);
      for (auto peer : req2) REQUIRE(nodes.contains(peer.first));

      // For coverage
      P2P::Message msgWrongSize(Utils::randBytes(12)); // msg is 11 bytes
      P2P::Message msgWrongCmd(Utils::makeBytes(bytes::join(
        P2P::getRequestTypePrefix(P2P::RequestType::Requesting),
        Utils::randBytes(8),
        P2P::getCommandPrefix(P2P::CommandType::Ping)
      )));
      P2P::Message msg2WrongType(Utils::makeBytes(bytes::join(
        P2P::getRequestTypePrefix(P2P::RequestType::Requesting),
        Utils::randBytes(8),
        P2P::getCommandPrefix(P2P::CommandType::RequestNodes)
      )));
      P2P::Message msg2WrongCmd(Utils::makeBytes(bytes::join(
        P2P::getRequestTypePrefix(P2P::RequestType::Answering),
        Utils::randBytes(8),
        P2P::getCommandPrefix(P2P::CommandType::Ping)
      )));
      REQUIRE_FALSE(P2P::RequestDecoder::requestNodes(msgWrongSize));
      REQUIRE_FALSE(P2P::RequestDecoder::requestNodes(msgWrongCmd));
      REQUIRE_THROWS(P2P::AnswerDecoder::requestNodes(msg2WrongType));
      REQUIRE_THROWS(P2P::AnswerDecoder::requestNodes(msg2WrongCmd));
    }

    SECTION("Encode/Decode requestValidatorTxs Request") {
      P2P::Message msg = P2P::RequestEncoder::requestValidatorTxs();
      bool req = P2P::RequestDecoder::requestValidatorTxs(msg);
      REQUIRE(req == true);

      TxValidator tx1(Hex::toBytes("f845808026a08a4591f48d6307bb4cb8a0b0088b544d923d00bc1f264c3fdf16f946fdee0b34a077a6f6e8b3e78b45478827604f070d03060f413d823eae7fab9b139be7a41d81"), 1);
      TxValidator tx2(Hex::toBytes("f86aa03051b7f769aaabd4ebb8ff991888c2891ef1d7b84cee2b44bb8274e8ed3687ff83139705820fa1a0609914de300a418c4ec1ef176efc1fbb64d73a0de0b7eee4fd31f11627e36412a01ed62c45f76f1ff05b4856ab7a6730c02bddfc891dd1fdd6b82469d41a07aaf9"), 1983);
      TxValidator tx3(Hex::toBytes("f86599bf2ac52475bb963dcc43d6f10349a55e1f30899461e358e0ad850248e27503824e22a0b7c4f669b54cb0fb656f5e0c26b27405ca20a90e391222b8a8277d760273b0ffa01a27ef8cb8824267dae1884df1e31d440dacd995b7da027012798e2bd14c2ae9"), 9983);
      const boost::unordered_flat_map<Hash, TxValidator, SafeHash> txs = {
        {tx1.hash(), tx1}, {tx2.hash(), tx2}, {tx3.hash(), tx3}
      };
      P2P::Message msg2 = P2P::AnswerEncoder::requestValidatorTxs(msg, txs);
      std::vector<TxValidator> req2 = P2P::AnswerDecoder::requestValidatorTxs(msg2, 1);
      for (TxValidator tx : req2) REQUIRE((txs.contains(tx.hash()) && txs.at(tx.hash()) == tx));

      // For coverage
      P2P::Message msgWrongSize(Utils::randBytes(12)); // msg is 11 bytes
      P2P::Message msgWrongCmd(Utils::makeBytes(bytes::join(
        P2P::getRequestTypePrefix(P2P::RequestType::Requesting),
        Utils::randBytes(8),
        P2P::getCommandPrefix(P2P::CommandType::Ping)
      )));
      P2P::Message msg2WrongType(Utils::makeBytes(bytes::join(
        P2P::getRequestTypePrefix(P2P::RequestType::Requesting),
        Utils::randBytes(8),
        P2P::getCommandPrefix(P2P::CommandType::RequestValidatorTxs)
      )));
      P2P::Message msg2WrongCmd(Utils::makeBytes(bytes::join(
        P2P::getRequestTypePrefix(P2P::RequestType::Answering),
        Utils::randBytes(8),
        P2P::getCommandPrefix(P2P::CommandType::Ping)
      )));
      REQUIRE_FALSE(P2P::RequestDecoder::requestValidatorTxs(msgWrongSize));
      REQUIRE_FALSE(P2P::RequestDecoder::requestValidatorTxs(msgWrongCmd));
      REQUIRE_THROWS(P2P::AnswerDecoder::requestValidatorTxs(msg2WrongType, 1));
      REQUIRE_THROWS(P2P::AnswerDecoder::requestValidatorTxs(msg2WrongCmd, 1));
    }

    SECTION("Encode/Decode requestTxs Request") {
      P2P::Message msg = P2P::RequestEncoder::requestTxs();
      bool req = P2P::RequestDecoder::requestTxs(msg);
      REQUIRE(req == true);

      TxBlock tx1(Hex::toBytes("0x02f87501826e5c8402faf0808510b3a67cc282520894eb38eab2a8d5f448d7d47005b64697e159aa284e88078cbf1fc56d4f1080c080a0763458eaffb9745026fc6360443e7ff8d171824d0410d48fdf06c08c7d4a8306a031b3d8f1753acc4239ffe0584536f12095651f72a61c684ef221aaa97a315328"), 1);
      TxBlock tx2(Hex::toBytes("0x02f87301808405f5e100850b5b977f998252089495944f9d42e181d76bb2c7e428410533aa3fed4a88012386f1806fe51080c080a0102fc0316ef07a9be233a270cdeb692e1666710bbdb8be67bf7d896fa96c6bafa038b6cbfdeb433911da6958a9dd3ac24d4ff39f11d1b985efca6d6d79a96a62ce"), 1);
      TxBlock tx3(Hex::toBytes("0x02f87501820cfd8405f5e100850b67b98b6b8252089480c67432656d59144ceff962e8faf8926599bcf8880de27d72f9c7632e80c001a057180e5af9ecbac905b17a45b56f1d93626190f1ec8df6a4a8cbbf7c0b8704a9a0166f15ae0e192835e1bf405b82c9faaaf9c8918a9d702441daa5168514377a17"), 1);
      const std::vector<TxBlock> txs = {tx1, tx2, tx3};
      P2P::Message msg2 = P2P::AnswerEncoder::requestTxs(msg, txs);
      std::vector<TxBlock> req2 = P2P::AnswerDecoder::requestTxs(msg2, 1);
      REQUIRE(req2 == txs);

      // For coverage
      P2P::Message msgWrongSize(Utils::randBytes(12)); // msg is 11 bytes
      P2P::Message msgWrongCmd(Utils::makeBytes(bytes::join(
        P2P::getRequestTypePrefix(P2P::RequestType::Requesting),
        Utils::randBytes(8),
        P2P::getCommandPrefix(P2P::CommandType::Ping)
      )));
      P2P::Message msg2WrongType(Utils::makeBytes(bytes::join(
        P2P::getRequestTypePrefix(P2P::RequestType::Requesting),
        Utils::randBytes(8),
        P2P::getCommandPrefix(P2P::CommandType::RequestTxs)
      )));
      P2P::Message msg2WrongCmd(Utils::makeBytes(bytes::join(
        P2P::getRequestTypePrefix(P2P::RequestType::Answering),
        Utils::randBytes(8),
        P2P::getCommandPrefix(P2P::CommandType::Ping)
      )));
      REQUIRE_FALSE(P2P::RequestDecoder::requestTxs(msgWrongSize));
      REQUIRE_FALSE(P2P::RequestDecoder::requestTxs(msgWrongCmd));
      REQUIRE_THROWS(P2P::AnswerDecoder::requestTxs(msg2WrongType, 1));
      REQUIRE_THROWS(P2P::AnswerDecoder::requestTxs(msg2WrongCmd, 1));
    }

    SECTION("Encode/Decode requestBlock Request") {
      P2P::Message msg = P2P::RequestEncoder::requestBlock(10, 100, 1000);
      uint64_t height = 0;
      uint64_t heightEnd = 0;
      uint64_t bytesLimit = 0;
      P2P::RequestDecoder::requestBlock(msg, height, heightEnd, bytesLimit);
      REQUIRE(height == 10);
      REQUIRE(heightEnd == 100);
      REQUIRE(bytesLimit == 1000);

      PrivKey validatorPrivKey(Hex::toBytes("0x4d5db4107d237df6a3d58ee5f70ae63d73d765d8a1214214d8a13340d0f2750d"));
      Hash nPrevBlockHash(Hex::toBytes("97a5ebd9bbb5e330b0b3c74b9816d595ffb7a04d4a29fb117ea93f8a333b43be"));
      uint64_t timestamp = 1678400843316;
      uint64_t nHeight = 100;
      TxBlock tx(Hex::toBytes("0x02f874821f9080849502f900849502f900825208942e951aa58c8b9b504a97f597bbb2765c011a8802880de0b6b3a764000080c001a0f56fe87778b4420d3b0f8eba91d28093abfdbea281a188b8516dd8411dc223d7a05c2d2d71ad3473571ff637907d72e6ac399fe4804641dbd9e2d863586c57717d"), 1);
      std::vector<TxBlock> txs;
      for (uint64_t i = 0; i < 10; i++) txs.emplace_back(tx);
      FinalizedBlock finalizedNewBlock = FinalizedBlock::createNewValidBlock(std::move(txs), {}, nPrevBlockHash, timestamp, nHeight, validatorPrivKey);
      const std::vector<std::shared_ptr<const FinalizedBlock>> blocks = {
        std::make_shared<const FinalizedBlock>(finalizedNewBlock)
      };
      P2P::Message msg2 = P2P::AnswerEncoder::requestBlock(msg, blocks);
      std::vector<FinalizedBlock> req2 = P2P::AnswerDecoder::requestBlock(msg2, 1);
      REQUIRE(req2[0] == finalizedNewBlock);

      // For coverage
      P2P::Message msgWrongSize(Utils::randBytes(40)); // msg is 35 bytes
      P2P::Message msgWrongCmd(Utils::makeBytes(bytes::join(
        P2P::getRequestTypePrefix(P2P::RequestType::Requesting),
        Utils::randBytes(8),
        P2P::getCommandPrefix(P2P::CommandType::Ping),
        UintConv::uint64ToBytes(height),
        UintConv::uint64ToBytes(heightEnd),
        UintConv::uint64ToBytes(bytesLimit)
      )));
      P2P::Message msg2WrongType(Utils::makeBytes(bytes::join(
        P2P::getRequestTypePrefix(P2P::RequestType::Requesting),
        Utils::randBytes(8),
        P2P::getCommandPrefix(P2P::CommandType::RequestBlock),
        UintConv::uint64ToBytes(height),
        UintConv::uint64ToBytes(heightEnd),
        UintConv::uint64ToBytes(bytesLimit)
      )));
      P2P::Message msg2WrongCmd(Utils::makeBytes(bytes::join(
        P2P::getRequestTypePrefix(P2P::RequestType::Answering),
        Utils::randBytes(8),
        P2P::getCommandPrefix(P2P::CommandType::Ping),
        UintConv::uint64ToBytes(height),
        UintConv::uint64ToBytes(heightEnd),
        UintConv::uint64ToBytes(bytesLimit)
      )));
      REQUIRE_THROWS(P2P::RequestDecoder::requestBlock(msgWrongSize, height, heightEnd, bytesLimit));
      REQUIRE_THROWS(P2P::RequestDecoder::requestBlock(msgWrongCmd, height, heightEnd, bytesLimit));
      REQUIRE_THROWS(P2P::AnswerDecoder::requestBlock(msg2WrongType, 1));
      REQUIRE_THROWS(P2P::AnswerDecoder::requestBlock(msg2WrongCmd, 1));
    }

    SECTION("Encode/Decode broadcastValidatorTx Request") {
      TxValidator tx(Hex::toBytes("f845808026a08a4591f48d6307bb4cb8a0b0088b544d923d00bc1f264c3fdf16f946fdee0b34a077a6f6e8b3e78b45478827604f070d03060f413d823eae7fab9b139be7a41d81"), 1);
      P2P::Message msg = P2P::BroadcastEncoder::broadcastValidatorTx(tx);
      TxValidator req = P2P::BroadcastDecoder::broadcastValidatorTx(msg, 1);
      REQUIRE(req == tx);

      // For coverage
      const Bytes& txSer = tx.rlpSerialize();
      P2P::Message msgWrongType(Utils::makeBytes(bytes::join(
        P2P::getRequestTypePrefix(P2P::RequestType::Requesting),
        UintConv::uint64ToBytes(FNVHash()(txSer)),
        P2P::getCommandPrefix(P2P::CommandType::BroadcastValidatorTx),
        txSer
      )));
      P2P::Message msgWrongId(Utils::makeBytes(bytes::join(
        P2P::getRequestTypePrefix(P2P::RequestType::Broadcasting),
        Utils::randBytes(8),
        P2P::getCommandPrefix(P2P::CommandType::BroadcastValidatorTx),
        txSer
      )));
      P2P::Message msgWrongCmd(Utils::makeBytes(bytes::join(
        P2P::getRequestTypePrefix(P2P::RequestType::Broadcasting),
        UintConv::uint64ToBytes(FNVHash()(txSer)),
        P2P::getCommandPrefix(P2P::CommandType::Ping),
        txSer
      )));
      REQUIRE_THROWS(P2P::BroadcastDecoder::broadcastValidatorTx(msgWrongType, 1));
      REQUIRE_THROWS(P2P::BroadcastDecoder::broadcastValidatorTx(msgWrongId, 1));
      REQUIRE_THROWS(P2P::BroadcastDecoder::broadcastValidatorTx(msgWrongCmd, 1));
    }

    SECTION("Encode/Decode broadcastTx Request") {
      TxBlock tx(Hex::toBytes("0x02f87501826e5c8402faf0808510b3a67cc282520894eb38eab2a8d5f448d7d47005b64697e159aa284e88078cbf1fc56d4f1080c080a0763458eaffb9745026fc6360443e7ff8d171824d0410d48fdf06c08c7d4a8306a031b3d8f1753acc4239ffe0584536f12095651f72a61c684ef221aaa97a315328"), 1);
      P2P::Message msg = P2P::BroadcastEncoder::broadcastTx(tx);
      TxBlock req = P2P::BroadcastDecoder::broadcastTx(msg, 1);
      REQUIRE(req == tx);

      // For coverage
      const Bytes& txSer = tx.rlpSerialize();
      P2P::Message msgWrongType(Utils::makeBytes(bytes::join(
        P2P::getRequestTypePrefix(P2P::RequestType::Requesting),
        UintConv::uint64ToBytes(FNVHash()(txSer)),
        P2P::getCommandPrefix(P2P::CommandType::BroadcastTx),
        txSer
      )));
      P2P::Message msgWrongId(Utils::makeBytes(bytes::join(
        P2P::getRequestTypePrefix(P2P::RequestType::Broadcasting),
        Utils::randBytes(8),
        P2P::getCommandPrefix(P2P::CommandType::BroadcastTx),
        txSer
      )));
      P2P::Message msgWrongCmd(Utils::makeBytes(bytes::join(
        P2P::getRequestTypePrefix(P2P::RequestType::Broadcasting),
        UintConv::uint64ToBytes(FNVHash()(txSer)),
        P2P::getCommandPrefix(P2P::CommandType::Ping),
        txSer
      )));
      REQUIRE_THROWS(P2P::BroadcastDecoder::broadcastTx(msgWrongType, 1));
      REQUIRE_THROWS(P2P::BroadcastDecoder::broadcastTx(msgWrongId, 1));
      REQUIRE_THROWS(P2P::BroadcastDecoder::broadcastTx(msgWrongCmd, 1));
    }

    SECTION("Encode/Decode broadcastBlock Request") {
      PrivKey validatorPrivKey(Hex::toBytes("0x4d5db4107d237df6a3d58ee5f70ae63d73d765d8a1214214d8a13340d0f2750d"));
      Hash nPrevBlockHash(Hex::toBytes("97a5ebd9bbb5e330b0b3c74b9816d595ffb7a04d4a29fb117ea93f8a333b43be"));
      uint64_t timestamp = 1678400843316;
      uint64_t nHeight = 100;
      TxBlock tx(Hex::toBytes("0x02f874821f9080849502f900849502f900825208942e951aa58c8b9b504a97f597bbb2765c011a8802880de0b6b3a764000080c001a0f56fe87778b4420d3b0f8eba91d28093abfdbea281a188b8516dd8411dc223d7a05c2d2d71ad3473571ff637907d72e6ac399fe4804641dbd9e2d863586c57717d"), 1);
      std::vector<TxBlock> txs;
      for (uint64_t i = 0; i < 10; i++) txs.emplace_back(tx);
      FinalizedBlock finalizedNewBlock = FinalizedBlock::createNewValidBlock(std::move(txs), {}, nPrevBlockHash, timestamp, nHeight, validatorPrivKey);

      P2P::Message msg = P2P::BroadcastEncoder::broadcastBlock(
        std::make_shared<FinalizedBlock>(finalizedNewBlock)
      );
      FinalizedBlock req = P2P::BroadcastDecoder::broadcastBlock(msg, 1);
      REQUIRE(req == finalizedNewBlock);

      // For coverage
      const Bytes& blockSer = finalizedNewBlock.serializeBlock();
      P2P::Message msgWrongType(Utils::makeBytes(bytes::join(
        P2P::getRequestTypePrefix(P2P::RequestType::Requesting),
        UintConv::uint64ToBytes(FNVHash()(blockSer)),
        P2P::getCommandPrefix(P2P::CommandType::BroadcastBlock),
        blockSer
      )));
      P2P::Message msgWrongId(Utils::makeBytes(bytes::join(
        P2P::getRequestTypePrefix(P2P::RequestType::Broadcasting),
        Utils::randBytes(8),
        P2P::getCommandPrefix(P2P::CommandType::BroadcastBlock),
        blockSer
      )));
      P2P::Message msgWrongCmd(Utils::makeBytes(bytes::join(
        P2P::getRequestTypePrefix(P2P::RequestType::Broadcasting),
        UintConv::uint64ToBytes(FNVHash()(blockSer)),
        P2P::getCommandPrefix(P2P::CommandType::Ping),
        blockSer
      )));
      REQUIRE_THROWS(P2P::BroadcastDecoder::broadcastBlock(msgWrongType, 1));
      REQUIRE_THROWS(P2P::BroadcastDecoder::broadcastBlock(msgWrongId, 1));
      REQUIRE_THROWS(P2P::BroadcastDecoder::broadcastBlock(msgWrongCmd, 1));
    }

    SECTION("Encode/Decode notifyInfo Request") {
      PrivKey genesisSigner(Hex::toBytes("0x4d48bdf34d65ef2bed2e4ee9020a7d3162b494ac31d3088153425f286f3d3c8c"));
      FinalizedBlock genesis = FinalizedBlock::createNewValidBlock({},{}, Hash(), 1656356646000000, 0, genesisSigner);
      P2P::NodeID node1{boost::asio::ip::address_v4::from_string("127.0.0.1"), 8000};
      P2P::NodeID node2{boost::asio::ip::address_v6::from_string("::1"), 8001};
      const boost::unordered_flat_map<P2P::NodeID, P2P::NodeType, SafeHash> nodes = {
        {node1, P2P::NodeType::NORMAL_NODE}, {node2, P2P::NodeType::DISCOVERY_NODE}
      };
      Options opts = Options::binaryDefaultOptions("options.json");

      P2P::Message msg = P2P::NotificationEncoder::notifyInfo(
        std::make_shared<FinalizedBlock>(genesis), nodes, opts
      );
      P2P::NodeInfo req = P2P::NotificationDecoder::notifyInfo(msg);
      REQUIRE(req.latestBlockHash() == genesis.getHash());
      std::vector<P2P::NodeID> peers = req.peers();
      for (P2P::NodeID peer : peers) REQUIRE(nodes.contains(peer));

      // For coverage
      P2P::Message msgWrongType(Utils::makeBytes(bytes::join(
        P2P::getRequestTypePrefix(P2P::RequestType::Requesting),
        Utils::randBytes(8),
        P2P::getCommandPrefix(P2P::CommandType::NotifyInfo)
      )));
      P2P::Message msgWrongCmd(Utils::makeBytes(bytes::join(
        P2P::getRequestTypePrefix(P2P::RequestType::Notifying),
        Utils::randBytes(8),
        P2P::getCommandPrefix(P2P::CommandType::Ping)
      )));
      REQUIRE_THROWS(P2P::NotificationDecoder::notifyInfo(msgWrongType));
      REQUIRE_THROWS(P2P::NotificationDecoder::notifyInfo(msgWrongCmd));
    }
  }
}

