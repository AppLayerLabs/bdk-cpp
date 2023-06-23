#ifndef ABI_H
#define ABI_H

#include <string>
#include <any>

#include "../utils/hex.h"
#include "../utils/json.hpp"
#include "../utils/utils.h"

/// Namespace for Solidity ABI-related operations.
namespace ABI {
  /**
 * Enum for the types of Solidity variables.
 * Equivalency is as follows:
 * - uint8 = uint8 (Solidity) = uint8_t (C++)
 * - uint8Arr = uint8[] (Solidity) = std::vector<uint8_t> (C++)
 * - uint16 = uint16 (Solidity) = uint16_t (C++)
 * - uint16Arr = uint16[] (Solidity) = std::vector<uint16_t> (C++)
 * - uint32 = uint32 (Solidity) = uint32_t (C++)
 * - uint32Arr = uint32[] (Solidity) = std::vector<uint32_t> (C++)
 * - uint64 = uint64 (Solidity) = uint64_t (C++)
 * - uint64Arr = uint64[] (Solidity) = std::vector<uint64_t> (C++)
 * - uint256 = uint256 (Solidity) = uint256_t (C++)
 * - uint256Arr = uint256[] (Solidity) = std::vector<uint256_t> (C++)
 * - address = address (Solidity) = Address (C++)
 * - addressArr = address[] (Solidity) = std::vector<Address> (C++)
 * - bool = bool (Solidity) = bool (C++)
 * - boolArr = bool[] (Solidity) = vector<bool> (C++)
 * - bytes = bytes (Solidity) = Bytes (C++)
 * - bytesArr = bytes[] (Solidity) = std::vector<Bytes> (C++)
 * - string = string (Solidity) = std::string (C++)
 * - stringArr = string[] (Solidity) = std::vector<std::string> (C++)
 */
enum Types {
  uint8,
  uint8Arr,
  uint16,
  uint16Arr,
  uint32,
  uint32Arr,
  uint64,
  uint64Arr,
  uint256,
  uint256Arr,
  address,
  addressArr,
  boolean,
  booleanArr,
  bytes,
  bytesArr,
  string,
  stringArr
};

/**
* Enum for the types of Solidity functions.
*@tparam T The type to map to an ABI type.
*/
template <typename T>
struct TypeToEnum;

/**
* Helper struct to map a type to an ABI type.
* Specializations for each type are defined below.
* @tparam T The type to map to an ABI type.
*/
template <typename T>
struct ABIType {
  static constexpr Types value = Types::uint256; ///< Default ABI type is uint256.
};

/**
* Specialization for std::vector<T>.
* This is used for all vector types, including bytesArr and stringArr.
* @tparam T The type to map to an ABI type.
*/
template <typename T>
struct ABIType<std::vector<T>> {
  static constexpr Types value = Types::uint256Arr; ///< ABI type is uint256Arr.
};

/**
* Specialization for address.
*/
template <>
struct ABIType<Address> {
  static constexpr Types value = Types::address; ///< ABI type is address.
};

/**
* Specialization for bool.
*/
template <>
struct ABIType<bool> {
  static constexpr Types value = Types::boolean; ///< ABI type is boolean.
};

/**
* Specialization for std::string.
*/
template <>
struct ABIType<std::string> {
  static constexpr Types value = Types::string; ///< ABI type is string.
};

/**
* Specialization for Bytes.
*/
template <>
struct ABIType<Bytes> {
  static constexpr Types value = Types::bytes; ///< ABI type is bytes.
};

/**
* Specialization for uint8_t.
*/
template <>
struct ABIType<uint8_t> {
  static constexpr Types value = Types::uint8; ///< ABI type is uint8.
};

/**
* Specialization for uint16_t.
*/
template <>
struct ABIType<uint16_t> {
  static constexpr Types value = Types::uint16; ///< ABI type is uint16.
};

/**
* Specialization for uint32_t.
*/
template <>
struct ABIType<uint32_t> {
  static constexpr Types value = Types::uint32; ///< ABI type is uint32.
};

/**
* Specialization for uint64_t.
*/
template <>
struct ABIType<uint64_t> {
  static constexpr Types value = Types::uint64; ///< ABI type is uint64.
};

/**
* Struct to map a type to an ABI type.
* @tparam T The type to map to an ABI type.
*/
template <typename T>
struct TypeToEnum {
  static constexpr Types value = ABIType<T>::value; ///< ABI type.
};

/**
* Specializations for reference types
* @tparam T The type to map to an ABI type.
*/
template <typename T>
struct TypeToEnum<T&> : TypeToEnum<T> {};

/**
* Specializations for const reference types
* @tparam T The type to map to an ABI type.
*/
template <typename T>
struct TypeToEnum<const T&> : TypeToEnum<T> {};

/**
* Specializations for vector reference types
* @tparam T The type to map to an ABI type.
*/
template <typename T>
struct TypeToEnum<std::vector<T>&> : TypeToEnum<std::vector<T>> {};

/**
* Specializations for const vector reference types
* @tparam T The type to map to an ABI type.
*/
template <typename T>
struct TypeToEnum<const std::vector<T>&> : TypeToEnum<std::vector<T>> {};

/**
 * This function returns the ABI type string for a given ABI type.
 * @param type The ABI type.
 * @return The ABI type string.
 */
std::string inline getStringFromABIEnum(Types type) {
  const std::unordered_map<Types, std::string> typeMappings = {
    {Types::uint8, "uint8"},
    {Types::uint8Arr, "uint8[]"},
    {Types::uint16, "uint16"},
    {Types::uint16Arr, "uint16[]"},
    {Types::uint32, "uint32"},
    {Types::uint32Arr, "uint32[]"},
    {Types::uint64, "uint64"},
    {Types::uint64Arr, "uint64[]"},
    {Types::uint256, "uint256"},
    {Types::uint256Arr, "uint256[]"},
    {Types::address, "address"},
    {Types::addressArr, "address[]"},
    {Types::boolean, "bool"},
    {Types::booleanArr, "bool[]"},
    {Types::bytes, "bytes"},
    {Types::bytesArr, "bytes[]"},
    {Types::string, "string"},
    {Types::stringArr, "string[]"}
  };

  auto it = typeMappings.find(type);
  if (it != typeMappings.end()) {
    return it->second;
  } else {
    throw std::runtime_error("Unsupported ABI type");
  }
}

/**
 * This function returns the ABI type for a given ABI type string.
 * @param type The ABI type string.
 * @return The ABI type.
 */
Types inline getABIEnumFromString(const std::string& type) {
  static const std::unordered_map<std::string, Types> typeMappings = {
    {"uint8", Types::uint8},
    {"uint8[]", Types::uint8Arr},
    {"uint16", Types::uint16},
    {"uint16[]", Types::uint16Arr},
    {"uint32", Types::uint32},
    {"uint32[]", Types::uint32Arr},
    {"uint64", Types::uint64},
    {"uint64[]", Types::uint64Arr},
    {"uint256", Types::uint256},
    {"uint256[]", Types::uint256Arr},
    {"address", Types::address},
    {"address[]", Types::addressArr},
    {"bool", Types::boolean},
    {"bool[]", Types::booleanArr},
    {"bytes", Types::bytes},
    {"bytes[]", Types::bytesArr},
    {"string", Types::string},
    {"string[]", Types::stringArr}
  };

  auto it = typeMappings.find(type);
  if (it != typeMappings.end()) {
    return it->second;
  } else {
    throw std::runtime_error("Invalid type");
  }
}

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
       *@throw std::runtime_error if the function header is invalid or if header
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
       * @param start The index of the vector to start decoding from.
       * @return The decoded 256-bit unsigned integer.
       *@throw std::runtime_error if data is too short.
       */
      uint256_t decodeUint256(const BytesArrView data, const uint64_t& start) const;

