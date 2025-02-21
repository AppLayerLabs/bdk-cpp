/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef SYSTEMCONTRACT_H
#define SYSTEMCONTRACT_H

#include "../dynamiccontract.h"
#include "../variables/safeaddress.h"
#include "../variables/safeunorderedmap.h"
#include "../variables/safevector.h"
#include "../variables/safeint.h"
#include "../variables/safebool.h"

/**
 * Chain governor contract
 *
 * NOTE: All token amounts are saved internally in uint64_t and multiplied by 1'000'000'000,
 * so any amounts under 0.000000001 are kept by the contract (effectively burned).
 *
 * NOTE: Maximum supported voting power or delegation is ~9.2 billion tokens (int64_t, since
 * that's the type used by CometBFT for validator voting power): it should not be possible to
 * generate that many tokens (total) but we should do our best to protect against this (it's
 * only a problem in the current overflow protection code if intermediary calculations exceed
 * int256_t, so it's never a problem).
 */
class SystemContract : public DynamicContract {
  private:
    void registerContractFunctions() override; ///< Register the contract functions.

    void doRegister() {
      SystemContract::registerContractFunctions();
      this->numSlots_.enableRegister();
      this->maxSlots_.enableRegister();
      this->targetSlots_.enableRegister();
      this->stakes_.enableRegister();
      this->delegations_.enableRegister();
      this->validators_.enableRegister();
      this->validatorVotes_.enableRegister();
      this->delegationDeltas_.enableRegister();
    }

    static constexpr uint64_t AMOUNT_ENCODING_SCALE = 1000000000ULL; // 1e9

    /// Encode a 256-bit amount to 64 bits
    static uint64_t encodeAmount(const uint256_t &amountWei) {
      uint256_t remainder = amountWei % AMOUNT_ENCODING_SCALE;
      if (remainder != 0) {
        throw DynamicException("Cannot send dust to the system contract");
      }
      uint256_t quotient = amountWei / AMOUNT_ENCODING_SCALE;
      if (quotient > std::numeric_limits<uint64_t>::max()) {
        throw DynamicException("System contract deposit too large");
      }
      return static_cast<uint64_t>(quotient);
    }

    /// Decode a 256-bit amount from 64 bits
    static uint256_t decodeAmount(uint64_t amount64) {
      return uint256_t(amount64) * AMOUNT_ENCODING_SCALE;
    }

    static PubKey pubKeyFromString(const std::string& pubKeyStr);

    void recordDelegationDelta(const PubKey& validator, const uint64_t& delta, const bool& positive);

    SafeUint64_t numSlots_; /// The active number of validator slots.
    SafeUint64_t maxSlots_; /// The maximum value for numSlots_.

    /// Elected validators' votes on the future number of validator slots.
    /// If a number different to numSlots_ is agreed by >2/3 of votes in
    /// targetSlots_, then that number will be the new numSlots_.
    SafeUnorderedMap<PubKey, uint64_t> targetSlots_;

    /// Optimization: avoid recomputing targetSlots_ if no voteSlots() called.
    SafeBool targetSlotsModified_;

    /// Liquid deposits by users.
    SafeUnorderedMap<Address, uint64_t> stakes_;

    /// Validator delegations (votes) by users.
    /// Maps voter address to delegations, where each delegation is a validator key
    /// and a 64-bit encoded delegation (vote) amount.
    SafeUnorderedMap<Address, boost::unordered_flat_map<PubKey, uint64_t, SafeHash>> delegations_;

    /// Sorted list of all validator candidates; the first numSlots_ elements are the elected validators.
    SafeVector<PubKey> validators_;

    /// Sorted list of all validator candidate vote totals.
    /// Indices in validatorVotes_ match indices in validators_.
    /// For simplicity, it is just not allowed for two validators to have the exact same amount of votes,
    /// so any inbound vote that will cause a value collision will loop decreasing the delegated amount by
    /// 1 unit until the collision is resolved (or the delegation fails with an amount of 0).
    SafeVector<uint64_t> validatorVotes_;

    /// List of queued validator delegation changes (increases/decreases in votes).
    /// Delegation (vote) deltas are accumulated during block (tx) processing and applied
    /// after all txs in a block are processed, generating the new validator ranking and
    /// thus the needed validator updates for CometBFT.
    /// This is int256_t because we need the delta to be an uint64_t in magnitude in either
    /// direction (positive or negative). Could be int128_t as well, but int256_t matches
    /// the intermediary type used for calculating and validating the delta.
    SafeUnorderedMap<PubKey, int256_t> delegationDeltas_;

  public:
    using ConstructorArguments = std::tuple<
      std::vector<std::string>&,
      const uint64_t&,
      const uint64_t&
    >;

