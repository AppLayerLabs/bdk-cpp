/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "jsonabi.h"

bool JsonAbi::isTuple(const std::string& type) {
  // Tuples are always as "(type1,type2,...)" (alternatively, "(type1,type2,...)[]").
  // Check if first and last characters are "()" (not counting the "[]" at the end, if there are any).
  return (JsonAbi::isArray(type))
    ? (type[0] == '(' && type[type.size() - 2 * JsonAbi::countTupleArrays(type) - 1] == ')')
    : (type[0] == '(' && type[type.size() - 1] == ')');
}

bool JsonAbi::isArray(const std::string& type) {
  // Arrays are always as "type...[]" - check if the last two characters are "[]".
  return (type[type.size() - 2] == '[' && type[type.size() - 1] == ']');
}

uint64_t JsonAbi::countTupleArrays(const std::string& type) {
  // Count how many "[]" there are at the end of string, after the tuple.
  // Remember we need to skip anything inside "(...)".
  // Array tuples are always as "(type1,type2,...)[]" // or "(type1,type2,...)[][]" or "(type1,type2,...)[][][]" etc.
  uint64_t count = 0;
  for (int i = type.size() - 1; i >= 0; i--, i--) {
    if (type[i] == ']' && type[i - 1] == '[') count++; else break;
  }
  return count;
}

std::vector<std::string> JsonAbi::getTupleTypes(const std::string& type) {
  // Tuples are always as "(type1,type2,...)" - don't forget to remove "(...)" and "[]..." if it's an array.
  // Vector will return "type1", "type2", ...
  std::vector<std::string> types;
  std::string tupleString = type.substr(1); // Skip "("
  if (JsonAbi::isArray(type)) tupleString = tupleString.substr(
    0, tupleString.size() - 2 * JsonAbi::countTupleArrays(type) // If array, delete "[]"
  );
  tupleString = tupleString.substr(0, tupleString.size() - 1); // Remove ")"
  std::string tmp;
  uint16_t tupleCounter = 0; // We need to check if we're inside a tuple, so we don't split on "," inside "(...)".
  for (const char& c : tupleString) {
    if (c == '(') ++tupleCounter;
    if (c == ')') --tupleCounter;
    if (c == ',' && !tupleCounter) {
      types.push_back(tmp);
      tmp = "";
    } else {
      if (c != ' ') tmp += c; // Prevent e.g. "(int, double)" second type coming out as " double" with the space in it
    }
  }
  // Push the last type and return.
  types.push_back(tmp);
  return types;
}

json JsonAbi::handleTupleComponents(const std::vector<std::string>& tupleTypes) {
  json obj = json::array();
  for (const auto& type : tupleTypes) {
    json compObj = json::object();
    // Handle any type that is not a tuple.
    if (!JsonAbi::isTuple(type)) {
      compObj["internalType"] = type;
      compObj["type"] = type;
      obj.push_back(compObj);
      continue;
    }
    // Handle the tuple type.
    compObj["components"] = JsonAbi::handleTupleComponents(JsonAbi::getTupleTypes(type));
    if (!JsonAbi::isArray(type)) {
      // Type is not an array
      compObj["type"] = "tuple";
      obj.push_back(compObj);
      continue;
    }
    // Type is an array - check how many nested arrays we have (tuple[], tuple[][], tuple[][][], etc..)
    auto count = JsonAbi::countTupleArrays(type);
    std::string arrayType = "tuple";
    for (int i = 0; i < count; i++) arrayType += "[]";
    compObj["type"] = arrayType;
    obj.push_back(compObj);
  }
  return obj;
}

