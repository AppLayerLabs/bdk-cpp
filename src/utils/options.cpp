/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "options.h"

Options::Options(
  const std::string& rootPath, const std::string& web3clientVersion,
  const uint64_t& version, const uint64_t& chainID, const Address& chainOwner,
  const uint16_t& p2pPort, const uint16_t& httpPort,
  const uint16_t& minDiscoveryConns, const uint16_t& minNormalConns,
  const uint16_t& maxDiscoveryConns, const uint16_t& maxNormalConns,
  const uint64_t& eventBlockCap, const uint64_t& eventLogCap,
  const uint64_t& stateDumpTrigger,
  const uint32_t& minValidators,
  const std::vector<std::pair<boost::asio::ip::address, uint64_t>>& discoveryNodes,
  const FinalizedBlock& genesisBlock, const uint64_t genesisTimestamp, const PrivKey& genesisSigner,
  const std::vector<std::pair<Address, uint256_t>>& genesisBalances,
  const std::vector<Address>& genesisValidators
) : rootPath_(rootPath), web3clientVersion_(web3clientVersion),
  version_(version), chainID_(chainID), chainOwner_(chainOwner), p2pPort_(p2pPort), httpPort_(httpPort),
  minDiscoveryConns_(minDiscoveryConns), minNormalConns_(minNormalConns),
  maxDiscoveryConns_(maxDiscoveryConns), maxNormalConns_(maxNormalConns),
  eventBlockCap_(eventBlockCap), eventLogCap_(eventLogCap),
  stateDumpTrigger_(stateDumpTrigger),
  minValidators_(minValidators),
  coinbase_(Address()), isValidator_(false), discoveryNodes_(discoveryNodes),
  genesisBlock_(genesisBlock), genesisBalances_(genesisBalances), genesisValidators_(genesisValidators)
{
  json options;
  if (std::filesystem::exists(rootPath + "/options.json")) return;
  options["rootPath"] = rootPath;
  options["web3clientVersion"] = web3clientVersion;
  options["version"] = version;
  options["chainID"] = chainID;
  options["chainOwner"] = chainOwner.hex(true);
  options["p2pPort"] = p2pPort;
  options["httpPort"] = httpPort;
  options["minDiscoveryConns"] = minDiscoveryConns;
  options["minNormalConns"] = minNormalConns;
  options["maxDiscoveryConns"] = maxDiscoveryConns;
  options["maxNormalConns"] = maxNormalConns;
  options["eventBlockCap"] = eventBlockCap;
  options["eventLogCap"] = eventLogCap;
  options["stateDumpTrigger"] = stateDumpTrigger;
  options["minValidators"] = minValidators;
  options["discoveryNodes"] = json::array();
  for (const auto& [address, port] : discoveryNodes) {
    options["discoveryNodes"].push_back(json::object({
      {"address", address.to_string()},
      {"port", port}
    }));
  }
  options["genesis"] = json::object();
  options["genesis"]["timestamp"] = genesisTimestamp;
  options["genesis"]["signer"] = genesisSigner.hex(true);
  options["genesis"]["balances"] = json::array();
  for (const auto& [address, balance] : this->genesisBalances_) {
    options["genesis"]["balances"].push_back(json::object({
      {"address", address.hex(true)},
      {"balance", balance.str()}
    }));
  }
  options["genesis"]["validators"] = json::array();
  for (const auto& validator : this->genesisValidators_) {
    options["genesis"]["validators"].push_back(validator.hex(true));
  }
  std::filesystem::create_directories(rootPath);
  std::ofstream o(rootPath + "/options.json");
  o << options.dump(2) << std::endl;
  o.close();
}

Options::Options(
  const std::string& rootPath, const std::string& web3clientVersion,
  const uint64_t& version, const uint64_t& chainID, const Address& chainOwner,
  const uint16_t& p2pPort, const uint16_t& httpPort,
  const uint16_t& minDiscoveryConns, const uint16_t& minNormalConns,
  const uint16_t& maxDiscoveryConns, const uint16_t& maxNormalConns,
  const uint64_t& eventBlockCap, const uint64_t& eventLogCap,
  const uint64_t& stateDumpTrigger,
  const uint32_t& minValidators,
  const std::vector<std::pair<boost::asio::ip::address, uint64_t>>& discoveryNodes,
  const FinalizedBlock& genesisBlock, const uint64_t genesisTimestamp, const PrivKey& genesisSigner,
  const std::vector<std::pair<Address, uint256_t>>& genesisBalances,
  const std::vector<Address>& genesisValidators,
  const PrivKey& privKey
) : rootPath_(rootPath), web3clientVersion_(web3clientVersion),
  version_(version), chainID_(chainID), chainOwner_(chainOwner), p2pPort_(p2pPort), httpPort_(httpPort),
  minDiscoveryConns_(minDiscoveryConns), minNormalConns_(minNormalConns),
  maxDiscoveryConns_(maxDiscoveryConns), maxNormalConns_(maxNormalConns),
  eventBlockCap_(eventBlockCap), eventLogCap_(eventLogCap),
  stateDumpTrigger_(stateDumpTrigger),
  minValidators_(minValidators),
  discoveryNodes_(discoveryNodes), coinbase_(Secp256k1::toAddress(Secp256k1::toUPub(privKey))),
  isValidator_(true), genesisBlock_(genesisBlock), genesisBalances_(genesisBalances), genesisValidators_(genesisValidators)
{
  if (std::filesystem::exists(rootPath + "/options.json")) return;
  json options;
  options["rootPath"] = rootPath;
  options["web3clientVersion"] = web3clientVersion;
  options["version"] = version;
  options["chainID"] = chainID;
  options["chainOwner"] = chainOwner.hex(true);
  options["p2pPort"] = p2pPort;
  options["httpPort"] = httpPort;
  options["minDiscoveryConns"] = minDiscoveryConns;
  options["minNormalConns"] = minNormalConns;
  options["maxDiscoveryConns"] = maxDiscoveryConns;
  options["maxNormalConns"] = maxNormalConns;
  options["eventBlockCap"] = eventBlockCap;
  options["eventLogCap"] = eventLogCap;
  options["stateDumpTrigger"] = stateDumpTrigger;
  options["minValidators"] = minValidators;
  options["discoveryNodes"] = json::array();
  for (const auto& [address, port] : discoveryNodes) {
    options["discoveryNodes"].push_back(json::object({
      {"address", address.to_string()},
      {"port", port}
    }));
  }
  options["genesis"] = json::object();
  options["genesis"]["timestamp"] = genesisTimestamp;
  options["genesis"]["signer"] = genesisSigner.hex(true);
  options["genesis"]["balances"] = json::array();
  for (const auto& [address, balance] : this->genesisBalances_) {
    options["genesis"]["balances"].push_back(json::object({
      {"address", address.hex(true)},
      {"balance", balance.str()}
    }));
  }
  options["genesis"]["validators"] = json::array();
  for (const auto& validator : this->genesisValidators_) {
    options["genesis"]["validators"].push_back(validator.hex(true));
  }
  options["privKey"] = privKey.hex();
  std::filesystem::create_directories(rootPath);
  std::ofstream o(rootPath + "/options.json");
  o << options.dump(2) << std::endl;
  o.close();
}

