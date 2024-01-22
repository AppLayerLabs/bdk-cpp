/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef SIMPLECONTRACT_H
#define SIMPLECONTRACT_H

#include "../dynamiccontract.h"
#include "../variables/safestring.h"
#include "../../utils/utils.h" // SafeUintX_t aliases declared here

/**
 * SimpleContract is a simple contract that stores a name and a value.
 * It is used to test the contract manager.
 */
class SimpleContract : public DynamicContract {
  private:
    SafeString name_; ///< The name of the contract.
    SafeUint256_t value_; ///< The value of the contract.
    void registerContractFunctions() override; ///< Register the contract functions.

  public:
    /// Event for when the name changes.
    void nameChanged(const EventParam<std::string, false>& name) { this->emitEvent(__func__,  std::make_tuple(name)); }

    /// Event for when the value changes.
    void valueChanged(const EventParam<uint256_t, true>& value) { this->emitEvent(__func__, std::make_tuple(value)); }

    /// Event for when the name and value change. used for testing json abi generation
    void nameAndValueChanged(const EventParam<std::string, true>& name, const EventParam<uint256_t, true>& value) {
      this->emitEvent(__func__, std::make_tuple(name, value));
    }

    /// Event for when the name and value change (as tuple), used for testing json abi generation
    void nameAndValueTupleChanged(const EventParam<std::tuple<std::string, uint256_t>, true>& nameAndValue) {
      this->emitEvent(__func__, std::make_tuple(nameAndValue));
    }

    using ConstructorArguments = std::tuple<const std::string&, const uint256_t&>; ///< The constructor arguments type.

    /**
     * Constructor from create. Create contract and save it to database.
     * @param name The name of the contract.
     * @param value The value of the contract.
     * @param interface The interface to the contract manager.
     * @param address The address of the contract.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain ID.
     * @param db The database to use.
     */
    SimpleContract(
      const std::string& name,
      const uint256_t& value,
      ContractManagerInterface &interface,
      const Address& address,
      const Address& creator,
      const uint64_t& chainId,
      const std::unique_ptr<DB> &db
    );

    /**
     * Constructor from load. Load contract from database.
     * @param interface The interface to the contract manager.
     * @param address The address of the contract.
     * @param db The database to use.
     */
    SimpleContract(
      ContractManagerInterface &interface,
      const Address& address,
      const std::unique_ptr<DB> &db
    );

    ~SimpleContract() override; ///< Destructor.
    ///< function setName(string memory argName) public
    void setName(const std::string& argName);
    ///< function setNames(string[] memory argName) public, the final name is the concatenation of all names
    void setNames(const std::vector<std::string>& argName);
    ///< function setValue(uint256 argValue) public
    void setValue(const uint256_t& argValue);
    ///< function setValues(uint256[] memory argValue) public, the final value is the sum of all values
    void setValues(const std::vector<uint256_t>& argValue);
    ///< function setNamesAndValues(string[] memory argName, uint256[] memory argValue) public,
    ///< the final name is the concatenation of all names, the final value is the sum of all values
    void setNamesAndValues(const std::vector<std::string>& argName, const std::vector<uint256_t>& argValue);
    ///< function setNamesAndValuesInTuple(NameAndValue[] memory argNameAndValue) public,
    ///< the final name is the concatenation of all names, the final value is the sum of all values
    void setNamesAndValuesInTuple(const std::vector<std::tuple<std::string, uint256_t>>& argNameAndValue);
    ///< function setNamesAndValuesInArrayOfArrays(NameAndValue[][] memory argNameAndValue) public.
    ///< the final name is the concatenation of all names, the final value is the sum of all values
    void setNamesAndValuesInArrayOfArrays(const std::vector<std::vector<std::tuple<std::string, uint256_t>>>& argNameAndValue);
    ///< function getName() public view returns(string memory)
    std::string getName() const;
    ///< function getNames(const uint256_t& i) public view returns(string[] memory) return string[] of size i with this->name_ as all elements.
    std::vector<std::string> getNames(const uint256_t& i) const;
    ///< function getValue() public view returns(uint256)
    uint256_t getValue() const;
    ///< Function getValue(uint256 i) public view returns(uint256) return this->value_ + i.
    /// For testing overloading functions...
    uint256_t getValue(const uint256_t& i) const;
    ///< function getValues(const uint256_t& i) public view returns(uint256[] memory) return uint256[] of size i with this->value_ as all elements.
    std::vector<uint256_t> getValues(const uint256_t& i) const;
    ///< function getNameAndValue() public view returns(string memory, uint256)
    std::tuple<std::string, uint256_t> getNameAndValue() const;
    ///< function getNamesAndValues(const uint256_t& i) public view returns(string[] memory, uint256[] memory)
    ///< return string[] of size i with this->name_ as all elements, return uint256[] of size i with this->value_ as all elements.
    std::tuple<std::vector<std::string>, std::vector<uint256_t>> getNamesAndValues(const uint256_t& i) const;
    ///< function getNamesAndValuesInTuple(const uint256_t& i) public view returns(NameAndValue[] memory)
    ///< return (string, uint256)[] of size i with this->name_ and this->value_ as all elements.
    std::vector<std::tuple<std::string, uint256_t>> getNamesAndValuesInTuple(const uint256_t& i) const;
    ///< function getNamesAndValuesInArrayOfArrays(const uint256_t& i) public view returns(NameAndValue[][] memory)
    ///< return (string, uint256)[][] of size i with this->name_ and this->value_ as all elements.
    std::vector<std::vector<std::tuple<std::string, uint256_t>>> getNamesAndValuesInArrayOfArrays(const uint256_t& i) const;

