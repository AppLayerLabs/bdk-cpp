#ifndef RANDOMGEN_H
#define RANDOMGEN_H

#include <boost/lexical_cast.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/random.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include "strings.h"

/**
 * Custom Pseudo Random Number Generator (RNG) for use in rdPoS.
 */

class RandomGen {
  private:
    Hash seed;            ///< RNG seed.
    std::mutex seedLock;  ///< Mutex for managing read/write access to the seed.
  public:
    /**
     * Alias for the result type.
     * Implemented in conformity with [UniformRandomBitGenerator](https://en.cppreference.com/w/cpp/named_req/UniformRandomBitGenerator).
     */
    typedef uint256_t result_type;

    /**
     * Constructor.
     * @param seed A random seed for initialization.
     */
    RandomGen(const Hash& seed) : seed(seed) {};

    /// Getter for `seed`.
    inline const Hash& getSeed() const { return this->seed; }

    /// Setter for `seed`.
    inline void setSeed(const Hash& seed) { seedLock.lock(); this->seed = seed; seedLock.unlock(); }

    /// Return the maximum numeric limit of a 256-bit unsigned integer.
    static inline uint256_t max() { return std::numeric_limits<result_type>::max(); }

    /// Return the minimum numeric limit of a 256-bit unsigned integer.
    static inline uint256_t min() { return std::numeric_limits<result_type>::min(); }

    /**
     * Shuffle the elements in a vector.
     * @param v A vector of any given type.
     */
    template <typename Vector> void shuffle(Vector& v) {
      this->seedLock.lock();
      for (uint64_t i = 0; i < v.size(); ++i) {
        this->seed = Utils::sha3(this->seed.get());
        //std::cout << this->seed.hex() << std::endl; // Uncomment to print seed
        uint64_t n = i + this->seed.toUint256() % (v.size() - i);
        std::swap(v[n], v[i]);
      }
      this->seedLock.unlock();
    }

    uint256_t operator()(); ///< Generate and return a new random seed.
};

#endif  // RANDOMGEN_H
