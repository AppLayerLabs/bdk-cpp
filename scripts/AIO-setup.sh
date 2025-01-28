# Copyright (c) [2023-2024] [AppLayer Developers]
# This software is distributed under the MIT License.
# See the LICENSE.txt file in the project root for more information.

## A given network has a minimum requirement of 5 Validators, 6 Normal Nodes and 1 Discovery Node.
## Discovery Nodes are described within the options.json file.
## This script is meant to be executed within the   main directory of this project
## It will create a new directory, called "local_testnet" and will create
## New directories for each Validator and Discovery Node created (5 and 1 respectively)
## This script is also meant to build the project within a new folder called "build_local_testnet"
## It does not clean the build folder, only the local_testnet folder

# Kill the tmux terminals "local_testnet_validatorX" and "local_testnet_discovery"
tmux kill-session -t local_testnet_validator1
tmux kill-session -t local_testnet_validator2
tmux kill-session -t local_testnet_validator3
tmux kill-session -t local_testnet_validator4
tmux kill-session -t local_testnet_validator5
tmux kill-session -t local_testnet_normal1
tmux kill-session -t local_testnet_normal2
tmux kill-session -t local_testnet_normal3
tmux kill-session -t local_testnet_normal4
tmux kill-session -t local_testnet_normal5
tmux kill-session -t local_testnet_normal6
tmux kill-session -t local_testnet_discovery

# Set default values
CLEAN=false # Clean the build folder
DEPLOY=true # Deploy the executables to the local_testnet folder
ONLY_DEPLOY=false # Only deploy, do not build
DEBUG=OFF # Build the project in debug mode
CORES=$(grep -c ^processor /proc/cpuinfo) # Number of cores for parallel build

for arg in "$@"
do
  case $arg in
    --clean)
      CLEAN=true
      shift
      ;;
    --no-deploy)
      DEPLOY=false
      shift
      ;;
    --only-deploy)
      ONLY_DEPLOY=true
      DEPLOY=true
      shift
      ;;
    --debug=*)
      DEBUG="${arg#*=}"
      shift
      ;;
    --cores=*)
      CORES="${arg#*=}"
      shift
      ;;
    *)
      shift
      ;;
  esac
done

#If no-deploy and only-deploy are passed, exit
if [ "$DEPLOY" = false ] && [ "$ONLY_DEPLOY" = true ]; then
  echo "Please run this script with either the --no-deploy flag or the --only-deploy flag"
  exit 1
fi

# Check if the user is running this script from the main directory of the project
if [ ! -f "scripts/AIO-setup.sh" ]; then
  echo "Please run this script from the main directory of the project"
  exit 1
fi

# If only-deploy is true and build_local_testnet doesn't exist, exit
if [ "$ONLY_DEPLOY" = true ]; then
  if [ ! -d "build_local_testnet" ]; then
    echo "Please run this script with the --only-deploy flag after compiling the project"
    exit 1
  fi
fi

# Clean the build folder if the user specified the --clean flag
if [ "$CLEAN" = true ]; then
  if [ -d "build_local_testnet" ]; then
    rm -rf build_local_testnet
  fi
  echo "Running a clean build"
fi

# Create the new directory "build_local_testnet" if it doesn't exist
if [ ! -d "build_local_testnet" ]; then
  mkdir build_local_testnet
fi

# Create the new directory "local_testnet", empty it if it already exists
if [ -d "local_testnet" ]; then
  rm -rf local_testnet
fi
mkdir local_testnet

if [ "$ONLY_DEPLOY" = false ]; then
  # Build the project
  cd build_local_testnet
  cmake -DDEBUG=$DEBUG -DBUILD_TESTS=OFF -DBUILD_VARIABLES_TESTS=OFF ..
  make -j${CORES}
fi

