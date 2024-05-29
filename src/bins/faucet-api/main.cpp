#include "src/faucetmanager.h"
#include <filesystem>
// In order to construct the faucet manager, need to load the following:
// const std::vector<WorkerAccount>& faucetWorkers,
// const uint64_t& chainId,
// const std::pair<net::ip::address_v4, uint16_t>& httpEndpoint,
// const uint16_t& port
// For that we ask the user:
// A file path to a list of private keyssss (one hex per line)
// The chain ID
// The HTTP endpoint (for HTTP client) (IP:PORT)
// The port for the server
int main() {
  Log::logToCout = true;
  std::vector<WorkerAccount> faucetWorkers;
  uint64_t chainId;
  std::pair<net::ip::address_v4, uint16_t> httpEndpoint;
  uint16_t port;



  std::cout << "Welcome to the faucet API provider" << std::endl;
  std::cout << "This API provider is designed to load a list of keys from a file and provide a faucet service" << std::endl;
  std::cout << "Using the keys provided to sign transactions" << std::endl;

  std::cout << "Please type the file path to the list of private keys (emtpy for default: \"privkeys.txt\"): " << std::endl;
  std::string filePath;
  std::getline(std::cin, filePath);
  if (filePath.empty()) {
    filePath = "privkeys.txt";
  }
  if (!std::filesystem::is_regular_file(filePath)) {
    throw DynamicException("Invalid file path for private keys");
  }

  std::ifstream file(filePath);
  std::string line;
  while (std::getline(file, line)) {
    Bytes key = Hex::toBytes(line);
    if (key.size() != 32) {
      throw DynamicException("Invalid private key");
    }
    faucetWorkers.push_back(WorkerAccount(PrivKey(key)));
  }

  std::cout << "Please provide the chain Id (empty for default: 808080): " << std::endl;
  std::string chainIdStr;
  std::getline(std::cin, chainIdStr);

  if (chainIdStr.empty()) {
    chainId = 808080;
  } else {
    for (const auto& c : chainIdStr) {
      if (!std::isdigit(c)) {
        throw DynamicException("Invalid chain Id");
      }
    }
    chainId = std::stoull(chainIdStr);
  }

  std::cout << "Please provide the HTTP endpoint (IP:PORT) (empty for default: 127.0.0.1:8090): " << std::endl;
  std::string httpEndpointStr;
  std::getline(std::cin, httpEndpointStr);
  if (httpEndpointStr.empty()) {
    httpEndpoint = std::make_pair(net::ip::address_v4::from_string("127.0.0.1"), 8090);
  } else {
    std::vector<std::string> parts;
    boost::split(parts, httpEndpointStr, boost::is_any_of(":"));
    if (parts.size() != 2) {
      throw DynamicException("Invalid HTTP endpoint");
    }
    try {
      httpEndpoint = std::make_pair(net::ip::address_v4::from_string(parts[0]), std::stoul(parts[1]));
    } catch (const std::exception& e) {
      throw DynamicException("Invalid HTTP endpoint");
    }
  }

  std::cout << "Please provide the port for the server (empty for default: 28888): " << std::endl;
  std::string portStr;
  std::getline(std::cin, portStr);
  if (portStr.empty()) {
    port = 28888;
  } else {
    for (const auto& c : portStr) {
      if (!std::isdigit(c)) {
        throw DynamicException("Invalid port");
      }
    }
    port = std::stoull(portStr);
  }

  std::cout << "Loaded: " << faucetWorkers.size() << " PrivKeys" << std::endl;
  std::cout << "ChainID: " << chainId << std::endl;
  std::cout << "HTTP endpoint: " << httpEndpoint.first << ":" << httpEndpoint.second << std::endl;
  std::cout << "Port: " << port << std::endl;
  std::cout << "Please type anything to start the faucet" << std::endl;
  std::string start;
  std::getline(std::cin, start);

  Faucet::Manager manager(faucetWorkers, chainId, httpEndpoint, port);
  manager.setup();
  manager.run();








  return 0;
}