    /// Register the contract structure.
    static void registerContract() {
      ContractReflectionInterface::registerContractMethods<
        SimpleContract, const std::string&, uint256_t,
        ContractManagerInterface&,
        const Address&, const Address&, const uint64_t&,
        const std::unique_ptr<DB>&
      >(
        std::vector<std::string>{"name_", "value_"},
        std::make_tuple("setName", &SimpleContract::setName, FunctionTypes::NonPayable, std::vector<std::string>{"argName"}),
        std::make_tuple("setNames", &SimpleContract::setNames, FunctionTypes::NonPayable, std::vector<std::string>{"argName"}),
        std::make_tuple("setValue", &SimpleContract::setValue, FunctionTypes::NonPayable, std::vector<std::string>{"argValue"}),
        std::make_tuple("setValues", &SimpleContract::setValues, FunctionTypes::NonPayable, std::vector<std::string>{"argValue"}),
        std::make_tuple("setNamesAndValues", &SimpleContract::setNamesAndValues, FunctionTypes::NonPayable, std::vector<std::string>{"argName", "argValue"}),
        std::make_tuple("setNamesAndValuesInTuple", &SimpleContract::setNamesAndValuesInTuple, FunctionTypes::NonPayable, std::vector<std::string>{"argNameAndValue"}),
        std::make_tuple("setNamesAndValuesInArrayOfArrays", &SimpleContract::setNamesAndValuesInArrayOfArrays, FunctionTypes::NonPayable, std::vector<std::string>{"argNameAndValue"}),
        std::make_tuple("getName", &SimpleContract::getName, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("getNames", &SimpleContract::getNames, FunctionTypes::View, std::vector<std::string>{"i"}),
        std::make_tuple("getValue", static_cast<uint256_t(SimpleContract::*)() const>(&SimpleContract::getValue), FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("getValue", static_cast<uint256_t(SimpleContract::*)(const uint256_t&) const>(&SimpleContract::getValue), FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("getValues", &SimpleContract::getValues, FunctionTypes::View, std::vector<std::string>{"i"}),
        std::make_tuple("getNameAndValue", &SimpleContract::getNameAndValue, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("getNamesAndValues", &SimpleContract::getNamesAndValues, FunctionTypes::View, std::vector<std::string>{"i"}),
        std::make_tuple("getNamesAndValuesInTuple", &SimpleContract::getNamesAndValuesInTuple, FunctionTypes::View, std::vector<std::string>{"i"}),
        std::make_tuple("getNamesAndValuesInArrayOfArrays", &SimpleContract::getNamesAndValuesInArrayOfArrays, FunctionTypes::View, std::vector<std::string>{"i"})
      );
      ContractReflectionInterface::registerContractEvents<SimpleContract>(
        std::make_tuple("nameChanged", false, &SimpleContract::nameChanged, std::vector<std::string>{"name"}),
        std::make_tuple("valueChanged", false, &SimpleContract::valueChanged, std::vector<std::string>{"value"}),
        std::make_tuple("nameAndValueChanged", false, &SimpleContract::nameAndValueChanged, std::vector<std::string>{"name", "value"}),
        std::make_tuple("nameAndValueTupleChanged", false, &SimpleContract::nameAndValueTupleChanged, std::vector<std::string>{"nameAndValue"})
      );
    }
};

#endif // SIMPLECONTRACT_H
