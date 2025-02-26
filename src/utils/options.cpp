/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "options.h"
#include "utils.h"

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
  // Options ctor should not write files.
  // If you actually want to dump the object to its rootPath + options.json
  //  default/expected location, call toFile() at some point after construction.

  // TODO: The rootPath should not really be in the options.json file.
  // Rather, the rootPath is wherever the options.json file happens to be in a system.
  // Given a rootPath, you expect to find node options *in* it.
  // Complete node configuration at runtime will always contain more information than what you can find in
  //  one or more static/const node configuration files.
  // One way to solve this:
  //  class Options {} // 1:1 to Json options file (remove rootPath from it)
  //  class Config { Options options; std::string rootPath; } // Full node config (static/file + dynamic/toggleable/non-const).
  // Then you pass Config objects around the various components, not Options objects.
  // In production, the rootPath should be taken from ProcessOptions (i.e. process/executable cmd-line switches)
  //  or inferred from the current working directory if none given (e.g. ./blockchain, ./bdk, etc...)
  // In testing, you have multiple nodes in the same process, so the various rootPaths come from the
  //  dynamically-generated test dump subdirectories (one for each test node).
}

Options Options::fromFile(const std::string& rootPath) {
  try {
    json options = Utils::readJson(rootPath + "/options.json");

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

bool Options::toFile() {
  if (std::filesystem::exists(rootPath_ + "/options.json")) {
    return false;
  }

  json options;
  options["rootPath"] = rootPath_;
  options["web3clientVersion"] = web3clientVersion_;
  options["version"] = version_;
  options["chainID"] = chainID_;
  options["chainOwner"] = chainOwner_.hex(true);
  options["chainOwnerInitialBalance"] = chainOwnerInitialBalance_.str();
  options["httpPort"] = httpPort_;
  options["eventBlockCap"] = eventBlockCap_;
  options["eventLogCap"] = eventLogCap_;
  options["stateDumpTrigger"] = stateDumpTrigger_;
  options["indexingMode"] = indexingMode_.toString();
  options["cometBFT"] = cometBFT_;

  std::filesystem::create_directories(rootPath_);
  std::ofstream o(rootPath_ + "/options.json");
  o << options.dump(2) << std::endl;
  o.close();

  return true;
}
