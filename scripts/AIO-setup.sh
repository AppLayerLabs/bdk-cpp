## A given network has a minimum requirement of 5 Validators and 1 Discovery Node.
## Discovery Nodes are described within the options.json file.
## This script is meant to be executed within the main directory of this project
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
tmux kill-session -t local_testnet_discovery

# Check if the user is running this script from the main directory of the project
if [ ! -f "scripts/AIO-setup.sh" ]; then
  echo "Please run this script from the main directory of the project"
  exit 1
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

## Get number of cores for parallel build
CORES=$(grep -c ^processor /proc/cpuinfo)

## Build the project
cd build_local_testnet
cmake ..
make -j${CORES}

## Copy the orbitersdkd and orbitersdk-discovery executables to the local_testnet directory
cp orbitersdkd ../local_testnet
cp orbitersdkd-discovery ../local_testnet

## Create the directories for the Validators and Discovery Node and copy the executables
cd ../local_testnet
for i in {1..5}; do
 mkdir local_testnet_validator$i
 cp orbitersdkd local_testnet_validator$i
done

mkdir local_testnet_discovery
cp orbitersdkd-discovery local_testnet_discovery

## Create the JSON file for the Discovery Node
echo '{
        "rootPath": "local_testnet_discovery",
        "web3clientVersion": "OrbiterSDK/cpp/linux_x86-64/0.0.1",
        "version": 1,
        "chainID": 8080,
        "wsPort": 8080,
        "httpPort": 8081,
        "privKey": "0000000000000000000000000000000000000000000000000000000000000000"
      }' >> local_testnet_discovery/options.json

## Create the JSON file for the Validators
echo '{
        "rootPath": "local_testnet_discovery",
        "web3clientVersion": "OrbiterSDK/cpp/linux_x86-64/0.0.1",
        "version": 1,
        "chainID": 8080,
        "wsPort": 8081,
        "httpPort": 8090,
        "privKey": "0xba5e6e9dd9cbd263969b94ee385d885c2d303dfc181db2a09f6bf19a7ba26759",
        "discoveryNodes": [
          {
            "address" : "127.0.0.1",
            "port" : 8080
          }
        ]
      }' >>  cp orbitersdkd local_testnet_validator1/options.json

echo '{
        "rootPath": "local_testnet_discovery",
        "web3clientVersion": "OrbiterSDK/cpp/linux_x86-64/0.0.1",
        "version": 1,
        "chainID": 8080,
        "wsPort": 8082,
        "httpPort": 8091,
        "privKey": "0xfd84d99aa18b474bf383e10925d82194f1b0ca268e7a339032679d6e3a201ad4",
        "discoveryNodes": [
          {
            "address" : "127.0.0.1",
            "port" : 8080
          }
        ]
      }' >>  cp orbitersdkd local_testnet_validator2/options.json

echo '{
        "rootPath": "local_testnet_discovery",
        "web3clientVersion": "OrbiterSDK/cpp/linux_x86-64/0.0.1",
        "version": 1,
        "chainID": 8080,
        "wsPort": 8083,
        "httpPort": 8092,
        "privKey": "0x66ce71abe0b8acd92cfd3965d6f9d80122aed9b0e9bdd3dbe018230bafde5751",
        "discoveryNodes": [
          {
            "address" : "127.0.0.1",
            "port" : 8080
          }
        ]
      }' >>  cp orbitersdkd local_testnet_validator3/options.json

echo '{
        "rootPath": "local_testnet_discovery",
        "web3clientVersion": "OrbiterSDK/cpp/linux_x86-64/0.0.1",
        "version": 1,
        "chainID": 8080,
        "wsPort": 8084,
        "httpPort": 8093,
        "privKey": "0x856aeb3b9c20a80d1520a2406875f405d336e09475f43c478eb4f0dafb765fe7",
        "discoveryNodes": [
          {
            "address" : "127.0.0.1",
            "port" : 8080
          }
        ]
        "discoveryNodes": [
          {
            "address" : "127.0.0.1",
            "port" : 8080
          }
        ]
      }' >>  cp orbitersdkd local_testnet_validator4/options.json

echo '{
        "rootPath": "local_testnet_discovery",
        "web3clientVersion": "OrbiterSDK/cpp/linux_x86-64/0.0.1",
        "version": 1,
        "chainID": 8080,
        "wsPort": 8085,
        "httpPort": 8094,
        "privKey": "0x81f288dd776f4edfe256d34af1f7d719f511559f19115af3e3d692e741faadc6",
        "discoveryNodes": [
          {
            "address" : "127.0.0.1",
            "port" : 8080
          }
        ]
      }' >>  cp orbitersdkd local_testnet_validator5/options.json





