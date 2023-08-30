/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "options.h"

Options::Options(
  const std::string& rootPath, const std::string& web3clientVersion,
  const uint64_t& version, const uint64_t& chainID,
  const uint16_t& wsPort, const uint16_t& httpPort,
  const std::vector<std::pair<boost::asio::ip::address, uint64_t>>& discoveryNodes
) : rootPath_(rootPath), web3clientVersion_(web3clientVersion),
  version_(version), chainID_(chainID), wsPort_(wsPort),
  httpPort_(httpPort), coinbase_(Address()), isValidator_(false), discoveryNodes_(discoveryNodes)
{
  json options;
  if (std::filesystem::exists(rootPath + "/options.json")) return;
  options["rootPath"] = rootPath;
  options["web3clientVersion"] = web3clientVersion;
  options["version"] = version;
  options["chainID"] = chainID;
  options["wsPort"] = wsPort;
  options["httpPort"] = httpPort;
  options["discoveryNodes"] = json::array();
  for (const auto& [address, port] : discoveryNodes) {
    options["discoveryNodes"].push_back(json::object({
      {"address", address.to_string()},
      {"port", port}
    }));
  }
  options["isValidator"] = this->isValidator_;
  std::filesystem::create_directories(rootPath);
  std::ofstream o(rootPath + "/options.json");
  o << options.dump(2) << std::endl;
  o.close();
}

Options::Options(
  const std::string& rootPath, const std::string& web3clientVersion,
  const uint64_t& version, const uint64_t& chainID,
  const uint16_t& wsPort, const uint16_t& httpPort,
  const std::vector<std::pair<boost::asio::ip::address, uint64_t>>& discoveryNodes,
  const PrivKey& privKey
) : rootPath_(rootPath), web3clientVersion_(web3clientVersion),
  version_(version), chainID_(chainID), wsPort_(wsPort),
  httpPort_(httpPort), discoveryNodes_(discoveryNodes), coinbase_(Secp256k1::toAddress(Secp256k1::toUPub(privKey))),
  isValidator_(true)
{
  if (std::filesystem::exists(rootPath + "/options.json")) return;
  json options;
  options["rootPath"] = rootPath;
  options["web3clientVersion"] = web3clientVersion;
  options["version"] = version;
  options["chainID"] = chainID;
  options["wsPort"] = wsPort;
  options["httpPort"] = httpPort;
  options["discoveryNodes"] = json::array();
  for (const auto& [address, port] : discoveryNodes) {
    options["discoveryNodes"].push_back(json::object({
      {"address", address.to_string()},
      {"port", port}
    }));
  }
  options["privKey"] = privKey.hex();
  std::filesystem::create_directories(rootPath);
  std::ofstream o(rootPath + "/options.json");
  o << options.dump(2) << std::endl;
  o.close();
}

const PrivKey Options::getValidatorPrivKey() const {
  json options;
  std::ifstream i(rootPath_ + "/options.json");
  i >> options;
  i.close();
  if (options.contains("privKey")) {
    const auto privKey = options["privKey"].get<std::string>();
    return PrivKey(Hex::toBytes(privKey));
  }
  return PrivKey();
}

Options Options::fromFile(const std::string& rootPath) {
  try {
    // Check if rootPath is valid
    if (!std::filesystem::exists(rootPath + "/options.json")) {
      std::filesystem::create_directory(rootPath);
      return Options(rootPath, "OrbiterSDK/cpp/linux_x86-64/0.1.2", 2, 8080, 8080, 8081, {});
    }

    std::ifstream i(rootPath + "/options.json");
    json options;
    i >> options;
    i.close();

    std::vector<std::pair<boost::asio::ip::address, uint64_t>> discoveryNodes;
    for (const auto& node : options["discoveryNodes"]) {
      discoveryNodes.push_back(std::make_pair(
        boost::asio::ip::address::from_string(node["address"].get<std::string>()),
        node["port"].get<uint64_t>()
      ));
    }

    if (options.contains("privKey")) {
      const auto privKey = options["privKey"].get<std::string>();
      return Options(
        options["rootPath"].get<std::string>(),
        options["web3clientVersion"].get<std::string>(),
        options["version"].get<uint64_t>(),
        options["chainID"].get<uint64_t>(),
        options["wsPort"].get<uint64_t>(),
        options["httpPort"].get<uint64_t>(),
        discoveryNodes,
        PrivKey(Hex::toBytes(privKey))
      );
    }

    return Options(
      options["rootPath"].get<std::string>(),
      options["web3clientVersion"].get<std::string>(),
      options["version"].get<uint64_t>(),
      options["chainID"].get<uint64_t>(),
      options["wsPort"].get<uint64_t>(),
      options["httpPort"].get<uint64_t>(),
      discoveryNodes
    );
  } catch (std::exception &e) {
    std::cerr << "Could not create blockchain directory: " << e.what() << std::endl;
    throw "Could not create blockchain directory.";
  }
}

