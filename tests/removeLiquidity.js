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
    "function addLiquidityAVAX(address,uint256,uint256,uint256,address,uint256) external payable",
    "function removeLiquidityAVAX(address,uint256,uint256,uint256,address,uint256) external"
]
const erc20Token = new ethers.Contract("0x010101010101010101010174657374746f6b656e", erc20Abi, signer)
const uniswapContract = new ethers.Contract("0x00000000000000000000000000756e6973776170", uniswapAbi, signer)

async function main() {
    console.log(ethers.utils.formatUnits(await erc20Token.balanceOf("0xe6a2d1ef7d7129d2a422af0a725629a0a1fbdec4"),0))


    try {
        const txResponse = await uniswapContract.removeLiquidityAVAX(
            "0x010101010101010101010174657374746f6b656e",
            BigNumber.from("3000000000000000000"),
            0, // Ignored for now
            0, // Ignored for now
            "0xe6a2d1ef7d7129d2a422af0a725629a0a1fbdec4",
            0 // Ignored for now
        )

        

        //const result = txResponse.wait();

    } catch (err) {
        console.log(err)
    }

}

main()
