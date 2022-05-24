#include "Bridge.h"


Bridge::bridgeUserRequest Bridge::getBridgeRequest(std::string txId) {
    // Build json request using given txid.
    bridgeUserRequest ret;
    json request;

    request["jsonrpc"] = "2.0";
    request["method"] = "eth_getTransactionReceipt";
    request["params"] = json::array();
    request["id"] = 1;
    request["params"].push_back(txId);

    // Get transaction receipt and parse it to a json.
    json answer = json::parse(HTTPClient::fujiRequest(request.dump()));

    // Parse event from transaction receipt.
    std::string eventAbi;

    for (auto item : answer["result"]["logs"]) {
        for (auto topics : item["topics"]) {
            if (topics.get<std::string>() == bridgeTopic) {
                eventAbi = item["data"].get<std::string>();
            }
        }
    }

    // Example ABI given from the event of the contract
    // event bridgeUserRequest(address token, address user, uint256 amount, uint256 tokenDecimals, string tokenName, string tokenSymbol);
    // 0  000000000000000000000000130cc865abeb6bfcce84f0e8eff121a630f87124 <- Token
    // 1  000000000000000000000000b09b05636ba59ba0daf79c925ad100c04bd6c500 <- User
    // 2  00000000000000000000000000000000000000000000000000005af3107a4000 <- Amount
    // 3  0000000000000000000000000000000000000000000000000000000000000012 <- tokenDecimals
    // 4  00000000000000000000000000000000000000000000000000000000000000c0 <- tokenName String Start
    // 5  0000000000000000000000000000000000000000000000000000000000000100 <- tokenSymbol string Start
    // 6  0000000000000000000000000000000000000000000000000000000000000009 <- tokenName size
    // 7  54657374546f6b656e0000000000000000000000000000000000000000000000 <- tokenName string (in hex)
    // 8  0000000000000000000000000000000000000000000000000000000000000003 <- tokenSymbol size
    // 9  5454540000000000000000000000000000000000000000000000000000000000 <- tokenSymbol string (in hex)

    // Split ABI string to vector of 64 characters (32 bytes) each.
    std::vector<std::string> abi;
    for (size_t i = 0; i < eventAbi.size(); i += 64) {
        abi.push_back(eventAbi.substr(i, 64));
    }

    ret.token = Utils::parseHex(abi[0], {"address"})[0];
    ret.user = Utils::parseHex(abi[1], {"address"})[0];
    ret.amount = boost::lexical_cast<dev::u256>(Utils::parseHex(abi[2], {"uint"})[0]);
    ret.tokenDecimals = boost::lexical_cast<uint32_t>(Utils::parseHex(abi[3], {"uint"})[0]);
    ret.tokenName = Utils::hexToUtf8(abi[7], boost::lexical_cast<uint32_t>(Utils::parseHex(abi[6],{"uint"})[0]));
    ret.tokenSymbol = Utils::hexToUtf8(abi[9], boost::lexical_cast<uint32_t>(Utils::parseHex(abi[8],{"uint"})[0]));

    return ret;
};