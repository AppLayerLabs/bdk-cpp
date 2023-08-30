#ifndef SAFEBASE_H
#define SAFEBASE_H

#include <memory>

// Forward declarations.
class DynamicContract;
class SafeBase;
void registerVariableUse(DynamicContract &contract, SafeBase &variable);

/**
 * Base class for all safe variables. Used to safely store a variable within a contract.
 * @see SafeAddress
 * @see SafeBool
 * @see SafeString
 * @see SafeUnorderedMap
 */
class SafeBase {
  private:
    /**
     * Pointer to the contract that owns the variable.
     * Why pointer and not reference?
     * Certain operators return a new copy of the variable, and such copy is
     * not stored within the contract, only within the function. Thus, the
     * contract pointer is not available because we don't need to register a
     * variable that will be destroyed, regardless of whether it reverts or not.
     * Variables of the contract should be initialized during the constructor of
     * such contract. Passing the contract as a pointer allows us to register it
     * to the contract, and call commit() and/or revert() properly.
     */
    DynamicContract* owner_ = nullptr;

  protected:
    /// Indicates whether the variable is already registered within the contract.
    mutable bool registered_ = false;

    /// Getter for `owner`.
    inline DynamicContract* getOwner() const { return owner_; }

    /// Register the use of the variable within the contract.
    void markAsUsed() {
      if (owner_ != nullptr && !registered_) {
        registerVariableUse(*owner_, *this);
        registered_ = true;
      }
    }

    /**
     * Check if the variable is initialized (and initialize it if not).
     * @throw std::runtime_error if not overridden by the child class.
     */
    inline virtual void check() const {
      throw std::runtime_error("Derived Class from SafeBase does not override check()");
    };

    /**
     * Check if the variable is registered within the contract.
     * @return `true` if the variable is registered, `false` otherwise.
     */
    inline bool isRegistered() const { return registered_; }

  public:
    /// Empty constructor. Should be used only within local variables within functions.
    SafeBase() : owner_(nullptr) {};

    /**
     * Constructor.
     * Only variables built with this constructor will be registered within the contract.
     * @param owner A pointer to the owner contract.
     */
    SafeBase(DynamicContract* owner) : owner_(owner) {};

    /**
     * Constructor for variables that are not registered within the contract.
     * Should be used only within local variables within functions.
     * @param other The variable to copy from.
     */
    SafeBase(SafeBase& other) : owner_(nullptr) {};

    /**
     * Commit a structure value to the contract.
     * Should always be overridden by the child class.
     * Child class should always do `this->registered = false;` at the end of commit().
     * @throw std::runtime_error if not overridden by the child class.
     */
    inline virtual void commit() {
      throw std::runtime_error("Derived Class from SafeBase does not override commit()");
    };

    /**
     * Revert a structure value (nullify).
     * Should always be overridden by the child class.
     * Child class should always do `this->registered = false;` at the end of revert().
     * @throw std::runtime_error if not overridden by the child class.
     */
    inline virtual void revert() const {
      throw std::runtime_error("Derived Class from SafeBase does not override revert()");
    };
};

#endif // SAFEBASE_H
