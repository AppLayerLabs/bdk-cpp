#ifndef OWNABLE_H
#define OWNABLE_H

#include "../dynamiccontract.h"
#include "../variables/safeaddress.h"


/// Ownable Contract
/// Based on OpenZeppelin v5.0.2 Ownable contract
class Ownable : virtual public DynamicContract {
  private:

    SafeAddress owner_;

    void checkOwner_() const;

    void transferOwnership_(const Address& newOwner);

    void registerContractFunctions() override;


  public:
    using ConstructorArguments = std::tuple<Address>;

    void ownershipTransferred(const EventParam<Address, true>& previousOwner, const EventParam<Address, true>& newOwner) {
      this->emitEvent(__func__, std::make_tuple(previousOwner, newOwner));
    }

    // Constructor for loading contract from DB.
    Ownable(
      const Address& address, const DB& db
    );

    // Constructor to be used when creating a new contract.
    Ownable(
      const Address& initialOwner, const Address &address, const Address &creator, const uint64_t &chainId
    );

    // Constructor for derived types!
    Ownable(
      const std::string &derivedTypeName,
      const Address& initialOwner, const Address &address, const Address &creator, const uint64_t &chainId
    );

    void onlyOwner() const;

    Address owner() const;

    void renounceOwnership();

    void transferOwnership(const Address& newOwner);


    static void registerContract() {
      ContractReflectionInterface::registerContractMethods<
        Ownable, const Address&
      >(
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

    /// Dump method
    DBBatch dump() const override;
};




#endif // OWNABLE_H