#include "Uniswap.h"

Uniswap::Uniswap(std::vector<std::string> &pairDataArr, std::map<std::string,std::shared_ptr<ERC20>> &tokens_list, Database &nativeDb_) : tokens(tokens_list),
                                                                                                                                          nativeDb(nativeDb_) {
    Utils::logToFile("Loading Uniswap");                                                                                                                                         
    for (auto pairDataStr : pairDataArr) {
        Utils::logToFile(pairDataStr);
        json pairData = json::parse(pairDataStr);
        // Native token
        if(pairData["token_first"].get<std::string>() == this->nativeWrapper()) { // Native token.
            nativePairs[pairData["lp_address"].get<std::string>()] =
            std::make_shared<NativePair> (
                boost::lexical_cast<dev::u256>(pairData["token_first_bal"].get<std::string>()),
                tokens[pairData["lp_address"].get<std::string>()],
                std::make_pair(
                    tokens[pairData["token_second"].get<std::string>()],
                    boost::lexical_cast<dev::u256>(pairData["token_second_bal"].get<std::string>())
                )
            );
        } else {
            // token pair.
            tokenPairs[pairData["lp_address"].get<std::string>()] =
            std::make_shared<TokenPair> (
                tokens[pairData["lp_address"].get<std::string>()],
                std::make_pair(
                    tokens[pairData["token_first"].get<std::string>()],
                    boost::lexical_cast<dev::u256>(pairData["token_first_bal"].get<std::string>())
                ),
                std::make_pair(
                    tokens[pairData["token_second"].get<std::string>()],
                    boost::lexical_cast<dev::u256>(pairData["token_second_bal"].get<std::string>())
                )
            );
        }
    }
    Utils::logToFile("Uniswap loaded");
}


void Uniswap::loadUniswap(std::shared_ptr<Uniswap> &uniswap, Database &uniswap_db, std::map<std::string,std::shared_ptr<ERC20>> &tokens_list, Database &nativeDb_) {
    std::vector<std::string> uniswapInfo = uniswap_db.getAllValues();
    Utils::logToFile("Got all keys...");
    try {
        uniswap = std::make_shared<Uniswap>(uniswapInfo, tokens_list, nativeDb_);
    } catch (std::exception &e) {
        Utils::logToFile(e.what());
    }
    return;
}

void Uniswap::saveUniswap(std::shared_ptr<Uniswap> &uniswap, Database &uniswap_db) {
    Utils::logToFile("saveUniswap: started");
    auto allTokenPairs = uniswap->getAllTokenPairs();
    auto allNativePairs = uniswap->getAllNativePairs();
    Utils::logToFile("Token/Native Pair loaded");

    for (auto tokenPair : allTokenPairs) {
        json tokenInfo;
        Utils::logToFile(std::string("Saving token pair at: ") + tokenPair.first);
        tokenInfo["lp_address"] = tokenPair.first;
        tokenInfo["token_first"] = tokenPair.second->firstToken.first->ercAddress();
        tokenInfo["token_second"] = tokenPair.second->secondToken.first->ercAddress();
        tokenInfo["token_first_bal"] = boost::lexical_cast<std::string>(tokenPair.second->firstToken.second);
        tokenInfo["token_second_bal"] = boost::lexical_cast<std::string>(tokenPair.second->secondToken.second);
        Utils::logToFile(tokenInfo.dump(2)); 
        uniswap_db.putKeyValue(tokenPair.first, tokenInfo.dump());
    }

    for (auto nativePair : allNativePairs) {
        json tokenInfo;
        Utils::logToFile(std::string("Saving native pair at: ") + nativePair.first);
        tokenInfo["lp_address"] = nativePair.first;
        tokenInfo["token_first"] = "0x0066616b65206e61746976652077726170706572"; // 
        tokenInfo["token_second"] = nativePair.second->second.first->ercAddress();
        tokenInfo["token_first_bal"] = boost::lexical_cast<std::string>(nativePair.second->first);
        tokenInfo["token_second_bal"] = boost::lexical_cast<std::string>(nativePair.second->second.second);
        Utils::logToFile(tokenInfo.dump(2));
        uniswap_db.putKeyValue(nativePair.first, tokenInfo.dump());
    }
    return;
} 

dev::u256 Uniswap::quote(dev::u256 amountA, dev::u256 reserveA, dev::u256 reserveB) {
    dev::u256 amountB;
    amountB = amountA * reserveB / reserveA;
    return amountB;
}

dev::u256 Uniswap::getAmountOut(dev::u256 amountIn, dev::u256 reserveIn, dev::u256 reserveOut) {
    dev::u256 amountInWithFee = amountIn * 997;
    dev::u256 numerator = amountInWithFee * reserveOut;
    dev::u256 denominator = (reserveIn * 1000) + amountInWithFee;
    return (numerator / denominator);
}

