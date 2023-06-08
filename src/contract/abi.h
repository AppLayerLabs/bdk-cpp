#ifndef ABI_H
#define ABI_H

#include <string>

#include "../utils/hex.h"
#include "../utils/json.hpp"
#include "../utils/utils.h"

/// Namespace for Solidity ABI-related operations.
namespace ABI {
  /**
   * Enum for the types of Solidity variables.
   * Equivalency is as follows:
   * - uint256 = uint256 (Solidity) = uint256_t (C++)
   * - uint256Arr = uint256[] (Solidity) = std::vector<uint256_t> (C++)
   * - address = address (Solidity) = Address (C++)
   * - addressArr = address[] (Solidity) = std::vector<Address> (C++)
   * - bool = bool (Solidity) = bool (C++)
   * - boolArr = bool[] (Solidity) = vector<bool> (C++)
   * - bytes = bytes (Solidity) = Bytes (C++)
   * - bytesArr = bytes[] (Solidity) = std::vector<Bytes> (C++)
   * - string = string (Solidity) = Bytes (C++)
   * - stringArr = string[] (Solidity) = std::vector<std::string> (C++)
   */
  enum Types {
    uint256, uint256Arr, address, addressArr, boolean, booleanArr,
    bytes, bytesArr, string, stringArr
  };

  /// Class that encodes and packs native data types into Solidity ABI strings.
  class Encoder {
    private:
      Bytes data_; ///< Encoded Solidity ABI string, as RAW BYTES. Use Hex::fromBytes().get() to print it properly.
      Functor functor; ///< Functor of the function to call. (if any)

      /**
       * Encode a function header into Solidity ABI format.
       * Requires the full function header, no spaces between args
       * (e.g. `func(arg1,arg2)`). The function will SHA3-hash the header
       * and return the first 4 hex bytes of the hash (aka the "functor").
       * @param func The function header to encode.
       * @return The encoded functor hex string.
       */
      Functor encodeFunction(const std::string_view func) const;

      /**
       * Encode a 256-bit unsigned integer into Solidity ABI format.
       * @param num The 256-bit unsigned integer to encode.
       * @return The encoded uint256 hex string, padded 32 hex bytes to the LEFT.
       */
      Bytes encodeUint256(const uint256_t& num) const;

      /**
       * Encode a 20-byte address into Solidity ABI format.
       * @param add The 20-byte address to encode.
       * @return The encoded address hex string, padded 32 bytes to the LEFT.
       */
      Bytes encodeAddress(const Address& add) const;

      /**
       * Encode a boolean into Solidity ABI format.
       * @param b The boolean to encode.
       * @return The encoded boolean hex string, padded 32 bytes to the LEFT.
       */
      Bytes encodeBool(bool b) const;

      /**
       * Encode a raw bytes or UTF-8 string into Solidity ABI format.
       * Solidity bytes and string types are parsed the exact same way,
       * so we can use the same function for encoding both types.
       * @param bytes The raw bytes or UTF-8 string to encode.
       * @return The encoded hex bytes or string,
       *         padded to the nearest multiple of 32 bytes to the RIGHT.
       */
      Bytes encodeBytes(const BytesArrView bytes) const;

      /**
       * Encode a 256-bit unsigned integer array into Solidity ABI format.
       * @param numV The 256-bit unsigned integer array to encode.
       * @return The encoded uint256[] hex string, with the proper offsets and lengths.
       */
      Bytes encodeUint256Arr(const std::vector<uint256_t>& numV) const;

      /**
       * Encode a 20-byte address array into Solidity ABI format.
       * @param addV The 20-byte address array to encode.
       * @return The encoded address[] hex string, with the proper offsets and lengths.
       */
      Bytes encodeAddressArr(const std::vector<Address>& addV) const;

      /**
       * Encode a boolean array into Solidity ABI format.
       * @param bV The boolean array to encode.
       * @return The encoded bool[] hex string, with the proper offsets and lengths.
       */
      Bytes encodeBoolArr(const std::vector<bool>& bV) const;

      /**
       * Encode a raw bytes or UTF-8 string array into Solidity ABI format.
       * See `encodeBytes()` for more details.
       * @param bytesV The raw bytes or UTF-8 string array to encode.
       * @return The encoded bytes[] or string[] hex string, with the proper offsets and lengths.
       */
      Bytes encodeBytesArr(const std::vector<BytesArrView>& bytesV) const;

    public:
      /// Alias for variant type, for easier handling.
      typedef std::vector<std::variant<
        uint256_t, std::vector<uint256_t>, Address, std::vector<Address>,
        bool, std::vector<bool>, Bytes, std::vector<Bytes>, std::string, std::vector<std::string>
      >> EncVar;

      /**
       * Constructor.
       * Automatically encodes the data during construction.
       * Throws on error.
       * @param data A list of variables to encode.
       * @param func (optional) The full function header to encode.
       *             Defaults to an empty string.
       *@throws std::runtime_error if the function header is invalid or if header
       *and data do not match.
       * TODO: change std::string_view func to be only the function name, and derive the respective argument types from data.
       */
      Encoder(const ABI::Encoder::EncVar& data, const std::string_view func = "");

      /// Getter for `data`.
      const Bytes& getData() const { return this->data_; }

      /// Getter for 'functor'
      const Functor& getFunctor() const { return this->functor; }

      /**
       * Get the length of `data`.
       * @return The total size of the data string.
       */
      size_t size() const { return this->data_.size(); }
  };

