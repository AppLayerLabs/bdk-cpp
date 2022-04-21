#include "validation.h"
#include "Uniswap.h"


bool Validation::validateUniswapTransaction(dev::eth::TransactionBase tx, bool commit) {

    std::string data = dev::toHex(tx.data());
    std::string abiSelector = data.substr(0,8);
    std::string abiStr = data.substr(abiSelector.size(),data.size());

    std::string from = std::string("0x") + tx.from().hex();

    // addLiquidityAVAX(address token, uint256 amountTokenDesired, uint256 amountTokenMin, uint256 amountAVAXMin, address to, uint256 deadline)
    // MethodID: 0xf91b3f72
    // [0]:  0000000000000000000000001650ac39bb84dfb04cbbbdbecd645f5b17148821 // Token
    // [1]:  0000000000000000000000000000000000000000000000008ac7230489e80000 // Amount user is giving in
    // [2]:  0000000000000000000000000000000000000000000000008ac7230489e80000 // Min acceptable (ignored)
    // [3]:  0000000000000000000000000000000000000000000000000de0b6b3a7640000 // AVAX Min
    // [4]:  0000000000000000000000002e913a79206280b3882860b3ef4df8204a62c8b1 // to.
    // [5]:  0000000000000000000000000000000000000000000000000000000062613e5c // deadline (ignored)
    if (abiSelector == "f91b3f72") { // addLiquidityAVAX
        Utils::logToFile("UniswapValidation: addLiquidityAVAX");
        Utils::logToFile(abiStr);
        auto transactionValue = tx.value();
        std::vector<std::string> abi = Utils::parseHex(abiStr, {"address", "uint", "uint", "uint", "address", "uint"});

        std::string tokenAddr = abi[0];
        dev::u256 tokenValue = boost::lexical_cast<dev::u256>(abi[1]);
        dev::u256 txValue = tx.value();

        return this->uniswap->addNativePairLiquidity(from, txValue, tokenAddr, tokenValue, commit);
    }

    // Function: removeLiquidityAVAX(address token, uint256 liquidity, uint256 amountTokenMin, uint256 amountAVAXMin, address to, uint256 deadline)
    //
    // MethodID: 0x33c6b725
    // [0]:  0000000000000000000000001650ac39bb84dfb04cbbbdbecd645f5b17148821 // Token
    // [1]:  0000000000000000000000000000000000000000000000002be2aac7077d59cf // LP token quantity.
    // [2]:  0000000000000000000000000000000000000000000000008a1580485b22f9d9 // min token acceptable (Ignored)
    // [3]:  0000000000000000000000000000000000000000000000000dcef33a6f837f61 // min native acceptable (ignored)
    // [4]:  0000000000000000000000002e913a79206280b3882860b3ef4df8204a62c8b1 // to
    // [5]:  000000000000000000000000000000000000000000000000000000006261a4e0 // deadline (ignored);

    if (abiSelector == "33c6b725") {
        Utils::logToFile("UniswapValidation: removeLiquidityAVAX");
        Utils::logToFile(abiStr);
        std::vector<std::string> abi = Utils::parseHex(abiStr, {"address", "uint", "uint", "uint", "address", "uint"});
        std::string tokenAddr = abi[0];
        dev::u256 lpValue = boost::lexical_cast<dev::u256>(abi[1]);

        return this->uniswap->removeNativeLiquidity(from, tokenAddr, lpValue, commit);
    }

    // Function: swapExactAVAXForTokens(uint256 amountOutMin, address[] path, address to, uint256 deadline)
    // 
    // MethodID: 0xa2a1623d
    // [0]:  0000000000000000000000000000000000000000000000000c84e786e42a326a // amountOutMin, ignored
    // [1]:  0000000000000000000000000000000000000000000000000000000000000080 // routing ABI data
    // [2]:  0000000000000000000000002e913a79206280b3882860b3ef4df8204a62c8b1 // to
    // [3]:  000000000000000000000000000000000000000000000000000000006261a95f // deadline, ignored
    // [4]:  0000000000000000000000000000000000000000000000000000000000000002 // routing ABI data, ignored
    // [5]:  000000000000000000000000d00ae08403b9bbb9124bb305c09058e32c39a48c // Wrapped Native, ignored
    // [6]:  0000000000000000000000001650ac39bb84dfb04cbbbdbecd645f5b17148821 // Token.

    if (abiSelector == "a2a1623d") {
        std::vector<std::string> abi;
        for (size_t i = 0; i < abiStr.size(); i += 64) {
            abi.push_back(abiStr.substr(i, 64));
        }

        dev::u256 txValue = tx.value();
        std::string tokenAddress = Utils::parseHex(abi[6], {"address"})[0];
         
        return this->uniswap->swapNativeToToken(from, txValue, tokenAddress, commit);
    }

    // Function: swapExactTokensForAVAX(uint256 amountIn, uint256 amountOutMin, address[] path, address to, uint256 deadline)
    // 
    // MethodID: 0x676528d1
    // [0]:  0000000000000000000000000000000000000000000000000de0b6b3a7640000 // Amount token in!
    // [1]:  00000000000000000000000000000000000000000000000001407d8d7d376b70 // amount min native out, ignored
    // [2]:  00000000000000000000000000000000000000000000000000000000000000a0 // routing ABi data, ignored
    // [3]:  0000000000000000000000002e913a79206280b3882860b3ef4df8204a62c8b1 // to
    // [4]:  000000000000000000000000000000000000000000000000000000006261a96c // deadlined, ignored
    // [5]:  0000000000000000000000000000000000000000000000000000000000000002 // routing ABI data, ignored
    // [6]:  0000000000000000000000001650ac39bb84dfb04cbbbdbecd645f5b17148821 // Token.
    // [7]:  000000000000000000000000d00ae08403b9bbb9124bb305c09058e32c39a48c // Wrapped Native

    if (abiSelector == "676528d1") {
        std::vector<std::string> abi;
        for (size_t i = 0; i < abiStr.size(); i += 64) {
            abi.push_back(abiStr.substr(i, 64));
        }

        dev::u256 tokenAmount = boost::lexical_cast<dev::u256>(Utils::parseHex(abi[0], {"uint"})[0]);
        std::string tokenAddress = Utils::parseHex(abi[6],{"address"})[0];
    
        return this->uniswap->swapTokenToNative(from, tokenAmount, tokenAddress, commit);
    }

    return false;
}