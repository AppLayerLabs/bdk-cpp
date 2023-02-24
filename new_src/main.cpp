#include <iostream>
#include "new_src/net/p2p/p2pmanager.h"
// Dummy main.cpp file
int main() {
	P2P::Manager p2pNode1(boost::asio::ip::address::from_string("127.0.0.1"), 8080, P2P::NodeType::NORMAL_NODE);
	P2P::Manager p2pNode2(boost::asio::ip::address::from_string("127.0.0.1"), 8081, P2P::NodeType::NORMAL_NODE);
	P2P::Manager p2pNode3(boost::asio::ip::address::from_string("127.0.0.1"), 8082, P2P::NodeType::NORMAL_NODE);
	p2pNode1.startServer();
	p2pNode2.startServer();
	p2pNode3.startServer();

	std::this_thread::sleep_for(std::chrono::seconds(5));
	p2pNode2.connectToServer("127.0.0.1", 8080);
	p2pNode3.connectToServer("127.0.0.1", 8080);
	std::this_thread::sleep_for(std::chrono::seconds(3));
	auto sessionList = p2pNode2.getSessionsIDs();
	for (auto& session : sessionList) {
		auto otherNodeList = p2pNode2.requestNodes(session);
		for (auto& node : otherNodeList) {
			std::cout << std::get<0>(node) << " " << std::get<1>(node).hex().get() << " " << std::get<2>(node) << " " << std::get<3>(node) << std::endl;
		}
	}
	std::this_thread::sleep_for(std::chrono::hours(24));
  return 0;
}