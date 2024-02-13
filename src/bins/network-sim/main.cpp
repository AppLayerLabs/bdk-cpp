#include "src/networksimulator.h"

/**
 *  OrbiterSDK Network Simulator
 *  Built to stress and test the capabilities of the OrbiterSDK network
 *  It requires a running instance OrbiterSDK network to connect to (AIO-setup.sh is recommended for local instances)
 *  It works as following:
 *  1. The simulator will setup a given number of accounts (Packet size * Worker size)
 *     with a given amount of native tokens using the chain owner private key.
 *  2. The simulator will then create X workers where each worker
 *     will send a "packet" containing one transaction from each account to the chain owner, given that worker HTTP port
 *     containing a given amount of native tokens.
 *  3. Each worker will follow the following cycle:
 *    - Create a packet containing a transaction from each account to the chain owner
 *    - Send the packet containing the transactions through the respective worker HTTP port
 *    - Wait for the packet to be confirmed by the network
 *
 *  4. The simulator will count and print the following
 *     - Packet creation time
 *     - Packet send time
 *     - Packet confirmation time
 *     - Total time
 *
 * When executing the simulator, it will ask for the following parameters:
 * 1. Chain owner PrivKey: The private key of the chain owner
 * 2. Chain ID: The ID of the chain to connect to (for transactions)
 * 3. Packet size: The number of transactions to send in each packet (also the number of accounts to create)
 * 4. Packet count: The number of packets to send
 * 5. Init Native balance (wei): The amount of native tokens to send to each account
 * 6. Tx Native balance (wei): The amount of native tokens to send in each transaction
 * 7. Worker threads: The number of worker threads to use to create/send the packets
 * 8. HTTP IP:PORT: The IP and port of each worker HTTP server
 *
 */

