#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

# Anti-sudo prevention
if [ $(id -u) -eq 0 ]; then
  echo "Please run this script as a normal user."
  exit
fi

# Check dependencies before starting
LIB_DEPS=""
EXE_DEPS=""
if ! [ "$(/sbin/ldconfig -p | grep libabsl)" ]; then LIB_DEPS+="libabsl "; fi
if ! [ "$(/sbin/ldconfig -p | grep libboost_chrono)" ]; then LIB_DEPS+="libboost-chrono "; fi
if ! [ "$(/sbin/ldconfig -p | grep libboost_filesystem)" ]; then LIB_DEPS+="libboost-filesystem "; fi
if ! [ "$(/sbin/ldconfig -p | grep libboost_nowide)" ]; then LIB_DEPS+="libboost-nowide "; fi
if ! [ "$(/sbin/ldconfig -p | grep libboost_program_options)" ]; then LIB_DEPS+="libboost-program-options "; fi
if ! [ "$(/sbin/ldconfig -p | grep libboost_system)" ]; then LIB_DEPS+="libboost-system "; fi
if ! [ "$(/sbin/ldconfig -p | grep libboost_thread)" ]; then LIB_DEPS+="libboost-thread "; fi
if ! [ "$(/sbin/ldconfig -p | grep libcares)" ]; then LIB_DEPS+="libcares "; fi
if ! [ "$(/sbin/ldconfig -p | grep libcrypto++)" ]; then LIB_DEPS+="libcrypto++ "; fi
if ! [ "$(/sbin/ldconfig -p | grep libgrpc)" ]; then LIB_DEPS+="libgrpc "; fi
if ! [ "$(/sbin/ldconfig -p | grep libgrpc++)" ]; then LIB_DEPS+="libgrpc++ "; fi
if ! [ "$(/sbin/ldconfig -p | grep libleveldb)" ]; then LIB_DEPS+="libleveldb "; fi
if ! [ "$(/sbin/ldconfig -p | grep libscrypt)" ]; then LIB_DEPS+="libscrypt "; fi
if ! [ "$(/sbin/ldconfig -p | grep libsnappy)" ]; then LIB_DEPS+="libsnappy "; fi
if ! [ "$(/sbin/ldconfig -p | grep libssl)" ]; then LIB_DEPS+="libssl "; fi
if ! [ "$(/sbin/ldconfig -p | grep libz)" ]; then LIB_DEPS+="zlib "; fi
if ! [ $(command -v autoconf) ]; then EXE_DEPS+="autoconf "; fi
if ! [ $(command -v cmake) ]; then EXE_DEPS+="cmake "; fi
if ! [ $(command -v curl) ]; then EXE_DEPS+="curl "; fi
if ! [ $(command -v g++) ]; then EXE_DEPS+="g++ "; fi
if ! [ $(command -v gcc) ]; then EXE_DEPS+="gcc "; fi
if ! [ $(command -v git) ]; then EXE_DEPS+="git "; fi
if ! [ $(command -v go) ]; then EXE_DEPS+="go "; fi
if ! [ $(command -v grpc_cpp_plugin) ]; then EXE_DEPS+="protobuf-compiler-grpc "; fi
if ! [ $(command -v jq) ]; then EXE_DEPS+="jq "; fi
if ! [ $(command -v libtoolize) ]; then EXE_DEPS+="libtool "; fi
if ! [ $(command -v make) ]; then EXE_DEPS+="make "; fi
if ! [ $(command -v openssl) ]; then EXE_DEPS+="openssl "; fi
if ! [ $(command -v pkg-config) ]; then EXE_DEPS+="pkg-config "; fi
if ! [ $(command -v protoc) ]; then EXE_DEPS+="protobuf-compiler "; fi
if ! [ $(command -v tmux) ]; then EXE_DEPS+="tmux "; fi
if [ -n "$LIB_DEPS" ] || [ -n "$EXE_DEPS" ]; then
  echo "Please install the missing dependencies for the script to run:"
  if [ -n "$LIB_DEPS" ]; then echo -e "Libraries:\n$LIB_DEPS"; fi
  if [ -n "$EXE_DEPS" ]; then echo -e "Programs:\n$EXE_DEPS"; fi
  exit