    /**
     * Constructor for loading contract from DB.
     * @param address The address of the contract.
     * @param db The database to use.
     */
    SystemContract(const Address& address, const DB& db);

    /**
     * Create the system contract.
     * This constructor is invoked by the node itself (class Blockchain) to create exactly
     * one instance of this contract at genesis.
     * @param initialValidatorPubKeys The initial validator set. The voting power assigned on genesis
     * is irrelevant as the delegation amount will be set to zero by a dummy voter address.
     * @param initialNumSlots The initial number of validator slots.
     * @param maxSlots The maximum number of validator slots.
     * @param address The address of the contract.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain ID.
     */
    SystemContract(
      const std::vector<std::string>& initialValidatorPubKeys,
      const uint64_t& initialNumSlots, const uint64_t& maxSlots,
      const Address& address, const Address& creator, const uint64_t& chainId
    );

    /**
     * An user deposits native tokens into the system contract.
     * The tx sender (caller) is the depositor, and the tx value is the amount of native tokens deposited.
     */
    void stake();

    /**
     * An user withdraws native tokens from the system contract.
     * The tx sender (caller) is requesting the withdrawal.
     * @param amount Amount of native tokens to withdraw in wei.
     */
    void unstake(const uint256_t& amount);

    /**
     * An user votes for a validator.
     * The tx sender (caller) is the user that is delegating tokens to a validator key.
     * @param validatorPubKey 33-byte Secp256k1 validador public key to vote, in hex (66 hex char string).
     * @param amount Amount of staked tokens to delegate to the validator key.
     */
    void delegate(const std::string& validatorPubKey, const uint256_t& amount);

    /**
     * An user unvotes from a validator.
     * The tx sender (caller) is the user that is undelegating tokens from a validator key.
     * @param validatorPubKey 33-byte Secp256k1 validador public key to unvote, in hex (66 hex char string).
     * @param amount Amount of staked tokens to undelegate from the validator key.
     */
    void undelegate(const std::string& validatorPubKey, const uint256_t& amount);

    /**
     * An elected validator sets its vote for the number of validator slots.
     * The tx sender (caller) must be the account for validatorPubKey.
     * @param validatorPubKey 33-byte Secp256k1 validador public key that is voting for a number of slots (must match caller account).
     * @param slots Chosen target number of validator slots to vote for.
     */
    void voteSlots(const std::string& validatorPubKey, const uint64_t& slots);

    /**
     * Called by the node at the end of block processing to obtain the pending validator set delta.
     * NOTE: Unregistered function, not callable from other contracts.
     * @param validatorDeltas Validator set updates, which are validator public keys and their new voting powers (0 to remove).
     */
    void finishBlock(std::vector<std::pair<PubKey, uint64_t>>& validatorDeltas);

    /**
     * Get a reference to the ordered validator list.
     * NOTE: Unregistered function, not callable from other contracts.
     * @return List of all validator public keys which have delegations (may be more than the number of slots).
     */
    SafeVector<PubKey>& getValidators() { return validators_; }

    /**
     * Get a reference to the ordered validator delegation amounts list.
     * NOTE: Unregistered function, not callable from other contracts.
     * @return List of all validator delegations (may be more than the number of slots).
     */
    SafeVector<uint64_t>& getValidatorVotes() { return validatorVotes_; };

    /**
     * Get the current number of validator slots.
     * NOTE: Unregistered function, not callable from other contracts.
     * @return Current number of validator slots.
     */
    uint64_t getNumSlots() { return numSlots_.get(); }

    /// Register the contract.
    static void registerContract() {
      ContractReflectionInterface::registerContractMethods<SystemContract>(
        std::vector<std::string>{"initialValidators", "initialNumSlots", "maxSlots"},
        std::make_tuple("stake", &SystemContract::stake, FunctionTypes::Payable, std::vector<std::string>{}),
        std::make_tuple("unstake", &SystemContract::unstake, FunctionTypes::NonPayable, std::vector<std::string>{"amount"}),
        std::make_tuple("delegate", &SystemContract::delegate, FunctionTypes::NonPayable, std::vector<std::string>{"validatorPubKey", "amount"}),
        std::make_tuple("undelegate", &SystemContract::undelegate, FunctionTypes::NonPayable, std::vector<std::string>{"validatorPubKey", "amount"}),
        std::make_tuple("voteSlots", &SystemContract::voteSlots, FunctionTypes::NonPayable, std::vector<std::string>{"validatorPubKey", "slots"})
      );
      // FIXME/TODO: add validator set update events (one per {validator,votes} change)
      // FIXME/TODO: add numslots update event ({int new_numslots})
      //ContractReflectionInterface::registerContractEvents<SystemContract>(
      //);
    }

    DBBatch dump() const override; ///< Dump method.
};

#endif // SYSTEMCONTRACT_H
