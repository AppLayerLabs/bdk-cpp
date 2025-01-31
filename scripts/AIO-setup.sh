# Copyright (c) [2023-2024] [AppLayer Developers]
# This software is distributed under the MIT License.
# See the LICENSE.txt file in the project root for more information.

# Main deploy script for the BDK project.
# A given BDK network has a minimum requirement of 5 Validators, 6 Normal Nodes and 1 Discovery Node.
# The script MUST be called from the project's root directory for paths to work.

# The script does the following:
# - Create a new folder called "local_testnet" with several subfolders for each node (Discovery, Normal and Validator)
# - Build the project inside a folder called "build_local_testnet"
# - Copy the required stuff to the local_testnet folder (bdkd, bdkd-discovery,
#   options.json for each node based on what was configured in the nodeopts folder)
# - Launch each node in a separate tmux session in a given order (Discovery, then Validator, then Normal)
# TODO: deploy script shouldn't build the project, it should reuse the already existing build in the build folder

# ============================================================================

# Helper function for writing options.json in a node's subfolder.
# Usage: `gen_node_opts nodeFolder rootPath httpPort indexingMode`
# $1 = node folder for the JSON file to be written to (e.g. `local_testnet_discovery`)
# $2 = root path for the JSON file (e.g. `blockchain`)
# $3 = HTTP port for the node to operate on  (e.g. `8081`)
# $4 = indexing mode for the node to operate on (e.g. `RPC_TRACE`)
gen_node_opts() {
  # Check if options.json already exists, copy it if not.
  # Copy user-defined options.json, or defaults.json if the former does not exist.
  # Always force copy (thus always overwriting configs for each deploy)
  if [ ! -f local_testnet/$1/$2/options.json ]; then
    if [ -f nodeopts/options.json ]; then
      cp -f nodeopts/options.json local_testnet/$1/$2/
    else
      cp -f nodeopts/defaults.json local_testnet/$1/$2/options.json
    fi
  fi

  # Replace parameters in file with the given ones
  sed -i '/rootPath/c\    "rootPath": "'$2'",' local_testnet/$1/$2/options.json
  sed -i '/httpPort/c\    "httpPort": '$3',' local_testnet/$1/$2/options.json
  sed -i '/indexingMode/c\    "indexingMode": "'$4'",' local_testnet/$1/$2/options.json
}

# ============================================================================

# Kill the tmux terminals "local_testnet_validatorX" and "local_testnet_discovery"
# TODO: this should be better coded so we can use the "#!/bin/bash -e" shebang (quit on error)
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
  cd .. # Back to root

  # Copy and configure the JSON files for each node
  gen_node_opts "local_testnet_discovery" "discoveryNode" "9999" "RPC"
  gen_node_opts "local_testnet_validator1" "blockchain" "8090" "RPC_TRACE"
  gen_node_opts "local_testnet_validator2" "blockchain" "8091" "RPC_TRACE"
  gen_node_opts "local_testnet_validator3" "blockchain" "8092" "RPC"
  gen_node_opts "local_testnet_validator4" "blockchain" "8093" "RPC"
  gen_node_opts "local_testnet_validator5" "blockchain" "8094" "RPC"
  gen_node_opts "local_testnet_normal1" "blockchain" "8095" "RPC"
  gen_node_opts "local_testnet_normal2" "blockchain" "8096" "RPC"
  gen_node_opts "local_testnet_normal3" "blockchain" "8097" "RPC"
  gen_node_opts "local_testnet_normal4" "blockchain" "8098" "RPC"
  gen_node_opts "local_testnet_normal5" "blockchain" "8099" "RPC"
  gen_node_opts "local_testnet_normal6" "blockchain" "8100" "RPC"

  # Launch each node in sequence through tmux - Discovery, then Validators, then Normals.
  # Last part of the command string ensures the session won't exit when closing the terminal
  echo "Launching Discovery Node"
  cd local_testnet/local_testnet_discovery
  tmux new-session -d -s local_testnet_discovery './bdkd-discovery || bash && bash'
  sleep 1
  cd ../..

  echo "Launching Validator 1"
  cd local_testnet/local_testnet_validator1
  tmux new-session -d -s local_testnet_validator1 './bdkd || bash && bash'
  cd ../..

  echo "Launching Validator 2"
  cd local_testnet/local_testnet_validator2
  tmux new-session -d -s local_testnet_validator2 './bdkd || bash && bash'
  cd ../..

  echo "Launching Validator 3"
  cd local_testnet/local_testnet_validator3
  tmux new-session -d -s local_testnet_validator3 './bdkd || bash && bash'
  cd ../..

  echo "Launching Validator 4"
  cd local_testnet/local_testnet_validator4
  tmux new-session -d -s local_testnet_validator4 './bdkd || bash && bash'
  cd ../..

  echo "Launching Validator 5"
  cd local_testnet/local_testnet_validator5
  tmux new-session -d -s local_testnet_validator5 './bdkd || bash && bash'
  cd ../..

  echo "Launching Normal Node 1"
  cd local_testnet/local_testnet_normal1
  tmux new-session -d -s local_testnet_normal1 './bdkd || bash && bash'
  cd ../..

  echo "Launching Normal Node 2"
  cd local_testnet/local_testnet_normal2
  tmux new-session -d -s local_testnet_normal2 './bdkd || bash && bash'
  cd ../..

  echo "Launching Normal Node 3"
  cd local_testnet/local_testnet_normal3
  tmux new-session -d -s local_testnet_normal3 './bdkd || bash && bash'
  cd ../..

  echo "Launching Normal Node 4"
  cd local_testnet/local_testnet_normal4
  tmux new-session -d -s local_testnet_normal4 './bdkd || bash && bash'
  cd ../..

  echo "Launching Normal Node 5"
  cd local_testnet/local_testnet_normal5
  tmux new-session -d -s local_testnet_normal5 './bdkd || bash && bash'
  cd ../..

  echo "Launching Normal Node 6"
  cd local_testnet/local_testnet_normal6
  tmux new-session -d -s local_testnet_normal6 './bdkd > output.txt || bash && bash'
  cd ../..

  # Finish deploying
  GREEN=$'\e[0;32m'
  NC=$'\e[0m'
  echo "${GREEN}Success! All nodes are launched${NC}"
  echo "You can now access your local testnet at http://127.0.0.1 through your favorite Web3 client:"
  awk '/httpPort/' local_testnet/local_testnet_validator1/blockchain/options.json
  awk '/chainID/' local_testnet/local_testnet_validator1/blockchain/options.json
  echo "You can also attach your terminal to the tmux session to see the logs related to transactions and blocks being processed:"
  echo "\"tmux ls\" to list all sessions, then \"tmux a -t session_name\" to attach"
  echo "e.g. \"tmux a -t local_testnet_validator1\" to attach to the first Validator's session"
  echo "OR (CTRL+B, S) inside \"tmux\" (no params) to do it all interactively"
  echo "\"tmux kill-server\" to bring the nodes down if required, then re-run the script to bring them up again"
fi