      /**
       * Decode a 20-byte address from the given Solidity data string.
       * Throws if data is too short.
       * @param data The Solidity data bytes to decode.
       * @param start The index of the vector to start decoding from.
       * @return The decoded 20-byte address.
       *@throw std::runtime_error if data is too short.
       */
      Address decodeAddress(const BytesArrView data, const uint64_t& start) const;

      /**
       * Decode a boolean from the given Solidity data string.
       * Throws if data is too short.
       * @param data The Solidity data bytes to decode.
       * @param start The index of the vector to start decoding from.
       * @return The decoded boolean.
       *@throw std::runtime_error if data is too short.
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
       * @throw std::runtime_error if data is too short.
       */
      std::string decodeString(const BytesArrView data, const uint64_t& start) const;

      /**
       * Decode a 256-bit unsigned integer array from the given Solidity data
       * string. Throws if data is too short.
       * @param data The Solidity data bytes to decode.
       * @param start The index of the vector to start decoding from.
       * @return The decoded 256-bit unsigned integer array.
       * @throw std::runtime_error if data is too short.
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
       * @throw std::runtime_error if data is too short.
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
       * @throw std::runtime_error if data is too short.
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
       * @throw std::runtime_error if data is too short.
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
        * @throw std::runtime_error if data is too short.
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
      Decoder(const std::vector<Types>& types, const BytesArrView bytes);

      /**
       * Get a specific data type from the decoded `data` list.
       * @param index The index of the data type to get.
       * @return The decoded data type.
       * @throw std::out_of_range if index is out of range.
       * @throw std::runtime_error if type mismatch.
       */
      template <typename T> T getData(const uint64_t &index) const {
        if (index >= this->data_.size()) throw std::out_of_range("Index out of range");
        if (std::holds_alternative<T>(this->data_[index])) return std::get<T>(this->data_[index]);
        throw std::runtime_error("Type mismatch");
      }

    /**
     * Get a specific data type from the decoded `data` list.
     * @param index The index of the data type to get.
     * @param type The expected Solidity type of the data.
     * @return The decoded data type.
     * @throw std::runtime_error if type mismatch.
     */
    std::any getDataDispatch(int index, Types type) {
      switch (type) {
        case Types::uint256:
        case Types::uint8:
        case Types::uint16:
        case Types::uint32:
        case Types::uint64:
        return this->getData<uint256_t>(index);
        case Types::uint8Arr:
        case Types::uint16Arr:
        case Types::uint32Arr:
        case Types::uint64Arr:
        case Types::uint256Arr:
        return this->getData<std::vector<uint256_t>>(index);
        case Types::address: return this->getData<Address>(index);
        case Types::addressArr: return this->getData<std::vector<Address>>(index);
        case Types::boolean: return this->getData<bool>(index);
        case Types::booleanArr: return this->getData<std::vector<bool>>(index);
        case Types::bytes: return this->getData<Bytes>(index);
        case Types::bytesArr: return this->getData<std::vector<Bytes>>(index);
        case Types::string: return this->getData<std::string>(index);
        case Types::stringArr: return this->getData<std::vector<std::string>>(index);
        default: throw std::runtime_error("Invalid ABI::Types type");
      }
    }

      /**
       * Get the size of the `data` list.
       * @return The total number of decoded types.
       */
      size_t getDataSize() const { return this->data_.size(); }
  };
}; // namespace ABI

#endif // ABI_H
