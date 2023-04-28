#ifndef SAFEBASE_H
#define SAFEBASE_H

#include <memory>

/// Forward Declarations
class DynamicContract;
class SafeBase;
void registerVariableUse(DynamicContract &contract, SafeBase &variable);

/**
 * Base class for all safe variables.
 * This class is used to safely store a variable within a contract.
 * @see SafeAddress
 * @see SafeBool
 * @see SafeString
 * @see SafeUint8_t
 * @see SafeUint16_t
 * @see SafeUint32_t
 * @see SafeUint64_t
 * @see SafeUnorderedMap
 */
class SafeBase {
private:
  /// Pointer back to the contract.
  /// Why pointer and not reference?
  /// Certain operators returns a new copy of the variable.
  /// Such copy is not stored within the contract, only within the function, and
  /// thus the contract pointer is not available Because we don't need to
  /// register a variable that will be destroyed regardless if reverts or not.
  /// Variables of the contract should be initialized during the constructor of
  /// such contract Passing the contract as a pointer allows us to register it
  /// to the contract, and call commit()/revert() properly.
  DynamicContract *owner = nullptr;

protected:
  /** Register the use of this variable within the contract
   */
  void markAsUsed() {
    if (owner != nullptr) {
      if (!registered) {
        registerVariableUse(*owner, *this);
        registered = true;
      }
    }
  }

  /// Boolean to check if the variable is already registered within the contract
  mutable bool registered = false;

  /** Template function to check if struct is initialized.
   * If not, initialize it.
   */
  inline virtual void check() const {
    throw std::runtime_error(
        "Derived Class from SafeBase does not override check()");
  };

  /**
  * Check if the variable is registered within the contract.
  @return True if the variable is registered within the contract, false
  otherwise.
  */
  inline bool isRegistered() const { return registered; }

  /**
  * Getter for the pointer back to the contract.
  @return Pointer back to the contract.
  */
  inline DynamicContract *getOwner() const { return owner; }

public:
  /**Default Constructor.
   * This should be used only within local variables within functions only.
   */
  SafeBase() : owner(nullptr){};

  /**Normal Constructor.
   *Only variables built with this constructor will be registered within the
   *contract.
   *@param owner Pointer back to the contract.
   */
  SafeBase(DynamicContract *owner) : owner(owner){};

  /**Normal Constructor for variables that are not registered within the
   *contract. This should be used only within local variables within
   *functions.
   *@param other The variable to copy from.
   */
  SafeBase(SafeBase &other) : owner(nullptr){};

  /**
  Template function to commit a structure value to the contract.
  * Should always be overridden by the child class
  *Child class should always do this->registered = false; at the end of
  commit()
  */
  inline virtual void commit() {
    throw std::runtime_error(
        "Derived Class from SafeBase does not override commit()");
  };

  /**
  Template function to revert a structure value (nullify)
  * Should always be overridden by the child class
  * Child class should always do this->registered = false; at the end of
  revert()
  */
  inline virtual void revert() const {
    throw std::runtime_error(
        "Derived Class from SafeBase does not override check()");
  };
};

#endif // SAFEBASE_H