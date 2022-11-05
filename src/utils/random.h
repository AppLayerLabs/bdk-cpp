#ifndef RANDOM_H
#define RANDOM_H

#include "utils.h"

#include <boost/lexical_cast.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/random.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>


class RandomGen {
  private:
    Hash _seed;
    std::mutex seed_mutex;
  public:
    RandomGen(const Hash& seed) : _seed(seed) {};

    typedef uint256_t result_type;

    uint256_t max() {
      return std::numeric_limits<uint256_t>::max();
    }

    uint256_t min() {
      return std::numeric_limits<uint256_t>::min();
    }
 
    uint256_t operator()() {
      seed_mutex.lock();
      _seed = Utils::sha3(_seed.get());
      auto result = _seed.toUint256();
      seed_mutex.unlock();
      return result;
    }
 
    template <typename Vector>
    void shuffleVector(Vector& vector) {
      seed_mutex.lock();
      for (uint64_t i = 0; i < vector.size(); ++i) {
        this->_seed = Utils::sha3(this->_seed.get());
        std::cout << _seed.hex() << std::endl;
        uint64_t n = i + this->_seed.toUint256() % (vector.size() - i);
        std::swap(vector[n],vector[i]);
      }
      seed_mutex.unlock();
      return;
    };

    void setSeed(const Hash& seed) {
      seed_mutex.lock();
      this->_seed = seed;
      seed_mutex.unlock();
      return;
    }
};

#endif