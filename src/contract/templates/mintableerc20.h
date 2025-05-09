/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef MINTABLEERC20_H
#define MINTABLEERC20_H

#include <memory>

#include "../../utils/db.h"
#include "../../utils/utils.h"
#include "../../utils/contractreflectioninterface.h"
#include "../dynamiccontract.h"
#include "../variables/safestring.h"
#include "../variables/safeunorderedmap.h"
#include "ownable.h"
#include "standards/erc20.h"

/// Template for a Mintable ERC20 contract.
class ERC20Mintable : virtual public ERC20, virtual public Ownable {
  private:
    /// Function for calling the register functions for contracts
    void registerContractFunctions() override;

  public:
    /// ConstructorArguments is a tuple of the contract constructor arguments in the order they appear in the constructor.
    using ConstructorArguments = std::tuple<
      const std::string &, const std::string &, const uint8_t &
    >;

    /**
     * Constructor for loading contract from DB.
     * @param address The address where the contract will be deployed.
     * @param db Reference to the database object.
     */
    ERC20Mintable(const Address& address, const DB& db);

    /**
     * Constructor to be used when creating a new contract.
     * @param erc20_name The name of the token.
     * @param erc20_symbol The symbol of the token.
     * @param erc20_decimals The decimals of the token.
     * @param address The address where the contract will be deployed.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain id of the contract.
     */
    ERC20Mintable(
      const std::string &erc20_name, const std::string &erc20_symbol,
      const uint8_t &erc20_decimals,
      const Address &address, const Address &creator,
      const uint64_t &chainId
    );

    /// Destructor.
    ~ERC20Mintable() override;

    void mint(const Address &to, const uint256_t &amount);
    void burn(const uint256_t& value); // Only the owner of the token can burn tokens

    /// Register contract using ContractReflectionInterface.
    static void registerContract() {
      static std::once_flag once;
      std::call_once(once, []() {
        DynamicContract::registerContractMethods<ERC20Mintable>(
          std::vector<std::string>{"erc20_name", "erc20_symbol", "erc20_decimals"},
          std::make_tuple("mint", &ERC20Mintable::mint, FunctionTypes::NonPayable, std::vector<std::string>{}),
          std::make_tuple("burn", &ERC20Mintable::burn, FunctionTypes::NonPayable, std::vector<std::string>{})
        );
      });
    }

    /// Dump method
    DBBatch dump() const override;
};

#endif // MINTABLEERC20_H
