#ifndef ABI_H
#define ABI_H

#include <string>
#include "solidity.h"
#include "../utils/utils.h"

#include "../libs/json.hpp"

namespace ABI {
  enum Types {
    uint256, uint256Arr,
    address, addressArr,
    boolean, booleanArr,
    bytes, bytesArr,
    string, stringArr
  };

  class Encoder {
    private:
      std::string data;

      std::string encodeFunction(std::string func);
      std::string encodeUint256(uint256_t num);
      std::string encodeAddress(Address add);
      std::string encodeBool(bool b);
      std::string encodeBytes(std::string bytes);
      std::string encodeUint256Arr(std::vector<uint256_t> numV);
      std::string encodeAddressArr(std::vector<Address> addV);
      std::string encodeBoolArr(std::vector<bool> bV);
      std::string encodeBytesArr(std::vector<std::string> bytesV);

    public:
      Encoder(std::vector<std::variant<
        uint256_t, std::vector<uint256_t>, Address, std::vector<Address>,
        bool, std::vector<bool>, std::string, std::vector<std::string>
      >> data, std::string func = "");  // Both bytes and string are stored as std::string

      const std::string& get() { return this->data; }

      size_t size() { return this->data.length(); }
  };

  class Decoder {
    private:
      std::vector<std::variant<
        uint256_t, std::vector<uint256_t>, Address, std::vector<Address>,
        bool, std::vector<bool>, std::string, std::vector<std::string>
      >> data;  // Both bytes and string are stored as std::string

      // Helper functions to parse each type
      uint256_t decodeUint256(const std::string &data, const uint64_t &start);
      Address decodeAddress(const std::string &data, const uint64_t &start);
      bool decodeBool(const std::string &data, const uint64_t &start);
      // As we are dealing with data as raw bytes, interpreting a solidity byte/string is the same, so we can use the same function for both.
      // A solidity string would return a UTF-8 encoded string, but a solidity bytes would return a byte string.
      std::string decodeBytes(const std::string &data, const uint64_t &start);
      std::vector<uint256_t> decodeUint256Arr(const std::string &data, const uint64_t &start);
      std::vector<Address> decodeAddressArr(const std::string &data, const uint64_t &start);
      std::vector<bool> decodeBoolArr(const std::string &data, const uint64_t &start);
      std::vector<std::string> decodeBytesArr(const std::string &data, const uint64_t &start);

    public:
      Decoder(std::vector<ABI::Types> const &types, std::string const &abiData); // Data as *bytes*

      template <typename T> T get(uint64_t const &index) {
        if (index >= data.size()) {
          throw std::out_of_range("Index out of range");
        }
        if (std::holds_alternative<T>(data[index])) {
          return std::get<T>(data[index]);
        }
        throw std::runtime_error("Type mismatch");
      }

      size_t size() { return data.size(); }
  };

  class JSONEncoder {
    private:
      /**
       * Check if a given type is an array.
       * @param &type Type value that will be checked.
       * @return `true` if type is array, `false` otherwise.
       */
      bool isTypeArray(Types const &type);

    public:
      /**
       * Constructor.
       * @param jsonInterface The JSONEncoder ABI as a JSON object.
       * @param address The JSONEncoder's address.
       * @param options (optional) The transaction options to override for the
       *                JSONEncoder as a JSON object. Defaults to NULL.
       */
      JSONEncoder(const json& jsonInterface);

      /**
       * List of methods from the JSONEncoder, as key and value pairs.
       * Key is the method name, value is a vector with each of the method's parameter types.
       */
      std::map<std::string,std::vector<Types>> methods;

      /**
       * List of functors from the JSONEncoder.
       * Key is the method name, value is the method hash (first 8 bytes of the ABI hash).
       */
      std::map<std::string,std::string> functors;
      /**
       * ABI constructor. Uses the custom Solidity class.
       * **Pay attention to the arg order! "args, func" = this one.**
       * @param arguments The function's arguments as a JSON object,
       *                      each with "type" and "value" (or "t" and "v").
       *                      e.g. `{{"t", "string"}, {"v", "Hello!%"}}`
       * @param function The function's name.
       * @param &error Error object.
       * @return The %Solidity encoded function ABI.
       */
      std::string operator() (const std::string &function ,const json &arguments);

  };
};

#endif  // ABI_H
