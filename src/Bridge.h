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

    inline const std::string bridgeFujiContract =   "0xf9f69ac5c744104a73849cefc86519fd518273de";
                                                    // BridgeContract -> Hex
    inline const std::string bridgeNativeContract = "0x000000000000427269646765436f6e7472616374";
    inline const std::string bridgeTopic = "0x5a36801d4ec996ac305bb61fdab651fb595d0e50bcf094ee73d6f226eed0cafb";
    inline const std::string bridgePrivKey = "1fa56224e3bb9ed9c4959efa1bcbaeed542e841e0c70968885e6aace8b1babfb";

    bridgeUserRequest getBridgeRequest(std::string txId);


}

#endif // BRIDGE_H