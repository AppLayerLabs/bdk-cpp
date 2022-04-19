#ifndef VALIDATION_H
#define VALIDATION_H

#include <iostream>
#include <memory>
#include "db.h"
#include "block.h"
#include "ERC20.h"

class Validation : std::enable_shared_from_this<Validation> {
    private:
        std::string nodeID;

        Database blocksDb; // Key -> blockHash()
                            // Value -> json block
        
        Database confirmedTxs; // key -> txHash.
                                // value -> "" (if exists == confirmed);
        
        Database txToBlock; // key -> txhash.
                            // value -> blockHash.
        
        Database accountsDb; // Key -> underscored user address "0x..."
                              // Value -> json balance of native and erc20 tokens.
        
        Database nonceDb;  // key -> address
                      // value -> nonce. (string).

        Database tokenDB; // key -> token address
                          // value -> ERC20 info in json.

        std::map<std::string,ERC20> tokens; // Key -> contract address
                                            // value -> erc20 contract info. 

    public:

    // Initialize the validation class
    // Returns best block to answer avalanchego.
    Block initialize();

    Validation(std::string nodeID_) : nodeID(nodeID_) { initialize(); }

    bool validateBlock();
    bool validateTransaction(dev::eth::TransactionBase tx);


    void cleanAndClose();
};



#endif // VALIDATION_H