#!/usr/bin/env bash

isRunningAvalancheGo1=$(tmux ls | grep -i "avalanchego-1")
isRunningAvalancheGo2=$(tmux ls | grep -i "avalanchego-2")
isRunningAvalancheGo3=$(tmux ls | grep -i "avalanchego-3")
isRunningAvalancheGo4=$(tmux ls | grep -i "avalanchego-4")
isRunningAvalancheGo5=$(tmux ls | grep -i "avalanchego-5")

if [ -n "$isRunningAvalancheGo1" ]; then tmux send-keys -t avalanchego-1 C-c; fi
if [ -n "$isRunningAvalancheGo2" ]; then tmux send-keys -t avalanchego-2 C-c; fi
if [ -n "$isRunningAvalancheGo3" ]; then tmux send-keys -t avalanchego-3 C-c; fi
if [ -n "$isRunningAvalancheGo4" ]; then tmux send-keys -t avalanchego-4 C-c; fi
if [ -n "$isRunningAvalancheGo5" ]; then tmux send-keys -t avalanchego-5 C-c; fi