if [ "$DEPLOY" = true ]; then
  if [ "$ONLY_DEPLOY" = true ]; then
    cd build_local_testnet
  fi
  # Copy the bdkd and bdkd-discovery executables to the local_testnet directory
  cp src/bins/bdkd/bdkd ../local_testnet
  cp src/bins/bdkd-discovery/bdkd-discovery ../local_testnet

  # Create the directories for the Validators and Discovery Node and copy the executables
  cd ../local_testnet
  for i in $(seq 1 5); do
    mkdir local_testnet_validator$i
    mkdir local_testnet_validator$i/blockchain
    cp bdkd local_testnet_validator$i
  done
  for i in $(seq 1 6); do
    mkdir local_testnet_normal$i
    mkdir local_testnet_normal$i/blockchain
    cp bdkd local_testnet_normal$i
  done
  mkdir local_testnet_discovery
  mkdir local_testnet_discovery/discoveryNode
  cp bdkd-discovery local_testnet_discovery

  # Create the JSON files for the Discovery Node, Validators and Normal Nodes
  # TODO: do something better than this enormous wall of text
  echo '{
    "rootPath": "discoveryNode",
    "web3clientVersion": "bdk/cpp/linux_x86-64/0.2.0",
    "version": 1,
    "chainID": 808080,
    "httpPort": 9999,
    "eventBlockCap": 2000,
    "eventLogCap": 10000,
    "stateDumpTrigger": 1000,
    "chainOwner": "0x00dead00665771855a34155f5e7405489df2c3c6",
    "indexingMode": "RPC",
    "cometBFT": {
      "genesis.json": {
        "genesis_time": "2024-09-17T18:26:34.583377166Z",
        "initial_height": "0",
        "consensus_params": {
          "block": {
            "max_bytes": "22020096",
            "max_gas": "-1"
          },
          "evidence": {
            "max_age_num_blocks": "100000",
            "max_age_duration": "172800000000000",
            "max_bytes": "1048576"
          },
          "validator": {
            "pub_key_types": [
              "secp256k1"
            ]
          },
          "version": {
            "app": "0"
          },
          "abci": {
            "vote_extensions_enable_height": "0"
          }
        },
        "validators": [
          {
            "address": "A146C3E02DB4F8AAD5E859E35F4F7BCC094F0B13",
            "pub_key": {
              "type": "tendermint/PubKeySecp256k1",
              "value": "AiA6uTAC2S62d1DcwRAAj0hSosCdkCa1aTWlXWJeFA9W"
            },
            "power": "10",
            "name": "node0"
          }
        ],
        "app_hash": ""
      },
      "node_key.json": {
        "priv_key": {
          "type": "tendermint/PrivKeyEd25519",
          "value": "GKZ5kO56LhcaeRrOIefJtA2ogaPxQw+R6xBiznQD+290PZ/N5ZbBwCa9DoVA7FIeUeNofpHLtFK4UE0ACep5oA=="
        }
      },
      "priv_validator_key.json": {
        "address": "A146C3E02DB4F8AAD5E859E35F4F7BCC094F0B13",
        "pub_key": {
          "type": "tendermint/PubKeySecp256k1",
          "value": "AiA6uTAC2S62d1DcwRAAj0hSosCdkCa1aTWlXWJeFA9W"
        },
        "priv_key": {
          "type": "tendermint/PrivKeySecp256k1",
          "value": "+8+j8W0W3B9H68JbLoUTieIU4aNWjsumkuU8fQPN6tY="
        }
      },
      "config.toml": {
        "p2p": {
          "laddr": "tcp://0.0.0.0:20001",
          "allow_duplicate_ip": true,
          "addr_book_strict": false
        },
        "rpc": {
          "laddr": "tcp://0.0.0.0:20002"
        }
      }
    }
  }' >> local_testnet_discovery/discoveryNode/options.json

  # Create the JSON file for the Validators
  echo '{
    "rootPath": "blockchain",
    "web3clientVersion": "bdk/cpp/linux_x86-64/0.2.0",
    "version": 1,
    "chainID": 808080,
    "httpPort": 8090,
    "eventBlockCap": 2000,
    "eventLogCap": 10000,
    "stateDumpTrigger": 1000,
    "chainOwner": "0x00dead00665771855a34155f5e7405489df2c3c6",
    "indexingMode": "RPC_TRACE",
    "cometBFT": {
      "genesis.json": {
        "genesis_time": "2024-09-17T18:26:34.583377166Z",
        "initial_height": "0",
        "consensus_params": {
          "block": {
            "max_bytes": "22020096",
            "max_gas": "-1"
          },
          "evidence": {
            "max_age_num_blocks": "100000",
            "max_age_duration": "172800000000000",
            "max_bytes": "1048576"
          },
          "validator": {
            "pub_key_types": [
              "secp256k1"
            ]
          },
          "version": {
            "app": "0"
          },
          "abci": {
            "vote_extensions_enable_height": "0"
          }
        },
        "validators": [
          {
            "address": "A146C3E02DB4F8AAD5E859E35F4F7BCC094F0B13",
            "pub_key": {
              "type": "tendermint/PubKeySecp256k1",
              "value": "AiA6uTAC2S62d1DcwRAAj0hSosCdkCa1aTWlXWJeFA9W"
            },
            "power": "10",
            "name": "node0"
          }
        ],
        "app_hash": ""
      },
      "node_key.json": {
        "priv_key": {
          "type": "tendermint/PrivKeyEd25519",
          "value": "GKZ5kO56LhcaeRrOIefJtA2ogaPxQw+R6xBiznQD+290PZ/N5ZbBwCa9DoVA7FIeUeNofpHLtFK4UE0ACep5oA=="
        }
      },
      "priv_validator_key.json": {
        "address": "A146C3E02DB4F8AAD5E859E35F4F7BCC094F0B13",
        "pub_key": {
          "type": "tendermint/PubKeySecp256k1",
          "value": "AiA6uTAC2S62d1DcwRAAj0hSosCdkCa1aTWlXWJeFA9W"
        },
        "priv_key": {
          "type": "tendermint/PrivKeySecp256k1",
          "value": "+8+j8W0W3B9H68JbLoUTieIU4aNWjsumkuU8fQPN6tY="
        }
      },
      "config.toml": {
        "p2p": {
          "laddr": "tcp://0.0.0.0:20001",
          "allow_duplicate_ip": true,
          "addr_book_strict": false
        },
        "rpc": {
          "laddr": "tcp://0.0.0.0:20002"
        }
      }
    }
  }' >> local_testnet_validator1/blockchain/options.json

  echo '{
    "rootPath": "blockchain",
    "web3clientVersion": "bdk/cpp/linux_x86-64/0.2.0",
    "version": 1,
    "chainID": 808080,
    "httpPort": 8091,
    "eventBlockCap": 2000,
    "eventLogCap": 10000,
    "stateDumpTrigger": 1000,
    "chainOwner": "0x00dead00665771855a34155f5e7405489df2c3c6",
    "indexingMode": "RPC_TRACE",
    "cometBFT": {
      "genesis.json": {
        "genesis_time": "2024-09-17T18:26:34.583377166Z",
        "initial_height": "0",
        "consensus_params": {
          "block": {
            "max_bytes": "22020096",
            "max_gas": "-1"
          },
          "evidence": {
            "max_age_num_blocks": "100000",
            "max_age_duration": "172800000000000",
            "max_bytes": "1048576"
          },
          "validator": {
            "pub_key_types": [
              "secp256k1"
            ]
          },
          "version": {
            "app": "0"
          },
          "abci": {
            "vote_extensions_enable_height": "0"
          }
        },
        "validators": [
          {
            "address": "A146C3E02DB4F8AAD5E859E35F4F7BCC094F0B13",
            "pub_key": {
              "type": "tendermint/PubKeySecp256k1",
              "value": "AiA6uTAC2S62d1DcwRAAj0hSosCdkCa1aTWlXWJeFA9W"
            },
            "power": "10",
            "name": "node0"
          }
        ],
        "app_hash": ""
      },
      "node_key.json": {
        "priv_key": {
          "type": "tendermint/PrivKeyEd25519",
          "value": "GKZ5kO56LhcaeRrOIefJtA2ogaPxQw+R6xBiznQD+290PZ/N5ZbBwCa9DoVA7FIeUeNofpHLtFK4UE0ACep5oA=="
        }
      },
      "priv_validator_key.json": {
        "address": "A146C3E02DB4F8AAD5E859E35F4F7BCC094F0B13",
        "pub_key": {
          "type": "tendermint/PubKeySecp256k1",
          "value": "AiA6uTAC2S62d1DcwRAAj0hSosCdkCa1aTWlXWJeFA9W"
        },
        "priv_key": {
          "type": "tendermint/PrivKeySecp256k1",
          "value": "+8+j8W0W3B9H68JbLoUTieIU4aNWjsumkuU8fQPN6tY="
        }
      },
      "config.toml": {
        "p2p": {
          "laddr": "tcp://0.0.0.0:20001",
          "allow_duplicate_ip": true,
          "addr_book_strict": false
        },
        "rpc": {
          "laddr": "tcp://0.0.0.0:20002"
        }
      }
    }
  }' >> local_testnet_validator2/blockchain/options.json

  echo '{
    "rootPath": "blockchain",
    "web3clientVersion": "bdk/cpp/linux_x86-64/0.2.0",
    "version": 1,
    "chainID": 808080,
    "httpPort": 8092,
    "eventBlockCap": 2000,
    "eventLogCap": 10000,
    "stateDumpTrigger": 1000,
    "chainOwner": "0x00dead00665771855a34155f5e7405489df2c3c6",
    "indexingMode": "RPC",
    "cometBFT": {
      "genesis.json": {
        "genesis_time": "2024-09-17T18:26:34.583377166Z",
        "initial_height": "0",
        "consensus_params": {
          "block": {
            "max_bytes": "22020096",
            "max_gas": "-1"
          },
          "evidence": {
            "max_age_num_blocks": "100000",
            "max_age_duration": "172800000000000",
            "max_bytes": "1048576"
          },
          "validator": {
            "pub_key_types": [
              "secp256k1"
            ]
          },
          "version": {
            "app": "0"
          },
          "abci": {
            "vote_extensions_enable_height": "0"
          }
        },
        "validators": [
          {
            "address": "A146C3E02DB4F8AAD5E859E35F4F7BCC094F0B13",
            "pub_key": {
              "type": "tendermint/PubKeySecp256k1",
              "value": "AiA6uTAC2S62d1DcwRAAj0hSosCdkCa1aTWlXWJeFA9W"
            },
            "power": "10",
            "name": "node0"
          }
        ],
        "app_hash": ""
      },
      "node_key.json": {
        "priv_key": {
          "type": "tendermint/PrivKeyEd25519",
          "value": "GKZ5kO56LhcaeRrOIefJtA2ogaPxQw+R6xBiznQD+290PZ/N5ZbBwCa9DoVA7FIeUeNofpHLtFK4UE0ACep5oA=="
        }
      },
      "priv_validator_key.json": {
        "address": "A146C3E02DB4F8AAD5E859E35F4F7BCC094F0B13",
        "pub_key": {
          "type": "tendermint/PubKeySecp256k1",
          "value": "AiA6uTAC2S62d1DcwRAAj0hSosCdkCa1aTWlXWJeFA9W"
        },
        "priv_key": {
          "type": "tendermint/PrivKeySecp256k1",
          "value": "+8+j8W0W3B9H68JbLoUTieIU4aNWjsumkuU8fQPN6tY="
        }
      },
      "config.toml": {
        "p2p": {
          "laddr": "tcp://0.0.0.0:20001",
          "allow_duplicate_ip": true,
          "addr_book_strict": false
        },
        "rpc": {
          "laddr": "tcp://0.0.0.0:20002"
        }
      }
    }
  }' >> local_testnet_validator3/blockchain/options.json

  echo '{
    "rootPath": "blockchain",
    "web3clientVersion": "bdk/cpp/linux_x86-64/0.2.0",
    "version": 1,
    "chainID": 808080,
    "httpPort": 8093,
    "eventBlockCap": 2000,
    "eventLogCap": 10000,
    "stateDumpTrigger": 1000,
    "chainOwner": "0x00dead00665771855a34155f5e7405489df2c3c6",
    "indexingMode": "RPC",
    "cometBFT": {
      "genesis.json": {
        "genesis_time": "2024-09-17T18:26:34.583377166Z",
        "initial_height": "0",
        "consensus_params": {
          "block": {
            "max_bytes": "22020096",
            "max_gas": "-1"
          },
          "evidence": {
            "max_age_num_blocks": "100000",
            "max_age_duration": "172800000000000",
            "max_bytes": "1048576"
          },
          "validator": {
            "pub_key_types": [
              "secp256k1"
            ]
          },
          "version": {
            "app": "0"
          },
          "abci": {
            "vote_extensions_enable_height": "0"
          }
        },
        "validators": [
          {
            "address": "A146C3E02DB4F8AAD5E859E35F4F7BCC094F0B13",
            "pub_key": {
              "type": "tendermint/PubKeySecp256k1",
              "value": "AiA6uTAC2S62d1DcwRAAj0hSosCdkCa1aTWlXWJeFA9W"
            },
            "power": "10",
            "name": "node0"
          }
        ],
        "app_hash": ""
      },
      "node_key.json": {
        "priv_key": {
          "type": "tendermint/PrivKeyEd25519",
          "value": "GKZ5kO56LhcaeRrOIefJtA2ogaPxQw+R6xBiznQD+290PZ/N5ZbBwCa9DoVA7FIeUeNofpHLtFK4UE0ACep5oA=="
        }
      },
      "priv_validator_key.json": {
        "address": "A146C3E02DB4F8AAD5E859E35F4F7BCC094F0B13",
        "pub_key": {
          "type": "tendermint/PubKeySecp256k1",
          "value": "AiA6uTAC2S62d1DcwRAAj0hSosCdkCa1aTWlXWJeFA9W"
        },
        "priv_key": {
          "type": "tendermint/PrivKeySecp256k1",
          "value": "+8+j8W0W3B9H68JbLoUTieIU4aNWjsumkuU8fQPN6tY="
        }
      },
      "config.toml": {
        "p2p": {
          "laddr": "tcp://0.0.0.0:20001",
          "allow_duplicate_ip": true,
          "addr_book_strict": false
        },
        "rpc": {
          "laddr": "tcp://0.0.0.0:20002"
        }
      }
    }
  }' >> local_testnet_validator4/blockchain/options.json

  echo '{
    "rootPath": "blockchain",
    "web3clientVersion": "bdk/cpp/linux_x86-64/0.2.0",
    "version": 1,
    "chainID": 808080,
    "httpPort": 8094,
    "eventBlockCap": 2000,
    "eventLogCap": 10000,
    "stateDumpTrigger": 1000,
    "chainOwner": "0x00dead00665771855a34155f5e7405489df2c3c6",
    "indexingMode": "RPC",
    "cometBFT": {
      "genesis.json": {
        "genesis_time": "2024-09-17T18:26:34.583377166Z",
        "initial_height": "0",
        "consensus_params": {
          "block": {
            "max_bytes": "22020096",
            "max_gas": "-1"
          },
          "evidence": {
            "max_age_num_blocks": "100000",
            "max_age_duration": "172800000000000",
            "max_bytes": "1048576"
          },
          "validator": {
            "pub_key_types": [
              "secp256k1"
            ]
          },
          "version": {
            "app": "0"
          },
          "abci": {
            "vote_extensions_enable_height": "0"
          }
        },
        "validators": [
          {
            "address": "A146C3E02DB4F8AAD5E859E35F4F7BCC094F0B13",
            "pub_key": {
              "type": "tendermint/PubKeySecp256k1",
              "value": "AiA6uTAC2S62d1DcwRAAj0hSosCdkCa1aTWlXWJeFA9W"
            },
            "power": "10",
            "name": "node0"
          }
        ],
        "app_hash": ""
      },
      "node_key.json": {
        "priv_key": {
          "type": "tendermint/PrivKeyEd25519",
          "value": "GKZ5kO56LhcaeRrOIefJtA2ogaPxQw+R6xBiznQD+290PZ/N5ZbBwCa9DoVA7FIeUeNofpHLtFK4UE0ACep5oA=="
        }
      },
      "priv_validator_key.json": {
        "address": "A146C3E02DB4F8AAD5E859E35F4F7BCC094F0B13",
        "pub_key": {
          "type": "tendermint/PubKeySecp256k1",
          "value": "AiA6uTAC2S62d1DcwRAAj0hSosCdkCa1aTWlXWJeFA9W"
        },
        "priv_key": {
          "type": "tendermint/PrivKeySecp256k1",
          "value": "+8+j8W0W3B9H68JbLoUTieIU4aNWjsumkuU8fQPN6tY="
        }
      },
      "config.toml": {
        "p2p": {
          "laddr": "tcp://0.0.0.0:20001",
          "allow_duplicate_ip": true,
          "addr_book_strict": false
        },
        "rpc": {
          "laddr": "tcp://0.0.0.0:20002"
        }
      }
    }
  }' >> local_testnet_validator5/blockchain/options.json

  # Create the json file for the Normal Nodes
  echo '{
    "rootPath": "blockchain",
    "web3clientVersion": "bdk/cpp/linux_x86-64/0.2.0",
    "version": 1,
    "chainID": 808080,
    "httpPort": 8095,
    "eventBlockCap": 2000,
    "eventLogCap": 10000,
    "stateDumpTrigger": 1000,
    "chainOwner": "0x00dead00665771855a34155f5e7405489df2c3c6",
    "indexingMode": "RPC",
    "cometBFT": {
      "genesis.json": {
        "genesis_time": "2024-09-17T18:26:34.583377166Z",
        "initial_height": "0",
        "consensus_params": {
          "block": {
            "max_bytes": "22020096",
            "max_gas": "-1"
          },
          "evidence": {
            "max_age_num_blocks": "100000",
            "max_age_duration": "172800000000000",
            "max_bytes": "1048576"
          },
          "validator": {
            "pub_key_types": [
              "secp256k1"
            ]
          },
          "version": {
            "app": "0"
          },
          "abci": {
            "vote_extensions_enable_height": "0"
          }
        },
        "validators": [
          {
            "address": "A146C3E02DB4F8AAD5E859E35F4F7BCC094F0B13",
            "pub_key": {
              "type": "tendermint/PubKeySecp256k1",
              "value": "AiA6uTAC2S62d1DcwRAAj0hSosCdkCa1aTWlXWJeFA9W"
            },
            "power": "10",
            "name": "node0"
          }
        ],
        "app_hash": ""
      },
      "node_key.json": {
        "priv_key": {
          "type": "tendermint/PrivKeyEd25519",
          "value": "GKZ5kO56LhcaeRrOIefJtA2ogaPxQw+R6xBiznQD+290PZ/N5ZbBwCa9DoVA7FIeUeNofpHLtFK4UE0ACep5oA=="
        }
      },
      "priv_validator_key.json": {
        "address": "A146C3E02DB4F8AAD5E859E35F4F7BCC094F0B13",
        "pub_key": {
          "type": "tendermint/PubKeySecp256k1",
          "value": "AiA6uTAC2S62d1DcwRAAj0hSosCdkCa1aTWlXWJeFA9W"
        },
        "priv_key": {
          "type": "tendermint/PrivKeySecp256k1",
          "value": "+8+j8W0W3B9H68JbLoUTieIU4aNWjsumkuU8fQPN6tY="
        }
      },
      "config.toml": {
        "p2p": {
          "laddr": "tcp://0.0.0.0:20001",
          "allow_duplicate_ip": true,
          "addr_book_strict": false
        },
        "rpc": {
          "laddr": "tcp://0.0.0.0:20002"
        }
      }
    }
  }' >> local_testnet_normal1/blockchain/options.json

  echo '{
    "rootPath": "blockchain",
    "web3clientVersion": "bdk/cpp/linux_x86-64/0.2.0",
    "version": 1,
    "chainID": 808080,
    "httpPort": 8096,
    "eventBlockCap": 2000,
    "eventLogCap": 10000,
    "stateDumpTrigger": 1000,
    "chainOwner": "0x00dead00665771855a34155f5e7405489df2c3c6",
    "indexingMode": "RPC",
    "cometBFT": {
      "genesis.json": {
        "genesis_time": "2024-09-17T18:26:34.583377166Z",
        "initial_height": "0",
        "consensus_params": {
          "block": {
            "max_bytes": "22020096",
            "max_gas": "-1"
          },
          "evidence": {
            "max_age_num_blocks": "100000",
            "max_age_duration": "172800000000000",
            "max_bytes": "1048576"
          },
          "validator": {
            "pub_key_types": [
              "secp256k1"
            ]
          },
          "version": {
            "app": "0"
          },
          "abci": {
            "vote_extensions_enable_height": "0"
          }
        },
        "validators": [
          {
            "address": "A146C3E02DB4F8AAD5E859E35F4F7BCC094F0B13",
            "pub_key": {
              "type": "tendermint/PubKeySecp256k1",
              "value": "AiA6uTAC2S62d1DcwRAAj0hSosCdkCa1aTWlXWJeFA9W"
            },
            "power": "10",
            "name": "node0"
          }
        ],
        "app_hash": ""
      },
      "node_key.json": {
        "priv_key": {
          "type": "tendermint/PrivKeyEd25519",
          "value": "GKZ5kO56LhcaeRrOIefJtA2ogaPxQw+R6xBiznQD+290PZ/N5ZbBwCa9DoVA7FIeUeNofpHLtFK4UE0ACep5oA=="
        }
      },
      "priv_validator_key.json": {
        "address": "A146C3E02DB4F8AAD5E859E35F4F7BCC094F0B13",
        "pub_key": {
          "type": "tendermint/PubKeySecp256k1",
          "value": "AiA6uTAC2S62d1DcwRAAj0hSosCdkCa1aTWlXWJeFA9W"
        },
        "priv_key": {
          "type": "tendermint/PrivKeySecp256k1",
          "value": "+8+j8W0W3B9H68JbLoUTieIU4aNWjsumkuU8fQPN6tY="
        }
      },
      "config.toml": {
        "p2p": {
          "laddr": "tcp://0.0.0.0:20001",
          "allow_duplicate_ip": true,
          "addr_book_strict": false
        },
        "rpc": {
          "laddr": "tcp://0.0.0.0:20002"
        }
      }
    }
  }' >> local_testnet_normal2/blockchain/options.json

  echo '{
    "rootPath": "blockchain",
    "web3clientVersion": "bdk/cpp/linux_x86-64/0.2.0",
    "version": 1,
    "chainID": 808080,
    "httpPort": 8097,
    "eventBlockCap": 2000,
    "eventLogCap": 10000,
    "stateDumpTrigger": 1000,
    "chainOwner": "0x00dead00665771855a34155f5e7405489df2c3c6",
    "indexingMode": "RPC",
    "cometBFT": {
      "genesis.json": {
        "genesis_time": "2024-09-17T18:26:34.583377166Z",
        "initial_height": "0",
        "consensus_params": {
          "block": {
            "max_bytes": "22020096",
            "max_gas": "-1"
          },
          "evidence": {
            "max_age_num_blocks": "100000",
            "max_age_duration": "172800000000000",
            "max_bytes": "1048576"
          },
          "validator": {
            "pub_key_types": [
              "secp256k1"
            ]
          },
          "version": {
            "app": "0"
          },
          "abci": {
            "vote_extensions_enable_height": "0"
          }
        },
        "validators": [
          {
            "address": "A146C3E02DB4F8AAD5E859E35F4F7BCC094F0B13",
            "pub_key": {
              "type": "tendermint/PubKeySecp256k1",
              "value": "AiA6uTAC2S62d1DcwRAAj0hSosCdkCa1aTWlXWJeFA9W"
            },
            "power": "10",
            "name": "node0"
          }
        ],
        "app_hash": ""
      },
      "node_key.json": {
        "priv_key": {
          "type": "tendermint/PrivKeyEd25519",
          "value": "GKZ5kO56LhcaeRrOIefJtA2ogaPxQw+R6xBiznQD+290PZ/N5ZbBwCa9DoVA7FIeUeNofpHLtFK4UE0ACep5oA=="
        }
      },
      "priv_validator_key.json": {
        "address": "A146C3E02DB4F8AAD5E859E35F4F7BCC094F0B13",
        "pub_key": {
          "type": "tendermint/PubKeySecp256k1",
          "value": "AiA6uTAC2S62d1DcwRAAj0hSosCdkCa1aTWlXWJeFA9W"
        },
        "priv_key": {
          "type": "tendermint/PrivKeySecp256k1",
          "value": "+8+j8W0W3B9H68JbLoUTieIU4aNWjsumkuU8fQPN6tY="
        }
      },
      "config.toml": {
        "p2p": {
          "laddr": "tcp://0.0.0.0:20001",
          "allow_duplicate_ip": true,
          "addr_book_strict": false
        },
        "rpc": {
          "laddr": "tcp://0.0.0.0:20002"
        }
      }
    }
  }' >> local_testnet_normal3/blockchain/options.json

  echo '{
    "rootPath": "blockchain",
    "web3clientVersion": "bdk/cpp/linux_x86-64/0.2.0",
    "version": 1,
    "chainID": 808080,
    "httpPort": 8098,
    "eventBlockCap": 2000,
    "eventLogCap": 10000,
    "stateDumpTrigger": 1000,
    "chainOwner": "0x00dead00665771855a34155f5e7405489df2c3c6",
    "indexingMode": "RPC",
    "cometBFT": {
      "genesis.json": {
        "genesis_time": "2024-09-17T18:26:34.583377166Z",
        "initial_height": "0",
        "consensus_params": {
          "block": {
            "max_bytes": "22020096",
            "max_gas": "-1"
          },
          "evidence": {
            "max_age_num_blocks": "100000",
            "max_age_duration": "172800000000000",
            "max_bytes": "1048576"
          },
          "validator": {
            "pub_key_types": [
              "secp256k1"
            ]
          },
          "version": {
            "app": "0"
          },
          "abci": {
            "vote_extensions_enable_height": "0"
          }
        },
        "validators": [
          {
            "address": "A146C3E02DB4F8AAD5E859E35F4F7BCC094F0B13",
            "pub_key": {
              "type": "tendermint/PubKeySecp256k1",
              "value": "AiA6uTAC2S62d1DcwRAAj0hSosCdkCa1aTWlXWJeFA9W"
            },
            "power": "10",
            "name": "node0"
          }
        ],
        "app_hash": ""
      },
      "node_key.json": {
        "priv_key": {
          "type": "tendermint/PrivKeyEd25519",
          "value": "GKZ5kO56LhcaeRrOIefJtA2ogaPxQw+R6xBiznQD+290PZ/N5ZbBwCa9DoVA7FIeUeNofpHLtFK4UE0ACep5oA=="
        }
      },
      "priv_validator_key.json": {
        "address": "A146C3E02DB4F8AAD5E859E35F4F7BCC094F0B13",
        "pub_key": {
          "type": "tendermint/PubKeySecp256k1",
          "value": "AiA6uTAC2S62d1DcwRAAj0hSosCdkCa1aTWlXWJeFA9W"
        },
        "priv_key": {
          "type": "tendermint/PrivKeySecp256k1",
          "value": "+8+j8W0W3B9H68JbLoUTieIU4aNWjsumkuU8fQPN6tY="
        }
      },
      "config.toml": {
        "p2p": {
          "laddr": "tcp://0.0.0.0:20001",
          "allow_duplicate_ip": true,
          "addr_book_strict": false
        },
        "rpc": {
          "laddr": "tcp://0.0.0.0:20002"
        }
      }
    }
  }' >> local_testnet_normal4/blockchain/options.json

  echo '{
    "rootPath": "blockchain",
    "web3clientVersion": "bdk/cpp/linux_x86-64/0.2.0",
    "version": 1,
    "chainID": 808080,
    "httpPort": 8099,
    "eventBlockCap": 2000,
    "eventLogCap": 10000,
    "stateDumpTrigger": 1000,
    "chainOwner": "0x00dead00665771855a34155f5e7405489df2c3c6",
    "indexingMode": "RPC",
    "cometBFT": {
      "genesis.json": {
        "genesis_time": "2024-09-17T18:26:34.583377166Z",
        "initial_height": "0",
        "consensus_params": {
          "block": {
            "max_bytes": "22020096",
            "max_gas": "-1"
          },
          "evidence": {
            "max_age_num_blocks": "100000",
            "max_age_duration": "172800000000000",
            "max_bytes": "1048576"
          },
          "validator": {
            "pub_key_types": [
              "secp256k1"
            ]
          },
          "version": {
            "app": "0"
          },
          "abci": {
            "vote_extensions_enable_height": "0"
          }
        },
        "validators": [
          {
            "address": "A146C3E02DB4F8AAD5E859E35F4F7BCC094F0B13",
            "pub_key": {
              "type": "tendermint/PubKeySecp256k1",
              "value": "AiA6uTAC2S62d1DcwRAAj0hSosCdkCa1aTWlXWJeFA9W"
            },
            "power": "10",
            "name": "node0"
          }
        ],
        "app_hash": ""
      },
      "node_key.json": {
        "priv_key": {
          "type": "tendermint/PrivKeyEd25519",
          "value": "GKZ5kO56LhcaeRrOIefJtA2ogaPxQw+R6xBiznQD+290PZ/N5ZbBwCa9DoVA7FIeUeNofpHLtFK4UE0ACep5oA=="
        }
      },
      "priv_validator_key.json": {
        "address": "A146C3E02DB4F8AAD5E859E35F4F7BCC094F0B13",
        "pub_key": {
          "type": "tendermint/PubKeySecp256k1",
          "value": "AiA6uTAC2S62d1DcwRAAj0hSosCdkCa1aTWlXWJeFA9W"
        },
        "priv_key": {
          "type": "tendermint/PrivKeySecp256k1",
          "value": "+8+j8W0W3B9H68JbLoUTieIU4aNWjsumkuU8fQPN6tY="
        }
      },
      "config.toml": {
        "p2p": {
          "laddr": "tcp://0.0.0.0:20001",
          "allow_duplicate_ip": true,
          "addr_book_strict": false
        },
        "rpc": {
          "laddr": "tcp://0.0.0.0:20002"
        }
      }
    }
  }' >> local_testnet_normal5/blockchain/options.json

  echo '{
    "rootPath": "blockchain",
    "web3clientVersion": "bdk/cpp/linux_x86-64/0.2.0",
    "version": 1,
    "chainID": 808080,
    "httpPort": 8100,
    "eventBlockCap": 2000,
    "eventLogCap": 10000,
    "stateDumpTrigger": 1000,
    "chainOwner": "0x00dead00665771855a34155f5e7405489df2c3c6",
    "indexingMode": "RPC",
    "cometBFT": {
      "genesis.json": {
        "genesis_time": "2024-09-17T18:26:34.583377166Z",
        "initial_height": "0",
        "consensus_params": {
          "block": {
            "max_bytes": "22020096",
            "max_gas": "-1"
          },
          "evidence": {
            "max_age_num_blocks": "100000",
            "max_age_duration": "172800000000000",
            "max_bytes": "1048576"
          },
          "validator": {
            "pub_key_types": [
              "secp256k1"
            ]
          },
          "version": {
            "app": "0"
          },
          "abci": {
            "vote_extensions_enable_height": "0"
          }
        },
        "validators": [
          {
            "address": "A146C3E02DB4F8AAD5E859E35F4F7BCC094F0B13",
            "pub_key": {
              "type": "tendermint/PubKeySecp256k1",
              "value": "AiA6uTAC2S62d1DcwRAAj0hSosCdkCa1aTWlXWJeFA9W"
            },
            "power": "10",
            "name": "node0"
          }
        ],
        "app_hash": ""
      },
      "node_key.json": {
        "priv_key": {
          "type": "tendermint/PrivKeyEd25519",
          "value": "GKZ5kO56LhcaeRrOIefJtA2ogaPxQw+R6xBiznQD+290PZ/N5ZbBwCa9DoVA7FIeUeNofpHLtFK4UE0ACep5oA=="
        }
      },
      "priv_validator_key.json": {
        "address": "A146C3E02DB4F8AAD5E859E35F4F7BCC094F0B13",
        "pub_key": {
          "type": "tendermint/PubKeySecp256k1",
          "value": "AiA6uTAC2S62d1DcwRAAj0hSosCdkCa1aTWlXWJeFA9W"
        },
        "priv_key": {
          "type": "tendermint/PrivKeySecp256k1",
          "value": "+8+j8W0W3B9H68JbLoUTieIU4aNWjsumkuU8fQPN6tY="
        }
      },
      "config.toml": {
        "p2p": {
          "laddr": "tcp://0.0.0.0:20001",
          "allow_duplicate_ip": true,
          "addr_book_strict": false
        },
        "rpc": {
          "laddr": "tcp://0.0.0.0:20002"
        }
      }
    }
  }' >> local_testnet_normal6/blockchain/options.json

  # Launch the Discovery Node through tmux
  echo "Launching Discovery Node"
  cd local_testnet_discovery
  tmux new-session -d -s local_testnet_discovery './bdkd-discovery || bash && bash'
  sleep 1

  # Launch the Validators through tmux, don't exit the tmux session when closing the terminal
  echo "Launching Validator 1"
  cd ../local_testnet_validator1
  tmux new-session -d -s local_testnet_validator1 './bdkd || bash && bash'

  echo "Launching Validator 2"
  cd ../local_testnet_validator2
  tmux new-session -d -s local_testnet_validator2 './bdkd || bash && bash'

  echo "Launching Validator 3"
  cd ../local_testnet_validator3
  tmux new-session -d -s local_testnet_validator3 './bdkd || bash && bash'

  echo "Launching Validator 4"
  cd ../local_testnet_validator4
  tmux new-session -d -s local_testnet_validator4 './bdkd || bash && bash'

  echo "Launching Validator 5"
  cd ../local_testnet_validator5
  tmux new-session -d -s local_testnet_validator5 './bdkd || bash && bash'

  # Launch the Normal Nodes through tmux, don't exit the tmux session when closing the terminal
  echo "Launching Normal Node 1"
  cd ../local_testnet_normal1
  tmux new-session -d -s local_testnet_normal1 './bdkd || bash && bash'

  echo "Launching Normal Node 2"
  cd ../local_testnet_normal2
  tmux new-session -d -s local_testnet_normal2 './bdkd || bash && bash'

  echo "Launching Normal Node 3"
  cd ../local_testnet_normal3
  tmux new-session -d -s local_testnet_normal3 './bdkd || bash && bash'

  echo "Launching Normal Node 4"
  cd ../local_testnet_normal4
  tmux new-session -d -s local_testnet_normal4 './bdkd || bash && bash'

  echo "Launching Normal Node 5"
  cd ../local_testnet_normal5
  tmux new-session -d -s local_testnet_normal5 './bdkd || bash && bash'

  echo "Launching Normal Node 6"
  cd ../local_testnet_normal6
  tmux new-session -d -s local_testnet_normal6 './bdkd > output.txt || bash && bash'

  # Finish deploying
  GREEN=$'\e[0;32m'
  NC=$'\e[0m'
  echo "${GREEN}Success! All nodes are launched${NC}"
  echo "You can now access the local testnet through your favorite web3 client"
  echo "RPC URL: http://127.0.0.1:8090"
  echo "ChainID: 808080"
  echo "You can also attach your terminal to the tmux session to see the logs related to transactions and blocks being processed."
  echo "\"tmux a -t local_testnet_validator1\" to attach to the tmux session of the first validator"
fi

