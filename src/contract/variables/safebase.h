/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef SAFEBASE_H
#define SAFEBASE_H

#include <memory>
#include <cassert>
#include "../utils/dynamicexception.h"

// Forward declarations.
class DynamicContract;
class SafeBase;
void registerVariableUse(DynamicContract &contract, SafeBase &variable);

/**
 * Base class for all safe variables. Used to safely store a variable within a contract.
 * @see SafeAddress, SafeBool, SafeInt_t, SafeUint_t, SafeString, SafeUnorderedMap, SafeTuple, SafeVector
 */
class SafeBase {
  private:
    /**
     * Pointer to the contract that owns the variable. Why pointer and not reference?
     * Certain operators return a new copy of the variable, and such copy is
     * not stored within the contract, only within the function.
     * Thus, the contract pointer is not available because we don't need to register
     * a variable that will be destroyed, regardless of whether it reverts or not.
     * Variables of the contract should be initialized during the constructor of
     * such contract. Passing the contract as a pointer allows us to register it
     * to the contract, and call commit() and/or revert() properly.
     */
    DynamicContract* owner_ = nullptr;

  protected:
    mutable bool registered_ = false; ///< Indicates whether the variable is already registered within the contract.
    bool shouldRegister_ = false; ///< Indicates whether the variable should be registered within the contract.

    inline DynamicContract* getOwner() const { return this->owner_; } ///< Getter for `owner`.

    /// Register the use of the variable within the contract.
    void markAsUsed() {
      if (this->owner_ != nullptr && !this->registered_ && this->shouldRegister_) {
        registerVariableUse(*owner_, *this);
        this->registered_ = true;
      }
    }

    /**
     * Check if the variable is initialized (and initialize it if not).
     * @throw DynamicException if not overridden by the child class.
     */
    inline virtual void check() const {
      throw DynamicException("Derived Class from SafeBase does not override check()");
    };

    /**
     * Check if the variable is registered within the contract.
     * @return `true` if the variable is registered, `false` otherwise.
     */
    inline bool isRegistered() const { return this->registered_; }

  public:
    /// Empty constructor. Should be used only within local variables within functions.
    SafeBase() : owner_(nullptr) {};

    /**
     * Constructor with owner. Only variables built with this constructor will be registered within the contract.
     * @param owner A pointer to the owner contract.
     */
    explicit SafeBase(DynamicContract* owner) : owner_(owner) {};

    /**
     * Constructor for variables that are not registered within the contract.
     * Should be used only within local variables within functions.
     */
    SafeBase(const SafeBase&) : owner_(nullptr) {};

    void enableRegister() { this->shouldRegister_ = true; } ///< Enable variable registration.

   /**
    * Commit a structure value to the contract. Should always be overridden by the child class.
    * Child class should always do `this->registered = false;` at the end of commit().
    */
    inline virtual void commit() { assert(false); }

   /**
    * Revert a structure value (nullify). Should always be overridden by the child class.
    * Child class should always do `this->registered = false;` at the end of revert().
    */
    inline virtual void revert() const { assert(false); }
};

#endif // SAFEBASE_H