fi

LOGFILE=($(pwd)/AIO-setup.log)                          # Output log file for the script
SUBNETOOORD=($(dirname $(pwd))/build/subnetooord)       # subnetooord executable path
AVALANCHE_ROOT_PATH="$HOME/go/src/github.com/ava-labs"  # AvalancheGo root folder

# Build subnetooord
cd ../build/
cmake .. && make -j$(nproc)

# Clone and build latest AvalancheGo
mkdir -p $AVALANCHE_ROOT_PATH
cd $AVALANCHE_ROOT_PATH
git clone https://github.com/ava-labs/avalanchego
cd avalanchego
chmod +x scripts/build.sh
./scripts/build.sh

# Setup script files for 5 local nodes
mkdir node1 node2 node3 node4 node5
echo -n "./../build/avalanchego --public-ip=127.0.0.1 --http-port=9650 --staking-port=9651 --db-dir=db/node1 --network-id=local --staking-tls-cert-file=$(pwd)/staking/local/staker1.crt --staking-tls-key-file=$(pwd)/staking/local/staker1.key" >> node1/start1.sh
echo -n "./../build/avalanchego --public-ip=127.0.0.1 --http-port=9652 --staking-port=9653 --db-dir=db/node2 --network-id=local --bootstrap-ips=127.0.0.1:9651 --bootstrap-ids=NodeID-7Xhw2mDxuDS44j42TCB6U5579esbSt3Lg --staking-tls-cert-file=$(pwd)/staking/local/staker2.crt --staking-tls-key-file=$(pwd)/staking/local/staker2.key" >> node2/start2.sh
echo -n "./../build/avalanchego --public-ip=127.0.0.1 --http-port=9654 --staking-port=9655 --db-dir=db/node3 --network-id=local --bootstrap-ips=127.0.0.1:9651 --bootstrap-ids=NodeID-7Xhw2mDxuDS44j42TCB6U5579esbSt3Lg --staking-tls-cert-file=$(pwd)/staking/local/staker3.crt --staking-tls-key-file=$(pwd)/staking/local/staker3.key" >> node3/start3.sh
echo -n "./../build/avalanchego --public-ip=127.0.0.1 --http-port=9656 --staking-port=9657 --db-dir=db/node4 --network-id=local --bootstrap-ips=127.0.0.1:9651 --bootstrap-ids=NodeID-7Xhw2mDxuDS44j42TCB6U5579esbSt3Lg --staking-tls-cert-file=$(pwd)/staking/local/staker4.crt --staking-tls-key-file=$(pwd)/staking/local/staker4.key" >> node4/start4.sh
echo -n "./../build/avalanchego --public-ip=127.0.0.1 --http-port=9658 --staking-port=9659 --db-dir=db/node5 --network-id=local --bootstrap-ips=127.0.0.1:9651 --bootstrap-ids=NodeID-7Xhw2mDxuDS44j42TCB6U5579esbSt3Lg --staking-tls-cert-file=$(pwd)/staking/local/staker5.crt --staking-tls-key-file=$(pwd)/staking/local/staker5.key" >> node5/start5.sh
printf "{\n  \"rpcport\": 8080\n}" >> node1/config.json
printf "{\n  \"rpcport\": 8081\n}" >> node2/config.json
printf "{\n  \"rpcport\": 8082\n}" >> node3/config.json
printf "{\n  \"rpcport\": 8083\n}" >> node4/config.json
printf "{\n  \"rpcport\": 8084\n}" >> node5/config.json

chmod +x node1/start1.sh node2/start2.sh node3/start3.sh node4/start4.sh node5/start5.sh

# Start the local nodes using tmux and wait 30 seconds for bootstrapping
tmux new-session -d -s avalanchego-1 "cd node1 && ./start1.sh"
tmux new-session -d -s avalanchego-2 "cd node2 && ./start2.sh"
tmux new-session -d -s avalanchego-3 "cd node3 && ./start3.sh"
tmux new-session -d -s avalanchego-4 "cd node4 && ./start4.sh"
tmux new-session -d -s avalanchego-5 "cd node5 && ./start5.sh"
sleep 15