PrivKey Options::getValidatorPrivKey() const {
  json options;
  std::ifstream i(this->rootPath_ + "/options.json");
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
      return Options::binaryDefaultOptions(rootPath);
    }

    std::ifstream i(rootPath + "/options.json");
    json options;
    i >> options;
    i.close();

    std::vector<std::pair<boost::asio::ip::address, uint64_t>> discoveryNodes;
    for (const auto& node : options["discoveryNodes"]) {
      discoveryNodes.emplace_back(
        boost::asio::ip::address::from_string(node["address"].get<std::string>()),
        node["port"].get<uint64_t>()
      );
    }

    const PrivKey genesisSigner(Hex::toBytes(options["genesis"]["signer"].get<std::string>()));
    MutableBlock genesis(Hash(), 0, 0);
    FinalizedBlock genesisFinal = genesis.finalize(genesisSigner, options["genesis"]["timestamp"].get<uint64_t>());

    std::vector<Address> genesisValidators;
    for (const auto& validator : options["genesis"]["validators"]) {
      genesisValidators.push_back(Address(Hex::toBytes(validator.get<std::string>())));
    }

    std::vector<std::pair<Address, uint256_t>> genesisBalances;
    for (const auto& balance : options["genesis"]["balances"]) {
      genesisBalances.emplace_back(
        Address(Hex::toBytes(balance["address"].get<std::string>())),
        uint256_t(balance["balance"].get<std::string>())
      );
    }

    if (options.contains("privKey")) {
      return Options(
        options["rootPath"].get<std::string>(),
        options["web3clientVersion"].get<std::string>(),
        options["version"].get<uint64_t>(),
        options["chainID"].get<uint64_t>(),
        Address(Hex::toBytes(options["chainOwner"].get<std::string>())),
        options["p2pPort"].get<uint64_t>(),
        options["httpPort"].get<uint64_t>(),
        options["minDiscoveryConns"].get<uint16_t>(),
        options["minNormalConns"].get<uint16_t>(),
        options["maxDiscoveryConns"].get<uint16_t>(),
        options["maxNormalConns"].get<uint16_t>(),
        options["eventBlockCap"].get<uint64_t>(),
        options["eventLogCap"].get<uint64_t>(),
        options["stateDumpTrigger"].get<uint64_t>(),
        options["minValidators"].get<uint32_t>(),
        discoveryNodes,
        genesisFinal,
        options["genesis"]["timestamp"].get<uint64_t>(),
        genesisSigner,
        genesisBalances,
        genesisValidators,
        PrivKey(Hex::toBytes(options["privKey"].get<std::string>()))
      );
    }

    return Options(
      options["rootPath"].get<std::string>(),
      options["web3clientVersion"].get<std::string>(),
      options["version"].get<uint64_t>(),
      options["chainID"].get<uint64_t>(),
      Address(Hex::toBytes(options["chainOwner"].get<std::string>())),
      options["p2pPort"].get<uint64_t>(),
      options["httpPort"].get<uint64_t>(),
      options["minDiscoveryConns"].get<uint16_t>(),
      options["minNormalConns"].get<uint16_t>(),
      options["maxDiscoveryConns"].get<uint16_t>(),
      options["maxNormalConns"].get<uint16_t>(),
      options["eventBlockCap"].get<uint64_t>(),
      options["eventLogCap"].get<uint64_t>(),
      options["stateDumpTrigger"].get<uint64_t>(),
      options["minValidators"].get<uint32_t>(),
      discoveryNodes,
      genesisFinal,
      options["genesis"]["timestamp"].get<uint64_t>(),
      genesisSigner,
      genesisBalances,
      genesisValidators
    );
  } catch (std::exception &e) {
    throw DynamicException("Could not create blockchain directory: " + std::string(e.what()));
  }
}

