#include "Uniswap.h"

Uniswap::Uniswap(std::vector<std::string> &pairDataArr, std::map<std::string,std::shared_ptr<ERC20>> &tokens_list) : tokens(tokens_list) {
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


void Uniswap::loadUniswap(std::shared_ptr<Uniswap> uniswap, Database &uniswap_db, std::map<std::string,std::shared_ptr<ERC20>> &tokens_list) {
    std::vector<std::string> uniswapInfo = uniswap_db.getAllKeys();
    uniswap = std::make_shared<Uniswap>(uniswapInfo, tokens_list);
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