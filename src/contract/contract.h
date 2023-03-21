#ifndef CONTRACT_H
#define CONTRACT_H

#include <cstdint>
#include <memory>
#include <string>

#include "contractmanager.h"

#include "../utils/db.h"
#include "../utils/strings.h"
#include "../utils/tx.h"
#include "../utils/utils.h"

// Forward declarations.
class ContractManager;
class State;

class ContractGlobals {
  protected:
    /* Global variables */
    static Address coinbase;                ///< Coinbase address (creator of current block).
    static uint256_t blockHeight;           ///< Current block height.
    static uint256_t blockTimestamp;        ///< Current block timestamp.
  public:
    /// Getter for `coinbase`.
    static const Address& getCoinbase() { return coinbase; }

    /// Getter for `blockHeight`.
    static const uint256_t& getBlockHeight() { return blockHeight; }

    /// Getter for `blockTimestamp`.
    static const uint256_t& getBlockTimestamp() { return blockTimestamp; }

    friend State; // State can update private global vars, this is done before calling ethCall() with a TxBlock.
};

/**
 * Native abstraction of a smart contract.
 * All contracts have to inherit this class.
 */
class Contract : public ContractGlobals {
  protected:
    /* Contract-specific variables */
    const std::string name;                                  ///< Name of the contract, used to identify the Contract Class.
    const Address address;                                   ///< Address where the contract is deployed.
    const uint64_t chainId;                                  ///< Chain where the contract is deployed.
    const std::unique_ptr<DB> &db;                           ///< Reference to the DB where the contract stored deployed.

  public:
    /**
     * Constructor.
     * @param address The address where the contract will be deployed.
     * @param chainId The chain where the contract wil be deployed.
     * @param contractManager Pointer to the contract manager.
     */
    Contract(
      const Address& address, const uint64_t& chainId, const std::unique_ptr<DB> &db
    ) : address(address), chainId(chainId), db(db) {}

    ~Contract() {}

    /**
     * Invoke a contract function using a transaction.
     * Used by the %State class when calling `processNewBlock()`.
     * @param tx The transaction to use for call.
     */
    virtual void ethCall(const TxBlock& tx) {};

    /**
     * Invoke a const (solidity view) contract function using a data string.
     * Used by RPC for answering `eth_call`.
     * @param data The data string to use for call.
     * @return An encoded %Solidity hex string with the desired function result.
     */
    virtual const std::string ethCall(const std::string& data) const { return ""; };

    /// Getters
    const std::string& getName() const { return name; }
    const Address& getAddress() const { return address; }
    const uint64_t& getChainId() const { return chainId; }
};

#endif  // CONTRACT_H
