#!/usr/bin/env bash

isRunningAvalancheGo1=$(tmux ls | grep -i "avalanchego-1")
isRunningAvalancheGo2=$(tmux ls | grep -i "avalanchego-2")
isRunningAvalancheGo3=$(tmux ls | grep -i "avalanchego-3")
isRunningAvalancheGo4=$(tmux ls | grep -i "avalanchego-4")
isRunningAvalancheGo5=$(tmux ls | grep -i "avalanchego-5")

if [ -z "$isRunningAvalancheGo1" ]; then tmux new-session -d -s avalanchego-1 "cd ~/go/src/github.com/ava-labs/avalanchego/node1 && ./start1.sh"; fi
if [ -z "$isRunningAvalancheGo2" ]; then tmux new-session -d -s avalanchego-2 "cd ~/go/src/github.com/ava-labs/avalanchego/node2 && ./start2.sh"; fi
if [ -z "$isRunningAvalancheGo3" ]; then tmux new-session -d -s avalanchego-3 "cd ~/go/src/github.com/ava-labs/avalanchego/node3 && ./start3.sh"; fi
if [ -z "$isRunningAvalancheGo4" ]; then tmux new-session -d -s avalanchego-4 "cd ~/go/src/github.com/ava-labs/avalanchego/node4 && ./start4.sh"; fi
if [ -z "$isRunningAvalancheGo5" ]; then tmux new-session -d -s avalanchego-5 "cd ~/go/src/github.com/ava-labs/avalanchego/node5 && ./start5.sh"; fi

