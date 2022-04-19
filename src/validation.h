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
                                // value -> transaction RLP
        
        Database txToBlock; // key -> txhash.
                            // value -> blockHash.
        
        Database accountsDb; // Key -> underscored user address "0x..."
                              // Value -> json balance of native and erc20 tokens.
        
        Database nonceDb;  // key -> address
                      // value -> nonce. (string).

        Database tokenDB; // key -> token address
                          // value -> ERC20 info in json.

        std::map<std::string,std::shared_ptr<ERC20>> tokens; // Key -> contract address
                                            // value -> erc20 contract info. 
        
        std::map<std::string,dev::eth::TransactionBase> mempool; // Tx hash -> tx data.

        std::mutex lock;
    public:

    // Initialize the validation class
    // Returns best block to answer avalanchego.
    Block initialize();

    Validation(std::string nodeID_) : nodeID(nodeID_) { }

    std::string getTxToBlock(std::string txHash);
    std::string getAccountNonce(std::string address);
    std::string getConfirmedTx(std::string txHash);
    Block getBlock(std::string blockKey);
    Block getLatestBlock();
    Block createNewBlock();
    bool validateBlock(Block block);
    bool validateTransaction(dev::eth::TransactionBase tx);
    bool addTxToMempool(dev::eth::TransactionBase tx);

    void faucet(std::string address);


    std::string getAccountBalanceFromDB(std::string address);
    std::string processEthCall(json &methods);
    void cleanAndClose();
    void createNewERC20(json &methods);
};



#endif // VALIDATION_H