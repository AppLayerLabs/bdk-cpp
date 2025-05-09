/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef SIMPLECONTRACT_H
#define SIMPLECONTRACT_H

#include "../dynamiccontract.h"
#include "../variables/safestring.h"
#include "../variables/safetuple.h"
#include "../variables/safeuint.h"

/**
 * SimpleContract is a simple contract that stores a name, number and tuple.
 * It is used to test the Contract Manager.
 */
class SimpleContract : public DynamicContract {
  private:
    SafeString name_; ///< The name of the contract.
    SafeUint256_t number_; ///< The number of the contract.
    SafeTuple<std::string, uint256_t> tuple_;  ///< Name and number as a tuple.
    void registerContractFunctions() override; ///< Register the contract functions.

  public:
    /// Event for when the name changes.
    void nameChanged(const EventParam<std::string, true>& name) {
      this->emitEvent(__func__, std::make_tuple(name));
    }

    /// Event for when the number changes.
    void numberChanged(const EventParam<uint256_t, false>& number) {
      this->emitEvent(__func__, std::make_tuple(number));
    }

    /// Event for when the name and number tuple changes.
    void tupleChanged(const EventParam<std::tuple<std::string, uint256_t>, true>& tuple) {
      this->emitEvent(__func__, std::make_tuple(tuple));
    }

    /// Event for when the name and number change. Used for testing JSON ABI generation.
    void nameAndNumberChanged(const EventParam<std::string, true>& name, const EventParam<uint256_t, true>& number) {
      this->emitEvent(__func__, std::make_tuple(name, number));
    }

    /// Event for when the name and number change (as tuple). Used for testing JSON ABI generation
    void nameAndNumberTupleChanged(const EventParam<std::tuple<std::string, uint256_t>, true>& nameAndNumber) {
      this->emitEvent(__func__, std::make_tuple(nameAndNumber));
    }

    /// The constructor argument types.
    using ConstructorArguments = std::tuple<
      const std::string&, const uint256_t&, const std::tuple<std::string, uint256_t>&
    >;

    /**
     * Constructor from create. Create contract and save it to database.
     * @param name The name of the contract.
     * @param number The number of the contract.
     * @param tuple The name and number tuple of the contract.
     * @param address The address of the contract.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain ID.
     */
    SimpleContract(
      const std::string& name,
      const uint256_t& number,
      const std::tuple<std::string, uint256_t>& tuple,
      const Address& address,
      const Address& creator,
      const uint64_t& chainId
    );

    /**
     * Constructor from load. Load contract from database.
     * @param address The address of the contract.
     * @param db The database to use.
     */
    SimpleContract(
      const Address& address,
      const DB& db
    );

    ~SimpleContract() override; ///< Destructor.

    /// function setName(string memory argName) public
    void setName(const std::string& argName);

    /// function setNames(string[] memory argName) public, the final name is the concatenation of all names
    void setNames(const std::vector<std::string>& argName);

    /// function setNumber(uint256 argNumber) public
    void setNumber(const uint256_t& argNumber);

    /// function setNumbers(uint256[] memory argNumber) public, the final value is the sum of all values
    void setNumbers(const std::vector<uint256_t>& argNumber);

    /// function setNamesAndNumbers(string[] memory argName, uint256[] memory argNumber) public,
    /// the final name is the concatenation of all names, the final value is the sum of all values
    void setNamesAndNumbers(const std::vector<std::string>& argName, const std::vector<uint256_t>& argNumber);

    /// function setNamesAndNumbersInTuple(NameAndNumber[] memory argNameAndNumber) public,
    /// the final name is the concatenation of all names, the final value is the sum of all values
    void setNamesAndNumbersInTuple(const std::vector<std::tuple<std::string, uint256_t>>& argNameAndNumber);

    /// function setNamesAndNumbersInArrayOfArrays(NameAndNumber[][] memory argNameAndNumber) public.
    /// the final name is the concatenation of all names, the final value is the sum of all values
    void setNamesAndNumbersInArrayOfArrays(const std::vector<std::vector<std::tuple<std::string, uint256_t>>>& argNameAndNumber);

    /// equivalent to function setTuple(string name, uint256 number) public
    void setTuple(const std::tuple<std::string, uint256_t>& argTuple);

    /// function getName() public view returns(string memory)
    std::string getName() const;

    /// function getNameNonView() public returns(string memory)
    std::string getNameNonView();

    /// function getNames(const uint256_t& i) public view returns(string[] memory) return string[] of size i with this->name_ as all elements.
    std::vector<std::string> getNames(const uint256_t& i) const;

    /// function getNumber() public view returns(uint256)
    uint256_t getNumber() const;

    // For testing overloading functions...
    /// Function getNumber(uint256 i) public view returns(uint256) return this->number_ + i.
    uint256_t getNumber(const uint256_t& i) const;

    /// function getNumbers(const uint256_t& i) public view returns(uint256[] memory) return uint256[] of size i with this->number_ as all elements.
    std::vector<uint256_t> getNumbers(const uint256_t& i) const;

    /// function getNameAndNumber() public view returns(string memory, uint256)
    std::tuple<std::string, uint256_t> getNameAndNumber() const;

