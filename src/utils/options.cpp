#include "options.h"




Options::Options(const std::string& rootPath,
        const std::string& web3clientVersion,
        const uint64_t& version,
        const uint64_t& chainID,
        const uint16_t& wsPort,
        const uint16_t& httpPort) :
  rootPath(rootPath),
  web3clientVersion(web3clientVersion),
  version(version),
  chainID(chainID),
  wsPort(wsPort),
  httpPort(httpPort),
  coinbase(Address()),
  isValidator(false) {
  json options;
  if (std::filesystem::exists(rootPath + "/options.json")) return;

  options["rootPath"] = rootPath;
  options["web3clientVersion"] = web3clientVersion;
  options["version"] = version;
  options["chainID"] = chainID;
  options["wsPort"] = wsPort;
  options["httpPort"] = httpPort;
  options["isValidator"] = isValidator;
  std::filesystem::create_directories(rootPath);
  std::ofstream o(rootPath + "/options.json");
  o << options.dump(2) << std::endl;
  o.close();
}

Options::Options(const std::string& rootPath,
        const std::string& web3clientVersion,
        const uint64_t& version,
        const uint64_t& chainID,
        const uint16_t& wsPort,
        const uint16_t& httpPort,
        const PrivKey& privKey
) :
  rootPath(rootPath),
  web3clientVersion(web3clientVersion),
  version(version),
  chainID(chainID),
  wsPort(wsPort),
  httpPort(httpPort),
  coinbase(Secp256k1::toAddress(Secp256k1::toUPub(privKey))),
  isValidator(true) {
  if (std::filesystem::exists(rootPath + "/options.json")) return;

  json options;
  options["rootPath"] = rootPath;
  options["web3clientVersion"] = web3clientVersion;
  options["version"] = version;
  options["chainID"] = chainID;
  options["wsPort"] = wsPort;
  options["httpPort"] = httpPort;
  options["privKey"] = privKey.hex();
  std::filesystem::create_directories(rootPath);
  std::ofstream o(rootPath + "/options.json");
  o << options.dump(2) << std::endl;
  o.close();
}

Options Options::fromFile(const std::string& rootPath) {
  if (!std::filesystem::exists(rootPath + "/options.json")) {
    return Options(rootPath, "OrbiterSDK/cpp/linux_x86-64/0.0.1", 1, 8080, 8080, 8081);
  }

  std::ifstream i(rootPath + "/options.json");
  json options;
  i >> options;
  i.close();

  if (options.contains("privKey")) {
    const auto privKey = options["privKey"].get<std::string>();
    return Options(
      options["rootPath"].get<std::string>(),
      options["web3clientVersion"].get<std::string>(),
      options["version"].get<uint64_t>(),
      options["chainID"].get<uint64_t>(),
      options["wsPort"].get<uint64_t>(),
      options["httpPort"].get<uint64_t>(),
      PrivKey(Hex::toBytes(privKey))
    );
  }

  return Options(
    options["rootPath"].get<std::string>(),
    options["web3clientVersion"].get<std::string>(),
    options["version"].get<uint64_t>(),
    options["chainID"].get<uint64_t>(),
    options["wsPort"].get<uint64_t>(),
    options["httpPort"].get<uint64_t>()
  );
}