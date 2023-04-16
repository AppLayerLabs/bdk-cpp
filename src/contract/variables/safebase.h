#ifndef SAFEBASE_H
#define SAFEBASE_H

#include <memory>

/// Forward Declarations
class Contract;

class SafeBase;

void registerVariableUse(Contract& contract, SafeBase& variable);

class SafeBase {
  private:
    /// Pointer back to the contract
    /// Why pointer and not reference?
    /// Certain operators returns a new copy of the variable.
    /// Such copy is not stored within the contract, only within the function, and thus the contract pointer is not available
    /// Because we don't need to register a variable that will be destroyed regardless if reverts or not.
    /// Variables of the contract should be initialized during the constructor of such contract
    /// Passing the contract as a pointer allows us to register it to the contract, and call commit()/revert() properly.
    Contract* owner = nullptr;

  protected:
    /// Register the use of this variable within the contract
    void markAsUsed() { if (owner != nullptr) { if (!registered) { registerVariableUse(*owner, *this); registered = true; }}}

    /// Boolean to check if the variable is already registered within the contract
    mutable bool registered = false;
    /// Should always be overridden by the child class
    inline virtual void check() const { throw std::runtime_error("Derived Class from SafeBase does not override check()"); };

    inline bool isRegistered() const { return registered; }
    inline Contract* getOwner() const { return owner; }

  public:

    /// Default Constructor.
    /// This should be used only within local variables within functions only.
    SafeBase() : owner(nullptr) {};

    /// Normal Constructor.
    /// Only variables built with this constructor will be registered within the contract.
    SafeBase(Contract* owner) : owner(owner) {};

    /// Normal Constructor for variables that are not registered within the contract.
    /// This should be used only within local variables within functions.
    SafeBase(SafeBase& other) : owner(nullptr) {};

    /// Should always be overridden by the child class
    /// Child class should always do this->registered = false; at the end of commit() and revert()
    inline virtual void commit() {  throw std::runtime_error("Derived Class from SafeBase does not override commit()"); };
    inline virtual void revert() const { throw std::runtime_error("Derived Class from SafeBase does not override check()"); };
};

#endif // SAFEBASE_H