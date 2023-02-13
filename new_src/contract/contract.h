#ifndef CONTRACT_H
#define CONTRACT_H

#include <cstdint>
#include <memory>
#include <string>

#include "../utils/db.h"
#include "../utils/strings.h"
#include "../utils/tx.h"
#include "../utils/utils.h"
#include "contractmanager.h"

class ContractManager; // Forward declaration.

/**
 * Native abstraction of a smart contract.
 * All contracts have to inherit this class.
 */
class Contract {
  private:
    /* Contract-specific variables */
    const Address address; ///< Address where the contract is deployed.
    const uint64_t chainId; ///< Chain where the contract is deployed.
    const std::unique_ptr<ContractManager> contractManager; ///< Pointer to the contract manager.

    /* Global variables */
    static Address coinbase; ///< Coinbase address (creator of current block).
    static uint256_t blockHeight; ///< Current block height.
    static uint256_t blockTimestamp; ///< Current block timestamp.

  public:
    /**
     * Constructor.
     * @param address The address where the contract will be deployed.
     * @param chainId The chain where the contract wil be deployed.
     * @param contractManager Pointer to the contract manager.
     */
    Contract(
      const Address& address, const uint64_t& chainId,
      const std::unique_ptr<ContractManager>& contractManager
    ) : address(address), chainId(chainId), contractManager(contractManager),
    coinbase("0x0000000000000000000000000000000000000000", false),
    blockHeight(0), blockTimestamp(0);

    /// Getter for `coinbase`.
    static const Address& getCoinbase() const { return this->coinbase; }

    /// Getter for `blockHeight`.
    static const uint256_t& getBlockHeight() const { return this->blockHeight; }

    /// Getter for `blockTimestamp`.
    static const uint256_t& getBlockTimestamp() const { return this->blockTimestamp; }

    /**
     * Invoke a contract function using a transaction.
     * Used by the %State class when calling `processNewBlock()`.
     * @param tx The transaction to use for call.
     */
    virtual void ethCall(const TxBlock& tx) {}

    /**
     * Invoke a contract function using a data string.
     * Used by RPC for answering `eth_call`.
     * @param data The data string to use for call.
     * @return An encoded %Solidity hex string with the desired function result.
     */
    virtual const std::string ethCall(const std::string& data) const {}

    friend State; // State can update private global vars
};

#endif  // CONTRACT_H
