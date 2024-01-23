#include "options.h"

/**
 * Options construtor cheatlist
 * const std::string& rootPath
 * const std::string& web3clientVersion,
 * const uint64_t& version,
 * const uint64_t& chainID,
 * const uint16_t& wsPort,
 * const uint16_t& httpPort,
 * const std::vector<std::pair<boost::asio::ip::address, uint64_t>>& discoveryNodes,
 */

Options Options::binaryDefaultOptions(const std::string& rootPath) {
  return {
    rootPath,                                                                  // Root Path
    "OrbiterSDK/cpp/linux_x86-64/0.1.2",                                     // web3clientVersion
    2,                                                                         // version
    8080,                                                                      // chainId
    8080,                                                                      // wsPort
    8081,                                                                      // httpPort
    {}                                                          // discoveryNodes
  };
}