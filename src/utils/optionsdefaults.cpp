/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "options.h"

Options Options::binaryDefaultOptions(const std::string& rootPath) {
  MutableBlock genesis(Hash(), 0, 0);
  PrivKey genesisSigner(Hex::toBytes("0x4d48bdf34d65ef2bed2e4ee9020a7d3162b494ac31d3088153425f286f3d3c8c"));
  FinalizedBlock genesisFinal = genesis.finalize(genesisSigner, 1656356646000000);
  std::vector<std::pair<Address, uint256_t>> genesisBalanceList;
  genesisBalanceList.emplace_back(
    Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")), uint256_t("1000000000000000000000")
  );
  std::vector<Address> genesisValidators;
  genesisValidators.push_back(Address(Hex::toBytes("0x7588b0f553d1910266089c58822e1120db47e572")));
  genesisValidators.push_back(Address(Hex::toBytes("0xcabf34a268847a610287709d841e5cd590cc5c00")));
  genesisValidators.push_back(Address(Hex::toBytes("0x5fb516dc2cfc1288e689ed377a9eebe2216cf1e3")));
  genesisValidators.push_back(Address(Hex::toBytes("0x795083c42583842774febc21abb6df09e784fce5")));
  genesisValidators.push_back(Address(Hex::toBytes("0xbec7b74f70c151707a0bfb20fe3767c6e65499e0")));

  /**
   * rootPath,
   * web3clientVersion,
   * version,
   * chainID,
   * chainOwner,
   * wsPort,
   * httpPort,
   * minDiscoveryConns,
   * minNormalConns,
   * maxDiscoveryConns,
   * maxNormalConns,
   * eventBlockCap,
   * eventLogCap,
   * minValidators,
   * discoveryNodes,
   * genesisBlock,
   * genesisTimestamp,
   * genesisSigner,
   * genesisBalances,
   * genesisValidators
   */
  return {
    rootPath,
    "BDK/cpp/linux_x86-64/0.2.0",
    2,
    8080,
    Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")),
    8080,
    8081,
    11,
    11,
    200,
    50,
    2000,
    10000,
    1000,
    4,
    {},
    genesisFinal,
    genesisFinal.getTimestamp(),
    genesisSigner,
    genesisBalanceList,
    genesisValidators
  };
}

