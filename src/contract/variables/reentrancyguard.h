/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef REENTRANCY_GUARD_H
#define REENTRANCY_GUARD_H

#include <atomic>

/**
 * The ReentrancyGuard class is used to prevent reentrancy attacks.
 * Similarly to std::unique_lock or std::shared_lock, ReentrancyGuard is a RAII object.
 * It is meant to be used within the first line of the function you want to protect against reentrancy attacks.
 * The constructor of ReentrancyGuard will check the bool and set it to true.
 * If the bool is already true, the constructor will throw an exception.
 * The destructor of ReentrancyGuard will set the bool to false.
 */

class ReentrancyGuard {
  private:
    bool &lock_; ///< Reference to the mutex.

  public:
    /**
     * Constructor.
     * @param lock Reference to the mutex.
     * @throw std::runtime_error if the mutex is already locked.
     */
    ReentrancyGuard(bool &lock) : lock_(lock) {
      if(lock_) {
        throw std::runtime_error("ReentrancyGuard: reentrancy attack detected");
      }
      lock_ = true;
    }

    /// Destructor.
    ~ReentrancyGuard() {
      lock_ = false;
    }
};


#endif