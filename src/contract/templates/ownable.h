/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef OWNABLE_H
#define OWNABLE_H

#include "../dynamiccontract.h"
#include "../variables/safeaddress.h"

/// Template for an ownable contract. Based on OpenZeppelin v5.0.2 Ownable contract
class Ownable : virtual public DynamicContract {
  private:
    SafeAddress owner_; ///< Owner of the contract.

    /**
     * Check that the contract caller is the owner.
     * @throw DynamicException if caller is not the owner.
     */
    void checkOwner_() const;

    /**
     * Transfer ownership of the contract to a new owner.
     * @param newOwner The address that will be the new owner.
     */
    void transferOwnership_(const Address& newOwner);

    void registerContractFunctions() override; ///< Register the contract functions.

  public:
    using ConstructorArguments = std::tuple<Address>; ///< The constructor argument types.

    /// Event for when ownership of the contract is transferred.
    void ownershipTransferred(const EventParam<Address, true>& previousOwner, const EventParam<Address, true>& newOwner) {
      this->emitEvent(__func__, std::make_tuple(previousOwner, newOwner));
    }

    /**
     * Constructor for loading contract from DB.
     * @param address The address of the contract.
     * @param db The database to use.
     */
    //Ownable(const Address& address, const DB& db);

    /**
     * Constructor to be used when creating a new contract.
     * @param initialOwner The address that will be the initial owner of the contract.
     * @param address The address of the contract.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain ID.
     */
    Ownable(
      const Address& initialOwner, const Address &address,
      const Address &creator, const uint64_t &chainId
    );

    /**
     * Constructor for derived types.
     * @param derivedTypeName The name of the derived type.
     * @param initialOwner The address that will be the initial owner of the contract.
     * @param address The address of the contract.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain ID.
     */
    Ownable(
      const std::string &derivedTypeName, const Address& initialOwner,
      const Address &address, const Address &creator, const uint64_t &chainId
    );

    void onlyOwner() const; ///< Wrapper for checkOwner_().
    Address owner() const; ///< Get the owner's address.
    void renounceOwnership(); ///< Renounce ownership of the contract. Ownership is transferred to an empty address (Address()).

    /**
     * Wrapper for transferOwnership_().
     * @param newOwner The address of the new owner.
     */
    void transferOwnership(const Address& newOwner);

    /// Register the contract.
    static void registerContract() {
      ContractReflectionInterface::registerContractMethods<Ownable, const Address&>(
        std::vector<std::string>{"initialOwner"},
        std::make_tuple("onlyOwner", &Ownable::onlyOwner, FunctionTypes::NonPayable, std::vector<std::string>{}),
        std::make_tuple("owner", &Ownable::owner, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("renounceOwnership", &Ownable::renounceOwnership, FunctionTypes::NonPayable, std::vector<std::string>{}),
        std::make_tuple("transferOwnership", &Ownable::transferOwnership, FunctionTypes::NonPayable, std::vector<std::string>{"newOwner"})
      );
      ContractReflectionInterface::registerContractEvents<Ownable>(
        std::make_tuple("ownershipTransferred", false, &Ownable::ownershipTransferred, std::vector<std::string>{"previousOwner","newOwner"})
      );
    }

    //DBBatch dump() const override; ///< Dump method.
};

#endif // OWNABLE_H
