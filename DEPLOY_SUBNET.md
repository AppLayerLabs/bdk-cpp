# How to deploy a Sparq Subnet

This guide will walk you through initializing a Sparq Subnet using Debian 11 and AvalancheGo.

## 1. Install Go

[Follow this guide to install go](https://www.itzgeek.com/how-tos/linux/debian/how-to-install-go-lang-on-debian-11-debian-10.html). Remember to replace the version with the latest one.

## 2. Install AvalancheGo

Install AvalancheGo in the recommended folder:

    mkdir ~/go/src/github.com/ava-labs/
    cd ~/go/src/github.com/ava-labs/
    git clone https://github.com/ava-labs/avalanchego
    cd avalanchego
    chmod +x scripts/build.sh
    ./scripts/build.sh

After that you should be ready to setup a node.

## 3. Setup testnet node

Create 5 scripts: `start1.sh`, `start2.sh`, `start3.sh`, `start4.sh` and `start5.sh`.

Then, put each command below in their respective scripts (one line per file):

    #!/bin/bash

    # This line in start1.sh
    ./build/avalanchego --public-ip=127.0.0.1 --http-port=9650 --staking-port=9651 --db-dir=db/node1 --network-id=local --staking-tls-cert-file=$(pwd)/staking/local/staker1.crt --staking-tls-key-file=$(pwd)/staking/local/staker1.key

    # This line in start2.sh
    ./build/avalanchego --public-ip=127.0.0.1 --http-port=9652 --staking-port=9653 --db-dir=db/node2 --network-id=local --bootstrap-ips=127.0.0.1:9651 --bootstrap-ids=NodeID-7Xhw2mDxuDS44j42TCB6U5579esbSt3Lg --staking-tls-cert-file=$(pwd)/staking/local/staker2.crt --staking-tls-key-file=$(pwd)/staking/local/staker2.key

    # This line in start3.sh
    ./build/avalanchego --public-ip=127.0.0.1 --http-port=9654 --staking-port=9655 --db-dir=db/node3 --network-id=local --bootstrap-ips=127.0.0.1:9651 --bootstrap-ids=NodeID-7Xhw2mDxuDS44j42TCB6U5579esbSt3Lg --staking-tls-cert-file=$(pwd)/staking/local/staker3.crt --staking-tls-key-file=$(pwd)/staking/local/staker3.key

    # This line in start4.sh
    ./build/avalanchego --public-ip=127.0.0.1 --http-port=9656 --staking-port=9657 --db-dir=db/node4 --network-id=local --bootstrap-ips=127.0.0.1:9651 --bootstrap-ids=NodeID-7Xhw2mDxuDS44j42TCB6U5579esbSt3Lg --staking-tls-cert-file=$(pwd)/staking/local/staker4.crt --staking-tls-key-file=$(pwd)/staking/local/staker4.key

    # This line in start5.sh
    ./build/avalanchego --public-ip=127.0.0.1 --http-port=9658 --staking-port=9659 --db-dir=db/node5 --network-id=local --bootstrap-ips=127.0.0.1:9651 --bootstrap-ids=NodeID-7Xhw2mDxuDS44j42TCB6U5579esbSt3Lg --staking-tls-cert-file=$(pwd)/staking/local/staker5.crt --staking-tls-key-file=$(pwd)/staking/local/staker5.key

`chmod +x` the scripts, open multiple terminals or a multiplexer like [tmux](https://github.com/tmux/tmux), and run the scripts in order.

Please note you're only required to run all scripts when you're setting up the subnet for the first time. Once it's properly set up you only need to run `start1.sh` to start it up, as that's the one we're focusing on here.

## 4. Create the Subnet

We'll need to do several requests to the first node in order to setup the subnet. Have [curl](https://curl.se/) ready.

### Create User

First of all, we need to create a user on your AvalancheGo node. Replace the `username` and `password` fields on each request below with your own.

Make sure to make a strong and complicated password, as AvalancheGo has requirements for blocking simple passwords.

**Store those credentials somewhere**. This should go without saying.

    curl --location --request POST '127.0.0.1:9650/ext/keystore' \
    --header 'Content-Type: application/json' \
    --data-raw '{
        "jsonrpc": "2.0",
        "id"     : 1,
        "method" : "keystore.createUser",
        "params" : {
            "username": "<your-username>",
            "password": "<your-password>"
        }
    }'

### Fund Network

After creating the user, we need to fund the local P-Chain network.

Private key was fetched from [AVAX docs](https://docs.avax.network/quickstart/fund-a-local-test-network#import-key-2).

    curl --location --request POST '127.0.0.1:9650/ext/bc/P' \
    --header 'Content-Type: application/json' \
    --data-raw '{
        "jsonrpc": "2.0",
        "id"     : 1,
        "method" : "platform.importKey",
        "params" : {
            "username"  : "<your-username>",
            "password"  : "<your-password>",
            "privateKey": "PrivateKey-ewoqjP7PxY4yr3iLTpLisriqt94hdyDFNgchSxGGztUrTXtNN"
        }
    }'

**Save the address output somewhere**. We'll use it later as a change address.

### Create Validator Addresses

With the local chain funded, we can create addresses that will be used as validators of the Subnet.

We need **two** addresses, so do the following request **twice** and note down both outputs:

    curl -X POST --data '{
        "jsonrpc": "2.0",
        "method" : "platform.createAddress",
        "params" : {
            "username": "<your-username>",
            "password": "<your-username>"
        },
        "id": 1
    }' -H 'content-type:application/json;' 127.0.0.1:9650/ext/P

