## A given network has a minimum requirement of 5 Validators, 6 Normal Nodes and 1 Discovery Node.
## Discovery Nodes are described within the options.json file.
## This script is meant to be executed within the   main directory of this project
## It will create a new directory, called "local_testnet" and will create
## New directories for each Validator and Discovery Node created (5 and 1 respectively)
## This script is also meant to build the project within a new folder called "build_local_testnet"
## It does not clean the build folder, only the local_testnet folder

## Kill the tmux terminals "local_testnet_validatorX" and "local_testnet_discovery"
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
DEBUG=ON # Build the project in debug mode
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

# Check if the user is running this script from the main directory of the project
if [ ! -f "scripts/AIO-setup.sh" ]; then
  echo "Please run this script from the main directory of the project"
  exit 1
fi

## Clean the build folder if the user specified the --clean flag
if [ "$CLEAN" = true ]; then
    if [ -d "build_local_testnet" ]; then
      rm -rf build_local_testnet
    fi
    echo "Running a clean build"
fi

## Create the new directory "build_local_testnet" if it doesn't exist
if [ ! -d "build_local_testnet" ]; then
  mkdir build_local_testnet
fi

## Create the new directory "local_testnet", empty it if it already exists
if [ -d "local_testnet" ]; then
  rm -rf local_testnet
fi
mkdir local_testnet

## Build the project
cd build_local_testnet
cmake -DDEBUG=$DEBUG ..
cmake --build . --target orbitersdkd orbitersdkd-discovery -- -j${CORES}