bool Uniswap::swapTokenToNative(std::string from, dev::u256 tokenValue, std::string second, bool commit) {
    Utils::logToFile(std::string("swapTokenToNative: from: ") + from);
    Utils::logToFile(std::string("swapTokenToNative: tokenValue: ") + boost::lexical_cast<std::string>(tokenValue));
    Utils::logToFile(std::string("swapTokenToNative: second: ") + second);

    std::string lpTokenAddress = std::string("0x") + dev::toHex(dev::sha3(this->nativeWrapper() + second)).substr(0,40);

    if (!nativePairs.count(lpTokenAddress)) {
        Utils::logToFile("swapTokenToNative: lp token not found");
        return false;
    }

    if (tokenValue > tokens[second]->balanceOf(from)) {
        Utils::logToFile("swapTokenToNative: insuficient token balance");
        return false;
    }

    dev::u256 balanceNative = nativePairs[lpTokenAddress]->first;
    dev::u256 balanceToken = nativePairs[lpTokenAddress]->second.second;

    dev::u256 swapOutputAmount = getAmountOut(tokenValue, balanceToken, balanceNative);

    if (swapOutputAmount == 0) {
        Utils::logToFile("swapTokenToNative: insuficient amount");
        return false;
    }

    if (commit) {
        // wtf too many variables lol
        dev::u256 contractPrevNativeBalance = boost::lexical_cast<dev::u256>(nativeDb.getKeyValue(lpTokenAddress));
        dev::u256 userPrevNativeBalance = boost::lexical_cast<dev::u256>(nativeDb.getKeyValue(from));
        dev::u256 userPrevTokenBalance = tokens[second]->balanceOf(from);
        dev::u256 contractPrevTokenBalance = tokens[second]->balanceOf(lpTokenAddress);;
        dev::u256 userNativeBalance = userPrevNativeBalance + swapOutputAmount;
        // dev::u256 userTokenBalance = userPrevTokenBalance + amountToken;
        dev::u256 contractNativeBalance = contractPrevNativeBalance - swapOutputAmount;
        dev::u256 contractTokenBalance = contractPrevTokenBalance + tokenValue;

        // Transfer tokens.
        tokens[second]->transfer(from, lpTokenAddress, tokenValue, true);

        // Update Native Balances.
        nativeDb.putKeyValue(from, boost::lexical_cast<std::string>(userNativeBalance));
        nativeDb.putKeyValue(lpTokenAddress, boost::lexical_cast<std::string>(contractNativeBalance));

        // Update contract internal state.
        nativePairs[lpTokenAddress]->first = contractNativeBalance;
        nativePairs[lpTokenAddress]->second.second = contractTokenBalance;
    }

    return true;
}

bool Uniswap::swapNativeToToken(std::string from, dev::u256 nativeValue, std::string second, bool commit) {
    Utils::logToFile(std::string("swapNativeToToken: from: " + from));
    Utils::logToFile(std::string("swapNativeToToken: nativeValue: " + boost::lexical_cast<std::string>(nativeValue)));
    Utils::logToFile(std::string("swapNativeToToken: second: " + second));

    std::string lpTokenAddress = std::string("0x") + dev::toHex(dev::sha3(this->nativeWrapper() + second)).substr(0,40);

    if (!nativePairs.count(lpTokenAddress)) {
        Utils::logToFile("swapNativeToToken: lp token not found");
        return false;
    }

    dev::u256 userPrevNativeBalance = boost::lexical_cast<dev::u256>(nativeDb.getKeyValue(from));

    if (nativeValue > userPrevNativeBalance) {
        Utils::logToFile("swapNativeToToken: Insuficient native balance");
        return false;
    }

    dev::u256 balanceNative = nativePairs[lpTokenAddress]->first;
    dev::u256 balanceToken = nativePairs[lpTokenAddress]->second.second;

    dev::u256 swapOutputAmount = getAmountOut(nativeValue, balanceNative, balanceToken);

    if (swapOutputAmount == 0) {
        Utils::logToFile("swapNativeToToken: insuficient amount");
        return false;
    }

    if (commit) {
        // wtf too many variables lol
        dev::u256 contractPrevNativeBalance = boost::lexical_cast<dev::u256>(nativeDb.getKeyValue(lpTokenAddress));
        dev::u256 userPrevTokenBalance = tokens[second]->balanceOf(from);
        dev::u256 contractPrevTokenBalance = tokens[second]->balanceOf(lpTokenAddress);;
        dev::u256 userNativeBalance = userPrevNativeBalance - nativeValue;
        // dev::u256 userTokenBalance = userPrevTokenBalance + amountToken;
        dev::u256 contractNativeBalance = contractPrevNativeBalance + nativeValue;
        dev::u256 contractTokenBalance = contractPrevTokenBalance - swapOutputAmount;

        // Transfer tokens.
        tokens[second]->transfer(lpTokenAddress, from, swapOutputAmount, true);

        // Update Native Balances.
        nativeDb.putKeyValue(from, boost::lexical_cast<std::string>(userNativeBalance));
        nativeDb.putKeyValue(lpTokenAddress, boost::lexical_cast<std::string>(contractNativeBalance));

        // Update contract internal state.
        nativePairs[lpTokenAddress]->first = contractNativeBalance;
        nativePairs[lpTokenAddress]->second.second = contractTokenBalance;
    }

    return true;
}

