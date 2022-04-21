#ifndef UNISWAP_H
#define UNISWAP_H

#include "ERC20.h"
#include "db.h"

#include <boost/multiprecision/integer.hpp>
#include <include/web3cpp/devcore/CommonData.h>
#include <include/web3cpp/devcore/SHA3.h>
class Uniswap : public std::enable_shared_from_this<Uniswap> {
    private:
         
    struct TokenPair {
        std::shared_ptr<ERC20> LP;
        std::pair<std::shared_ptr<ERC20>,dev::u256> firstToken;
        std::pair<std::shared_ptr<ERC20>,dev::u256> secondToken;

        TokenPair(std::shared_ptr<ERC20> _LP, std::pair<std::shared_ptr<ERC20>,dev::u256> _firstToken, std::pair<std::shared_ptr<ERC20>,dev::u256> _secondToken) {
            LP = _LP;
            firstToken = _firstToken;
            secondToken = _secondToken;
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
    Database &nativeDb; // Reference to native balances.

    dev::u256 quote(dev::u256 amountA, dev::u256 reserveA, dev::u256 reserveB);

    dev::u256 getAmountOut(dev::u256 amountIn, dev::u256 reserveIn, dev::u256 reserveOut);
    
    public:
    // view functions
    const std::string uniswapAddress() { return "0x00000000000000000000000000756e6973776170"; } // uniswap in hex.
    const std::string nativeWrapper() { return "0x0066616b65206e61746976652077726170706572"; }  // Only used for generating the LP token address. and saving to json.

    std::map<std::string,std::shared_ptr<TokenPair>> const getAllTokenPairs() { return tokenPairs; }
    std::map<std::string,std::shared_ptr<NativePair>> const getAllNativePairs() { return nativePairs; }

    // Constructor

    Uniswap(std::vector<std::string> &pairDataArr, std::map<std::string,std::shared_ptr<ERC20>> &tokens_list, Database &nativeDb_ );    
    
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

    static void loadUniswap(std::shared_ptr<Uniswap> &uniswap, Database &uniswap_db, std::map<std::string,std::shared_ptr<ERC20>> &tokens_list, Database &nativeDb_);
    static void saveUniswap(std::shared_ptr<Uniswap> &uniswap, Database &uniswap_db);

    bool addTokenPairLiquidity(std::string from, std::string first, std::string second, dev::u256 firstValue, dev::u256 secondValue, bool commit = false);
    bool addNativePairLiquidity(std::string from, dev::u256 nativeValue, std::string second, dev::u256 secondValue, bool commit = false);
    bool removeTokenLiquidity(std::string from, std::string first, std::string second, dev::u256 lpValue, bool commit = false);
    bool removeNativeLiquidity(std::string from, std::string second, dev::u256 lpValue, bool commit = false);
    bool swapNativeToToken(std::string from, dev::u256 nativeValue, std::string second, bool commit = false);
    bool swapTokenToNative(std::string from, dev::u256 tokenValue, std::string second, bool commit = false);
    bool swapTokenToToken(std::string from, dev::u256 firstValue, std::string first, std::string second, bool commit = false);
    bool tokenPairExists(std::string token_first, std::string token_second);
    bool nativePairExists(std::string token);

};

#endif