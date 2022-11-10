#include <iostream>
#include "../utils/utils.h"
#include "../core/block.h"
#include "../core/transaction.h"
#include "../contract/abi.h"

#include <boost/filesystem.hpp>

void createGenesis() {

  Hash node1PrivKey;
  Hash node2PrivKey;
  Hash node3PrivKey;
  Hash node4PrivKey;
  Hash node5PrivKey;

  {
    std::string node1PrivKeyTmp;
    std::string node2PrivKeyTmp;
    std::string node3PrivKeyTmp;
    std::string node4PrivKeyTmp;
    std::string node5PrivKeyTmp;

    
    std::cout << "Please input node 1 private key" << std::endl;
    std::getline(std::cin, node1PrivKeyTmp);
    std::cout << "Please input node 2 private key" << std::endl;
    std::getline(std::cin, node2PrivKeyTmp);
    std::cout << "Please input node 3 private key" << std::endl;
    std::getline(std::cin, node3PrivKeyTmp);
    std::cout << "Please input node 4 private key" << std::endl;
    std::getline(std::cin, node4PrivKeyTmp);
    std::cout << "Please input node 5 private key" << std::endl;
    std::getline(std::cin, node5PrivKeyTmp);

    // Clean up 0x from hexes.
    if (node1PrivKeyTmp[0] == '0' && node1PrivKeyTmp[1] == 'x') {
      node1PrivKeyTmp = node1PrivKeyTmp.substr(2);
    }

    if (node2PrivKeyTmp[0] == '0' && node2PrivKeyTmp[1] == 'x') {
      node2PrivKeyTmp = node2PrivKeyTmp.substr(2);
    }

    if (node3PrivKeyTmp[0] == '0' && node3PrivKeyTmp[1] == 'x') {
      node3PrivKeyTmp = node3PrivKeyTmp.substr(2);
    }

    if (node4PrivKeyTmp[0] == '0' && node4PrivKeyTmp[1] == 'x') {
      node4PrivKeyTmp = node4PrivKeyTmp.substr(2);
    }

    if (node5PrivKeyTmp[0] == '0' && node5PrivKeyTmp[1] == 'x') {
      node5PrivKeyTmp = node5PrivKeyTmp.substr(2);
    }


    if (node1PrivKeyTmp.size() != 64 || node2PrivKeyTmp.size() != 64 || node3PrivKeyTmp.size() != 64 || node4PrivKeyTmp.size() != 64 || node5PrivKeyTmp.size() != 64) {
      std::cout << "Invalid private key" << std::endl;
      return;
    }

    node1PrivKey = Hash(Utils::hexToBytes(node1PrivKeyTmp));
    node2PrivKey = Hash(Utils::hexToBytes(node2PrivKeyTmp));
    node3PrivKey = Hash(Utils::hexToBytes(node3PrivKeyTmp));
    node4PrivKey = Hash(Utils::hexToBytes(node4PrivKeyTmp));
    node5PrivKey = Hash(Utils::hexToBytes(node5PrivKeyTmp));
  }

  boost::filesystem::path contractPath = (boost::filesystem::current_path().parent_path().string() + "/src/tools/blockManager.json");

  std::ifstream contractFile(contractPath.string());
  json contractJson = json::parse(contractFile);

  ABI::JSONEncoder contract(contractJson);

  Hash node1Random = Hash::random();
  Hash node2Random = Hash::random();
  Hash node3Random = Hash::random();
  Hash node4Random = Hash::random();
  Hash node5Random = Hash::random();

  Hash node1Hash = Utils::sha3(node1Random.get_view());
  Hash node2Hash = Utils::sha3(node2Random.get_view());
  Hash node3Hash = Utils::sha3(node3Random.get_view());
  Hash node4Hash = Utils::sha3(node4Random.get_view());
  Hash node5Hash = Utils::sha3(node5Random.get_view());

  // TODO: I'm currently working on this, don't touch! (Ita)
}

int main() {
  std::cout << "Creating a new genesis..." << std::endl;
  createGenesis();
  return 0;
}