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
 * Custom Random Number Generator (RNG) for use in rdPoS.
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
    inline Hash& getSeed() { return this->seed; }

    /// Setter for `seed`.
    inline void setSeed(const Hash& seed) { seedLock.lock(); this->seed = seed; seedLock.unlock(); }

    /// Return the maximum numeric limit of a 256-bit unsigned integer.
    inline uint256_t max() { return std::numeric_limits<uint256_t>::max(); }

    /// Return the minimum numeric limit of a 256-bit unsigned integer.
    inline uint256_t min() { return std::numeric_limits<uint256_t>::min(); }

    /**
     * Shuffle the elements in a vector.
     * @param v A vector of any given type.
     */
    template <typename Vector> void shuffle(Vector& v);

    uint256_t operator()(); ///< Generate and return a new random seed.
};

#endif  // RANDOMGEN_H
