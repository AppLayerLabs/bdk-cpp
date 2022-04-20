#include "Uniswap.h"

Uniswap::Uniswap(std::vector<std::string> &pairDataArr, std::map<std::string,std::shared_ptr<ERC20>> &tokens_list, Database &nativeDb_) : tokens(tokens_list),
                                                                                                                                          nativeDb(nativeDb_) {
    for (auto pairDataStr : pairDataArr) {
        json pairData = json::parse(pairDataStr);
        // Native token
        if(pairData["token_first"].get<std::string>() == this->nativeWrapper) { // Native token.
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
}


void Uniswap::loadUniswap(std::shared_ptr<Uniswap> uniswap, Database &uniswap_db, std::map<std::string,std::shared_ptr<ERC20>> &tokens_list, Database &nativeDb_) {
    std::vector<std::string> uniswapInfo = uniswap_db.getAllKeys();
    uniswap = std::make_shared<Uniswap>(uniswapInfo, tokens_list, nativeDb_);
    return;
}

void Uniswap::saveUniswap(std::shared_ptr<Uniswap> uniswap, Database &uniswap_db) {
    auto allTokenPairs = uniswap->getAllTokenPairs();
    auto allNativePairs = uniswap->getAllNativePairs();

    for (auto tokenPair : allTokenPairs) {
        json tokenInfo;
        tokenInfo["lp_address"] = tokenPair.first;
        tokenInfo["token_first"] = tokenPair.second->firstToken.first->ercAddress();
        tokenInfo["token_second"] = tokenPair.second->secondToken.first->ercAddress();
        tokenInfo["token_first_bal"] = boost::lexical_cast<std::string>(tokenPair.second->firstToken.second);
        tokenInfo["token_second_bal"] = boost::lexical_cast<std::string>(tokenPair.second->secondToken.second); 
        uniswap_db.putKeyValue(tokenPair.first, tokenInfo.dump());
    }

    for (auto nativePair : allNativePairs) {
        json tokenInfo;
        tokenInfo["lp_address"] = nativePair.first;
        tokenInfo["token_first"] = "0x0066616b65206e61746976652077726170706572"; // 
        tokenInfo["token_second"] = nativePair.second->second.first->ercAddress();
        tokenInfo["token_first_bal"] = boost::lexical_cast<std::string>(nativePair.second->second.second);
        tokenInfo["token_second_bal"] = boost::lexical_cast<std::string>(nativePair.second->first);
        uniswap_db.putKeyValue(nativePair.first, tokenInfo.dump());
    }
    return;
} 

dev::u256 Uniswap::quote(dev::u256 amountA, dev::u256 reserveA, dev::u256 reserveB) {
    dev::u256 amountB;
    amountB = amountA * reserveB / reserveA;
    return amountB;
}

bool Uniswap::addTokenPairLiquidity(std::string from, std::string first, std::string second, dev::u256 firstValue, dev::u256 secondValue, bool commit) {
    // Check if LP Token already exists.
    std::string lpTokenAddr = std::string ("0x") + dev::toHex(dev::sha3(first + second));
    if (tokens.count(lpTokenAddr)) {
        dev::u256 nativeBalance = firstValue;
        dev::u256 tokenBalance = secondValue;
        std::string userNativeBalStr = nativeDb.getKeyValue(from);
        if (userNativeBalStr == "") { return false; }
        dev::u256 userNativeBalance = boost::lexical_cast<dev::u256>(userNativeBalStr);
        std::string lpNativeBalStr = nativeDb.getKeyValue(lpTokenAddr);
        if (lpNativeBalStr == "") { return false; }
        dev::u256 lpNativeBal = boost::lexical_cast<dev::u256>(lpNativeBalStr);
        dev::u256 userTokenBalance = tokens[second]->balanceOf(from);
        if (userNativeBalance >= nativeBalance || userTokenBalance >= tokenBalance) {
            // Insuficient balance;
            return false;
        }
        dev::u256 amountA;
        dev::u256 amountB;
        dev::u256 reservesFirst = nativePairs[lpTokenAddr]->first;
        dev::u256 reservesSecond = nativePairs[lpTokenAddr]->second.second;
        dev::u256 totalSupply = tokens[lpTokenAddr]->totalSupply();
        dev::u256 amountBOptimal = this->quote(firstValue, reservesFirst, reservesSecond);
        // Calculate how much will be deposited into the contract.
        if (amountBOptimal <= secondValue) {
            amountA = firstValue;
            amountB = amountBOptimal;
        } else {
            dev::u256 amountAOptimal = this->quote(secondValue, reservesSecond, reservesFirst);
            if (amountAOptimal <= firstValue) { return false; } // Not enough.
            amountA = amountAOptimal;
            amountB = secondValue;
        }
        
        if (commit) {
            dev::u256 userFinalNativeBal = userNativeBalance - amountA;
            dev::u256 contractFinalBal = lpNativeBal + amountA;
            nativeDb.putKeyValue(from, boost::lexical_cast<std::string>(userFinalNativeBal));
            nativeDb.putKeyValue(lpTokenAddr, boost::lexical_cast<std::string>(contractFinalBal));
            tokens[second]->transfer(from, lpTokenAddr, amountB);

            dev::u256 lpValue = std::min(
                amountA * totalSupply / reservesFirst,
                amountB * totalSupply / reservesSecond
            );
            tokens[lpTokenAddr]->mint(from, lpValue);
        }
    } else {
        dev::u256 nativeBalance = firstValue;
        dev::u256 tokenBalance = secondValue;
        std::string userNativeBalStr = nativeDb.getKeyValue(from);
        if (userNativeBalStr == "") { return false; }
        dev::u256 userNativeBalance = boost::lexical_cast<dev::u256>(userNativeBalStr);
        dev::u256 userTokenBalance = tokens[second]->balanceOf(from);
        if (userNativeBalance >= nativeBalance || userTokenBalance >= tokenBalance) {
            // Insuficient balance;
            return false;
        }

        dev::u256 lpTokenValue = boost::multiprecision::sqrt(nativeBalance * tokenBalance);
        if (commit) {
            json newToken;
            newToken["name"] = "LPT"; // LP Token
            newToken["symbol"] = "LPT";
            newToken["decimals"] = 18;
            newToken["totalSupply"] = boost::lexical_cast<std::string>(lpTokenValue);
            newToken["balances"] = json::array();
            json tmpBalJson;
            tmpBalJson["address"] = from;
            tmpBalJson["value"] = boost::lexical_cast<std::string>(lpTokenValue);
            newToken["balances"].push_back(tmpBalJson);
            newToken["allowances"] = json::array();
            
            // Create the LP token.
            tokens[lpTokenAddr] = std::make_shared<ERC20>(newToken);
            // Correct balances...
            userNativeBalance = userNativeBalance - nativeBalance;
            nativeDb.putKeyValue(from, boost::lexical_cast<std::string>(userNativeBalance));
            nativeDb.putKeyValue(lpTokenAddr, boost::lexical_cast<std::string>(nativeBalance));
            tokens[second]->transfer(from, lpTokenAddr, tokenBalance);
        }
    }

    return true;
}