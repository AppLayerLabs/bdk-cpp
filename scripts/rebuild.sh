#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

# Anti-sudo prevention
if [ $(id -u) -eq 0 ]; then
  echo "Please run this script as a normal user."
  exit
fi

echo "Cleaning currently running deamons."
isRunningAvalancheGo1=$((tmux ls || true) | grep -i "avalanchego-1" || true) 
isRunningAvalancheGo2=$((tmux ls || true) | grep -i "avalanchego-2" || true)  
isRunningAvalancheGo3=$((tmux ls || true) | grep -i "avalanchego-3" || true)  
isRunningAvalancheGo4=$((tmux ls || true) | grep -i "avalanchego-4" || true)  
isRunningAvalancheGo5=$((tmux ls || true) | grep -i "avalanchego-5" || true)  


if [ -n "$isRunningAvalancheGo1" ]; then tmux send-keys -t avalanchego-1 C-c; fi
if [ -n "$isRunningAvalancheGo2" ]; then tmux send-keys -t avalanchego-2 C-c; fi
if [ -n "$isRunningAvalancheGo3" ]; then tmux send-keys -t avalanchego-3 C-c; fi
if [ -n "$isRunningAvalancheGo4" ]; then tmux send-keys -t avalanchego-4 C-c; fi
if [ -n "$isRunningAvalancheGo5" ]; then tmux send-keys -t avalanchego-5 C-c; fi

# Wait until all tmux sessions stopped running.
while [ -n "$isRunningAvalancheGo1" ] || [ -n "$isRunningAvalancheGo2" ] || [ -n "$isRunningAvalancheGo3" ] || [ -n "$isRunningAvalancheGo4" ] || [ -n "$isRunningAvalancheGo5" ]
do
  echo "Waiting tmux's to stop..."
  sleep 3
  isRunningAvalancheGo1=$((tmux ls || true) | grep -i "avalanchego-1" || true)
  isRunningAvalancheGo2=$((tmux ls || true) | grep -i "avalanchego-2" || true)
  isRunningAvalancheGo3=$((tmux ls || true) | grep -i "avalanchego-3" || true)
  isRunningAvalancheGo4=$((tmux ls || true) | grep -i "avalanchego-4" || true)
  isRunningAvalancheGo5=$((tmux ls || true) | grep -i "avalanchego-5" || true)
done

SUBNETOOORD=($(dirname $(pwd))/build/subnetooord)       # subnetooord executable
AVA_PATH="$HOME/go/src/github.com/ava-labs/avalanchego" # AvalancheGo root folder
SUBNET_ID=$(find $AVA_PATH/build/plugins/* -maxdepth 1 -not -name "evm")  # previous subnetooord executable

# Build subnetooord
cd ../build && cmake --build . -j$(nproc)

# Clean some files
rm -rf $AVA_PATH/node1/{debug,log}.txt
rm -rf $AVA_PATH/node2/{debug,log}.txt
rm -rf $AVA_PATH/node3/{debug,log}.txt
rm -rf $AVA_PATH/node4/{debug,log}.txt
rm -rf $AVA_PATH/node5/{debug,log}.txt

# Copy the subnetooord executable to the AvalancheGo plugins folder using subnet ID as filename
rm -rf $SUBNET_ID
cp $SUBNETOOORD $SUBNET_ID

echo "Starting Daemons"

tmux new-session -d -s avalanchego-1 "cd ~/go/src/github.com/ava-labs/avalanchego/node1 && ./start1.sh"
tmux new-session -d -s avalanchego-2 "cd ~/go/src/github.com/ava-labs/avalanchego/node2 && ./start2.sh"
tmux new-session -d -s avalanchego-3 "cd ~/go/src/github.com/ava-labs/avalanchego/node3 && ./start3.sh"
tmux new-session -d -s avalanchego-4 "cd ~/go/src/github.com/ava-labs/avalanchego/node4 && ./start4.sh"
tmux new-session -d -s avalanchego-5 "cd ~/go/src/github.com/ava-labs/avalanchego/node5 && ./start5.sh"

