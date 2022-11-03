#include "utils.h"

#include <boost/lexical_cast.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/random.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>


// This uses mersenne twister as it's engine, it is 64 bits in size, not good for cryptographically secure applications.
// TODO: Figure out a way to use uint256_t on the engine itself.
class RandomGen {
  private:
    std::mt19937_64 gen;
    std::mutex gen_mutex;
  public:
    RandomGen(uint256_t seed) : gen(Utils::splitmix(std::hash<uint256_t>()(seed))) {};

    typedef uint256_t result_type;

    uint256_t max() {
      return std::numeric_limits<uint256_t>::max();
    }

    uint256_t min() {
      return std::numeric_limits<uint256_t>::min();
    }
 
    uint256_t operator()() {
      boost::random::uniform_int_distribution<uint256_t> distr(this->min(), this->max());
      gen_mutex.lock();
      auto result = distr(gen);
      gen_mutex.unlock();
      return result;
    }
 
    template <typename Vector>
    void shuffleVector(Vector& vector) {
      std::shuffle(vector.begin(), vector.end(), this->gen);
      return;
    };
};