json JsonAbi::parseMethodInput(const std::vector<std::pair<std::string,std::string>>& inputDesc) {
  json obj = json::array();
  for (const auto& input : inputDesc) {
    const auto& [type, name] = input;
    json inObj = json::object();
    // Handle any type that is not a tuple.
    if (!JsonAbi::isTuple(type)) {
      inObj["internalType"] = type;
      inObj["name"] = name;
      inObj["type"] = type;
      obj.push_back(inObj);
      continue;
    }
    // Handle the tuple type.
    inObj["components"] = JsonAbi::handleTupleComponents(JsonAbi::getTupleTypes(type));
    inObj["name"] = name;
    if (!JsonAbi::isArray(type)) {
      // Type is not an array
      inObj["type"] = "tuple";
      obj.push_back(inObj);
      continue;
    }
    // Type is an array - check how many nested arrays we have (tuple[], tuple[][], tuple[][][], etc..)
    auto count = JsonAbi::countTupleArrays(type);
    std::string arrayType = "tuple";
    for (int i = 0; i < count; i++) arrayType += "[]";
    inObj["type"] = arrayType;
    obj.push_back(inObj);
  }
  return obj;
}

json JsonAbi::parseMethodOutput(const std::vector<std::string>& outputDesc) {
  json obj = json::array();
  if (outputDesc.size() == 1 && outputDesc[0] == "") return obj;
  for (const auto& output : outputDesc) {
    json outObj = json::object();
    // Handle any type that is not a tuple.
    if (!JsonAbi::isTuple(output)) {
      outObj["internalType"] = output;
      outObj["name"] = "";
      outObj["type"] = output;
      obj.push_back(outObj);
      continue;
    }
    // Handle the tuple type.
    outObj["components"] = JsonAbi::handleTupleComponents(JsonAbi::getTupleTypes(output));
    if (!JsonAbi::isArray(output)) {
      // Type is not an array
      outObj["type"] = "tuple";
      obj.push_back(outObj);
      continue;
    }
    // Type is an array - check how many nested arrays we have (tuple[], tuple[][], tuple[][][], etc..)
    auto count = JsonAbi::countTupleArrays(output);
    std::string arrayType = "tuple";
    for (int i = 0; i < count; i++) arrayType += "[]";
    outObj["type"] = arrayType;
    obj.push_back(outObj);
  }
  return obj;
}

json JsonAbi::parseEventArgs(const std::vector<std::tuple<std::string, std::string, bool>>& args) {
  json obj = json::array();
  for (const auto& arg : args) {
    const auto& [type, name, indexed] = arg;
    json inObj = json::object();
    // Handle any type that is not a tuple.
    if (!JsonAbi::isTuple(type)) {
      inObj["indexed"] = indexed;
      inObj["internalType"] = type;
      inObj["name"] = name;
      inObj["type"] = type;
      obj.push_back(inObj);
      continue;
    }
    // Handle the tuple type.
    inObj["components"] = JsonAbi::handleTupleComponents(JsonAbi::getTupleTypes(type));
    inObj["indexed"] = indexed;
    inObj["name"] = name;
    if (!JsonAbi::isArray(type)) {
      // Type is not an array
      inObj["type"] = "tuple";
      obj.push_back(inObj);
      continue;
    }
    // Type is an array - check how many nested arrays we have (tuple[], tuple[][], tuple[][][], etc..)
    auto count = JsonAbi::countTupleArrays(type);
    std::string arrayType = "tuple";
    for (int i = 0; i < count; i++) arrayType += "[]";
    inObj["type"] = arrayType;
    obj.push_back(inObj);
  }
  return obj;
}

json JsonAbi::methodToJSON(const ABI::MethodDescription& desc) {
  json obj = json::object();
  obj["inputs"] = JsonAbi::parseMethodInput(desc.inputs);
  obj["name"] = desc.name;
  obj["outputs"] = JsonAbi::parseMethodOutput(desc.outputs);
  switch (desc.stateMutability) {
    case FunctionTypes::View: obj["stateMutability"] = "view"; break;
    case FunctionTypes::NonPayable: obj["stateMutability"] = "nonpayable"; break;
    case FunctionTypes::Payable: obj["stateMutability"] = "payable"; break;
  }
  obj["type"] = desc.type;
  return obj;
}

json JsonAbi::eventToJSON(const ABI::EventDescription& desc) {
  json obj = json::object();
  obj["anonymous"] = desc.anonymous;
  obj["inputs"] = JsonAbi::parseEventArgs(desc.args);
  obj["name"] = desc.name;
  obj["type"] = "event";
  return obj;
}

