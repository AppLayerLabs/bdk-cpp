#ifndef SAFEADDRESS_H
#define SAFEADDRESS_H

#include "../../utils/strings.h"
#include "safebase.h"

/**
 * Safe wrapper for an Address variable.
 * This class is used to safely store an Address variable within a contract.
 * @see SafeBase
 * @see Address
 */
class SafeAddress : public SafeBase {
private:
  Address address; ///< Address value.
  mutable std::unique_ptr<Address>
      addressPtr; ///< Pointer to the address value.

  /**
   * Check if the addressPtr is initialized.
   * If not, initialize it.
   */
  inline void check() const override {
    if (addressPtr == nullptr) {
      addressPtr = std::make_unique<Address>(address);
    }
  };

public:
  /// Only Variables built with this constructor will be registered within a
  /// contract.

  /**
   * Constructor
   * Only variables built with this constructor will be registered within a
   * contract.
   * @param owner The contract that owns this variable.
   * @param address The address initial value (default: Address()).
   */
  SafeAddress(DynamicContract *owner, const Address &address = Address())
      : SafeBase(owner), address(address),
        addressPtr(std::make_unique<Address>(address)){};

  /**
   * Constructor
   * @param address The address initial value (default: Address()).
   */
  SafeAddress(const Address &address = Address())
      : SafeBase(nullptr), address(Address()),
        addressPtr(std::make_unique<Address>(address)){};

  /**
   * Constructor used to create a SafeAddress from another SafeAddress (copy
   * constructor).
   * @param address The address initial value (default: Address()).
   */
  SafeAddress(const SafeAddress &other) : SafeBase(nullptr) {
    check();
    address = other.address;
    addressPtr = std::make_unique<Address>(*other.addressPtr);
  }

  /**
  Assignment operator.
  @param address The address to be assigned.
  @return The assigned address value.
  */
  inline Address &operator=(const Address &address) {
    check();
    markAsUsed();
    *addressPtr = address;
    return *addressPtr;
  };

  /**
  Assignment operator from another SafeAddress.
  @param other The other SafeAddress.
  @return The assigned address value.
  */
  inline Address &operator=(const SafeAddress &other) {
    check();
    markAsUsed();
    *addressPtr = other.get();
    return *addressPtr;
  };

  /**
  Equality operator from another SafeAddress.
  @param other The other SafeAddress.
  @return True if the addresses are equal, false otherwise.
  */
  inline bool operator==(const SafeAddress &other) const {
    check();
    return (*addressPtr == other.get());
  }

  /**
  Equality operator from an Address.
  @param other The Address.
  @return True if the addresses are equal, false otherwise.
  */
  inline bool operator==(const Address &other) const {
    check();
    return (*addressPtr == other);
  }

  /** Inequality operator from another SafeAddress.
  @param other The other SafeAddress.
  @return True if the addresses are not equal, false otherwise.
  */
  inline bool operator!=(const SafeAddress &other) const {
    check();
    return (*addressPtr != other.get());
  }

  /**
  Inequality operator from an Address.
  @param other The Address.
  @return True if the addresses are not equal, false otherwise.
  */
  inline bool operator!=(const Address &other) const {
    check();
    return (*addressPtr != other);
  }

  /**
   * Getter for the address value.
   * @return The address value.
   */
  inline const Address &get() const {
    check();
    return *addressPtr;
  };

  /**
   * Commit the address value, i.e., update the address value with the value of
   * the addressPtr.
   */
  inline void commit() override {
    check();
    address = *addressPtr;
    addressPtr = nullptr;
  };

  /**
   * Revert the address value, i.e., nullify the addressPtr.
   */
  inline void revert() const override { addressPtr = nullptr; };
};

#endif // SAFEADDRESS_H