# Create a user with random data (username and password)
USERNAME=$(openssl rand -base64 16)
PASSWORD=$(openssl rand -base64 32)
echo "USERNAME: $USERNAME --- PASSWORD: $PASSWORD" >> $LOGFILE
SETUP_USER_OUTPUT=$(curl --location --request POST '127.0.0.1:9650/ext/keystore' --header 'Content-Type: application/json' \
--data-raw '{
  "jsonrpc": "2.0",
  "id"     : 1,
  "method" : "keystore.createUser",
  "params" : {
    "username": "'$USERNAME'",
    "password": "'$PASSWORD'"
  }
}')
sleep 1

SECOND_SETUP_USER_OUTPUT=$(curl --location --request POST '127.0.0.1:9652/ext/keystore' --header 'Content-Type: application/json' \
--data-raw '{
  "jsonrpc": "2.0",
  "id"     : 1,
  "method" : "keystore.createUser",
  "params" : {
    "username": "'$USERNAME'",
    "password": "'$PASSWORD'"
  }
}')
sleep 1

THIRD_SETUP_USER_OUTPUT=$(curl --location --request POST '127.0.0.1:9654/ext/keystore' --header 'Content-Type: application/json' \
--data-raw '{
  "jsonrpc": "2.0",
  "id"     : 1,
  "method" : "keystore.createUser",
  "params" : {
    "username": "'$USERNAME'",
    "password": "'$PASSWORD'"
  }
}')
sleep 1

FORTH_SETUP_USER_OUTPUT=$(curl --location --request POST '127.0.0.1:9656/ext/keystore' --header 'Content-Type: application/json' \
--data-raw '{
  "jsonrpc": "2.0",
  "id"     : 1,
  "method" : "keystore.createUser",
  "params" : {
    "username": "'$USERNAME'",
    "password": "'$PASSWORD'"
  }
}')
sleep 1

FIFTH_SETUP_USER_OUTPUT=$(curl --location --request POST '127.0.0.1:9658/ext/keystore' --header 'Content-Type: application/json' \
--data-raw '{
  "jsonrpc": "2.0",
  "id"     : 1,
  "method" : "keystore.createUser",
  "params" : {
    "username": "'$USERNAME'",
    "password": "'$PASSWORD'"
  }
}')
sleep 1

# Fund the network
FUND_NETWORK_OUTPUT=$(curl --location --request POST '127.0.0.1:9650/ext/bc/P' --header 'Content-Type: application/json' \
--data-raw '{
  "jsonrpc": "2.0",
  "id"     : 1,
  "method" : "platform.importKey",
  "params" : {
    "username"  : "'$USERNAME'",
    "password"  : "'$PASSWORD'",
    "privateKey": "PrivateKey-ewoqjP7PxY4yr3iLTpLisriqt94hdyDFNgchSxGGztUrTXtNN"
  }
}')
FUNDING_ADDRESS=$(echo $FUND_NETWORK_OUTPUT | jq '.result.address' | sed 's/^"//' | sed 's/"$//')
echo "Funding Address: " $FUNDING_ADDRESS >> $LOGFILE
sleep 1

# Create Validator Addresses
CREATE_ADDRESS_1_OUTPUT=$(curl -X POST \
--data '{
  "jsonrpc": "2.0",
  "method" : "platform.createAddress",
  "params" : {
    "username": "'$USERNAME'",
    "password": "'$PASSWORD'"
    },
  "id": 1
}' -H 'content-type:application/json;' 127.0.0.1:9650/ext/P)

CREATE_ADDRESS_2_OUTPUT=$(curl -X POST \
--data '{
  "jsonrpc": "2.0",
  "method" : "platform.createAddress",
  "params" : {
    "username": "'$USERNAME'",
    "password": "'$PASSWORD'"
    },
  "id": 1
}' -H 'content-type:application/json;' 127.0.0.1:9650/ext/P)

CREATE_ADDRESS_3_OUTPUT=$(curl -X POST \
--data '{
  "jsonrpc": "2.0",
  "method" : "platform.createAddress",
  "params" : {
    "username": "'$USERNAME'",
    "password": "'$PASSWORD'"
    },
  "id": 1
}' -H 'content-type:application/json;' 127.0.0.1:9652/ext/P)


