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
const erc20Abi = [
    "function balanceOf(address) public view returns (uint256)"
]

const uniswapAbi = [
    "function addLiquidityAVAX(address,uint256,uint256,uint256,address,uint256) external payable"
]
const erc20Token = new ethers.Contract("0x96dd1f16dc8a5d2d21040dd018d9d6b90039a4ac", erc20Abi, signer)
const uniswapContract = new ethers.Contract("0x00000000000000000000000000756e6973776170", uniswapAbi, signer)

async function main() {
    console.log(ethers.utils.formatUnits(await erc20Token.balanceOf("0x798333f07163eb62d1e22cc2df1acfe597567882"),0))


    try {
        const txResponse = await uniswapContract.addLiquidityAVAX(
            "0x96dd1f16dc8a5d2d21040dd018d9d6b90039a4ac",
            BigNumber.from("10000000000000000000"),
            0, // Ignored for now
            0, // Ignored for now
            "0x798333f07163eb62d1e22cc2df1acfe597567882",
            0, // Ignored for now
            { value: ethers.utils.parseEther("1.0") }
        )
        //const result = txResponse.wait();

    } catch (err) {
        console.log(err)
    }

}

main()
