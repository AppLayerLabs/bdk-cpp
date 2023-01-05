#ifndef HASH_H
#define HASH_H

#include <memory>
#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/random.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include <secp256k1.h>
#include <secp256k1_ecdh.h>
#include <secp256k1_recovery.h>

#include "string.h"

using PrivKey = Hash;
using Pubkey = FixedStr<33>;
using UPubkey = FixedStr<65>;

namespace Secp256k1 {
  static const uint256_t ecConst(
    "115792089237316195423570985008687907852837564279074904382605163141518161494337"
  );
  UPubkey recover(const Signature& sig, const Hash& msg);
  Signature makeSig(const uint256_t& r, const uint256_t& s, const uint8_t& v);
  UPubkey toUPub(const PrivKey& key);
  UPubkey toUPub(const PubKey& key);
  Pubkey toPub(const PrivKey& key);
  Address toAddress(const UPubkey& key);
  Address toAddress(const Pubkey& key);
  Signature sign(const Hash& msg, const PrivKey& key);
  bool verify(const Hash& msg, const UPubkey& key, const Signature& sig);
};

struct SafeHash {
  uint64_t splitmix(uint64_t i);
  size_t operator()(uint64_t i);
  size_t operator()(const Address& add);
  size_t operator()(const std::string& str);
  size_t operator()(const boost::asio::ip::address add);
  template <typename T> size_t operator()(const std::shared_ptr<T>& ptr);
  template <unsigned N> size_t operator()(const FixedStr<N>& str);
};

class RandomGen {
  private:
    Hash seed;
    std::mutex seedLock;
  public:
    typedef uint256_t result_type; // Required for UniformRandomBitGenerator
    RandomGen(const Hash& seed) : seed(seed) {};
    inline Hash& getSeed() { return this->seed; }
    inline void setSeed(const Hash& seed) { seedLock.lock(); this->seed = seed; seedLock.unlock(); }
    inline uint256_t max() { return std::numeric_limits<uint256_t>::max(); }
    inline uint256_t min() { return std::numeric_limits<uint256_t>::min(); }
    template <typename Vector> void shuffle(Vector& v);
    uint256_t operator()();
};

#endif  // HASH_H
