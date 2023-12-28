// SPDX-License-Identifier: MIT
pragma solidity ^0.8;

contract SimpleContract {
    string private name_;
    uint256 private value_;

    /// Struct because solidity can't have tuple in the argument itself.
    struct NameAndValue {
        string name;
        uint256 value;
    }

    struct NameAndValueArr {
        string[] name;
        uint256[] value;
    }

    /// void setName(const std::string& argName);
    function setName(string memory argName) public {
        name_ = argName;
    }

    /// void setNames(const std::vector<std::string>& argName);
    function setNames(string[] memory argName) public {
        name_ = "";
        for (uint256 i = 0; i < argName.length; i++) {
            name_ = string(abi.encodePacked(name_, argName[i]));
        }
    }

    /// void setValue(const uint256_t& argValue);
    function setValue(uint256 argValue) public {
        value_ = argValue;
    }

    /// void setValues(const std::vector<uint256_t>& argValue);
    function setValues(uint256[] memory argValue) public {
        value_ = 0;
        for (uint256 i = 0; i < argValue.length; i++) {
            value_ += argValue[i];
        }
    }

    /// void setNamesAndValues(const std::vector<std::string>& argName, const std::vector<uint256_t>& argValue);
    function setNamesAndValues(string[] memory argName, uint256[] memory argValue) public {
        name_ = "";
        value_ = 0;
        for (uint256 i = 0; i < argName.length; i++) {
            name_ = string(abi.encodePacked(name_, argName[i]));
        }
        for (uint256 i = 0; i < argValue.length; i++) {
            value_ += argValue[i];
        }
    }

    /// void setNamesAndValuesInTuple(const std::vector<std::tuple<std::string, uint256_t>>& argNameAndValue);
    function setNamesAndValuesInTuple(NameAndValue[] memory argNameAndValue) public {
        name_ = "";
        value_ = 0;
        for (uint256 i = 0; i < argNameAndValue.length; i++) {
            name_ = string(abi.encodePacked(name_, argNameAndValue[i].name));
            value_ += argNameAndValue[i].value;
        }
    }

    /// std::string getName() const;
    function getName() public view returns(string memory) {
        return name_;
    }

    /// std::vector<std::string> getNames(const uint256_t& i) const;
    function getNames(uint256 i) public view returns(string[] memory) {
        string[] memory names = new string[](i);
        for (uint256 j = 0; j < i; j++) {
            names[j] = name_;
        }
        return names;
    }

    /// uint256_t getValue() const;
    function getValue() public view returns(uint256) {
        return value_;
    }

    /// std::vector<uint256_t> getValues(const uint256_t& i) const;
    function getValues(uint256 i) public view returns(uint256[] memory) {
        uint256[] memory values = new uint256[](i);
        for (uint256 j = 0; j < i; j++) {
            values[j] = value_;
        }
        return values;
    }

    /// std::tuple<std::string, uint256_t> getNameAndValue() const;
    function getNameAndValue() public view returns(NameAndValue memory) {
        NameAndValue memory nameAndValue;
        nameAndValue.name = name_;
        nameAndValue.value = value_;
        return nameAndValue;
    }

    /// std::tuple<std::vector<std::string>, std::vector<uint256_t>> getNamesAndValues(const uint256_t& i) const;
    function getNamesAndValues(uint256 i) public view returns(NameAndValueArr memory) {
        NameAndValueArr memory nameAndValues;
        nameAndValues.name = new string[](i);
        nameAndValues.value = new uint256[](i);
        for (uint256 j = 0; j < i; j++) {
            nameAndValues.name[j] = name_;
            nameAndValues.value[j] = value_;
        }
        return nameAndValues;
    }

    /// std::vector<std::tuple<std::string, uint256_t>> getNamesAndValuesInTuple(const uint256_t& i) const;
    function getNamesAndValuesInTuple(uint256 i) public view returns(NameAndValue[] memory) {
        NameAndValue[] memory nameAndValues = new NameAndValue[](i);
        for (uint256 j = 0; j < i; j++) {
            nameAndValues[j].name = name_;
            nameAndValues[j].value = value_;
        }
        return nameAndValues;
    }
}
