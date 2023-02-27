#include "../new_src/libs/catch2/catch_amalgamated.hpp"
#include "../new_src/utils/utils.h"
#include "../new_src/net/p2p/p2pmanagernormal.h"

using Catch::Matchers::Equals;

namespace TP2P {

  TEST_CASE("P2P Manager") {
    SECTION ("P2P::Manager Simple 3 node network") {
      P2P::ManagerNormal p2pNode1(boost::asio::ip::address::from_string("127.0.0.1"), 8080);
      P2P::ManagerNormal p2pNode2(boost::asio::ip::address::from_string("127.0.0.1"), 8081);
      P2P::ManagerNormal p2pNode3(boost::asio::ip::address::from_string("127.0.0.1"), 8082);

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
			
			// Disconnect then check.
			p2pNode1.disconnectSession(node2Id);
			std::this_thread::sleep_for(std::chrono::milliseconds(100));	

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

			// Stop the servers
			p2pNode1.stop();
			p2pNode2.stop();
			p2pNode3.stop();

			std::this_thread::sleep_for(std::chrono::milliseconds(100));

			REQUIRE(p2pNode1.getSessionsIDs().size() == 0);
			REQUIRE(p2pNode2.getSessionsIDs().size() == 0);
			REQUIRE(p2pNode3.getSessionsIDs().size() == 0);

			REQUIRE(p2pNode1.isServerRunning() == false);
			REQUIRE(p2pNode2.isServerRunning() == false);
			REQUIRE(p2pNode3.isServerRunning() == false);
    }
  }
};