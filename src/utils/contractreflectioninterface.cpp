#include "contractreflectioninterface.h"
#include "contract/dynamiccontract.h"

namespace ContractReflectionInterface {
  void inline registerCoreContractClasses(){
    meta::class_<ContractGlobals>();
    meta::class_<ContractLocals>().base_<ContractGlobals>();
    meta::class_<BaseContract>()
    .base_<ContractLocals>()
    .constructor_<std::string, Address, Address, uint64_t,
      std::unique_ptr<DB>>();
    meta::class_<DynamicContract>()
    .base_<BaseContract>()
    .constructor_<ContractManager::ContractManagerInterface &, std::string,
      Address, Address, uint64_t, const std::unique_ptr<DB> &>();
  }
}