int main() {
  /// Initial params.
  PrivKey chainOwnerPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
  uint64_t chainId = 808080;
  uint64_t packetSize = 5000;
  uint64_t packetCount = 10000;
  uint256_t initNativeBalance = uint256_t("100000000000000000000000"); // (100000.00)
  uint256_t txNativeBalance = uint256_t("1000000000000");              // (0.000001)
  uint64_t workerThreads = 1;
  std::vector<std::pair<net::ip::address_v4, uint16_t>> httpEndpoints {  };

  std::cout << "Welcome to the OrbiterSDK Network Simulator" << std::endl;
  std::cout << "This simulator is designed to test and stress the live network capabilities of OrbiterSDK" << std::endl;
  std::cout << "Please see the source code comments for more information on how configure and use this simulator" << std::endl;

  std::cout << "Please type the chain owner private key, nothing for default: " << std::endl;;
  std::string chainOwnerPrivKeyStr;
  std::getline(std::cin, chainOwnerPrivKeyStr);

  if (!chainOwnerPrivKeyStr.empty()) {
    static const std::regex hashFilter("^0x[0-9,a-f,A-F]{64}$");
    if (!std::regex_match(chainOwnerPrivKeyStr, hashFilter)) {
      std::cout << "Invalid private key" << std::endl;
      return 1;
    }
    chainOwnerPrivKey = PrivKey(Hex::toBytes(chainOwnerPrivKeyStr));
  };

  std::cout << "Please provide the chain Id: nothing for default (808080)" << std::endl;
  std::string chainIdStr;
  std::getline(std::cin, chainIdStr);
  if (!chainIdStr.empty())
  {
    for (const auto& c : chainIdStr) {
      if (!std::isdigit(c)) {
        std::cout << "Invalid chain Id" << std::endl;
        return 1;
      }
    }
    chainId = std::stoull(chainIdStr);
  }

  std::cout << "Please provide the packet size: nothing for default (5000)" << std::endl;
  std::string packetSizeStr;
  std::getline(std::cin, packetSizeStr);
  if (!packetSizeStr.empty())
  {
    for (const auto& c : packetSizeStr) {
      if (!std::isdigit(c)) {
        std::cout << "Invalid packet size" << std::endl;
        return 1;
      }
    }
    packetSize = std::stoull(packetSizeStr);
    if (packetSize > 100000) {
      std::cout << "Packet size is too large" << std::endl;
      return 1;
    }
  }

  std::cout << "Please provide a packet count: nothing for default (10000)" << std::endl;
  std::string packetCountStr;
  std::getline(std::cin, packetCountStr);
  if (!packetCountStr.empty())
  {
    for (const auto& c : packetCountStr) {
      if (!std::isdigit(c)) {
        std::cout << "Invalid packet count" << std::endl;
        return 1;
      }
    }
    packetCount = std::stoull(packetCountStr);
  }


  std::cout << "Please provide the initial native balance (wei): nothing for default (100000000000000000000000)" << std::endl;;
  std::string initNativeBalanceStr;
  std::getline(std::cin, initNativeBalanceStr);
  if (!initNativeBalanceStr.empty())
  {
    for (const auto& c : initNativeBalanceStr) {
      if (!std::isdigit(c)) {
        std::cout << "Invalid initial native balance" << std::endl;
        return 1;
      }
    }
    initNativeBalance = uint256_t(initNativeBalanceStr);
  }

  std::cout << "Please provide the transaction native balance (wei): nothing for default (1000000000000)" << std::endl;;
  std::string txNativeBalanceStr;
  std::getline(std::cin, txNativeBalanceStr);
  // Check if txNativeBalanceStr is a number
  if (!txNativeBalanceStr.empty())
  {
    for (const auto& c : txNativeBalanceStr) {
      if (!std::isdigit(c)) {
        std::cout << "Invalid transaction native balance" << std::endl;
        return 1;
      }
    }
    txNativeBalance = uint256_t(txNativeBalanceStr);
  }

  std::cout << "Please provide the number of worker threads: nothing for default (1)" << std::endl;
  std::string workerThreadsStr;
  std::getline(std::cin, workerThreadsStr);
  if (!workerThreadsStr.empty())
  {
    for (const auto& c : workerThreadsStr) {
      if (!std::isdigit(c)) {
        std::cout << "Invalid worker threads" << std::endl;
        return 1;
      }
    }
    workerThreads = std::stoull(workerThreadsStr);
  }

  for (uint64_t i = 0; i < workerThreads; i++) {
    std::cout << "Please provide the HTTP IP:PORT for worker " << i << ": (format: \"IP:PORT\", nothing for default (127.0.0.1, 8090))" << std::endl;
    std::string httpEndpointStr;
    std::getline(std::cin, httpEndpointStr);
    if (httpEndpointStr.empty()) {
      httpEndpoints.push_back(std::make_pair(net::ip::address_v4::from_string("127.0.0.1"), 8090));
    } else {
      std::vector<std::string> parts;
      boost::split(parts, httpEndpointStr, boost::is_any_of(":"));
      if (parts.size() != 2) {
        std::cout << "Invalid HTTP endpoint" << std::endl;
        return 1;
      }
      boost::system::error_code ec;
      auto ip = net::ip::address_v4::from_string(parts[0], ec);
      if (ec) {
        throw std::string("Invalid IP address: " + ec.message());
      }

      for (const char& c : parts[1]) {
        if (!std::isdigit(c)) {
          std::cout << "Invalid port" << std::endl;
          return 1;
        }
      }

      uint64_t port = std::stoull(parts[1]);
      if (port > 65535) {
        std::cout << "Invalid port" << std::endl;
        return 1;
      }
      httpEndpoints.emplace_back(ip, port);
    }
  }


  std::cout << std::endl << std::endl << std::endl;
  std::cout << "Starting the OrbiterSDK Network Simulator" << std::endl;
  std::cout << "Chain owner private key: " << chainOwnerPrivKey.hex(true) << std::endl;
  std::cout << "Chain ID: " << chainId << std::endl;
  std::cout << "Packet size: " << packetSize << std::endl;
  std::cout << "Packet count: " << packetCount << std::endl;
  std::cout << "Initial native balance (wei): " << initNativeBalance.str() << std::endl;
  std::cout << "Transaction native balance (wei): " << txNativeBalance.str() << std::endl;
  std::cout << "Worker threads: " << workerThreads << std::endl;
  std::cout << "HTTP endpoints: " << std::endl;
  for (const auto& endpoint : httpEndpoints) {
    std::cout << "  " << std::get<0>(endpoint).to_string() << ":" << std::get<1>(endpoint) << std::endl;
  }

  NetworkSimulator simulator(
    chainOwnerPrivKey,
    chainId,
    packetSize,
    packetCount,
    initNativeBalance,
    txNativeBalance,
    workerThreads,
    httpEndpoints
  );

  simulator.setup();
  std::cout << "Press anything to start the simulation..." << std::endl;
  std::cin.get();
  simulator.run();

  return 0;
}