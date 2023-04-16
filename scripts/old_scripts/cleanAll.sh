#!/usr/bin/env bash

# Anti-sudo prevention
if [ $(id -u) -eq 0 ]; then
  echo "Please run this script as a normal user."
  exit
fi

rm -rf ~/go/src/github.com/ava-labs/avalanchego/node1/debug.txt
rm -rf ~/go/src/github.com/ava-labs/avalanchego/node1/log.txt
rm -rf ~/go/src/github.com/ava-labs/avalanchego/node2/debug.txt
rm -rf ~/go/src/github.com/ava-labs/avalanchego/node2/log.txt
rm -rf ~/go/src/github.com/ava-labs/avalanchego/node3/debug.txt
rm -rf ~/go/src/github.com/ava-labs/avalanchego/node3/log.txt
rm -rf ~/go/src/github.com/ava-labs/avalanchego/node4/debug.txt
rm -rf ~/go/src/github.com/ava-labs/avalanchego/node4/log.txt
rm -rf ~/go/src/github.com/ava-labs/avalanchego/node5/debug.txt
rm -rf ~/go/src/github.com/ava-labs/avalanchego/node5/log.txt