CREATE_ADDRESS_4_OUTPUT=$(curl -X POST \
--data '{
  "jsonrpc": "2.0",
  "method" : "platform.createAddress",
  "params" : {
    "username": "'$USERNAME'",
    "password": "'$PASSWORD'"
    },
  "id": 1
}' -H 'content-type:application/json;' 127.0.0.1:9652/ext/P)

CREATE_ADDRESS_5_OUTPUT=$(curl -X POST \
--data '{
  "jsonrpc": "2.0",
  "method" : "platform.createAddress",
  "params" : {
    "username": "'$USERNAME'",
    "password": "'$PASSWORD'"
    },
  "id": 1
}' -H 'content-type:application/json;' 127.0.0.1:9654/ext/P)


CREATE_ADDRESS_6_OUTPUT=$(curl -X POST \
--data '{
  "jsonrpc": "2.0",
  "method" : "platform.createAddress",
  "params" : {
    "username": "'$USERNAME'",
    "password": "'$PASSWORD'"
    },
  "id": 1
}' -H 'content-type:application/json;' 127.0.0.1:9654/ext/P)


CREATE_ADDRESS_7_OUTPUT=$(curl -X POST \
--data '{
  "jsonrpc": "2.0",
  "method" : "platform.createAddress",
  "params" : {
    "username": "'$USERNAME'",
    "password": "'$PASSWORD'"
    },
  "id": 1
}' -H 'content-type:application/json;' 127.0.0.1:9656/ext/P)


CREATE_ADDRESS_8_OUTPUT=$(curl -X POST \
--data '{
  "jsonrpc": "2.0",
  "method" : "platform.createAddress",
  "params" : {
    "username": "'$USERNAME'",
    "password": "'$PASSWORD'"
    },
  "id": 1
}' -H 'content-type:application/json;' 127.0.0.1:9656/ext/P)


CREATE_ADDRESS_9_OUTPUT=$(curl -X POST \
--data '{
  "jsonrpc": "2.0",
  "method" : "platform.createAddress",
  "params" : {
    "username": "'$USERNAME'",
    "password": "'$PASSWORD'"
    },
  "id": 1
}' -H 'content-type:application/json;' 127.0.0.1:9658/ext/P)


CREATE_ADDRESS_10_OUTPUT=$(curl -X POST \
--data '{
  "jsonrpc": "2.0",
  "method" : "platform.createAddress",
  "params" : {
    "username": "'$USERNAME'",
    "password": "'$PASSWORD'"
    },
  "id": 1
}' -H 'content-type:application/json;' 127.0.0.1:9658/ext/P)



VALIDATOR_ADDRESS_1=$(echo $CREATE_ADDRESS_1_OUTPUT | jq '.result.address' | sed 's/^"//' | sed 's/"$//')
VALIDATOR_ADDRESS_2=$(echo $CREATE_ADDRESS_2_OUTPUT | jq '.result.address' | sed 's/^"//' | sed 's/"$//')
VALIDATOR_ADDRESS_3=$(echo $CREATE_ADDRESS_3_OUTPUT | jq '.result.address' | sed 's/^"//' | sed 's/"$//')
VALIDATOR_ADDRESS_4=$(echo $CREATE_ADDRESS_4_OUTPUT | jq '.result.address' | sed 's/^"//' | sed 's/"$//')
VALIDATOR_ADDRESS_5=$(echo $CREATE_ADDRESS_5_OUTPUT | jq '.result.address' | sed 's/^"//' | sed 's/"$//')
VALIDATOR_ADDRESS_6=$(echo $CREATE_ADDRESS_6_OUTPUT | jq '.result.address' | sed 's/^"//' | sed 's/"$//')
VALIDATOR_ADDRESS_7=$(echo $CREATE_ADDRESS_7_OUTPUT | jq '.result.address' | sed 's/^"//' | sed 's/"$//')
VALIDATOR_ADDRESS_8=$(echo $CREATE_ADDRESS_8_OUTPUT | jq '.result.address' | sed 's/^"//' | sed 's/"$//')
VALIDATOR_ADDRESS_9=$(echo $CREATE_ADDRESS_9_OUTPUT | jq '.result.address' | sed 's/^"//' | sed 's/"$//')
VALIDATOR_ADDRESS_10=$(echo $CREATE_ADDRESS_10_OUTPUT | jq '.result.address' | sed 's/^"//' | sed 's/"$//')

