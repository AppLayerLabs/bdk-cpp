/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef RANDOMGEN_H
#define RANDOMGEN_H

#include <boost/lexical_cast.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/random.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include "utils.h"

/// Custom Pseudo-Random Number Generator (PRNG) for use in rdPoS.
class RandomGen {
  private:
    Hash seed_;  ///< The seed used by the generator.
    mutable std::mutex seedLock_;  ///< Mutex for managing read/write access to the seed.

  public:
    /**
     * Alias for the result type.
     * Implemented in conformity with UniformRandomBitGenerator:
     * https://en.cppreference.com/w/cpp/named_req/UniformRandomBitGenerator
     */
    typedef uint256_t result_type;

    /**
     * Constructor.
     * @param seed A random seed for initialization.
     */
    RandomGen(const Hash& seed) : seed_(seed) {};

    /// Getter for `seed`.
    inline const Hash getSeed() const { std::lock_guard lock(seedLock_); return this->seed_; }

    /// Setter for `seed`.
    inline void setSeed(const Hash& seed) { std::lock_guard lock(seedLock_); this->seed_ = seed; }

    /// Return the maximum numeric limit of a 256-bit unsigned integer.
    static inline uint256_t max() { return std::numeric_limits<result_type>::max(); }

    /// Return the minimum numeric limit of a 256-bit unsigned integer.
    static inline uint256_t min() { return std::numeric_limits<result_type>::min(); }

    /**
     * Shuffle the elements of a given vector.
     * Vector is a std::vector of any given type.
     * @param v The vector to shuffle.
     */
    template <typename Vector> void shuffle(Vector& v) {
      std::lock_guard lock(seedLock_);
      for (uint64_t i = 0; i < v.size(); ++i) {
        this->seed_ = Utils::sha3(this->seed_.get());
        //std::cout << this->seed_.hex() << std::endl; // Uncomment to print seed
        uint64_t n = uint64_t(i + this->seed_.toUint256() % (v.size() - i));
        std::swap(v[n], v[i]);
      }
    }

    /// Call operator that generates and returns a new random seed.
    uint256_t operator()();
};

#endif  // RANDOMGEN_H
