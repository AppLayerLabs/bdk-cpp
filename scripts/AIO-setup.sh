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

## Clean the build folder if the user specified the --clean flag
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
  ## Build the project
  cd build_local_testnet
  cmake -DDEBUG=$DEBUG ..
  make -j${CORES}
fi

if [ "$DEPLOY" = true ]; then
  if [ "$ONLY_DEPLOY" = true ]; then
    cd build_local_testnet
  fi
  ## Copy the bdkd and bdkd-discovery executables to the local_testnet directory
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
  echo '{
    "rootPath": "discoveryNode",
    "web3clientVersion": "bdk/cpp/linux_x86-64/0.2.0",
    "version": 1,
    "chainID": 808080,
    "chainOwner": "0x00dead00665771855a34155f5e7405489df2c3c6",
    "p2pIp" : "127.0.0.1",
    "p2pPort": 8080,
    "httpPort": 9999,
    "minDiscoveryConns": 11,
    "minNormalConns": 11,
    "maxDiscoveryConns": 200,
    "maxNormalConns": 50,
    "eventBlockCap": 2000,
    "eventLogCap": 10000,
    "stateDumpTrigger" : 1000,
    "minValidators": 4,
    "privKey": "0000000000000000000000000000000000000000000000000000000000000000",
    "genesis" : {
      "validators": [
        "0x7588b0f553d1910266089c58822e1120db47e572",
        "0xcabf34a268847a610287709d841e5cd590cc5c00",
        "0x5fb516dc2cfc1288e689ed377a9eebe2216cf1e3",
        "0x795083c42583842774febc21abb6df09e784fce5",
        "0xbec7b74f70c151707a0bfb20fe3767c6e65499e0"
      ],
      "timestamp" : 1656356646000000,
      "signer" : "0x4d48bdf34d65ef2bed2e4ee9020a7d3162b494ac31d3088153425f286f3d3c8c",
      "balances": [
        { "address": "0x00dead00665771855a34155f5e7405489df2c3c6", "balance": "100000000000000000000000000000000000000000" }
      ]
    },
    "indexingMode" : "RPC"
  }' >> local_testnet_discovery/discoveryNode/options.json

  # Create the JSON file for the Validators
  echo '{
    "rootPath": "blockchain",
    "web3clientVersion": "bdk/cpp/linux_x86-64/0.2.0",
    "version": 1,
    "chainID": 808080,
    "chainOwner": "0x00dead00665771855a34155f5e7405489df2c3c6",
    "p2pIp" : "127.0.0.1",
    "p2pPort": 8081,
    "httpPort": 8090,
    "minDiscoveryConns": 11,
    "minNormalConns": 11,
    "maxDiscoveryConns": 200,
    "maxNormalConns": 50,
    "eventBlockCap": 2000,
    "eventLogCap": 10000,
    "stateDumpTrigger" : 1000,
    "minValidators": 4,
    "privKey": "0xba5e6e9dd9cbd263969b94ee385d885c2d303dfc181db2a09f6bf19a7ba26759",
    "genesis" : {
      "validators": [
        "0x7588b0f553d1910266089c58822e1120db47e572",
        "0xcabf34a268847a610287709d841e5cd590cc5c00",
        "0x5fb516dc2cfc1288e689ed377a9eebe2216cf1e3",
        "0x795083c42583842774febc21abb6df09e784fce5",
        "0xbec7b74f70c151707a0bfb20fe3767c6e65499e0"
      ],
      "timestamp" : 1656356646000000,
      "signer" : "0x4d48bdf34d65ef2bed2e4ee9020a7d3162b494ac31d3088153425f286f3d3c8c",
      "balances": [
        { "address": "0x00dead00665771855a34155f5e7405489df2c3c6", "balance": "100000000000000000000000000000000000000000" }
      ]
    },
    "discoveryNodes": [
      {
        "address" : "127.0.0.1",
        "port" : 8080
      }
    ],
    "indexingMode" : "RPC"
  }' >> local_testnet_validator1/blockchain/options.json

  echo '{
    "rootPath": "blockchain",
    "web3clientVersion": "bdk/cpp/linux_x86-64/0.2.0",
    "version": 1,
    "chainID": 808080,
    "chainOwner": "0x00dead00665771855a34155f5e7405489df2c3c6",
    "p2pIp" : "127.0.0.1",
    "p2pPort": 8082,
    "httpPort": 8091,
    "minDiscoveryConns": 11,
    "minNormalConns": 11,
    "maxDiscoveryConns": 200,
    "maxNormalConns": 50,
    "eventBlockCap": 2000,
    "eventLogCap": 10000,
    "stateDumpTrigger" : 1000,
    "minValidators": 4,
    "privKey": "0xfd84d99aa18b474bf383e10925d82194f1b0ca268e7a339032679d6e3a201ad4",
    "genesis" : {
      "validators": [
        "0x7588b0f553d1910266089c58822e1120db47e572",
        "0xcabf34a268847a610287709d841e5cd590cc5c00",
        "0x5fb516dc2cfc1288e689ed377a9eebe2216cf1e3",
        "0x795083c42583842774febc21abb6df09e784fce5",
        "0xbec7b74f70c151707a0bfb20fe3767c6e65499e0"
      ],
      "timestamp" : 1656356646000000,
      "signer" : "0x4d48bdf34d65ef2bed2e4ee9020a7d3162b494ac31d3088153425f286f3d3c8c",
      "balances": [
        { "address": "0x00dead00665771855a34155f5e7405489df2c3c6", "balance": "100000000000000000000000000000000000000000" }
      ]
    },
    "discoveryNodes": [
      {
        "address" : "127.0.0.1",
        "port" : 8080
      }
    ],
    "indexingMode" : "RPC"
  }' >> local_testnet_validator2/blockchain/options.json

  echo '{
    "rootPath": "blockchain",
    "web3clientVersion": "bdk/cpp/linux_x86-64/0.2.0",
    "version": 1,
    "chainID": 808080,
    "chainOwner": "0x00dead00665771855a34155f5e7405489df2c3c6",
    "p2pIp" : "127.0.0.1",
    "p2pPort": 8083,
    "httpPort": 8092,
    "minDiscoveryConns": 11,
    "minNormalConns": 11,
    "maxDiscoveryConns": 200,
    "maxNormalConns": 50,
    "eventBlockCap": 2000,
    "eventLogCap": 10000,
    "stateDumpTrigger" : 1000,
    "minValidators": 4,
    "privKey": "0x66ce71abe0b8acd92cfd3965d6f9d80122aed9b0e9bdd3dbe018230bafde5751",
    "genesis" : {
      "validators": [
        "0x7588b0f553d1910266089c58822e1120db47e572",
        "0xcabf34a268847a610287709d841e5cd590cc5c00",
        "0x5fb516dc2cfc1288e689ed377a9eebe2216cf1e3",
        "0x795083c42583842774febc21abb6df09e784fce5",
        "0xbec7b74f70c151707a0bfb20fe3767c6e65499e0"
      ],
      "timestamp" : 1656356646000000,
      "signer" : "0x4d48bdf34d65ef2bed2e4ee9020a7d3162b494ac31d3088153425f286f3d3c8c",
      "balances": [
        { "address": "0x00dead00665771855a34155f5e7405489df2c3c6", "balance": "100000000000000000000000000000000000000000" }
      ]
    },
    "discoveryNodes": [
      {
        "address" : "127.0.0.1",
        "port" : 8080
      }
    ],
    "indexingMode" : "RPC"
  }' >> local_testnet_validator3/blockchain/options.json

  echo '{
    "rootPath": "blockchain",
    "web3clientVersion": "bdk/cpp/linux_x86-64/0.2.0",
    "version": 1,
    "chainID": 808080,
    "chainOwner": "0x00dead00665771855a34155f5e7405489df2c3c6",
    "p2pIp" : "127.0.0.1",
    "p2pPort": 8084,
    "httpPort": 8093,
    "minDiscoveryConns": 11,
    "minNormalConns": 11,
    "maxDiscoveryConns": 200,
    "maxNormalConns": 50,
    "eventBlockCap": 2000,
    "eventLogCap": 10000,
    "stateDumpTrigger" : 1000,
    "minValidators": 4,
    "privKey": "0x856aeb3b9c20a80d1520a2406875f405d336e09475f43c478eb4f0dafb765fe7",
    "genesis" : {
      "validators": [
        "0x7588b0f553d1910266089c58822e1120db47e572",
        "0xcabf34a268847a610287709d841e5cd590cc5c00",
        "0x5fb516dc2cfc1288e689ed377a9eebe2216cf1e3",
        "0x795083c42583842774febc21abb6df09e784fce5",
        "0xbec7b74f70c151707a0bfb20fe3767c6e65499e0"
      ],
      "timestamp" : 1656356646000000,
      "signer" : "0x4d48bdf34d65ef2bed2e4ee9020a7d3162b494ac31d3088153425f286f3d3c8c",
      "balances": [
        { "address": "0x00dead00665771855a34155f5e7405489df2c3c6", "balance": "100000000000000000000000000000000000000000" }
      ]
    },
    "discoveryNodes": [
      {
        "address" : "127.0.0.1",
        "port" : 8080
      }
    ],
    "indexingMode" : "RPC"
  }' >> local_testnet_validator4/blockchain/options.json

  echo '{
    "rootPath": "blockchain",
    "web3clientVersion": "bdk/cpp/linux_x86-64/0.2.0",
    "version": 1,
    "chainID": 808080,
    "chainOwner": "0x00dead00665771855a34155f5e7405489df2c3c6",
    "p2pIp" : "127.0.0.1",
    "p2pPort": 8085,
    "httpPort": 8094,
    "minDiscoveryConns": 11,
    "minNormalConns": 11,
    "maxDiscoveryConns": 200,
    "maxNormalConns": 50,
    "eventBlockCap": 2000,
    "eventLogCap": 10000,
    "stateDumpTrigger" : 1000,
    "minValidators": 4,
    "privKey": "0x81f288dd776f4edfe256d34af1f7d719f511559f19115af3e3d692e741faadc6",
    "genesis" : {
      "validators": [
        "0x7588b0f553d1910266089c58822e1120db47e572",
        "0xcabf34a268847a610287709d841e5cd590cc5c00",
        "0x5fb516dc2cfc1288e689ed377a9eebe2216cf1e3",
        "0x795083c42583842774febc21abb6df09e784fce5",
        "0xbec7b74f70c151707a0bfb20fe3767c6e65499e0"
      ],
      "timestamp" : 1656356646000000,
      "signer" : "0x4d48bdf34d65ef2bed2e4ee9020a7d3162b494ac31d3088153425f286f3d3c8c",
      "balances": [
        { "address": "0x00dead00665771855a34155f5e7405489df2c3c6", "balance": "100000000000000000000000000000000000000000" }
      ]
    },
    "discoveryNodes": [
      {
        "address" : "127.0.0.1",
        "port" : 8080
      }
    ],
    "indexingMode" : "RPC"
  }' >> local_testnet_validator5/blockchain/options.json

  # Create the json file for the Normal Nodes
  echo '{
    "rootPath": "blockchain",
    "web3clientVersion": "bdk/cpp/linux_x86-64/0.2.0",
    "version": 1,
    "chainID": 808080,
    "chainOwner": "0x00dead00665771855a34155f5e7405489df2c3c6",
    "p2pIp" : "127.0.0.1",
    "p2pPort": 8086,
    "httpPort": 8095,
    "minDiscoveryConns": 11,
    "minNormalConns": 11,
    "maxDiscoveryConns": 200,
    "maxNormalConns": 50,
    "eventBlockCap": 2000,
    "eventLogCap": 10000,
    "stateDumpTrigger" : 1000,
    "minValidators": 4,
    "genesis" : {
      "validators": [
        "0x7588b0f553d1910266089c58822e1120db47e572",
        "0xcabf34a268847a610287709d841e5cd590cc5c00",
        "0x5fb516dc2cfc1288e689ed377a9eebe2216cf1e3",
        "0x795083c42583842774febc21abb6df09e784fce5",
        "0xbec7b74f70c151707a0bfb20fe3767c6e65499e0"
      ],
      "timestamp" : 1656356646000000,
      "signer" : "0x4d48bdf34d65ef2bed2e4ee9020a7d3162b494ac31d3088153425f286f3d3c8c",
      "balances": [
        { "address": "0x00dead00665771855a34155f5e7405489df2c3c6", "balance": "100000000000000000000000000000000000000000" }
      ]
    },
    "discoveryNodes": [
      {
        "address" : "127.0.0.1",
        "port" : 8080
      }
    ],
    "indexingMode" : "RPC"
  }' >> local_testnet_normal1/blockchain/options.json

  echo '{
    "rootPath": "blockchain",
    "web3clientVersion": "bdk/cpp/linux_x86-64/0.2.0",
    "version": 1,
    "chainID": 808080,
    "chainOwner": "0x00dead00665771855a34155f5e7405489df2c3c6",
    "p2pIp" : "127.0.0.1",
    "p2pPort": 8087,
    "httpPort": 8096,
    "minDiscoveryConns": 11,
    "minNormalConns": 11,
    "maxDiscoveryConns": 200,
    "maxNormalConns": 50,
    "eventBlockCap": 2000,
    "eventLogCap": 10000,
    "stateDumpTrigger" : 1000,
    "minValidators": 4,
    "genesis" : {
      "validators": [
        "0x7588b0f553d1910266089c58822e1120db47e572",
        "0xcabf34a268847a610287709d841e5cd590cc5c00",
        "0x5fb516dc2cfc1288e689ed377a9eebe2216cf1e3",
        "0x795083c42583842774febc21abb6df09e784fce5",
        "0xbec7b74f70c151707a0bfb20fe3767c6e65499e0"
      ],
      "timestamp" : 1656356646000000,
      "signer" : "0x4d48bdf34d65ef2bed2e4ee9020a7d3162b494ac31d3088153425f286f3d3c8c",
      "balances": [
        { "address": "0x00dead00665771855a34155f5e7405489df2c3c6", "balance": "100000000000000000000000000000000000000000" }
      ]
    },
    "discoveryNodes": [
      {
        "address" : "127.0.0.1",
        "port" : 8080
      }
    ],
    "indexingMode" : "RPC"
  }' >> local_testnet_normal2/blockchain/options.json

  echo '{
    "rootPath": "blockchain",
    "web3clientVersion": "bdk/cpp/linux_x86-64/0.2.0",
    "version": 1,
    "chainID": 808080,
    "chainOwner": "0x00dead00665771855a34155f5e7405489df2c3c6",
    "p2pIp" : "127.0.0.1",
    "p2pPort": 8088,
    "httpPort": 8097,
    "minDiscoveryConns": 11,
    "minNormalConns": 11,
    "maxDiscoveryConns": 200,
    "maxNormalConns": 50,
    "eventBlockCap": 2000,
    "eventLogCap": 10000,
    "stateDumpTrigger" : 1000,
    "minValidators": 4,
    "genesis" : {
      "validators": [
        "0x7588b0f553d1910266089c58822e1120db47e572",
        "0xcabf34a268847a610287709d841e5cd590cc5c00",
        "0x5fb516dc2cfc1288e689ed377a9eebe2216cf1e3",
        "0x795083c42583842774febc21abb6df09e784fce5",
        "0xbec7b74f70c151707a0bfb20fe3767c6e65499e0"
      ],
      "timestamp" : 1656356646000000,
      "signer" : "0x4d48bdf34d65ef2bed2e4ee9020a7d3162b494ac31d3088153425f286f3d3c8c",
      "balances": [
        { "address": "0x00dead00665771855a34155f5e7405489df2c3c6", "balance": "100000000000000000000000000000000000000000" }
      ]
    },
    "discoveryNodes": [
      {
        "address" : "127.0.0.1",
        "port" : 8080
      }
    ],
    "indexingMode" : "RPC"
  }' >> local_testnet_normal3/blockchain/options.json

  echo '{
    "rootPath": "blockchain",
    "web3clientVersion": "bdk/cpp/linux_x86-64/0.2.0",
    "version": 1,
    "chainID": 808080,
    "chainOwner": "0x00dead00665771855a34155f5e7405489df2c3c6",
    "p2pIp" : "127.0.0.1",
    "p2pPort": 8089,
    "httpPort": 8098,
    "minDiscoveryConns": 11,
    "minNormalConns": 11,
    "maxDiscoveryConns": 200,
    "maxNormalConns": 50,
    "eventBlockCap": 2000,
    "eventLogCap": 10000,
    "stateDumpTrigger" : 1000,
    "minValidators": 4,
    "genesis" : {
      "validators": [
        "0x7588b0f553d1910266089c58822e1120db47e572",
        "0xcabf34a268847a610287709d841e5cd590cc5c00",
        "0x5fb516dc2cfc1288e689ed377a9eebe2216cf1e3",
        "0x795083c42583842774febc21abb6df09e784fce5",
        "0xbec7b74f70c151707a0bfb20fe3767c6e65499e0"
      ],
      "timestamp" : 1656356646000000,
      "signer" : "0x4d48bdf34d65ef2bed2e4ee9020a7d3162b494ac31d3088153425f286f3d3c8c",
      "balances": [
        { "address": "0x00dead00665771855a34155f5e7405489df2c3c6", "balance": "100000000000000000000000000000000000000000" }
      ]
    },
    "discoveryNodes": [
      {
        "address" : "127.0.0.1",
        "port" : 8080
      }
    ],
    "indexingMode" : "RPC"
  }' >> local_testnet_normal4/blockchain/options.json

  echo '{
    "rootPath": "blockchain",
    "web3clientVersion": "bdk/cpp/linux_x86-64/0.2.0",
    "version": 1,
    "chainID": 808080,
    "chainOwner": "0x00dead00665771855a34155f5e7405489df2c3c6",
    "p2pIp" : "127.0.0.1",
    "p2pPort": 8110,
    "httpPort": 8099,
    "minDiscoveryConns": 11,
    "minNormalConns": 11,
    "maxDiscoveryConns": 200,
    "maxNormalConns": 50,
    "eventBlockCap": 2000,
    "eventLogCap": 10000,
    "stateDumpTrigger" : 1000,
    "minValidators": 4,
    "genesis" : {
      "validators": [
        "0x7588b0f553d1910266089c58822e1120db47e572",
        "0xcabf34a268847a610287709d841e5cd590cc5c00",
        "0x5fb516dc2cfc1288e689ed377a9eebe2216cf1e3",
        "0x795083c42583842774febc21abb6df09e784fce5",
        "0xbec7b74f70c151707a0bfb20fe3767c6e65499e0"
      ],
      "timestamp" : 1656356646000000,
      "signer" : "0x4d48bdf34d65ef2bed2e4ee9020a7d3162b494ac31d3088153425f286f3d3c8c",
      "balances": [
        { "address": "0x00dead00665771855a34155f5e7405489df2c3c6", "balance": "100000000000000000000000000000000000000000" }
      ]
    },
    "discoveryNodes": [
      {
        "address" : "127.0.0.1",
        "port" : 8080
      }
    ],
    "indexingMode" : "RPC"
  }' >> local_testnet_normal5/blockchain/options.json

  echo '{
    "rootPath": "blockchain",
    "web3clientVersion": "bdk/cpp/linux_x86-64/0.2.0",
    "version": 1,
    "chainID": 808080,
    "chainOwner": "0x00dead00665771855a34155f5e7405489df2c3c6",
    "p2pIp" : "127.0.0.1",
    "p2pPort": 8111,
    "httpPort": 8100,
    "minDiscoveryConns": 11,
    "minNormalConns": 11,
    "maxDiscoveryConns": 200,
    "maxNormalConns": 50,
    "eventBlockCap": 2000,
    "eventLogCap": 10000,
    "stateDumpTrigger" : 1000,
    "minValidators": 4,
    "genesis" : {
      "validators": [
        "0x7588b0f553d1910266089c58822e1120db47e572",
        "0xcabf34a268847a610287709d841e5cd590cc5c00",
        "0x5fb516dc2cfc1288e689ed377a9eebe2216cf1e3",
        "0x795083c42583842774febc21abb6df09e784fce5",
        "0xbec7b74f70c151707a0bfb20fe3767c6e65499e0"
      ],
      "timestamp" : 1656356646000000,
      "signer" : "0x4d48bdf34d65ef2bed2e4ee9020a7d3162b494ac31d3088153425f286f3d3c8c",
      "balances": [
        { "address": "0x00dead00665771855a34155f5e7405489df2c3c6", "balance": "100000000000000000000000000000000000000000" }
      ]
    },
    "discoveryNodes": [
      {
        "address" : "127.0.0.1",
        "port" : 8080
      }
    ],
    "indexingMode" : "RPC"
  }' >> local_testnet_normal6/blockchain/options.json

  echo '{
    "rootPath": "blockchain",
    "web3clientVersion": "bdk/cpp/linux_x86-64/0.2.0",
    "version": 1,
    "chainID": 808080,
    "chainOwner": "0x00dead00665771855a34155f5e7405489df2c3c6",
    "p2pIp" : "127.0.0.1",
    "p2pPort": 8110,
    "httpPort": 8099,
    "minDiscoveryConns": 11,
    "minNormalConns": 11,
    "maxDiscoveryConns": 200,
    "maxNormalConns": 50,
    "eventBlockCap": 2000,
    "eventLogCap": 10000,
    "stateDumpTrigger" : 1000,
    "minValidators": 4,
    "genesis" : {
      "validators": [
        "0x7588b0f553d1910266089c58822e1120db47e572",
        "0xcabf34a268847a610287709d841e5cd590cc5c00",
        "0x5fb516dc2cfc1288e689ed377a9eebe2216cf1e3",
        "0x795083c42583842774febc21abb6df09e784fce5",
        "0xbec7b74f70c151707a0bfb20fe3767c6e65499e0"
      ],
      "timestamp" : 1656356646000000,
      "signer" : "0x4d48bdf34d65ef2bed2e4ee9020a7d3162b494ac31d3088153425f286f3d3c8c",
      "balances": [
        { "address": "0x00dead00665771855a34155f5e7405489df2c3c6", "balance": "100000000000000000000000000000000000000000" }
      ]
    },
    "discoveryNodes": [
      {
        "address" : "127.0.0.1",
        "port" : 8080
      }
    ],
    "indexingMode" : "RPC"
  }' >> local_testnet_normal5/blockchain/options.json

  echo '{
    "rootPath": "blockchain",
    "web3clientVersion": "bdk/cpp/linux_x86-64/0.2.0",
    "version": 1,
    "chainID": 808080,
    "chainOwner": "0x00dead00665771855a34155f5e7405489df2c3c6",
    "p2pIp" : "127.0.0.1",
    "p2pPort": 8111,
    "httpPort": 8100,
    "minDiscoveryConns": 11,
    "minNormalConns": 11,
    "maxDiscoveryConns": 200,
    "maxNormalConns": 50,
    "eventBlockCap": 2000,
    "eventLogCap": 10000,
    "stateDumpTrigger" : 1000,
    "minValidators": 4,
    "genesis" : {
      "validators": [
        "0x7588b0f553d1910266089c58822e1120db47e572",
        "0xcabf34a268847a610287709d841e5cd590cc5c00",
        "0x5fb516dc2cfc1288e689ed377a9eebe2216cf1e3",
        "0x795083c42583842774febc21abb6df09e784fce5",
        "0xbec7b74f70c151707a0bfb20fe3767c6e65499e0"
      ],
      "timestamp" : 1656356646000000,
      "signer" : "0x4d48bdf34d65ef2bed2e4ee9020a7d3162b494ac31d3088153425f286f3d3c8c",
      "balances": [
        { "address": "0x00dead00665771855a34155f5e7405489df2c3c6", "balance": "100000000000000000000000000000000000000000" }
      ]
    },
    "discoveryNodes": [
      {
        "address" : "127.0.0.1",
        "port" : 8080
      }
    ],
    "indexingMode" : "RPC"
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
  tmux new-session -d -s local_testnet_normal6 './bdkd || bash && bash'

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

