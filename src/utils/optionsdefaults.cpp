#include "options.h"

/**
 * Options construtor cheatlist
 * const std::string& rootPath
 * const std::string& web3clientVersion,
 * const uint64_t& version,
 * const uint64_t& chainID,
 * const Address& chainOwner,
 * const uint16_t& wsPort,
 * const uint16_t& httpPort,
 * const std::vector<std::pair<boost::asio::ip::address, uint64_t>>& discoveryNodes,
 */

Options Options::binaryDefaultOptions(const std::string& rootPath) {
  return {
    rootPath,
    "OrbiterSDK/cpp/linux_x86-64/0.1.2",
    2,
    8080,
    Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")),
    8080,
    8081,
    {}
  };
}