echo "VALIDATOR 1 ADDRESS: " $VALIDATOR_ADDRESS_1 >> $LOGFILE
echo "VALIDATOR 2 ADDRESS: " $VALIDATOR_ADDRESS_2 >> $LOGFILE
echo "VALIDATOR 3 ADDRESS: " $VALIDATOR_ADDRESS_3 >> $LOGFILE
echo "VALIDATOR 4 ADDRESS: " $VALIDATOR_ADDRESS_4 >> $LOGFILE
echo "VALIDATOR 5 ADDRESS: " $VALIDATOR_ADDRESS_5 >> $LOGFILE
echo "VALIDATOR 6 ADDRESS: " $VALIDATOR_ADDRESS_6 >> $LOGFILE
echo "VALIDATOR 7 ADDRESS: " $VALIDATOR_ADDRESS_7 >> $LOGFILE
echo "VALIDATOR 8 ADDRESS: " $VALIDATOR_ADDRESS_8 >> $LOGFILE
echo "VALIDATOR 9 ADDRESS: " $VALIDATOR_ADDRESS_9 >> $LOGFILE
echo "VALIDATOR 10 ADDRESS: " $VALIDATOR_ADDRESS_10 >> $LOGFILE

# Create Subnet
CREATE_SUBNET_OUTPUT=$(curl -X POST \
--data '{
  "jsonrpc": "2.0",
  "method" : "platform.createSubnet",
  "params" : {
    "controlKeys": [
      "'$VALIDATOR_ADDRESS_1'",
      "'$VALIDATOR_ADDRESS_2'",
      "'$VALIDATOR_ADDRESS_3'",
      "'$VALIDATOR_ADDRESS_4'",
      "'$VALIDATOR_ADDRESS_5'",
      "'$VALIDATOR_ADDRESS_6'",
      "'$VALIDATOR_ADDRESS_7'",
      "'$VALIDATOR_ADDRESS_8'",
      "'$VALIDATOR_ADDRESS_9'",
      "'$VALIDATOR_ADDRESS_10'"
    ],
    "threshold": 2,
    "username" : "'$USERNAME'",
    "password" : "'$PASSWORD'"
    },
  "id": 1
}' -H 'content-type:application/json;' 127.0.0.1:9650/ext/P)

SUBNET_ID=$(echo $CREATE_SUBNET_OUTPUT | jq '.result.txID' | sed 's/^"//' | sed 's/"$//')
echo "SUBNET ID: " $SUBNET_ID >> $LOGFILE
sleep 1
# Get Node ID
NODE_ID_OUTPUT=$(curl -X POST \
--data '{
  "jsonrpc": "2.0",
  "id"     : 1,
  "method" : "info.getNodeID"
}' -H 'content-type:application/json;' 127.0.0.1:9650/ext/info)

NODE_ID=$(echo $NODE_ID_OUTPUT | jq '.result.nodeID' | sed 's/^"//' | sed 's/"$//')

SECOND_NODE_ID_OUTPUT=$(curl -X POST \
--data '{
  "jsonrpc": "2.0",
  "id"     : 1,
  "method" : "info.getNodeID"
}' -H 'content-type:application/json;' 127.0.0.1:9652/ext/info)

SECOND_NODE_ID=$(echo $SECOND_NODE_ID_OUTPUT | jq '.result.nodeID' | sed 's/^"//' | sed 's/"$//')

THIRD_NODE_ID_OUTPUT=$(curl -X POST \
--data '{
  "jsonrpc": "2.0",
  "id"     : 1,
  "method" : "info.getNodeID"
}' -H 'content-type:application/json;' 127.0.0.1:9654/ext/info)

THIRD_NODE_ID=$(echo $THIRD_NODE_ID_OUTPUT | jq '.result.nodeID' | sed 's/^"//' | sed 's/"$//')