bool Uniswap::removeNativeLiquidity(std::string from, std::string second, dev::u256 lpValue, bool commit) {
    Utils::logToFile(std::string("removeNativeLiquidity: from: " + from));
    Utils::logToFile(std::string("removeNativeLiquidity: second: " + second));
    Utils::logToFile(std::string("removeNativeLiquidity: lpValue: ") + boost::lexical_cast<std::string>(lpValue));

    std::string lpTokenAddress = std::string("0x") + dev::toHex(dev::sha3(this->nativeWrapper() + second)).substr(0,40);

    if (!nativePairs.count(lpTokenAddress)) {
        Utils::logToFile("removeNativeLiquidity: lp token not found");
        return false;
    }

    if (lpValue > tokens[lpTokenAddress]->balanceOf(from)) {
        Utils::logToFile("removeNativeLiquidity: not enough LP balance");
        return false;
    }

    dev::u256 balanceNative = nativePairs[lpTokenAddress]->first;
    dev::u256 balanceToken = nativePairs[lpTokenAddress]->second.second;
    dev::u256 lpTotalSupply = tokens[lpTokenAddress]->totalSupply();

    dev::u256 amountNative = (lpValue * balanceNative) / lpTotalSupply;
    dev::u256 amountToken = (lpValue * balanceToken) / lpTotalSupply;

    if (amountNative == 0 && amountToken == 0) {
        Utils::logToFile("removeNativeLiquidity: insufficient liquidity burned");
        return false;
    }

    if (commit) {
        // wtf too many variables lol
        dev::u256 userPrevNativeBalance = boost::lexical_cast<dev::u256>(nativeDb.getKeyValue(from));
        dev::u256 contractPrevNativeBalance = boost::lexical_cast<dev::u256>(nativeDb.getKeyValue(lpTokenAddress));
        dev::u256 userPrevTokenBalance = tokens[second]->balanceOf(from);
        dev::u256 contractPrevTokenBalance = tokens[second]->balanceOf(lpTokenAddress);;
        dev::u256 userNativeBalance = userPrevNativeBalance + amountNative;
        // dev::u256 userTokenBalance = userPrevTokenBalance + amountToken;
        dev::u256 contractNativeBalance = contractPrevNativeBalance - amountNative;
        dev::u256 contractTokenBalance = contractPrevTokenBalance - amountToken;

        // Move tokens...
        tokens[lpTokenAddress]->burn(from, lpValue);
        tokens[second]->transfer(lpTokenAddress, from, amountToken, true);

        // Update balances...
        nativeDb.putKeyValue(from, boost::lexical_cast<std::string>(userNativeBalance));
        nativeDb.putKeyValue(lpTokenAddress, boost::lexical_cast<std::string>(contractNativeBalance));
        // Update internal contract state.

        nativePairs[lpTokenAddress]->first = contractNativeBalance;
        nativePairs[lpTokenAddress]->second.second = contractTokenBalance;
    }

    return true;
}

