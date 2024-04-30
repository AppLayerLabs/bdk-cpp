/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "net/p2p/managerdiscovery.h"
#include "utils/options.h"
#include "iostream"

/// Executable with a discovery node for the given Options default chain.
int main() {
  // Local binary path + /blockchain
  std::string blockchainPath = std::filesystem::current_path().string() + std::string("/discoveryNode");
  const auto options = Options::fromFile(blockchainPath);
  P2P::ManagerDiscovery p2p(boost::asio::ip::address::from_string("127.0.0.1"), options);
  p2p.start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  p2p.startDiscovery();
  // Sleep Forever
  std::this_thread::sleep_until(std::chrono::system_clock::now() + std::chrono::hours(std::numeric_limits<int>::max()));
}