FORTH_NODE_ID_OUTPUT=$(curl -X POST \
--data '{
  "jsonrpc": "2.0",
  "id"     : 1,
  "method" : "info.getNodeID"
}' -H 'content-type:application/json;' 127.0.0.1:9656/ext/info)

FORTH_NODE_ID=$(echo $FORTH_NODE_ID_OUTPUT | jq '.result.nodeID' | sed 's/^"//' | sed 's/"$//')

FIFTH_NODE_ID_OUTPUT=$(curl -X POST \
--data '{
  "jsonrpc": "2.0",
  "id"     : 1,
  "method" : "info.getNodeID"
}' -H 'content-type:application/json;' 127.0.0.1:9658/ext/info)

FIFTH_NODE_ID=$(echo $FIFTH_NODE_ID_OUTPUT | jq '.result.nodeID' | sed 's/^"//' | sed 's/"$//')

# Copy the subnetooord executable to the AvalancheGo plugins folder using subnet ID as filename
cp $SUBNETOOORD $AVALANCHE_ROOT_PATH/avalanchego/build/plugins/$SUBNET_ID

# Stop AvalancheGo-1 and update script to include the subnet
while [ -n "$(tmux ls | grep -i "avalanchego-1")" ]
do
  tmux send-keys -t avalanchego-1 C-c
  sleep 1
done

while [ -n "$(tmux ls | grep -i "avalanchego-2")" ]
do
  tmux send-keys -t avalanchego-2 C-c
  sleep 1
done

while [ -n "$(tmux ls | grep -i "avalanchego-3")" ]
do
  tmux send-keys -t avalanchego-3 C-c
  sleep 1
done

while [ -n "$(tmux ls | grep -i "avalanchego-4")" ]
do
  tmux send-keys -t avalanchego-4 C-c
  sleep 1
done

while [ -n "$(tmux ls | grep -i "avalanchego-5")" ]
do
  tmux send-keys -t avalanchego-5 C-c
  sleep 1
done

echo " --whitelisted-subnets="$SUBNET_ID >> $AVALANCHE_ROOT_PATH/avalanchego/node1/start1.sh
echo " --whitelisted-subnets="$SUBNET_ID >> $AVALANCHE_ROOT_PATH/avalanchego/node2/start2.sh
echo " --whitelisted-subnets="$SUBNET_ID >> $AVALANCHE_ROOT_PATH/avalanchego/node3/start3.sh
echo " --whitelisted-subnets="$SUBNET_ID >> $AVALANCHE_ROOT_PATH/avalanchego/node4/start4.sh
echo " --whitelisted-subnets="$SUBNET_ID >> $AVALANCHE_ROOT_PATH/avalanchego/node5/start5.sh

# Start AvalancheGo-1 again ad wait 10 seconds for initialization
tmux new-session -d -s avalanchego-1 "cd node1 && ./start1.sh"
tmux new-session -d -s avalanchego-2 "cd node2 && ./start2.sh"
tmux new-session -d -s avalanchego-3 "cd node3 && ./start3.sh"
tmux new-session -d -s avalanchego-4 "cd node4 && ./start4.sh"
tmux new-session -d -s avalanchego-5 "cd node5 && ./start5.sh"
sleep 10

# Add subnet validator and wait 10 secs to start the subnet
START_TIME=$(date --date="1 minutes" +%s)
END_TIME=$(date --date="10 days" +%s)
SUBNET_VALIDATOR_OUTPUT=$(curl -X POST \
--data '{
  "jsonrpc": "2.0",
  "method" : "platform.addSubnetValidator",
  "params" : {
      "nodeID"    : "'$NODE_ID'",
      "subnetID"  : "'$SUBNET_ID'",
      "startTime" : '$START_TIME',
      "endTime"   : '$END_TIME',
      "weight"    : 30,
      "changeAddr": "'$FUNDING_ADDRESS'",
      "username"  : "'$USERNAME'",
      "password"  : "'$PASSWORD'"
  },
  "id": 1
}' -H 'content-type:application/json;' 127.0.0.1:9650/ext/P)