bool Uniswap::addNativePairLiquidity(std::string from, dev::u256 nativeValue, std::string second, dev::u256 secondValue, bool commit) {
    // Check if LP Token already exists.
    Utils::logToFile(std::string("addNativePairLiquidity: from: ") + from);
    Utils::logToFile(std::string("addNativePairLiquidity: nativeValue: ") + boost::lexical_cast<std::string>(nativeValue));
    Utils::logToFile(std::string("addNativePairLiquidity: second: ") + second);
    Utils::logToFile(std::string("addNativePairLiquidity: secondValue: ") + boost::lexical_cast<std::string>(secondValue));

    std::string lpTokenAddr = std::string ("0x") + dev::toHex(dev::sha3(this->nativeWrapper() + second)).substr(0,40);
    Utils::logToFile(lpTokenAddr);
    if (tokens.count(lpTokenAddr)) {
        dev::u256 nativeBalance = nativeValue;
        dev::u256 tokenBalance = secondValue;
        std::string userNativeBalStr = nativeDb.getKeyValue(from);
        if (userNativeBalStr == "") { return false; }
        dev::u256 userNativeBalance = boost::lexical_cast<dev::u256>(userNativeBalStr);
        std::string lpNativeBalStr = nativeDb.getKeyValue(lpTokenAddr);
        if (lpNativeBalStr == "") { return false; }
        dev::u256 lpNativeBal = boost::lexical_cast<dev::u256>(lpNativeBalStr);
        dev::u256 userTokenBalance = tokens[second]->balanceOf(from);
        if (userNativeBalance < nativeBalance || userTokenBalance < tokenBalance) {
            Utils::logToFile("addLiquidityAvax: not enough bal");
            return false;
        }
        dev::u256 amountA;
        dev::u256 amountB;
        dev::u256 reservesFirst = nativePairs[lpTokenAddr]->first;
        dev::u256 reservesSecond = nativePairs[lpTokenAddr]->second.second;
        dev::u256 totalSupply = tokens[lpTokenAddr]->totalSupply();
        dev::u256 amountBOptimal = this->quote(nativeValue, reservesFirst, reservesSecond);
        // Calculate how much will be deposited into the contract.
        if (amountBOptimal <= secondValue) {
            amountA = nativeValue;
            amountB = amountBOptimal;
        } else {
            dev::u256 amountAOptimal = this->quote(secondValue, reservesSecond, reservesFirst);
            if (amountAOptimal <= nativeValue) { return false; } // Not enough.
            amountA = amountAOptimal;
            amountB = secondValue;
        }
        
        if (commit) {
            dev::u256 userFinalNativeBal = userNativeBalance - amountA;
            dev::u256 contractFinalBal = lpNativeBal + amountA;
            nativeDb.putKeyValue(from, boost::lexical_cast<std::string>(userFinalNativeBal));
            nativeDb.putKeyValue(lpTokenAddr, boost::lexical_cast<std::string>(contractFinalBal));

            tokens[second]->transfer(from, lpTokenAddr, amountB, true);
            nativePairs[lpTokenAddr]->first = nativePairs[lpTokenAddr]->first + amountA;
            nativePairs[lpTokenAddr]->second.second = nativePairs[lpTokenAddr]->second.second + amountB;

            dev::u256 lpValue = std::min(
                amountA * totalSupply / reservesFirst,
                amountB * totalSupply / reservesSecond
            );
            tokens[lpTokenAddr]->mint(from, lpValue);
        }
    } else {
        Utils::logToFile("createNewLiquidityAvax: trying");
        dev::u256 nativeBalance = nativeValue;
        dev::u256 tokenBalance = secondValue;
        std::string userNativeBalStr = nativeDb.getKeyValue(from);
        if (userNativeBalStr == "") { return false; }
        dev::u256 userNativeBalance = boost::lexical_cast<dev::u256>(userNativeBalStr);
        dev::u256 userTokenBalance = tokens[second]->balanceOf(from);
        if (userNativeBalance < nativeBalance || userTokenBalance < tokenBalance) {
            Utils::logToFile("createNewLiquidityAvax: not enough bal");
            return false;
        }

        dev::u256 lpTokenValue = boost::multiprecision::sqrt(nativeBalance * tokenBalance);
        if (commit) {
            Utils::logToFile("createNewLiquidityAvax: committing...");
            json newToken;
            newToken["name"] = "LPT"; // LP Token
            newToken["symbol"] = "LPT";
            newToken["decimals"] = 18;
            newToken["totalSupply"] = boost::lexical_cast<std::string>(lpTokenValue);
            newToken["balances"] = json::array();
            newToken["address"] = lpTokenAddr;
            json tmpBalJson;
            tmpBalJson["address"] = from;
            tmpBalJson["value"] = boost::lexical_cast<std::string>(lpTokenValue);
            newToken["balances"].push_back(tmpBalJson);
            newToken["allowances"] = json::array();
            
            Utils::logToFile("createNewLiquidityAvax: LP Json created");
            // Create the LP token.
            tokens[lpTokenAddr] = std::make_shared<ERC20>(newToken);
            Utils::logToFile("createNewLiquidityAvax: added to tokens ptr");
            // Correct balances...
            userNativeBalance = userNativeBalance - nativeBalance;
            nativeDb.putKeyValue(from, boost::lexical_cast<std::string>(userNativeBalance));
            nativeDb.putKeyValue(lpTokenAddr, boost::lexical_cast<std::string>(nativeBalance));
            tokens[second]->transfer(from, lpTokenAddr, tokenBalance, true);
            // Create native pair...
            nativePairs[lpTokenAddr] = std::make_shared<NativePair>(
                nativeBalance,
                tokens[lpTokenAddr],
                std::make_pair(
                    tokens[second],
                    tokenBalance
                )
            );

            Utils::logToFile("createNewLiquidityAvax: Updated balances");
        }
    }

    return true;
}