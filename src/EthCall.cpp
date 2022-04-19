#include "validation.h"

// This function will become too big to be in one file one.

enum ContractType {
    ERC20,
    UNISWAP_FACTORY,
    UNISWAP_ROUTER,
    UNISWAP_LP
};


std::string Validation::processEthCall(json &methods) {
    std::string ret = "0x";
    std::string contract = methods["to"].get<std::string>();
    std::string data = methods["data"].get<std::string>();

    std::string abiSelector = data.substr(0,10);
    std::string abi = data.substr(abiSelector.size(), data.size());

    ContractType type;

    if (this->tokens.count(contract)) {
        type = ContractType::ERC20;
    }

    if (type == ContractType::ERC20) {
        if (abiSelector == "0x95d89b41") { // symbol().
            ret += Utils::uintToHex(boost::lexical_cast<std::string>(tokens[contract]->symbol().size()));
            ret += Utils::bytesToHex(tokens[contract]->symbol(), false);
            return ret;
        }
        if (abiSelector == "0x06fdde03") { // name().
            ret += Utils::uintToHex(boost::lexical_cast<std::string>(tokens[contract]->name().size()));
            ret += Utils::bytesToHex(tokens[contract]->symbol(), false);
            return ret;
        }
        if (abiSelector == "0x313ce567") { // decimals().
            ret += Utils::uintToHex(boost::lexical_cast<std::string>(tokens[contract]->decimals()));
            return ret;
        }
        if (abiSelector == "0x18160ddd") { // totalSupply().
            ret += Utils::uintToHex(boost::lexical_cast<std::string>(tokens[contract]->totalSupply()));
            return ret;
        }
        if (abiSelector == "0x70a08231") { // balanceOf(address)
            auto address = Utils::parseHex(abi, { "address" });
            ret = Utils::uintToHex(boost::lexical_cast<std::string>(tokens[contract]->balanceOf(address[0])));
            return ret;
        }
    }
    
}