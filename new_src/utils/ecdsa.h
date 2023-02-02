#ifndef ECDSA_H
#define ECDSA_H

#include <secp256k1.h>
#include <secp256k1_ecdh.h>
#include <secp256k1_recovery.h>

#include "strings.h"

using PrivKey = Hash;
using PubKey = FixedStr<33>;
using UPubKey = FixedStr<65>;

/**
 * Namespace for abstracting secp256k1 functions.
 */
namespace Secp256k1 {
  /**
   * Elliptic curve constant (2^256 - 2^32 - 2^9 - 2^8 - 2^7 - 2^6 - 2^4 - 1).
   * Equals to 0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141.
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
  UPubKey recover(const Signature& sig, const Hash& msg);

  /**
   * Create an ECDSA (Elliptic Curve Digital Signature Algorithm) signature.
   * @param r The first half (32 bytes) of the ECDSA signature.
   * @param s The second half (32 bytes) of the ECDSA signature.
   * @param v The recivery id (1 byte).
   * @returns The full ECDSA signature.
   */
  Signature makeSig(const uint256_t& r, const uint256_t& s, const uint8_t& v);

  /**
   * Check if an ECDSA signature is valid.
   * @param r The first half of the ECDSA signature.
   * @param s The second half of the ECDSA signature.
   * @param v The recovery ID.
   * @return `true` if the signature is valid, `false` otherwise.
   */
  bool verifySig(const uint256_t& r, const uint256_t& s, const uint8_t& v);

  /**
   * Derive an uncompressed public key from a private one.
   * @param key The private key to derive from.
   * @returns The derived uncompressed public key.
   */
  UPubKey toUPub(const PrivKey& key);

  /**
   * Derive an uncompressed public key from a compressed public one.
   * @param key The compressed public key to derive from.
   * @returns The derived uncompressed public key.
   */
  UPubKey toUPub(const PubKey& key);

  /**
   * Derive a compressed public key from a private one.
   * @param key The private key to derive from.
   * @return The derived compressed public key.
   */
  PubKey toPub(const PrivKey& key);

  /**
   * Derive an address from a given uncompressed public key.
   * @param key The uncompressed public key to derive from.
   * @return The derived address.
   */
  Address toAddress(const UPubKey& key);

  /**
   * Derive an address from a given compressed public key.
   * @param key The compressed public key to derive from.
   * @return The derived address.
   */
  Address toAddress(const PubKey& key);

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
  bool verify(const Hash& msg, const UPubKey& key, const Signature& sig);
};

#endif  // ECDSA_H