    /// function getNamesAndNumbers(const uint256_t& i) public view returns(string[] memory, uint256[] memory)
    /// return string[] of size i with this->name_ as all elements, return uint256[] of size i with this->number_ as all elements.
    std::tuple<std::vector<std::string>, std::vector<uint256_t>> getNamesAndNumbers(const uint256_t& i) const;

    /// function getNamesAndNumbersInTuple(const uint256_t& i) public view returns(NameAndNumber[] memory)
    /// return (string, uint256)[] of size i with this->name_ and this->number_ as all elements.
    std::vector<std::tuple<std::string, uint256_t>> getNamesAndNumbersInTuple(const uint256_t& i) const;

    /// function getNamesAndNumbersInArrayOfArrays(const uint256_t& i) public view returns(NameAndNumber[][] memory)
    /// return (string, uint256)[][] of size i with this->name_ and this->number_ as all elements.
    std::vector<std::vector<std::tuple<std::string, uint256_t>>> getNamesAndNumbersInArrayOfArrays(const uint256_t& i) const;

    /// equivalent to function getTuple() public view returns(string memory, uint256)
    std::tuple<std::string, uint256_t> getTuple() const;

    /// Register the contract structure.
    static void registerContract() {
      static std::once_flag once;
      std::call_once(once, []() {
        DynamicContract::registerContractMethods<SimpleContract>(
          std::vector<std::string>{"name_", "number_", "tuple_"},
          std::make_tuple("setName", &SimpleContract::setName, FunctionTypes::NonPayable, std::vector<std::string>{"argName"}),
          std::make_tuple("setNames", &SimpleContract::setNames, FunctionTypes::NonPayable, std::vector<std::string>{"argName"}),
          std::make_tuple("setNumber", &SimpleContract::setNumber, FunctionTypes::NonPayable, std::vector<std::string>{"argNumber"}),
          std::make_tuple("setNumbers", &SimpleContract::setNumbers, FunctionTypes::NonPayable, std::vector<std::string>{"argNumber"}),
          std::make_tuple("setNamesAndNumbers", &SimpleContract::setNamesAndNumbers, FunctionTypes::NonPayable, std::vector<std::string>{"argName", "argNumber"}),
          std::make_tuple("setNamesAndNumbersInTuple", &SimpleContract::setNamesAndNumbersInTuple, FunctionTypes::NonPayable, std::vector<std::string>{"argNameAndNumber"}),
          std::make_tuple("setNamesAndNumbersInArrayOfArrays", &SimpleContract::setNamesAndNumbersInArrayOfArrays, FunctionTypes::NonPayable, std::vector<std::string>{"argNameAndNumber"}),
          std::make_tuple("setTuple", &SimpleContract::setTuple, FunctionTypes::NonPayable, std::vector<std::string>{"argTuple"}),
          std::make_tuple("getName", &SimpleContract::getName, FunctionTypes::View, std::vector<std::string>{}),
          std::make_tuple("getNameNonView", &SimpleContract::getNameNonView, FunctionTypes::NonPayable, std::vector<std::string>{}),
          std::make_tuple("getNames", &SimpleContract::getNames, FunctionTypes::View, std::vector<std::string>{"i"}),
          std::make_tuple("getNumber", static_cast<uint256_t(SimpleContract::*)() const>(&SimpleContract::getNumber), FunctionTypes::View, std::vector<std::string>{}),
          std::make_tuple("getNumber", static_cast<uint256_t(SimpleContract::*)(const uint256_t&) const>(&SimpleContract::getNumber), FunctionTypes::View, std::vector<std::string>{}),
          std::make_tuple("getNumbers", &SimpleContract::getNumbers, FunctionTypes::View, std::vector<std::string>{"i"}),
          std::make_tuple("getNameAndNumber", &SimpleContract::getNameAndNumber, FunctionTypes::View, std::vector<std::string>{}),
          std::make_tuple("getNamesAndNumbers", &SimpleContract::getNamesAndNumbers, FunctionTypes::View, std::vector<std::string>{"i"}),
          std::make_tuple("getNamesAndNumbersInTuple", &SimpleContract::getNamesAndNumbersInTuple, FunctionTypes::View, std::vector<std::string>{"i"}),
          std::make_tuple("getNamesAndNumbersInArrayOfArrays", &SimpleContract::getNamesAndNumbersInArrayOfArrays, FunctionTypes::View, std::vector<std::string>{"i"}),
          std::make_tuple("getTuple", &SimpleContract::getTuple, FunctionTypes::View, std::vector<std::string>{})
        );
        ContractReflectionInterface::registerContractEvents<SimpleContract>(
          std::make_tuple("nameChanged", false, &SimpleContract::nameChanged, std::vector<std::string>{"name"}),
          std::make_tuple("numberChanged", false, &SimpleContract::numberChanged, std::vector<std::string>{"number"}),
          std::make_tuple("tupleChanged", false, &SimpleContract::tupleChanged, std::vector<std::string>{"tuple"}),
          std::make_tuple("nameAndNumberChanged", false, &SimpleContract::nameAndNumberChanged, std::vector<std::string>{"name", "number"}),
          std::make_tuple("nameAndNumberTupleChanged", false, &SimpleContract::nameAndNumberTupleChanged, std::vector<std::string>{"nameAndNumber"})
        );
      });
    }

    /// Dump method
    DBBatch dump() const override;
};

#endif // SIMPLECONTRACT_H
