#include "validation.h"

// This function will become too big to be in one file one.

enum ContractType {
    ERC20,
    UNISWAP
};

std::string Validation::processEthCall(json &methods) {
    std::string ret = "0x";
    std::string contract = methods[0]["to"].get<std::string>();
    std::string data = methods[0]["data"].get<std::string>();

    for (auto &c : contract) {
        if (std::isalpha(c)) {
            c = std::tolower(c);
        }
    }

    std::string abiSelector = data.substr(0,10);
    std::string abi = data.substr(abiSelector.size(), data.size());

    ContractType type;

    if (this->tokens.count(contract)) {
        type = ContractType::ERC20;
    }

    if (contract == this->uniswap->uniswapAddress()) {
        type = UNISWAP;
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
            Utils::logToFile(std::string(address[0] + " " + boost::lexical_cast<std::string>(address.size())));
            Utils::logToFile(abi);
            ret += Utils::uintToHex(boost::lexical_cast<std::string>(tokens[contract]->balanceOf(address[0])));
            return ret;
        }
    }

    if (type == ContractType::UNISWAP) {

    }
    return "0x";
}