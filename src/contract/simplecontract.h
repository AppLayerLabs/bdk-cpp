#ifndef SIMPLECONTRACT_H
#define SIMPLECONTRACT_H

#include "dynamiccontract.h"
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