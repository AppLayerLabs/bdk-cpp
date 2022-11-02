#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

# Anti-sudo prevention
if [ $(id -u) -eq 0 ]; then
  echo "Please run this script as a normal user."
  exit
fi

SUBNETOOORD=($(dirname $(pwd))/build/subnetooord)       # subnetooord executable
AVA_PATH="$HOME/go/src/github.com/ava-labs/avalanchego" # AvalancheGo root folder
SUBNET_ID=$(find $AVA_PATH/build/plugins/* -maxdepth 1 -not -name "evm")  # previous subnetooord executable

# Build subnetooord
cd ../build && cmake --build . -j$(nproc)

# Clean some files
rm -rf $AVA_PATH/node1/$(find $AVA_PATH/node1 -type d)
rm -rf $AVA_PATH/node1/{debug,log}.txt
rm -rf $AVA_PATH/node2/$(find $AVA_PATH/node2 -type d)
rm -rf $AVA_PATH/node2/{debug,log}.txt
rm -rf $AVA_PATH/node3/$(find $AVA_PATH/node3 -type d)
rm -rf $AVA_PATH/node3/{debug,log}.txt
rm -rf $AVA_PATH/node4/$(find $AVA_PATH/node4 -type d)
rm -rf $AVA_PATH/node4/{debug,log}.txt
rm -rf $AVA_PATH/node5/$(find $AVA_PATH/node5 -type d)
rm -rf $AVA_PATH/node5/{debug,log}.txt

# Copy the subnetooord executable to the AvalancheGo plugins folder using subnet ID as filename
rm -rf $SUBNET_ID
cp $SUBNETOOORD $SUBNET_ID

