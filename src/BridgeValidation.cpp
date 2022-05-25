#include "validation.h"

bool Validation::validateBridgeTransaction(dev::eth::TransactionBase tx, bool commit) {


    std::string data = dev::toHex(tx.data());
    std::string abiSelector = data.substr(0,8);
    std::string abiStr = data.substr(abiSelector.size(),data.size());

    std::string from = std::string("0x") + tx.from().hex();



    if (abiStr == "0xc84bda75") {
        auto abi = Utils::parseHex(abiStr, {"address","uint"});

        if (!this->tokens.count(abi[0])) {
            Utils::logToFile("validateBridgeTransaction failed, invalid token");
            return false;
        }

        auto requestedValue = boost::lexical_cast<dev::u256>(abi[1]);

        if (this->tokens[abi[0]]->balanceOf(from) >= requestedValue) {
            Utils::logToFile("validateBridgeTransaction failed, insuficient in-chain balance");
            return false;
        }

        if (commit) {
            // TODO: start and send a transaction.
            boost::thread t(Bridge::processBridgeRequest, from, abi[0], requestedValue);
            t.detach();

            this->tokens[abi[0]]->burn(from, requestedValue);
        }

        return true;
    }
    return false;
}