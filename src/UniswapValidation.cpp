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
    if (abiSelector == "0xf91b3f72") { // addLiquidityAVAX
        auto transactionValue = tx.value();
        std::vector<std::string> abi = Utils::parseHex(abiStr, {"address", "uint", "uint", "uint", "address", "uint"});

        std::string tokenAddr = abi[0];
        dev::u256 tokenValue = boost::lexical_cast<dev::u256>(abi[1]);
        dev::u256 txValue = tx.value();

        return this->uniswap->addNativePairLiquidity(from, txValue, tokenAddr, tokenValue, commit);
    }

    return false;
}