In this example, we'll use the following addresses:

    P-local1k7pvlr8xhddgg7x0syxm8zzj3mgrgft6ftrpgz
    P-local1qxky2u775w2g24cmgl432w3ccxzdq8qg4xl6xv

### Create Subnet

Now we can create the proper Subnet using the addresses from the previous step as control keys. Remember to use your own addresses, so replace the fields accordingly.

    curl -X POST --data '{
        "jsonrpc": "2.0",
        "method" : "platform.createSubnet",
        "params" : {
            "controlKeys": [
                "P-local1k7pvlr8xhddgg7x0syxm8zzj3mgrgft6ftrpgz",
                "P-local1qxky2u775w2g24cmgl432w3ccxzdq8qg4xl6xv"
            ],
            "threshold": 2,
            "username" : "<your-username>",
            "password" : "<your-password>"
        },
        "id": 1
    }' -H 'content-type:application/json;' 127.0.0.1:9650/ext/P

**Save the "TXID" output somewhere**. This will be used later as the Subnet ID.

In this example, we'll use the following ID: `2AJnQwG5rfG5opVi5cf2Uvq6JYmXakgXnAc3CB4JMB3hGiuenQ`

### Get Node ID

Do the following request to get the node's ID, and save it somewhere:

    curl -X POST --data '{
        "jsonrpc": "2.0",
        "id"     : 1,
        "method" : "info.getNodeID"
    }' -H 'content-type:application/json;' 127.0.0.1:9650/ext/info

In this example, we'll use the following ID: `NodeID-7Xhw2mDxuDS44j42TCB6U5579esbSt3Lg`

### Get subnetooord

Have a compiled executable of the `subnetooor` project, copy it to `build/plugins/` and rename it to the Subnet's ID. Example:

    cp ~/Github/SparqNet/subnetooord/build/subnetooord ~/go/src/github.com/ava-labs/avalanchego/build/plugins/
    mv ./build/plugins/subnetooord ./build/plugins/2AJnQwG5rfG5opVi5cf2Uvq6JYmXakgXnAc3CB4JMB3hGiuenQ

### Whitelist Subnet

Kill the `start1.sh` script, open it and add `--whitelisted-subnets=<your-subnet-id>` to the end of the line. The command should look like this:

  ./build/avalanchego --public-ip=127.0.0.1 --http-port=9650 --staking-port=9651 --db-dir=db/node1 --network-id=local --staking-tls-cert-file=$(pwd)/staking/local/staker1.crt --staking-tls-key-file=$(pwd)/staking/local/staker1.key **--whitelisted-subnets=2AJnQwG5rfG5opVi5cf2Uvq6JYmXakgXnAc3CB4JMB3hGiuenQ**

After that, run the script again to start the Subnet.

### Add Subnet Validator

Now we should add the local node as a validator. Replace the `nodeID`, `subnetID` and `changeAddr` fields below with your own, do the request:

    curl -X POST --data '{
        "jsonrpc": "2.0",
        "method" : "platform.addSubnetValidator",
        "params" : {
            "nodeID"    : "NodeID-7Xhw2mDxuDS44j42TCB6U5579esbSt3Lg",
            "subnetID"  : "2AJnQwG5rfG5opVi5cf2Uvq6JYmXakgXnAc3CB4JMB3hGiuenQ",
            "startTime" : '$(date --date="10 minutes" +%s)',
            "endTime"   : '$(date --date="30 days" +%s)',
            "weight"    : 30,
            "changeAddr": "<your-change-address>",
            "username"  : "<your-username>",
            "password"  : "<your-password>"
        },
        "id": 1
    }' -H 'content-type:application/json;' 127.0.0.1:9650/ext/P

and wait 10 minutes. You can check the pending validators using this request (replace `subnetID` with your own):

    curl -X POST --data '{
        "jsonrpc": "2.0",
        "method" : "platform.getPendingValidators",
        "params" : {
            "subnetID": "2AJnQwG5rfG5opVi5cf2Uvq6JYmXakgXnAc3CB4JMB3hGiuenQ"
        },
        "id": 1
    }' -H 'content-type:application/json;' 127.0.0.1:9650/ext/P

### Create Subnet Blockchain

Finally, we will create the blockchain for the Subnet. Replace both `subnetID` and `vmID` with your Subnet ID.

We currently don't know where `genesisData` is used later on, so keep it as it is and store it somewhere just in case.

    curl -X POST --data '{
        "jsonrpc": "2.0",
        "method": "platform.createBlockchain",
        "params" : {
            "subnetID"   : "2AJnQwG5rfG5opVi5cf2Uvq6JYmXakgXnAc3CB4JMB3hGiuenQ",
            "vmID"       : "2AJnQwG5rfG5opVi5cf2Uvq6JYmXakgXnAc3CB4JMB3hGiuenQ",
            "name"       : "Subnetooor",
            "genesisData": "0x68656c6c6f776f726c648f8f07af",
            "username"   : "<your-username>",
            "password"   : "<your-password>"
        },
        "id": 1
    }' -H 'content-type:application/json;' 127.0.0.1:9650/ext/P

## Extra stuff

The next steps are optional, just for informational purposes.

### Increase Subnet Balance

    curl 127.0.0.1:30000 --data '{
        "method"  : "IncreaseBalance",
        "address" : "<your-address>"
    }'

### Connect with Metamask

Add a new network in Metamask pointing to `http://localhost:30000`. Chain ID is 8848.

