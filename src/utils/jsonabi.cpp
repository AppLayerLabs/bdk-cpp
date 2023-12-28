/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "jsonabi.h"

bool JsonAbi::isTuple(const std::string &type) {
  /// Tuples always have the same format: "(type1,type2,...)"
  /// They can also be "(type1,type2,...)[]"
  if (JsonAbi::isArray(type)) {
    /// Check if the first and last characters, minus the [] at the end.
    return (type[0] == '(' && type[type.size() - 3] == ')');
  } else {
    /// Check if the first and last characters are "()"
    return (type[0] == '(' && type[type.size() - 1] == ')');
  }
}

bool JsonAbi::isArray(const std::string& type) {
  /// Arrays always have the same format: "type[]"
  /// Check if the last two characters are "[]"
  return (type[type.size() - 2] == '[' && type[type.size() - 1] == ']');
}

std::vector<std::string> JsonAbi::getTupleTypes(const std::string &type) {
  /// Tuples always have the same format: "(type1,type2,...)"
  /// Don't forget to remove the "(...)" and the "[]" if it's an array.
  /// the vector will return "type1", "type2", ...
  std::vector<std::string> types;
  /// Create the string skipping the first character (the "(")
  std::string tupleString = type.substr(1);
  /// If array, delete the last two characters (the "[]")
  if (JsonAbi::isArray(type)) {
    tupleString = tupleString.substr(0, tupleString.size() - 2);
  }
  /// Remove the last character (the ")")
  tupleString = tupleString.substr(0, tupleString.size() - 1);
  /// Split the string by commas.
  boost::split(types, tupleString, [](char c) { return c == ','; });
  return types;
}

json JsonAbi::handleTupleComponents(const std::vector<std::string> &tupleTypes) {
  json jsonObject = json::array();
  for (const auto &type : tupleTypes) {
    json componentObject = json::object();
    if (JsonAbi::isTuple(type)) {
      /// Handle the tuple type.
      componentObject["components"] = JsonAbi::handleTupleComponents(JsonAbi::getTupleTypes(type));
      if (JsonAbi::isArray(type)) {
        componentObject["type"] = "tuple[]";
      } else {
        componentObject["type"] = "tuple";
      }
    } else {
      /// Handle the non-tuple type.
      componentObject["internalType"] = type;
      componentObject["type"] = type;
    }
    jsonObject.push_back(componentObject);
  }
  return jsonObject;
}

json JsonAbi::parseMethodInput(const std::vector<std::pair<std::string,std::string>> &inputDescription) {
  json jsonObject = json::array();
  for (const auto &input : inputDescription) {
    const auto& [type, name] = input;
    json inputObject = json::object();
    if (JsonAbi::isTuple(type)) {
      /// Handle the tuple type.
      inputObject["components"] = JsonAbi::handleTupleComponents(JsonAbi::getTupleTypes(type));
      inputObject["name"] = name;
      if (JsonAbi::isArray(type)) {
        inputObject["type"] = "tuple[]";
      } else {
        inputObject["type"] = "tuple";
      }
    } else {
      /// Handle the non-tuple type.
      inputObject["internalType"] = type;
      inputObject["name"] = name;
      inputObject["type"] = type;
    }

    jsonObject.push_back(inputObject);
  }
  return jsonObject;
}

json JsonAbi::parseMethodOutput(const std::vector<std::string> &outputDescription) {
  json jsonObject = json::array();
  if (outputDescription.size() == 1 && outputDescription[0] == "") {
    return jsonObject;
  }
  for (const auto &output : outputDescription) {
    json outputObject = json::object();
    if (JsonAbi::isTuple(output)) {
      /// Handle the tuple type.
      outputObject["components"] = JsonAbi::handleTupleComponents(JsonAbi::getTupleTypes(output));
      if (JsonAbi::isArray(output)) {
        outputObject["type"] = "tuple[]";
      } else {
        outputObject["type"] = "tuple";
      }
    } else {
      /// Handle the non-tuple type.
      outputObject["internalType"] = output;
      outputObject["name"] = "";
      outputObject["type"] = output;
    }
    jsonObject.push_back(outputObject);
  }
  return jsonObject;
}


json JsonAbi::methodToJSON(const ABI::MethodDescription &description) {
  json jsonObject = json::object();
  jsonObject["inputs"] = JsonAbi::parseMethodInput(description.inputs);
  jsonObject["name"] = description.name;
  jsonObject["outputs"] = JsonAbi::parseMethodOutput(description.outputs);
  jsonObject["stateMutability"] = description.stateMutability;
  jsonObject["type"] = description.type;
  return jsonObject;

}

