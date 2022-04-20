#ifndef ERC20_H
#define ERC20_H

#include <string>
#include "db.h"
#include "json.hpp"

using json = nlohmann::ordered_json;


class ERC20 : public std::enable_shared_from_this<ERC20> {
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

    ERC20(const ERC20& other) {
        name_ = other.name_;
        symbol_ = other.symbol_;
        decimals_ = other.decimals_;
        totalSupply_ = other.totalSupply_;
        ercAddress_ = other.ercAddress_;
        balances_ = other.balances_;
        allowance_ = other.allowance_;
    }

    std::string name()                                  { return name_; };
    std::string symbol()                                { return symbol_; };
    uint64_t decimals()                                 { return decimals_; };
    dev::u256 totalSupply()                             { return totalSupply_; };
    std::string ercAddress()                            { return ercAddress_; };
    std::map<std::string,dev::u256> allBalances()       { return balances_; };
    std::map<std::string,allowanceInfo> allAllowances() { return allowance_; };
    
    // Write functions.
    // Commit == save changes to contract, if false: only check validity of such transaction.
    bool transfer(std::string from, std::string to, dev::u256 value, bool commit = false);
    bool transferFrom(std::string from, std::string to, dev::u256 value, bool commit = false);
    bool approve(std::string owner, std::string spender, dev::u256 value, bool commit = false);

    // Mint
    bool mint(std::string to, dev::u256 value);

    // View functions
    dev::u256 allowance(std::string owner, std::string spender);
    dev::u256 balanceOf(std::string address);


    static void loadAllERC20(Database &token_db, std::map<std::string,std::shared_ptr<ERC20>> &tokens);
    static bool saveAllERC20(std::map<std::string,std::shared_ptr<ERC20>> &tokens, Database &token_db);
};

#endif // ERC20_H