/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "options.h"

// Those are the "conceptual default" values for BDK to run
Options Options::genDefault(const std::string& rootPath) {
  // Generate default values for the CometBFT key
  json cometDefault = json::object();
  json cometDefaultValidator = json::object();

  cometDefaultValidator["address"] = "A146C3E02DB4F8AAD5E859E35F4F7BCC094F0B13";
  cometDefaultValidator["pub_key"] = json::object();
  cometDefaultValidator["pub_key"]["type"] = "tendermint/PubKeySecp256k1";
  cometDefaultValidator["pub_key"]["value"] = "AiA6uTAC2S62d1DcwRAAj0hSosCdkCa1aTWlXWJeFA9W";
  cometDefaultValidator["power"] = "10";
  cometDefaultValidator["name"] = "node0";

  cometDefault["genesis.json"] = json::object();
  cometDefault["genesis.json"]["genesis_time"] = "2024-09-17T18:26:34.583377166Z";
  cometDefault["genesis.json"]["initial_height"] = "0";
  cometDefault["genesis.json"]["consensus_params"] = json::object();
  cometDefault["genesis.json"]["consensus_params"]["block"] = json::object();
  cometDefault["genesis.json"]["consensus_params"]["block"]["max_bytes"] = "22020096";
  cometDefault["genesis.json"]["consensus_params"]["block"]["max_gas"] = "-1";
  cometDefault["genesis.json"]["consensus_params"]["evidence"] = json::object();
  cometDefault["genesis.json"]["consensus_params"]["evidence"]["max_age_num_blocks"] = "100000";
  cometDefault["genesis.json"]["consensus_params"]["evidence"]["max_age_duration"] = "172800000000000";
  cometDefault["genesis.json"]["consensus_params"]["evidence"]["max_bytes"] = "1048576";
  cometDefault["genesis.json"]["consensus_params"]["validator"] = json::object();
  cometDefault["genesis.json"]["consensus_params"]["validator"]["pub_key_types"] = json::array({"secp256k1"});
  cometDefault["genesis.json"]["consensus_params"]["version"] = json::object();
  cometDefault["genesis.json"]["consensus_params"]["version"]["app"] = "0";
  cometDefault["genesis.json"]["consensus_params"]["abci"] = json::object();
  cometDefault["genesis.json"]["consensus_params"]["abci"]["vote_extensions_enable_height"] = "0";
  cometDefault["genesis.json"]["validators"] = json::array({cometDefaultValidator});
  cometDefault["genesis.json"]["app_hash"] = "";

  cometDefault["node_key.json"] = json::object();
  cometDefault["node_key.json"]["priv_key"] = json::object();
  cometDefault["node_key.json"]["priv_key"]["type"] = "tendermint/PrivKeyEd25519";
  cometDefault["node_key.json"]["priv_key"]["value"] = "GKZ5kO56LhcaeRrOIefJtA2ogaPxQw+R6xBiznQD+290PZ/N5ZbBwCa9DoVA7FIeUeNofpHLtFK4UE0ACep5oA==";

  cometDefault["priv_validator_key.json"] = json::object();
  cometDefault["priv_validator_key.json"]["address"] = "A146C3E02DB4F8AAD5E859E35F4F7BCC094F0B13";
  cometDefault["priv_validator_key.json"]["pub_key"] = json::object();
  cometDefault["priv_validator_key.json"]["pub_key"]["type"] = "tendermint/PubKeySecp256k1";
  cometDefault["priv_validator_key.json"]["pub_key"]["value"] = "AiA6uTAC2S62d1DcwRAAj0hSosCdkCa1aTWlXWJeFA9W";
  cometDefault["priv_validator_key.json"]["priv_key"] = json::object();
  cometDefault["priv_validator_key.json"]["priv_key"]["type"] = "tendermint/PrivKeySecp256k1";
  cometDefault["priv_validator_key.json"]["priv_key"]["value"] = "+8+j8W0W3B9H68JbLoUTieIU4aNWjsumkuU8fQPN6tY=";

  cometDefault["config.toml"] = json::object();
  cometDefault["config.toml"]["p2p"] = json::object();
  cometDefault["config.toml"]["p2p"]["laddr"] = "tcp://0.0.0.0:20001";
  cometDefault["config.toml"]["p2p"]["allow_duplicate_ip"] = true;
  cometDefault["config.toml"]["p2p"]["addr_book_strict"] = false;
  cometDefault["config.toml"]["rpc"] = json::object();
  cometDefault["config.toml"]["rpc"]["laddr"] = "tcp://0.0.0.0:20002";

  // TODO: web3ClientVersion (second line) should pick the version from the CMake macro instead of being hardcoded
  return {
    rootPath,
    "BDK/cpp/linux_x86-64/0.2.0",
    2,
    8080,
    Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")),
    8081,
    2000,
    10000,
    1000,
    IndexingMode::RPC,
    cometDefault
  };
}

