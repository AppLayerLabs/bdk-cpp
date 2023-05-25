#ifndef SIMPLECONTRACT_H
#define SIMPLECONTRACT_H

#include "dynamiccontract.h"
#include "utils/contractreflectioninterface.h"
#include "variables/safestring.h"
#include "variables/safeuint256_t.h"

class SimpleContract : public DynamicContract {
private:
  // string name
  SafeString name;
  // uint256 value
  SafeUint256_t value;

  void registerContractFunctions() override;

public:
  /// Create new contract with given name and value.
  SimpleContract(const std::string &name, uint256_t value,
                 ContractManager::ContractManagerInterface &interface,
                 const Address &address, const Address &creator,
                 const uint64_t &chainId, const std::unique_ptr<DB> &db);

  /// Load contract from database.
  SimpleContract(ContractManager::ContractManagerInterface &interface,
                 const Address &address, const std::unique_ptr<DB> &db);

  ~SimpleContract() override;

  static void registerContract() {
    ContractReflectionInteface::registerContract<
        SimpleContract, std::string &, uint256_t,
        ContractManager::ContractManagerInterface &, Address, Address, uint64_t,
        const std::unique_ptr<DB> &>(
        std::make_pair("getName", &SimpleContract::getName),
        std::make_pair("getValue", &SimpleContract::getValue),
        std::make_pair("setName", &SimpleContract::setName),
        std::make_pair("setValue", &SimpleContract::setValue));
        
  }

  // function getName() public view returns(string memory)
  std::string getName() const;
  // function getValue() public view returns(uint256)
  std::string getValue() const;
  // function setName(string memory argName) public
  void setName(const std::string & argName);
  // function setValue(uint256 argValue) public
  void setValue(uint256_t argValue);
};

#endif // SIMPLECONTRACT_H