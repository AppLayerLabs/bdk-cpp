#ifndef UNISWAP_H
#define UNISWAP_H

#include "ERC20.h"
#include "db.h"

class Uniswap {
    private:
    std::string uniswapAddress = "0x00000000000000000000000000756e6973776170"; // uniswap in hex.
    std::string nativeWrapper = "0x0066616b65206e61746976652077726170706572"; // Only used for generating the LP token address. and saving to json.
         
    struct TokenPair {
        std::shared_ptr<ERC20> LP;
        std::pair<std::shared_ptr<ERC20>,dev::u256> first;
        std::pair<std::shared_ptr<ERC20>,dev::u256> second;

        TokenPair(std::shared_ptr<ERC20> _LP, std::pair<std::shared_ptr<ERC20>,dev::u256> _first, std::pair<std::shared_ptr<ERC20>,dev::u256> _second) {
            LP = _LP;
            first = _first;
            second = _second;
        }
    };
    
    struct NativePair {
        dev::u256 first;
        std::shared_ptr<ERC20> LP;

        std::pair<std::shared_ptr<ERC20>,dev::u256> second;

        NativePair(dev::u256 _first, std::shared_ptr<ERC20> _LP, std::pair<std::shared_ptr<ERC20>,dev::u256> _second) {
            first = _first;
            LP = _LP;
            second = _second;
        }
    };

    std::map<std::string,std::shared_ptr<TokenPair>> tokenPairs;
    std::map<std::string,std::shared_ptr<NativePair>> nativePairs;

    std::map<std::string,std::shared_ptr<ERC20>> &tokens; // Reference to token list.
    public:
    // Constructor

    Uniswap(std::vector<std::string> &pairDataArr, std::map<std::string,std::shared_ptr<ERC20>> &tokens_list);    
    
    /*
    JSON FILE STRUCTURE:

    {
        "lp_address" : "",
        "token_first" : "", // Always Native Wrapper if native pair.
        "token_second" : "",
        "token_first_bal" : "",
        "token_second_bal" : ""
    }
    */

    static void loadUniswap(std::shared_ptr<Uniswap> uniswap, Database &uniswap_db, std::map<std::string,std::shared_ptr<ERC20>> &tokens_list);
    static void saveUniswap(std::shared_ptr<Uniswap> uniswap, Database &uniswap_db);

    bool addTokenPairLiquidity(std::string first, std::string second, dev::u256 firstValue, dev::u256 secondValue);
    bool addNativePairLiquidity(dev::u256 nativeValue, std::string second, dev::u256 secondValue);
    bool removeTokenLiquidity(std::string first, std::string second, dev::u256 lpValue);
    bool removeNativeLiquidity(dev::u256 nativeValue, std::string second, dev::u256 lpValue);
    bool tokenPairExists(std::string token_first, std::string token_second);
    bool nativePairExists(std::string token);

};

#endif