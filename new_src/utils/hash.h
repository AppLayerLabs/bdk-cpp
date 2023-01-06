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

// TODO: capitalization should be the same for all aliases (PrivKey/PubKey/UPubKey)
using PrivKey = Hash;
using Pubkey = FixedStr<33>;
using UPubkey = FixedStr<65>;

/**
 * Namespace for abstracting secp256k1 functions.
 */
namespace Secp256k1 {
  /**
   * Elliptic curve constant (2^256 - 2^32 - 2^9 - 2^8 - 2^7 - 2^6 - 2^4 - 1).
   */
  static const uint256_t ecConst(
    "115792089237316195423570985008687907852837564279074904382605163141518161494337"
  );

  /**
   * Recover a public key from a signature and its hash.
   * @param sig The signature to use for recovery.
   * @param msg The message hash (in bytes) to use for recovery.
   * @return The recovered uncompressed public key, or an empty string on failure.
   */
  UPubkey recover(const Signature& sig, const Hash& msg);

  /**
   * Create an ECDSA (Elliptic Curve Digital Signature Algorithm) signature.
   * @param r The first half (32 bytes) of the ECDSA signature.
   * @param s The second half (32 bytes) of the ECDSA signature.
   * @param v The recivery id (1 byte).
   * @returns The full ECDSA signature.
   */
  Signature makeSig(const uint256_t& r, const uint256_t& s, const uint8_t& v);

  /**
   * Derive an uncompressed public key from a private one.
   * @param key The private key to derive from.
   * @returns The derived uncompressed public key.
   */
  UPubkey toUPub(const PrivKey& key);

  /**
   * Derive an uncompressed public key from a compressed public one.
   * @param key The compressed public key to derive from.
   * @returns The derived uncompressed public key.
   */
  UPubkey toUPub(const PubKey& key);

  /**
   * Derive a compressed public key from a private one.
   * @param key The private key to derive from.
   * @return The derived compressed public key.
   */
  Pubkey toPub(const PrivKey& key);

  /**
   * Derive an address from a given uncompressed public key.
   * @param key The uncompressed public key to derive from.
   * @return The derived address.
   */
  Address toAddress(const UPubkey& key);

  /**
   * Derive an address from a given compressed public key.
   * @param key The compressed public key to derive from.
   * @return The derived address.
   */
  Address toAddress(const Pubkey& key);

  /**
   * Sign a message using a given private key.
   * @param msg The message to sign.
   * @param key The private key to use for signing.
   * @return The signature of the message.
   */
  Signature sign(const Hash& msg, const PrivKey& key);

  /**
   * Verify a signature against a given public key and message.
   * @param msg The message to use for verification.
   * @param key The uncompressed public key to use for verification.
   * @param sig The signature to verify.
   * @return `true` if the signature is verified, `false` otherwise.
   */
  bool verify(const Hash& msg, const UPubkey& key, const Signature& sig);
};

/**
 * Custom hashing implementation for use in `std::unordered_map`.
 * Based on [this article](https://codeforces.com/blog/entry/62393).
 * The default `std::unordered_map` implementation uses `uint64_t` hashes,
 * which makes collisions possible by having many Accounts and distributing them
 * in a way that they have the same hash across all nodes.
 * This struct is a workaround for that, it's not perfect because it still uses
 * `uint64_t`, but it's better than nothing since nodes keeps different hashes.
 */
struct SafeHash {
  /**
   * Hash a given unsigned integer.
   * Based on [Sebastiano Vigna's original implementation](http://xorshift.di.unimi.it/splitmix64.c).
   * @param i A 64-bit unsigned integer.
   * @returns The hashed data as a 64-bit unsigned integer.
   */
  uint64_t splitmix(uint64_t i);

  /**
   * Wrapper for `splitmix()`.
   * @param i A 64-bit unsigned integer.
   * @returns The same as `splitmix()`.
   */
  size_t operator()(uint64_t i);

  /**
   * Wrapper for `splitmix()`.
   * @param add An Address object.
   * @returns The same as `splitmix()`.
   */
  size_t operator()(const Address& add);

  /**
   * Wrapper for `splitmix()`.
   * @param str A regular string.
   * @returns The same as `splitmix()`.
   */
  size_t operator()(const std::string& str);

  /**
   * Wrapper for `splitmix()`.
   * @param add A Boost ASIO address.
   * @returns The same as `splitmix()`.
   */
  size_t operator()(const boost::asio::ip::address add);

  /**
   * Wrapper for `splitmix()`.
   * @param ptr A shared pointer to any given type.
   * @returns The same as `splitmix()`.
   */
  template <typename T> size_t operator()(const std::shared_ptr<T>& ptr);

  /**
   * Wrapper for `splitmix()`.
   * @param str A %FixedStr of any size.
   * @returns The same as `splitmix()`.
   */
  template <unsigned N> size_t operator()(const FixedStr<N>& str);
};

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

#endif  // HASH_H
