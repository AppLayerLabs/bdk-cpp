/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef BTVENERGY_H
#define BTVENERGY_Hq

#include "standards/erc20.h"
#include "ownable.h"
#include "../variables/safeunorderedmap.h"
#include "../variables/safeuint.h"

class BTVEnergy : public virtual ERC20, public virtual Ownable {
  private:

    void registerContractFunctions() override;
  public:
    /// ConstructorArguments is a tuple of the contract constructor arguments in the order they appear in the constructor.
    using ConstructorArguments = std::tuple<
      const std::string &, const std::string &, const uint8_t &
    >;

    BTVEnergy(const Address& address, const DB& db);

    BTVEnergy(
      const std::string &erc20_name, const std::string &erc20_symbol,
      const uint8_t &erc20_decimals,
      const Address &address, const Address &creator,
      const uint64_t &chainId
    );

    void mint(const Address &to, const uint256_t &value);
    void burn(const Address &from, const uint256_t &value);

    /// Register contract using ContractReflectionInterface.
    static void registerContract() {
      ContractReflectionInterface::registerContractMethods<
        BTVEnergy, std::string &, std::string &, uint8_t &,
        const Address &,
        const Address &, const uint64_t &, DB&
      >(
        std::vector<std::string>{"erc20_name", "erc20_symbol", "erc20_decimals"},
        std::make_tuple("mint", &BTVEnergy::mint, FunctionTypes::NonPayable, std::vector<std::string>{"to","value"}),
        std::make_tuple("burn", &BTVEnergy::burn, FunctionTypes::NonPayable, std::vector<std::string>{"from","value"})
      );
    }
    DBBatch dump() const override;
};



#endif // BTVENERGY_H
