/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef OWNABLE_H
#define OWNABLE_H

#include "../dynamiccontract.h"
#include "../variables/safeaddress.h"
#include "../variables/safeunorderedmap.h"
#include "../variables/safevector.h"
#include "../variables/safeint.h"

/**
 * Chain governor contract
 *
 * NOTE: All token amounts are saved internally in uint64_t and multiplied by 1'000'000'000,
 * so any amounts under 0.000000001 are kept by the contract (effectively burned).
 *
 * NOTE: Maximum supported voting power or delegation is ~9.2 billion tokens: it should
 * not be possible to generate that many tokens (total) but we should do our best to
 * protect against this (it's only a problem in the current overflow protection code if
 * intermediary calculations exceed int256_t, so it's never a problem).
 *
 * TODO:
 * - Contract class is fully written and compiles.
 * - Register contract class / add to the contract template variant.
 * - Auto-deploy contract on initChain() (should be a fixed contract address)
 *   has to be initChain() since that's the one that gets the genesis file.
 * - Hook Blockchain::incomingBlock() to the system contract.
 *   - normal calls to the contract generate data structure changes
 *   - the newValidators is collected by incomingBlock() at its end, if any
 *     by calling the unregistered contract method directly.
 * - Write cometValidatorUpdates to a DB prefix in the blocks db.
 *   bytes33 pub key + uint64_t value which is the score
 * - Write unit test.
 * - PR branch.
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

    void recordDelegationDelta(const PubKey& validator, const uint64_t& delta, const bool& positive);
  
    SafeUint64_t numSlots_; /// The active number of validator slots.
    SafeUint64_t maxSlots_; /// The maximum value for numSlots_.

    /// Elected validators' votes on the future number of validator slots.
    /// If a number different to numSlots_ is agreed by >2/3 of votes in
    /// targetSlots_, then that number will be the new numSlots_.
    SafeUnorderedMap<PubKey, uint64_t> targetSlots_;

    /// Liquid deposits by users.
    SafeUnorderedMap<Address, uint64_t> stakes_;

    /// Validator delegations (votes) by users.
    /// Maps voter address to delegations, where each delegation is a validator key
    /// and a 64-bit encoded delegation (vote) amount.
    SafeUnorderedMap<Address, boost::unordered_flat_map<PubKey, uint64_t>> delegations_;

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
    SafeUnorderedMap<PubKey, int256_t> delegationDeltas_;

  public:
    using ConstructorArguments = std::tuple<
      std::vector<PubKey>&,
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
     * @param initialValidators The initial validator set. The voting power assigned on genesis
     * is irrelevant as the delegation amount will be set to zero by a dummy voter address.
     * @param initialNumSlots The initial number of validator slots.
     * @param maxSlots The maximum number of validator slots.
     * @param address The address of the contract.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain ID.
     */
    SystemContract(
      const std::vector<PubKey>& initialValidators,
      const uint64_t& initialNumSlots, const uint64_t& maxSlots,
      const Address& address, const Address& creator, const uint64_t& chainId
    );

    /// Deposit native tokens
    void stake();

    /// Withdraw native tokens
    void unstake(const uint256_t& amount);

    /// Vote for a validator
    void delegate(const PubKey& validator, const uint256_t& amount);

    /// Unvote a validator
    void undelegate(const PubKey& validator, const uint256_t& amount);

    /// Validator changes a slots vote
    /// The caller address must match the validator public key
    void voteslots(const PubKey& validator, const uint64_t& slots);

    /// Apply processing at the end of a block.
    /// This is an unregistered function called directly by the block processor
    /// after all block txs are applied to the machine state.
    void processBlock();

    /// Register the contract.
    static void registerContract() {
      // TODO: methods
      ContractReflectionInterface::registerContractMethods<SystemContract>(
        std::vector<std::string>{"initialValidators", "initialNumSlots", "maxSlots"}
        // stake
        // unstake
        // deletate
        // undelegate
        // voteslots
      );

      // REVIEW: should we add events for validator set updates generated and numSlots changes?
      //         I guess it wouldn't hurt to have those. Also helps testing (?)
      //ContractReflectionInterface::registerContractEvents<SystemContract>(
      //);
    }

    DBBatch dump() const override; ///< Dump method.
};

#endif // OWNABLE_H
