/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "options.h"

Options::Options(
  const std::string& rootPath,
  const std::string& web3clientVersion,
  const uint64_t& version,
  const uint64_t& chainID,
  const Address& chainOwner,
  const uint16_t& wsPort,
  const uint16_t& httpPort,
  const uint16_t& minDiscoveryConns,
  const uint16_t& minNormalConns,
  const uint16_t& maxDiscoveryConns,
  const uint16_t& maxNormalConns,
  const uint64_t& eventBlockCap,
  const uint64_t& eventLogCap,
  const uint32_t& minValidators,
  const bool isValidator,
  const std::vector<std::pair<boost::asio::ip::address, uint64_t>>& discoveryNodes,
  const Address coinbase,
  const Block& genesisBlock,
  const uint64_t genesisTimestamp,
  const PrivKey& genesisSigner,
  const std::vector<std::pair<Address, uint256_t>>& genesisBalances,
  const std::vector<Address>& genesisValidators,
  const PrivKey *privKey) :
  rootPath_(rootPath),
  web3clientVersion_(web3clientVersion),
  version_(version),
  chainID_(chainID),
  chainOwner_(chainOwner),
  wsPort_(wsPort),
  httpPort_(httpPort),
  minDiscoveryConns_(minDiscoveryConns),
  minNormalConns_(minNormalConns),
  maxDiscoveryConns_(maxDiscoveryConns),
  maxNormalConns_(maxNormalConns),
  eventBlockCap_(eventBlockCap),
  eventLogCap_(eventLogCap),
  minValidators_(minValidators),
  isValidator_(isValidator),
  discoveryNodes_(discoveryNodes),
  coinbase_(coinbase),
  genesisBlock_(genesisBlock),
  genesisBalances_(genesisBalances),
  genesisValidators_(genesisValidators) {

  json options;

  options["rootPath"] = rootPath;
  options["web3clientVersion"] = web3clientVersion;
  options["version"] = version;
  options["chainID"] = chainID;
  options["chainOwner"] = chainOwner.hex(true);
  options["wsPort"] = wsPort;
  options["httpPort"] = httpPort;
  options["minDiscoveryConns"] = minDiscoveryConns;
  options["minNormalConns"] = minNormalConns;
  options["maxDiscoveryConns"] = maxDiscoveryConns;
  options["maxNormalConns"] = maxNormalConns;
  options["eventBlockCap"] = eventBlockCap;
  options["eventLogCap"] = eventLogCap;
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
  // check if we have a private key
  if (privKey != nullptr)
    options["privKey"] = privKey->hex();

  if (!std::filesystem::exists(rootPath + "/options.json"))
    createOptionsFile(rootPath, options);
}

void Options::createOptionsFile(const std::string& rootPath, const json &options)
{
  try {
    std::filesystem::create_directory(rootPath);
    std::ofstream o(rootPath + "/options.json");
    o << options.dump(2) << std::endl;
    o.close();
  }
  catch (std::exception &e) {
    throw DynamicException(
      "Could not create rootPath directory: " + std::string(e.what())
      );
  }
}

json Options::getOptionsFromFile(const std::string& rootPath)
{
  json options;
  std::ifstream i(rootPath + "/options.json");

  i >> options;
  i.close();

  return options;
}

PrivKey Options::getValidatorPrivKey() const {
  json options = getOptionsFromFile(this->rootPath_);
  if (options.contains("privKey")) {
    const auto privKey = options["privKey"].get<std::string>();
    return PrivKey(Hex::toBytes(privKey));
  }
  return PrivKey();
}

std::string Options::resolveAddress(const std::string& host)
{
  std::string ipAddress = host;
  boost::system::error_code ec;
  try {
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::resolver resolver{io_context};
    boost::asio::ip::tcp::resolver::results_type endpoints =
      resolver.resolve(host, "", ec);

    if (ec.value() == 0) {
      boost::asio::ip::tcp::endpoint endpoint = *(endpoints.cbegin());
      ipAddress = endpoint.address().to_string();
    }
  }
  catch (std::exception& e) {
    Logger::logToDebug(LogType::INFO,
                       Log::options,
                       __func__,
                       "TCP Resolver Exception: " + std::string(e.what()));
  }
  return ipAddress;
}

Options Options::fromFile(const std::string& rootPath)
{
  // default values
  bool isValidator = false;
  PrivKey *privKey = nullptr;
  Address coinbase = Address();
  std::vector<Address> genesisValidators;
  std::vector<std::pair<Address, uint256_t>> genesisBalances;
  std::vector<std::pair<boost::asio::ip::address, uint64_t>> discoveryNodes;

  // check if options file is valid
  if (!std::filesystem::exists(rootPath + "/options.json")) {
    try {
      std::filesystem::create_directory(rootPath);
    }
    catch (std::exception &e) {
      throw DynamicException("Could not create blockchain directory: " + std::string(e.what()));
    }
    return Options::binaryDefaultOptions(rootPath);
  }

  json options = getOptionsFromFile(rootPath);
  for (const auto& node : options["discoveryNodes"]) {
    discoveryNodes.emplace_back(
      boost::asio::ip::address::from_string(
        resolveAddress(node["address"].get<std::string>())),
      node["port"].get<uint64_t>()
      );
  }
  const PrivKey genesisSigner(Hex::toBytes(options["genesis"]["signer"].get<std::string>()));
  Block genesis(Hash(), 0, 0);
  genesis.finalize(genesisSigner, options["genesis"]["timestamp"].get<uint64_t>());
  for (const auto& validator : options["genesis"]["validators"]) {
    genesisValidators.push_back(Address(Hex::toBytes(validator.get<std::string>())));
  }
  for (const auto& balance : options["genesis"]["balances"]) {
    genesisBalances.emplace_back(
      Address(Hex::toBytes(balance["address"].get<std::string>())),
      uint256_t(balance["balance"].get<std::string>())
      );
  }
  if (options.contains("privKey")) {
    isValidator = true;
    privKey = new PrivKey(Hex::toBytes(options["privKey"].get<std::string>()));
    coinbase = Secp256k1::toAddress(Secp256k1::toUPub(*privKey));
  }
  return Options(
    options["rootPath"].get<std::string>(),
    options["web3clientVersion"].get<std::string>(),
    options["version"].get<uint64_t>(),
    options["chainID"].get<uint64_t>(),
    Address(Hex::toBytes(options["chainOwner"].get<std::string>())),
    options["wsPort"].get<uint64_t>(),
    options["httpPort"].get<uint64_t>(),
    options["minDiscoveryConns"].get<uint16_t>(),
    options["minNormalConns"].get<uint16_t>(),
    options["maxDiscoveryConns"].get<uint16_t>(),
    options["maxNormalConns"].get<uint16_t>(),
    options["eventBlockCap"].get<uint64_t>(),
    options["eventLogCap"].get<uint64_t>(),
    options["minValidators"].get<uint32_t>(),
    isValidator,
    discoveryNodes,
    coinbase,
    genesis,
    options["genesis"]["timestamp"].get<uint64_t>(),
    genesisSigner,
    genesisBalances,
    genesisValidators,
    privKey);
}
