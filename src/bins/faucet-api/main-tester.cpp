#include "src/faucetmanager.h"
#include <filesystem>

// This is a "simulator", it will request the number of iterations to start banging the faucet endpoint
// The HTTP endpoint (for HTTP client) (IP:PORT)
int main() {
  std::vector<WorkerAccount> faucetWorkers;
  std::pair<net::ip::address_v4, uint16_t> httpEndpoint;
  uint64_t iterations;

  std::cout << "Welcome to the faucet API provider tester" << std::endl;
  std::cout << "This API provider is designed to generate random accounts and request funds from the faucet" << std::endl;
  std::cout << "It will dump the privkeys to the faucettester.txt" << std::endl;

  std::cout << "Please provide the HTTP endpoint (IP:PORT) (empty for default: 127.0.0.1:28888): " << std::endl;
  std::string httpEndpointStr;
  std::getline(std::cin, httpEndpointStr);
  if (httpEndpointStr.empty()) {
    httpEndpoint = std::make_pair(net::ip::address_v4::from_string("127.0.0.1"), 28888);
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

  // Ask for a iteration quantity to start banging the faucet endpoint
  std::cout << "Please type the number of iterations to start banging the faucet endpoint (empty for default: 25000): " << std::endl;
  std::string iterationsStr;
  std::getline(std::cin, iterationsStr);
  if (iterationsStr.empty()) {
    iterations = 25000;
  } else {
    for (const auto& c : iterationsStr) {
      if (!std::isdigit(c)) {
        throw DynamicException("Invalid iterations");
      }
    }
    iterations = std::stoull(iterationsStr);
  }

  std::cout << "Creating worker accounts..." << std::endl;

  for (uint64_t i = 0; i < iterations; i++) {
    faucetWorkers.emplace_back(PrivKey(Utils::randBytes(32)));
  }

  std::cout << "Worker accounts created size: " << faucetWorkers.size() << std::endl;
  std::cout << "Dumping privkeys to faucettester.txt" << std::endl;
  std::ofstream file("faucettester.txt");
  for (const auto& worker : faucetWorkers) {
    file << worker.privKey.hex(true) << std::endl;
  }
  file.close();

  std::cout << "Creating the requests..." << std::endl;
  std::vector<std::string> requests;
  for (const auto& worker : faucetWorkers) {
    requests.push_back(Faucet::Manager::makeDripToAddress(worker.address));
  }

  std::cout << "Requests created size: " << requests.size() << std::endl;
  std::cout << "Creating HTTP client..." << std::endl;

  HTTPSyncClient client(httpEndpoint.first.to_string(), std::to_string(httpEndpoint.second));


  client.connect();

  std::cout << "Type anything to start banging the faucet endpoint" << std::endl;
  std::string dummy;
  std::getline(std::cin, dummy);


  for (uint64_t i = 0 ; i < requests.size(); i++) {
    if (i % 100 == 0) {
      std::cout << "Iteration: " << i << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1)); /// Sleep for 1ms to avoid spamming the endpoint too much lol
    std::string response = client.makeHTTPRequest(requests[i]);
    json j = json::parse(response);
    if (!j.contains("result")) {
      std::cout << "Error: " << j.dump(2) << std::endl;
    }
    if (j["result"] != "0x1") {
      std::cout << "Error: " << j.dump(2) << std::endl;
    }
  }

  return 0;
}