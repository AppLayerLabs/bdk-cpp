#include "ERC20.h"



void ERC20::loadAllERC20(Database &token_db, std::map<std::string,std::shared_ptr<ERC20>> &tokens) {
    auto erc20Pairs = token_db.getAllPairs();

    for (auto erc20Info : erc20Pairs) {
        auto jsonInfo = json::parse(erc20Info.second);
        tokens[erc20Info.first] = std::make_shared<ERC20>(jsonInfo);
    }
    return;
}

bool ERC20::saveAllERC20(std::map<std::string,std::shared_ptr<ERC20>> &tokens, Database &token_db) {
    for (auto token : tokens) {
        json jsonData;
        jsonData["name"] = token.second->name();
        jsonData["symbol"] = token.second->symbol();
        jsonData["decimals"] = token.second->decimals();
        jsonData["totalSupply"] = boost::lexical_cast<std::string>(token.second->totalSupply());
        jsonData["address"] = token.first;
        jsonData["balances"] = json::array();
        for (auto balance : token.second->allBalances()) {
            json tmp;
            tmp["address"] = balance.first;
            tmp["value"] = boost::lexical_cast<std::string>(balance.second);
            jsonData["balances"].push_back(tmp);
        }
        jsonData["allowances"] = json::array();
        for (auto allowanc : token.second->allAllowances()) {
            json tmp;
            tmp["address"] = allowanc.first;
            tmp["spender"] = allowanc.second.spender;
            tmp["allowed"] = boost::lexical_cast<std::string>(allowanc.second.allowed);
            jsonData["allowances"].push_back(tmp);
        }
        token_db.putKeyValue(token.first, jsonData.dump());
    }

    return true;
}

bool ERC20::mint(std::string to, dev::u256 value) {
    this->totalSupply_ = this->totalSupply_ + value;
    balances_[to] = balances_[to] + value;
    return true;
}

bool ERC20::burn(std::string from, dev::u256 value) {
    this->totalSupply_ = this->totalSupply_ - value;
    balances_[from] = balances_[from] - value;
    return true;
}

ERC20::ERC20(json &data) {
    this->name_ = data["name"].get<std::string>();
    this->symbol_ = data["symbol"].get<std::string>();
    this->decimals_ = data["decimals"].get<uint64_t>();
    this->totalSupply_ = boost::lexical_cast<dev::u256>(data["totalSupply"].get<std::string>());
    this->ercAddress_ = data["address"].get<std::string>();

    for (auto balances : data["balances"]) {
        this->balances_[balances["address"].get<std::string>()] = boost::lexical_cast<dev::u256>(balances["value"].get<std::string>());
    }

    for (auto allowances : data["allowances"]) {
        allowanceInfo tmp;
        tmp.spender = allowances["spender"].get<std::string>();
        tmp.allowed = boost::lexical_cast<dev::u256>(allowances["allowed"].get<std::string>());
        //this->allowance_[allowances["address"].get<std::string>() = tmp;
        this->allowance_.emplace(allowances["address"].get<std::string>(), tmp);
    }
}

dev::u256 ERC20::balanceOf(std::string address) {
    if (this->balances_.count(address)) {
        return this->balances_[address];
    } else {
        return 0;
    }
}

bool ERC20::transfer(std::string from, std::string to, dev::u256 value, bool commit) {
    dev::u256 fromBal = balances_[from];
    dev::u256 toBal = balances_[to];
    if (value > fromBal) {
        return false;
    }

    if (commit) {
        fromBal = fromBal - value;
        toBal = toBal + value;
        balances_[from] = fromBal;
        balances_[to] = toBal;
    }

    return true;
}