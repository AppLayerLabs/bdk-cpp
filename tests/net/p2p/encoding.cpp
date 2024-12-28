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
      Bytes msgSmallV6{0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // missing 0000000000000001
      Bytes msgInvalidIp{0x02}; // only 00 (v4) or 01 (v6)
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
}

