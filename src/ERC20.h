#ifndef ERC20_H
#define ERC20_H

#include <string>
#include <memory>
#include "db.h"
#include "json.hpp"

using json = nlohmann::ordered_json;


class ERC20 {
    private:
        struct allowanceInfo {
            std::string spender;
            dev::u256 allowed;
        };
        std::string name_;
        std::string symbol_;
        uint64_t decimals_;
        dev::u256 totalSupply_;
        std::string ercAddress_;    
        std::map<std::string,dev::u256> balances_;
        std::map<std::string, allowanceInfo> allowance_; // key -> owner
                                                         // value -> spender + allowed.
    public:
    ERC20(json &data);

    std::string name()          { return name_; };
    std::string symbol()        { return symbol_; };
    uint64_t decimals()         { return decimals_; };
    dev::u256 totalSupply()     { return totalSupply_; };
    std::string ercAddress()    { return ercAddress_; };
    
    // Write functions.
    // Commit == save changes to contract, if false: only check validity of such transaction.
    bool transfer(std::string from, std::string to, dev::u256 value, bool commit = false);
    bool transferFrom(std::string from, std::string to, dev::u256 value, bool commit = false);
    bool approve(std::string owner, std::string spender, dev::u256 value, bool commit = false);

    // View functions
    dev::u256 allowance(std::string owner, std::string spender);
    dev::u256 balanceOf(std::string address);


    static std::map<std::string,ERC20> loadAllERC20(Database &token_db);
    static bool saveAllERC20(std::map<std::string,ERC20> &tokens, Database &token_db);
};

#endif // ERC20_H