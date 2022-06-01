// License: GPLv3
// kode by @shunduquar (shung) and Borg

const { ethers } = require("ethers")
const { spawn } = require("child_process")
const { cpus } = require("os")
const { BigNumber } = ethers;
require("dotenv").config()

const privateKey = process.env.PRIVATE_KEY
const rpcUri = process.env.RPC_URI

const provider = new ethers.providers.JsonRpcProvider(rpcUri)
const signer = new ethers.Wallet(privateKey, provider)
const walletAddress = signer.address.toLowerCase().substring(2)
const bridgeAbi = [
    "function bridgeTo(address token, uint256 amount) public"
]


const bridgeContract = new ethers.Contract("0x000000000000427269646765436f6e7472616374", bridgeAbi, signer)

async function main() {
    try {
        const txResponse = await bridgeContract.bridgeTo(
            "0x96dd1f16dc8a5d2d21040dd018d9d6b90039a4ac",
            BigNumber.from("100000000000000000000")
        )

        const result = txResponse.wait();

    } catch (err) {
        console.log(err)
    }

}

main()
