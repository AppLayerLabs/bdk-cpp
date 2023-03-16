#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/utils/utils.h"
#include "../../src/net/p2p/p2pmanagernormal.h"
#include "../../src/net/p2p/p2pmanagerdiscovery.h"
#include "../../src/core/rdpos.h"

using Catch::Matchers::Equals;

namespace TP2P {

  TEST_CASE("P2P Manager", "[net][p2p]") {
    SECTION ("P2P::Manager Simple 3 node network") {
      P2P::ManagerNormal p2pNode1(boost::asio::ip::address::from_string("127.0.0.1"), 8080, nullptr);
      P2P::ManagerNormal p2pNode2(boost::asio::ip::address::from_string("127.0.0.1"), 8081, nullptr);
      P2P::ManagerNormal p2pNode3(boost::asio::ip::address::from_string("127.0.0.1"), 8082, nullptr);

			Hash node1Id = p2pNode1.nodeId();
			Hash node2Id = p2pNode2.nodeId();
			Hash node3Id = p2pNode3.nodeId();

      p2pNode1.startServer();
      p2pNode2.startServer();
      p2pNode3.startServer();
			std::this_thread::sleep_for(std::chrono::milliseconds(100));


      REQUIRE(p2pNode1.isServerRunning() == true);
      REQUIRE(p2pNode2.isServerRunning() == true);
      REQUIRE(p2pNode3.isServerRunning() == true);

			p2pNode1.connectToServer("127.0.0.1", 8081);
			p2pNode1.connectToServer("127.0.0.1", 8082);
			p2pNode2.connectToServer("127.0.0.1", 8082);
			std::this_thread::sleep_for(std::chrono::milliseconds(100));

			auto node1SessionsIDs = p2pNode1.getSessionsIDs();
			auto node2SessionsIDs = p2pNode2.getSessionsIDs();
			auto node3SessionsIDs = p2pNode3.getSessionsIDs();			

			REQUIRE(node1SessionsIDs.size() == 2);
			REQUIRE(node2SessionsIDs.size() == 2);
			REQUIRE(node3SessionsIDs.size() == 2);

			// Try pinging each other
			for (auto session : node1SessionsIDs) {
				p2pNode1.ping(session);
			}

			for (auto session : node2SessionsIDs) {
				p2pNode2.ping(session);
			}
			
			for (auto session : node3SessionsIDs) {
				p2pNode3.ping(session);
			}
			
			// Stop discovery on nodes, disconnect and check.
			p2pNode1.stopDiscovery();
			p2pNode2.stopDiscovery();
			p2pNode3.stopDiscovery();
			p2pNode1.disconnectSession(node2Id);
			std::this_thread::sleep_for(std::chrono::seconds(5));	

			node1SessionsIDs = p2pNode1.getSessionsIDs();
			node2SessionsIDs = p2pNode2.getSessionsIDs();
			node3SessionsIDs = p2pNode3.getSessionsIDs();		
				
			REQUIRE(node1SessionsIDs.size() == 1);
			REQUIRE(node2SessionsIDs.size() == 1);
			REQUIRE(node3SessionsIDs.size() == 2);

			// Request Nodes from Node 3.
			auto nodesFromNode1 = p2pNode3.requestNodes(node1Id);
			auto nodesFromNode2 = p2pNode3.requestNodes(node2Id);
			
			REQUIRE(nodesFromNode1 == nodesFromNode2); // Node 1 and Node 2 should have the same nodes (only connected to the same node 3)

			// Start discovery, should recover the lost connection
			p2pNode1.startDiscovery();
			p2pNode2.startDiscovery();
			p2pNode3.startDiscovery();

			std::this_thread::sleep_for(std::chrono::seconds(5));
			
			node1SessionsIDs = p2pNode1.getSessionsIDs();
			node2SessionsIDs = p2pNode2.getSessionsIDs();
			node3SessionsIDs = p2pNode3.getSessionsIDs();		

			REQUIRE(node1SessionsIDs.size() == 2);
			REQUIRE(node2SessionsIDs.size() == 2);
			REQUIRE(node3SessionsIDs.size() == 2);

			// Try pinging again each other again.
			for (auto session : node1SessionsIDs) {
				p2pNode1.ping(session);
			}

			for (auto session : node2SessionsIDs) {
				p2pNode2.ping(session);
			}
			
			for (auto session : node3SessionsIDs) {
				p2pNode3.ping(session);
			}
			
			// Stop the servers
			p2pNode1.stop();
			p2pNode2.stop();
			p2pNode3.stop();

			std::this_thread::sleep_for(std::chrono::milliseconds(500));

			REQUIRE(p2pNode1.getSessionsIDs().size() == 0);
			REQUIRE(p2pNode2.getSessionsIDs().size() == 0);
			REQUIRE(p2pNode3.getSessionsIDs().size() == 0);

			REQUIRE(p2pNode1.isServerRunning() == false);
			REQUIRE(p2pNode2.isServerRunning() == false);
			REQUIRE(p2pNode3.isServerRunning() == false);
    }

		SECTION("10 P2P::ManagerNormal 1 P2P::ManagerDiscovery") {
			P2P::ManagerDiscovery p2pDiscoveryNode(boost::asio::ip::address::from_string("127.0.0.1"), 8080);
      P2P::ManagerNormal p2pNode1(boost::asio::ip::address::from_string("127.0.0.1"), 8081, nullptr);
      P2P::ManagerNormal p2pNode2(boost::asio::ip::address::from_string("127.0.0.1"), 8082, nullptr);
      P2P::ManagerNormal p2pNode3(boost::asio::ip::address::from_string("127.0.0.1"), 8083, nullptr);
			P2P::ManagerNormal p2pNode4(boost::asio::ip::address::from_string("127.0.0.1"), 8084, nullptr);
			P2P::ManagerNormal p2pNode5(boost::asio::ip::address::from_string("127.0.0.1"), 8085, nullptr);
			P2P::ManagerNormal p2pNode6(boost::asio::ip::address::from_string("127.0.0.1"), 8086, nullptr);
			P2P::ManagerNormal p2pNode7(boost::asio::ip::address::from_string("127.0.0.1"), 8087, nullptr);
			P2P::ManagerNormal p2pNode8(boost::asio::ip::address::from_string("127.0.0.1"), 8088, nullptr);
			P2P::ManagerNormal p2pNode9(boost::asio::ip::address::from_string("127.0.0.1"), 8089, nullptr);
			P2P::ManagerNormal p2pNode10(boost::asio::ip::address::from_string("127.0.0.1"), 8090, nullptr);

			p2pDiscoveryNode.startServer();
			p2pNode1.startServer();
			p2pNode2.startServer();
			p2pNode3.startServer();
			p2pNode4.startServer();
			p2pNode5.startServer();
			p2pNode6.startServer();
			p2pNode7.startServer();
			p2pNode8.startServer();
			p2pNode9.startServer();
			p2pNode10.startServer();
			
			std::this_thread::sleep_for(std::chrono::seconds(1));
			p2pNode1.connectToServer("127.0.0.1", 8080);
			p2pNode2.connectToServer("127.0.0.1", 8080);
			p2pNode3.connectToServer("127.0.0.1", 8080);
			p2pNode4.connectToServer("127.0.0.1", 8080);
			p2pNode5.connectToServer("127.0.0.1", 8080);
			p2pNode6.connectToServer("127.0.0.1", 8080);
			p2pNode7.connectToServer("127.0.0.1", 8080);
			p2pNode8.connectToServer("127.0.0.1", 8080);
			p2pNode9.connectToServer("127.0.0.1", 8080);
			p2pNode10.connectToServer("127.0.0.1", 8080);

			// After a while, the discovery thread should have found all the nodes and connected between each other.
			std::this_thread::sleep_for(std::chrono::seconds(10));

			auto nodeDiscoverySessionsIDs = p2pDiscoveryNode.getSessionsIDs();
			auto node1SessionsIDs = p2pNode1.getSessionsIDs();
			auto node2SessionsIDs = p2pNode2.getSessionsIDs();
			auto node3SessionsIDs = p2pNode3.getSessionsIDs();			
			auto node4SessionsIDs = p2pNode4.getSessionsIDs();
			auto node5SessionsIDs = p2pNode5.getSessionsIDs();
			auto node6SessionsIDs = p2pNode6.getSessionsIDs();
			auto node7SessionsIDs = p2pNode7.getSessionsIDs();
			auto node8SessionsIDs = p2pNode8.getSessionsIDs();
			auto node9SessionsIDs = p2pNode9.getSessionsIDs();
			auto node10SessionsIDs = p2pNode10.getSessionsIDs();

			REQUIRE(nodeDiscoverySessionsIDs.size() == 10);
			REQUIRE(node1SessionsIDs.size() == 10);
			REQUIRE(node2SessionsIDs.size() == 10);
			REQUIRE(node3SessionsIDs.size() == 10);
			REQUIRE(node4SessionsIDs.size() == 10);
			REQUIRE(node5SessionsIDs.size() == 10);
			REQUIRE(node6SessionsIDs.size() == 10);
			REQUIRE(node7SessionsIDs.size() == 10);
			REQUIRE(node8SessionsIDs.size() == 10);
			REQUIRE(node9SessionsIDs.size() == 10);
			REQUIRE(node10SessionsIDs.size() == 10);

			// Try pinging each other.

			for (auto session : nodeDiscoverySessionsIDs) {
				p2pDiscoveryNode.ping(session);
			}

			for (auto session : node1SessionsIDs) {
				p2pNode1.ping(session);
			}

			for (auto session : node2SessionsIDs) {
				p2pNode2.ping(session);
			}

			for (auto session : node3SessionsIDs) {
				p2pNode3.ping(session);
			}

			for (auto session : node4SessionsIDs) {
				p2pNode4.ping(session);
			}

			for (auto session : node5SessionsIDs) {
				p2pNode5.ping(session);
			}

			for (auto session : node6SessionsIDs) {
				p2pNode6.ping(session);
			}

			for (auto session : node7SessionsIDs) {
				p2pNode7.ping(session);
			}

			for (auto session : node8SessionsIDs) {
				p2pNode8.ping(session);
			}

			for (auto session : node9SessionsIDs) {
				p2pNode9.ping(session);
			}

			for (auto session : node10SessionsIDs) {
				p2pNode10.ping(session);
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			
			// Close all the nodes.
			p2pDiscoveryNode.stop();
			p2pNode1.stop();
			p2pNode2.stop();
			p2pNode3.stop();
			p2pNode4.stop();
			p2pNode5.stop();
			p2pNode6.stop();
			p2pNode7.stop();
			p2pNode8.stop();
			p2pNode9.stop();
			p2pNode10.stop();

			// Wait for the threads to finish.
			std::this_thread::sleep_for(std::chrono::seconds(1));

			REQUIRE(p2pDiscoveryNode.getSessionsIDs().size() == 0);
			REQUIRE(p2pNode1.getSessionsIDs().size() == 0);
			REQUIRE(p2pNode2.getSessionsIDs().size() == 0);
			REQUIRE(p2pNode3.getSessionsIDs().size() == 0);
			REQUIRE(p2pNode4.getSessionsIDs().size() == 0);
			REQUIRE(p2pNode5.getSessionsIDs().size() == 0);
			REQUIRE(p2pNode6.getSessionsIDs().size() == 0);
			REQUIRE(p2pNode7.getSessionsIDs().size() == 0);
			REQUIRE(p2pNode8.getSessionsIDs().size() == 0);
			REQUIRE(p2pNode9.getSessionsIDs().size() == 0);
			REQUIRE(p2pNode10.getSessionsIDs().size() == 0);

			REQUIRE(p2pDiscoveryNode.isServerRunning() == false);
			REQUIRE(p2pNode1.isServerRunning() == false);
			REQUIRE(p2pNode2.isServerRunning() == false);
			REQUIRE(p2pNode3.isServerRunning() == false);
			REQUIRE(p2pNode4.isServerRunning() == false);
			REQUIRE(p2pNode5.isServerRunning() == false);
			REQUIRE(p2pNode6.isServerRunning() == false);
			REQUIRE(p2pNode7.isServerRunning() == false);
			REQUIRE(p2pNode8.isServerRunning() == false);
			REQUIRE(p2pNode9.isServerRunning() == false);
			REQUIRE(p2pNode10.isServerRunning() == false);
		}
  }
};