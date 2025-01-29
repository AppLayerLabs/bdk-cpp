/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "options.h"

#include "dynamicexception.h"

IndexingMode::IndexingMode(std::string_view mode) {
  if (mode == "DISABLED") {
    value_ = DISABLED.value_;
  } else if (mode == "RPC") {
    value_ = RPC.value_;
  } else if (mode == "RPC_TRACE") {
    value_ = RPC_TRACE.value_;
  } else {
    std::stringstream errorMessage;
    errorMessage << "Invalid indexing mode value: \"" << mode << "\"";
    throw DynamicException(errorMessage.str());
  }
}

Options::Options(
  const std::string& rootPath, const std::string& web3clientVersion,
  const uint64_t& version, const uint64_t& chainID, const Address& chainOwner,
  const uint256_t& chainOwnerInitialBalance, const uint16_t& httpPort,
  const uint64_t& eventBlockCap, const uint64_t& eventLogCap,
  const uint64_t& stateDumpTrigger, IndexingMode indexingMode, const json& cometBFT
) : rootPath_(rootPath),
    web3clientVersion_(web3clientVersion),
    version_(version),
    chainID_(chainID),
    chainOwner_(chainOwner),
    chainOwnerInitialBalance_(chainOwnerInitialBalance),
    httpPort_(httpPort),
    eventBlockCap_(eventBlockCap),
    eventLogCap_(eventLogCap),
    stateDumpTrigger_(stateDumpTrigger),
    indexingMode_(indexingMode),
    cometBFT_(cometBFT)
{
  if (std::filesystem::exists(rootPath + "/options.json")) return;
  json options;
  options["rootPath"] = rootPath;
  options["web3clientVersion"] = web3clientVersion;
  options["version"] = version;
  options["chainID"] = chainID;
  options["chainOwner"] = chainOwner.hex(true);
  options["chainOwnerInitialBalance"] = chainOwnerInitialBalance.str();
  options["httpPort"] = httpPort;
  options["eventBlockCap"] = eventBlockCap;
  options["eventLogCap"] = eventLogCap;
  options["stateDumpTrigger"] = stateDumpTrigger;
  options["indexingMode"] = indexingMode_.toString();
  options["cometBFT"] = cometBFT;

  std::filesystem::create_directories(rootPath);
  std::ofstream o(rootPath + "/options.json");
  o << options.dump(2) << std::endl;
  o.close();
}

Options Options::fromFile(const std::string& rootPath) {
  try {
    // Check if options.json exists, throw if not
    if (!std::filesystem::exists(rootPath + "/options.json")) {
      throw DynamicException("Config file does not exist: " + rootPath + "/options.json");
    }

    std::ifstream i(rootPath + "/options.json");
    json options;
    i >> options;
    i.close();

    return Options(
      options["rootPath"].get<std::string>(),
      options["web3clientVersion"].get<std::string>(),
      options["version"].get<uint64_t>(),
      options["chainID"].get<uint64_t>(),
      Address(Hex::toBytes(options["chainOwner"].get<std::string>())),
      uint256_t(options["chainOwnerInitialBalance"].get<std::string>()),
      options["httpPort"].get<uint64_t>(),
      options["eventBlockCap"].get<uint64_t>(),
      options["eventLogCap"].get<uint64_t>(),
      options["stateDumpTrigger"].get<uint64_t>(),
      IndexingMode(options["indexingMode"].get<std::string>()),
      options["cometBFT"]
    );
  } catch (std::exception &e) {
    throw DynamicException("Could not create blockchain directory: " + std::string(e.what()));
  }
}