echo "SUBNET VALIDATOR OUTPUT: " $SUBNET_VALIDATOR_OUTPUT >> $LOGFILE
sleep 3
SECOND_SUBNET_VALIDATOR_OUTPUT=$(curl -X POST \
--data '{
  "jsonrpc": "2.0",
  "method" : "platform.addSubnetValidator",
  "params" : {
      "nodeID"    : "'$SECOND_NODE_ID'",
      "subnetID"  : "'$SUBNET_ID'",
      "startTime" : '$START_TIME',
      "endTime"   : '$END_TIME',
      "weight"    : 30,
      "changeAddr": "'$FUNDING_ADDRESS'",
      "username"  : "'$USERNAME'",
      "password"  : "'$PASSWORD'"
  },
  "id": 1
}' -H 'content-type:application/json;' 127.0.0.1:9650/ext/P)

echo "SUBNET VALIDATOR OUTPUT: " $SECOND_SUBNET_VALIDATOR_OUTPUT >> $LOGFILE
sleep 3
THIRD_SUBNET_VALIDATOR_OUTPUT=$(curl -X POST \
--data '{
  "jsonrpc": "2.0",
  "method" : "platform.addSubnetValidator",
  "params" : {
      "nodeID"    : "'$THIRD_NODE_ID'",
      "subnetID"  : "'$SUBNET_ID'",
      "startTime" : '$START_TIME',
      "endTime"   : '$END_TIME',
      "weight"    : 30,
      "changeAddr": "'$FUNDING_ADDRESS'",
      "username"  : "'$USERNAME'",
      "password"  : "'$PASSWORD'"
  },
  "id": 1
}' -H 'content-type:application/json;' 127.0.0.1:9650/ext/P)

echo "SUBNET VALIDATOR OUTPUT: " $THIRD_SUBNET_VALIDATOR_OUTPUT >> $LOGFILE
sleep 3
FORTH_SUBNET_VALIDATOR_OUTPUT=$(curl -X POST \
--data '{
  "jsonrpc": "2.0",
  "method" : "platform.addSubnetValidator",
  "params" : {
      "nodeID"    : "'$FORTH_NODE_ID'",
      "subnetID"  : "'$SUBNET_ID'",
      "startTime" : '$START_TIME',
      "endTime"   : '$END_TIME',
      "weight"    : 30,
      "changeAddr": "'$FUNDING_ADDRESS'",
      "username"  : "'$USERNAME'",
      "password"  : "'$PASSWORD'"
  },
  "id": 1
}' -H 'content-type:application/json;' 127.0.0.1:9650/ext/P)

echo "SUBNET VALIDATOR OUTPUT: " $FORTH_SUBNET_VALIDATOR_OUTPUT >> $LOGFILE
sleep 3
FIFTH_SUBNET_VALIDATOR_OUTPUT=$(curl -X POST \
--data '{
  "jsonrpc": "2.0",
  "method" : "platform.addSubnetValidator",
  "params" : {
      "nodeID"    : "'$FIFTH_NODE_ID'",
      "subnetID"  : "'$SUBNET_ID'",
      "startTime" : '$START_TIME',
      "endTime"   : '$END_TIME',
      "weight"    : 30,
      "changeAddr": "'$FUNDING_ADDRESS'",
      "username"  : "'$USERNAME'",
      "password"  : "'$PASSWORD'"
  },
  "id": 1
}' -H 'content-type:application/json;' 127.0.0.1:9650/ext/P)

echo "SUBNET VALIDATOR OUTPUT: " $FIFTH_SUBNET_VALIDATOR_OUTPUT >> $LOGFILE

echo "Waiting 1 minute to start subnet..."
sleep 70

# Create Subnet Blockchain
CREATE_SUBNET_OUTPUT=$(curl -X POST \
--data '{
  "jsonrpc": "2.0",
  "method": "platform.createBlockchain",
  "params" : {
      "subnetID"   : "'$SUBNET_ID'",
      "vmID"       : "'$SUBNET_ID'",
      "name"       : "Subnetooor",
      "genesisData": "0x68656c6c6f776f726c648f8f07af",
      "username"   : "'$USERNAME'",
      "password"   : "'$PASSWORD'"
  },
  "id": 1
}' -H 'content-type:application/json;' 127.0.0.1:9650/ext/P)

echo "CREATE SUBNET OUTPUT: " $CREATE_SUBNET_OUTPUT >> $LOGFILE