if [ "$DEPLOY" = true ]; then
  ## Copy the orbitersdkd and orbitersdk-discovery executables to the local_testnet directory
  cp orbitersdkd ../local_testnet
  cp orbitersdkd-discovery ../local_testnet

  ## Create the directories for the Validators and Discovery Node and copy the executables
  cd ../local_testnet
  for i in $(seq 1 5); do
  mkdir local_testnet_validator$i
  mkdir local_testnet_validator$i/blockchain
  cp orbitersdkd local_testnet_validator$i
  done

  for i in $(seq 1 6); do
  mkdir local_testnet_normal$i
  mkdir local_testnet_normal$i/blockchain
  cp orbitersdkd local_testnet_normal$i
  done

  mkdir local_testnet_discovery
  mkdir local_testnet_discovery/discoveryNode
  cp orbitersdkd-discovery local_testnet_discovery

  ## Create the JSON file for the Discovery Node
  echo '{
          "rootPath": "discoveryNode",
          "web3clientVersion": "OrbiterSDK/cpp/linux_x86-64/0.0.2",
          "version": 1,
          "chainID": 808080,
          "wsPort": 8080,
          "httpPort": 9999,
          "privKey": "0000000000000000000000000000000000000000000000000000000000000000"
        }' >> local_testnet_discovery/discoveryNode/options.json

  ## Create the JSON file for the Validators
  echo '{
          "rootPath": "blockchain",
          "web3clientVersion": "OrbiterSDK/cpp/linux_x86-64/0.0.2",
          "version": 1,
          "chainID": 808080,
          "wsPort": 8081,
          "httpPort": 8090,
          "privKey": "0xba5e6e9dd9cbd263969b94ee385d885c2d303dfc181db2a09f6bf19a7ba26759",
          "discoveryNodes": [
            {
              "address" : "127.0.0.1",
              "port" : 8080
            }
          ]
        }' >> local_testnet_validator1/blockchain/options.json

  echo '{
          "rootPath": "blockchain",
          "web3clientVersion": "OrbiterSDK/cpp/linux_x86-64/0.0.2",
          "version": 1,
          "chainID": 808080,
          "wsPort": 8082,
          "httpPort": 8091,
          "privKey": "0xfd84d99aa18b474bf383e10925d82194f1b0ca268e7a339032679d6e3a201ad4",
          "discoveryNodes": [
            {
              "address" : "127.0.0.1",
              "port" : 8080
            }
          ]
        }' >> local_testnet_validator2/blockchain/options.json

  echo '{
          "rootPath": "blockchain",
          "web3clientVersion": "OrbiterSDK/cpp/linux_x86-64/0.0.2",
          "version": 1,
          "chainID": 808080,
          "wsPort": 8083,
          "httpPort": 8092,
          "privKey": "0x66ce71abe0b8acd92cfd3965d6f9d80122aed9b0e9bdd3dbe018230bafde5751",
          "discoveryNodes": [
            {
              "address" : "127.0.0.1",
              "port" : 8080
            }
          ]
        }' >> local_testnet_validator3/blockchain/options.json

  echo '{
          "rootPath": "blockchain",
          "web3clientVersion": "OrbiterSDK/cpp/linux_x86-64/0.0.2",
          "version": 1,
          "chainID": 808080,
          "wsPort": 8084,
          "httpPort": 8093,
          "privKey": "0x856aeb3b9c20a80d1520a2406875f405d336e09475f43c478eb4f0dafb765fe7",
          "discoveryNodes": [
            {
              "address" : "127.0.0.1",
              "port" : 8080
            }
          ]
        }' >> local_testnet_validator4/blockchain/options.json

  echo '{
          "rootPath": "blockchain",
          "web3clientVersion": "OrbiterSDK/cpp/linux_x86-64/0.0.2",
          "version": 1,
          "chainID": 808080,
          "wsPort": 8085,
          "httpPort": 8094,
          "privKey": "0x81f288dd776f4edfe256d34af1f7d719f511559f19115af3e3d692e741faadc6",
          "discoveryNodes": [
            {
              "address" : "127.0.0.1",
              "port" : 8080
            }
          ]
        }' >> local_testnet_validator5/blockchain/options.json

  ## Create the json file for the Normal Nodes
  echo '{
          "rootPath": "blockchain",
          "web3clientVersion": "OrbiterSDK/cpp/linux_x86-64/0.0.2",
          "version": 1,
          "chainID": 808080,
          "wsPort": 8086,
          "httpPort": 8095,
          "discoveryNodes": [
            {
              "address" : "127.0.0.1",
              "port" : 8080
            }
          ]
        }' >> local_testnet_normal1/blockchain/options.json

  echo '{
          "rootPath": "blockchain",
          "web3clientVersion": "OrbiterSDK/cpp/linux_x86-64/0.0.2",
          "version": 1,
          "chainID": 808080,
          "wsPort": 8087,
          "httpPort": 8096,
          "discoveryNodes": [
            {
              "address" : "127.0.0.1",
              "port" : 8080
            }
          ]
        }' >> local_testnet_normal2/blockchain/options.json

  echo '{
          "rootPath": "blockchain",
          "web3clientVersion": "OrbiterSDK/cpp/linux_x86-64/0.0.2",
          "version": 1,
          "chainID": 808080,
          "wsPort": 8088,
          "httpPort": 8097,
          "discoveryNodes": [
            {
              "address" : "127.0.0.1",
              "port" : 8080
            }
          ]
        }' >> local_testnet_normal3/blockchain/options.json

  echo '{
          "rootPath": "blockchain",
          "web3clientVersion": "OrbiterSDK/cpp/linux_x86-64/0.0.2",
          "version": 1,
          "chainID": 808080,
          "wsPort": 8089,
          "httpPort": 8098,
          "discoveryNodes": [
            {
              "address" : "127.0.0.1",
              "port" : 8080
            }
          ]
        }' >> local_testnet_normal4/blockchain/options.json

  echo '{
          "rootPath": "blockchain",
          "web3clientVersion": "OrbiterSDK/cpp/linux_x86-64/0.0.2",
          "version": 1,
          "chainID": 808080,
          "wsPort": 8110,
          "httpPort": 8099,
          "discoveryNodes": [
            {
              "address" : "127.0.0.1",
              "port" : 8080
            }
          ]
        }' >> local_testnet_normal5/blockchain/options.json

  echo '{
          "rootPath": "blockchain",
          "web3clientVersion": "OrbiterSDK/cpp/linux_x86-64/0.0.2",
          "version": 1,
          "chainID": 808080,
          "wsPort": 8111,
          "httpPort": 8100,
          "discoveryNodes": [
            {
              "address" : "127.0.0.1",
              "port" : 8080
            }
          ]
        }' >> local_testnet_normal6/blockchain/options.json

  ## Launch the Discovery Node through tmux
  echo "Launching the Discovery Node"
  cd local_testnet_discovery
  tmux new-session -d -s local_testnet_discovery './orbitersdkd-discovery || bash && bash'

  sleep 1

  echo "Launching the Validators"
  ## Launch the Validators through tmux, don't exit the tmux session when you close the terminal
  echo "Launching Validator 1"
  cd ../local_testnet_validator1
  tmux new-session -d -s local_testnet_validator1 './orbitersdkd || bash && bash'

  echo "Launching Validator 2"
  cd ../local_testnet_validator2
  tmux new-session -d -s local_testnet_validator2 './orbitersdkd || bash && bash'

  echo "Launching Validator 3"
  cd ../local_testnet_validator3
  tmux new-session -d -s local_testnet_validator3 './orbitersdkd || bash && bash'

  echo "Launching Validator 4"
  cd ../local_testnet_validator4
  tmux new-session -d -s local_testnet_validator4 './orbitersdkd || bash && bash'

  echo "Launching Validator 5"
  cd ../local_testnet_validator5
  tmux new-session -d -s local_testnet_validator5 './orbitersdkd || bash && bash'

  ## Launch the Normal Nodes through tmux, don't exit the tmux session when you close the terminal

  echo "Launching Normal Node 1"
  cd ../local_testnet_normal1
  tmux new-session -d -s local_testnet_normal1 './orbitersdkd || bash && bash'

  echo "Launching Normal Node 2"
  cd ../local_testnet_normal2
  tmux new-session -d -s local_testnet_normal2 './orbitersdkd || bash && bash'

  echo "Launching Normal Node 3"
  cd ../local_testnet_normal3
  tmux new-session -d -s local_testnet_normal3 './orbitersdkd || bash && bash'

  echo "Launching Normal Node 4"
  cd ../local_testnet_normal4
  tmux new-session -d -s local_testnet_normal4 './orbitersdkd || bash && bash'

  echo "Launching Normal Node 5"
  cd ../local_testnet_normal5
  tmux new-session -d -s local_testnet_normal5 './orbitersdkd || bash && bash'

  echo "Launching Normal Node 6"
  cd ../local_testnet_normal6
  tmux new-session -d -s local_testnet_normal6 './orbitersdkd || bash && bash'

  echo "All nodes are launched"
  GREEN=$'\e[0;32m'
  NC=$'\e[0m'

  echo "${GREEN}Success! All nodes are launched${NC}"
  echo "You can now access the local testnet through your favorite web3 client"
  echo "RPC URL: http://127.0.0.1:8090"
  echo "ChainID: 808080"
  echo "You can also attach your terminal to the tmux session to see the logs related to transactions and blocks being processed."
  echo "\"tmux a -t local_testnet_validator1\" to attach to the tmux session of the first validator"
fi