  /// Class that unpacks and decodes a Solidity ABI string into their original data types.
  class Decoder {
    private:
      /// List with the decoded native data types.
      std::vector<std::variant<
        uint256_t, std::vector<uint256_t>, Address, std::vector<Address>,
        bool, std::vector<bool>, Bytes, std::vector<Bytes>, std::string, std::vector<std::string>
      >> data_;

      /**
       * Decode a 256-bit unsigned integer from the given Solidity data string.
       * Throws if data is too short.
       * @param data The Solidity data bytes to decode.
       * @start The index of the vector to start decoding from.
       * @return The decoded 256-bit unsigned integer.
       *@throws std::runtime_error if data is too short.
       */
      uint256_t decodeUint256(const BytesArrView data, const uint64_t& start) const;

      /**
       * Decode a 20-byte address from the given Solidity data string.
       * Throws if data is too short.
       * @param data The Solidity data bytes to decode.
       * @start The index of the vector to start decoding from.
       * @return The decoded 20-byte address.
       *@throws std::runtime_error if data is too short.
       */
      Address decodeAddress(const BytesArrView data, const uint64_t& start) const;

      /**
       * Decode a boolean from the given Solidity data string.
       * Throws if data is too short.
       * @param data The Solidity data bytes to decode.
       * @start The index of the vector to start decoding from.
       * @return The decoded boolean.
       *@throws std::runtime_error if data is too short.
       */
      bool decodeBool(const BytesArrView data, const uint64_t& start) const;

      /**
       * Decode a raw bytes from the given Solidity data string.
       * Decoding bytes and string in Solidity is done the exact same way,
       * as we are dealing with data as raw bytes anyway.
       * We differentiate the return types for convenience.
       * Throws if data is too short.
       * @param data The Solidity data bytes to decode.
       * @param start The index of the vector to start decoding from.
       * @return The decoded raw bytes
       * @throws std::runtime_error if data is too short.
       */
      Bytes decodeBytes(const BytesArrView data, const uint64_t& start) const;

      /**
       * Decode a raw UTF-8 string from the given Solidity data string.
       * Decoding bytes and string in Solidity is done the exact same way,
       * as we are dealing with data as raw bytes anyway.
       * We differentiate the return types for convenience.
       * Throws if data is too short.
       * @param data The Solidity data bytes to decode.
       * @param start The index of the vector to start decoding from.
       * @return The decoded string
       * @throws std::runtime_error if data is too short.
       */
      std::string decodeString(const BytesArrView data, const uint64_t& start) const;

      /**
       * Decode a 256-bit unsigned integer array from the given Solidity data
       * string. Throws if data is too short.
       * @param data The Solidity data bytes to decode.
       * @param start The index of the vector to start decoding from.
       * @return The decoded 256-bit unsigned integer array.
       * @throws std::runtime_error if data is too short.
       */
      std::vector<uint256_t> decodeUint256Arr(
        const BytesArrView data, const uint64_t& start
      ) const;

      /**
       * Decode a 20-byte address array from the given Solidity data string.
       * Throws if data is too short.
       * @param data The Solidity data bytes to decode.
       * @param start The index of the vector to start decoding from.
       * @return The decoded 20-byte address array.
       * @throws std::runtime_error if data is too short.
       */
      std::vector<Address> decodeAddressArr(
        const BytesArrView data, const uint64_t& start
      ) const;

      /**
       * Decode a boolean array from the given Solidity data string.
       * Throws if data is too short.
       * @param data The Solidity data bytes to decode.
       * @param start The index of the vector to start decoding from.
       * @return The decoded boolean array.
       * @throws std::runtime_error if data is too short.
       */
      std::vector<bool> decodeBoolArr(
        const BytesArrView data, const uint64_t& start
      ) const;

      /**
       * Decode a raw bytes array from the given Solidity data
       * string. See `decodeBytes()` for more details. Throws if data is too short.
       * @param data The Solidity data bytes to decode.
       * @param start The index of the vector to start decoding from.
       * @return The decoded raw bytes.
       * @throws std::runtime_error if data is too short.
       */
      std::vector<Bytes> decodeBytesArr(
        const BytesArrView data, const uint64_t& start
      ) const;

      /**
        * Decode a raw bytes array from the given Solidity data
        * string. See `decodeBytes()` for more details. Throws if data is too short.
        * @param data The Solidity data bytes to decode.
        * @param start The index of the vector to start decoding from.
        * @return The decoded raw bytes.
        * @throws std::runtime_error if data is too short.
        */
      std::vector<std::string> decodeStringArr(
        const BytesArrView data, const uint64_t& start
      ) const;

  public:
      /**
       * Constructor. Automatically decodes the data during construction.
       * @param types An ordered list of expected Solidity types to decode.
       * @param bytes The full Solidity ABI string to decode, AS A RAW BYTES STRING.
       */
      Decoder(const std::vector<ABI::Types>& types, const BytesArrView bytes);

      /**
       * Get a specific data type from the decoded `data` list.
       * @param index The index of the data type to get.
       * @return The decoded data type.
       * @throws std::out_of_range if index is out of range.
       * @throws std::runtime_error if type mismatch.
       */
      template <typename T> T getData(const uint64_t &index) const {
        if (index >= this->data_.size()) throw std::out_of_range("Index out of range");
        if (std::holds_alternative<T>(this->data_[index])) return std::get<T>(this->data_[index]);
        throw std::runtime_error("Type mismatch");
      }

      /**
       * Get the size of the `data` list.
       * @return The total number of decoded types.
       */
      size_t getDataSize() const { return this->data_.size(); }
  };
}; // namespace ABI

#endif // ABI_H
