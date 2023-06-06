#ifndef SIMPLECONTRACT_H
#define SIMPLECONTRACT_H

#include "dynamiccontract.h"
#include "src/utils/contractreflectioninterface.h"
#include "variables/safestring.h"
#include "variables/safeuint256_t.h"
#include <any>

class SimpleContract : public DynamicContract {
private:
  SafeString name;     // string name
  SafeUint256_t value; // uint256 value
  void registerContractFunctions() override;

public:
  // Constructor from scratch. Create new contract with given name and value.
  SimpleContract(const std::string &name, uint256_t value,
                 ContractManager::ContractManagerInterface &interface,
                 const Address &address, const Address &creator,
                 const uint64_t &chainId, const std::unique_ptr<DB> &db);

  // Constructor from load. Load contract from database.
  SimpleContract(ContractManager::ContractManagerInterface &interface,
                 const Address &address, const std::unique_ptr<DB> &db);

  static void registerContract() {
    ContractReflectionInterface::registerContract<
        SimpleContract, const std::string &, uint256_t,
        ContractManager::ContractManagerInterface &, const Address &,
        const Address &, const uint64_t &, const std::unique_ptr<DB> &>(
        std::vector<std::string>{"name", "value"}, // Constructor arguments.
        std::make_tuple("getName", &SimpleContract::getName, "view",
                        std::vector<std::string>{}),
        std::make_tuple("getValue", &SimpleContract::getValue, "view",
                        std::vector<std::string>{}),
        std::make_tuple("setName", &SimpleContract::setName, "nonpayable",
                        std::vector<std::string>{"argName"}),
        std::make_tuple("setValue", &SimpleContract::setValue, "nonpayable",
                        std::vector<std::string>{"argValue"}));
  }

template <typename R, typename T>
void registerMemberFunction(const std::string& funcSignature, R(T::*memFunc)() const, T* instance) {
  bool hasArgs = ContractReflectionInterface::methodHasArguments(funcSignature);;
  std::string methodMutability = ContractReflectionInterface::getMethodMutability(funcSignature);
  if (!hasArgs && methodMutability == "view") {
    this->registerViewFunction(Utils::sha3(funcSignature + "()").get().substr(0,4), [instance, memFunc](const ethCallInfo &callInfo) {
        return (instance->*memFunc)();
    });
  }
  else if (!hasArgs && methodMutability == "nonpayable") {
    this->registerFunction(Utils::sha3(funcSignature + "()").get().substr(0,4), [instance, memFunc](const ethCallInfo &callInfo) {
        return (instance->*memFunc)();
    });
  }
  else if (!hasArgs && methodMutability == "payable") {
    this->registerPayableFunction(Utils::sha3(funcSignature + "()").get().substr(0,4), [instance, memFunc](const ethCallInfo &callInfo) {
        return (instance->*memFunc)();
    });
  }
  else {
    throw std::runtime_error("Invalid function signature.");
  }
}

template <typename R, typename T>
void registerMemberFunction(const std::string& funcSignature, R(T::*memFunc)(), T* instance) {
  bool hasArgs = ContractReflectionInterface::methodHasArguments(funcSignature);;
  std::string methodMutability = ContractReflectionInterface::getMethodMutability(funcSignature);
  if (!hasArgs && methodMutability == "view") {
    //view functions must be const, so we can't register them
    throw std::runtime_error("Invalid function signature. View functions must be const.");
  }
  else if (!hasArgs && methodMutability == "nonpayable") {
    this->registerFunction(Utils::sha3(funcSignature + "()").get().substr(0,4), [instance, memFunc](const ethCallInfo &callInfo) {
        return (instance->*memFunc)();
    });
  }
  else if (!hasArgs && methodMutability == "payable") {
    this->registerPayableFunction(Utils::sha3(funcSignature + "()").get().substr(0,4), [instance, memFunc](const ethCallInfo &callInfo) {
        return (instance->*memFunc)();
    });
  }
  else {
    throw std::runtime_error("Invalid function signature.");
  }
}


template <typename T, typename R, typename... Args, std::size_t... Is>
auto tryCallFuncWithTuple(T* instance, R(T::*memFunc)(Args...),
                          const std::vector<std::any>& dataVec,
                          std::index_sequence<Is...>) {
    try {
        return (instance->*memFunc)(std::any_cast<Args>(dataVec[Is])...);
    } catch (const std::bad_any_cast& ex) {
        std::string errorMessage = "Mismatched argument types. Attempted casting failed with: ";
        ((errorMessage += ("\nAttempted to cast to type: " + std::string(typeid(Args).name()) +
                          ", Actual type in the any: " +
                          (dataVec[Is].has_value() ? std::string(dataVec[Is].type().name()) : "Empty any"))), ...);
        throw std::runtime_error(errorMessage);
    }
}


template <typename R, typename... Args, typename T>
void registerMemberFunction(const std::string& funcSignature, R(T::*memFunc)(Args...), T* instance) {
  std::vector<std::string> args = ContractReflectionInterface::getMethodArgumentsTypesString(funcSignature);
  std::string fullSignature = funcSignature + "(";
  if (args.size() == 1) {
    fullSignature += args[0] + ")";
  }
  else {
    for (int i = 0; i < args.size() - 1; i++) {
      fullSignature += args[i] + ",";
    }
    fullSignature += args[args.size() - 1] + ")";
  }
  this->registerFunction(Utils::sha3(fullSignature).get().substr(0,4), [this, instance, memFunc, funcSignature](const ethCallInfo &callInfo) {
    std::vector<ABI::Types> types = ContractReflectionInterface::getMethodArgumentsTypesABI(funcSignature);
    ABI::Decoder decoder(types, std::get<5>(callInfo).substr(4));
    std::vector<std::any> dataVector;
    for (int i = 0; i < types.size(); i++) {
            dataVector.push_back(decoder.getDataDispatch(i, types[i]));
        }
        return tryCallFuncWithTuple(instance, memFunc, dataVector, std::index_sequence_for<Args...>());
    });
}





// template <typename R, typename T>
// void registerMemberFunction(const std::string& funcSignature, R(T::*memFunc)(const std::string&), T* instance) {
//   bool result = ContractReflectionInterface::methodHasArguments(funcSignature);
//   std::cout << "Method " << funcSignature << " has arguments: " << result << std::endl;
//     this->registerFunction(Utils::sha3(funcSignature).get().substr(0,4), [instance, memFunc](const ethCallInfo &callInfo) {
//         std::vector<ABI::Types> types = { ABI::Types::string };
//         ABI::Decoder decoder(types, std::get<5>(callInfo).substr(4));
//         return (instance->*memFunc)(decoder.getData<std::string>(0));
//     });
// }



  // Destructor.
  ~SimpleContract() override;

  std::string
  getName() const; // function getName() public view returns(string memory)
  std::string
  getValue() const; // function getValue() public view returns(uint256)
  void setName(const std::string
                   &argName); // function setName(string memory argName) public
  void
  setValue(uint256_t argValue); // function setValue(uint256 argValue) public
};

#endif // SIMPLECONTRACT_H