#ifndef BRIDGE_H
#define BRIDGE_H

#include <string>
#include "validation.h"
#include "httpclient.h"


// Namespace for Bridge information.


namespace Bridge {

    struct bridgeUserRequest {
        std::string token;
        std::string user;
        dev::u256 amount;
        uint32_t tokenDecimals; 
        std::string tokenName;
        std::string tokenSymbol;
    };

    inline const std::string bridgeContract = "0x950b8602bbee3819f043dd6a30923a35d6e2ab5e";
    inline const std::string bridgeTopic = "0x5a36801d4ec996ac305bb61fdab651fb595d0e50bcf094ee73d6f226eed0cafb";
    inline const std::string bridgePrivKey = "0d461d653b06f6806f1740d5894f65a6c76c6c8ad011ba52f1b10e36b342a26a";

    bridgeUserRequest getBridgeRequest(std::string txId);


}

#endif // BRIDGE_H