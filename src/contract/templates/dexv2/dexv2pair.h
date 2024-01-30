/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef DEXV2PAIR_H
#define DEXV2PAIR_H

#include <memory>

#include "../../../utils/contractreflectioninterface.h"
#include "../../../utils/db.h"
#include "../../../utils/utils.h"
#include "../../abi.h"
#include "../../dynamiccontract.h"
#include "../../variables/reentrancyguard.h"
#include "../../variables/safeaddress.h"
#include "../../variables/safestring.h"
#include "../../variables/safeunorderedmap.h"
#include "../erc20.h"
#include "uq112x112.h"

/// Template for an DEXV2Pair contract.
class DEXV2Pair : public ERC20 {
  protected:
    /// Solidity: uint public constant MINIMUM_LIQUIDITY;
    const uint256_t MINIMUM_LIQUIDITY = 1000;

    /// Solidity: address private _factory
    SafeAddress factory_;

    /// Solidity: address private _token0
    SafeAddress token0_;

    /// Solidity: address private _token1
    SafeAddress token1_;

    /// Solidity: uint112 private _reserve0
    SafeUint112_t reserve0_;

    /// Solidity: uint112 private _reserve1
    SafeUint112_t reserve1_;

    /// Solidity: uint32 private _blockTimestampLast (seconds!)
    SafeUint32_t blockTimestampLast_;

    /// Solidity: uint256 private _price0CumulativeLast
    SafeUint256_t price0CumulativeLast_;

    /// Solidity: uint256 private _price1CumulativeLast
    SafeUint256_t price1CumulativeLast_;

    /// Solidity: uint256 private _kLast
    SafeUint256_t kLast_;

    /// Function for calling the register functions for contracts.
    void registerContractFunctions() override;

    /**
     * Function to be called when transfering ERC20 tokens.
     * @param token The address of the token to transfer.
     * @param to The address to transfer the tokens to.
     * @param value The amount of tokens to transfer.
     */
    void _safeTransfer(const Address& token, const Address& to, const uint256_t& value);

    /**
     * Update reserves and, on the first call per block, price accumulators.
     * Solidity counterpart: function _update(uint balance0, uint balance1, uint112 reserve0, uint112 reserve1) private
     * @param balance0 The balance of token0.
     * @param balance1 The balance of token1.
     * @param reserve0 The reserve of token0.
     * @param reserve1 The reserve of token1.
     */
    void _update(const uint256_t& balance0, const uint256_t& balance1, const uint256_t& reserve0, const uint256_t& reserve1);

    /**
     * Mint the fee for DEX corresponding to the increase in sqrt(k).
     * If fee is on, mint liquidity equivalent to 1/6th of the growth in sqrt(k).
     * @return `true` if fee is on, `false` otherwise.
     */
    bool _mintFee(uint112_t reserve0, uint112_t reserve1);

  public:
    /**
     * ConstructorArguments is a tuple of the contract constructor arguments in the order they appear in the constructor.
     */
    using ConstructorArguments = std::tuple<>;

    /**
     * Constructor for loading contract from DB.
     * @param interface Reference to the contract manager interface.
     * @param address The address where the contract will be deployed.
     * @param db Reference to the database object.
    */
    DEXV2Pair(
      ContractManagerInterface& interface,
      const Address& address, const std::unique_ptr<DB>& db
    );

    /**
     * Constructor to be used when creating a new contract.
     * @param interface Reference to the contract manager interface.
     * @param address The address where the contract will be deployed.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain where the contract wil be deployed.
     * @param db Reference to the database object.
     */
    DEXV2Pair(
      ContractManagerInterface &interface,
      const Address &address, const Address &creator, const uint64_t &chainId,
      const std::unique_ptr<DB> &db
    );

    /// Destructor.
    ~DEXV2Pair() override;


    /**
     * Initialize the contract
     * To be called by DEXFactory after contract creation.
     * @param token0 The address of token0.
     * @param token1 The address of token1.
     */
    void initialize(const Address& token0, const Address& token1);

    /**
     * Get the reserves of the ERC2OPair
     * Direct std::pair function call so it can be utilized by others contracts in the C++ side.
     */
    std::pair<uint256_t, uint256_t> getReservess() const;

    /**
     * Get the reserves of the ERC20Pair.
     * Solidity counterpart: function getReserves() external view returns (uint112 reserve0, uint112 reserve1, uint32 blockTimestampLast)
     * @returns std::tuple<uint256_t, uint256_t, uint256_t> (reserve0, reserve1, blockTimestampLast)
     */
    std::tuple<uint256_t, uint256_t, uint256_t> getReserves() const;

    /**
     * Get the factory address of the ERC20Pair.
     * @return The factory address.
     */
    Address factory() const;

    /**
     * Get the token0 address of the ERC20Pair.
     * @return The token0 address.
     */
    Address token0() const;

    /**
     * Get the token1 address of the ERC20Pair.
     * @return The token1 address.
     */
    Address token1() const;

    /**
     * Get the price0CumulativeLast of the ERC20Pair.
     * @return The price0CumulativeLast.
     */
    uint256_t price0CumulativeLast() const;

    /**
     * Get the price1CumulativeLast of the ERC20Pair.
     * @return The price1CumulativeLast.
     */
    uint256_t price1CumulativeLast() const;

    /**
     * Get the kLast of the ERC20Pair.
     * @return The kLast.
     */
    uint256_t kLast() const;

    /**
     * Mint an amount of ERC20 tokens to the specified address based on the current reserves.
     * This low-level function should be called from a contract which performs important safety checks (router).
     * Solidity counterpart: function mint(address to) external lock returns (uint liquidity)
     * @param to The address to mint tokens to.
     * @return The amount of tokens minted.
     */
    uint256_t mint(const Address& to);

    /**
     * Burn an amount of ERC20 tokens and send the corresponding amounts of tokens to the specified addresses.
     * This low-level function should be called from a contract which performs important safety checks (router).
     * Solidity counterpart: function burn(address to) external lock returns (uint amount0, uint amount1)
     * @param to The address to burn tokens from.
     * @return std::tuple<uint256_t, uint256_t> (amount0, amount1) encoded in Bytes
     */
    std::tuple<uint256_t, uint256_t> burn(const Address& to);

    /**
     * Swap an amount of ERC20 tokens for the other ERC20 token.
     * This low-level function should be called from a contract which performs important safety checks (router).
     * Solidity counterpart: function swap(uint amount0Out, uint amount1Out, address to, bytes calldata data) external lock
     * Currently data is not supported.
     */
    void swap(const uint256_t& amount0Out, const uint256_t& amount1Out, const Address& to);

    /**
     * Skim an amount of ERC20 tokens from the reserves.
     * This low-level function should be called from a contract which performs important safety checks (router).
     * Solidity counterpart: function skim(address to) external
     */
    void skim(const Address& to);

    /// Sync the reserves to the current balances.
    void sync();

    /// Register contract class via ContractReflectionInterface.
    static void registerContract() {
      ContractReflectionInterface::registerContractMethods<
        DEXV2Pair, ContractManagerInterface &,
        const Address &, const Address &, const uint64_t &,
        const std::unique_ptr<DB> &
      >(
        std::vector<std::string>{},
        std::make_tuple("initialize", &DEXV2Pair::initialize, FunctionTypes::NonPayable, std::vector<std::string>{"token0_", "token1_"}),
        std::make_tuple("getReserves", &DEXV2Pair::getReserves, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("factory", &DEXV2Pair::factory, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("token0", &DEXV2Pair::token0, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("token1", &DEXV2Pair::token1, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("price0CumulativeLast", &DEXV2Pair::price0CumulativeLast, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("price1CumulativeLast", &DEXV2Pair::price1CumulativeLast, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("kLast", &DEXV2Pair::kLast, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("mint", &DEXV2Pair::mint, FunctionTypes::NonPayable, std::vector<std::string>{"to"}),
        std::make_tuple("burn", &DEXV2Pair::burn, FunctionTypes::NonPayable, std::vector<std::string>{"to"}),
        std::make_tuple("swap", &DEXV2Pair::swap, FunctionTypes::NonPayable, std::vector<std::string>{"amount0Out", "amount1Out", "to"}),
        std::make_tuple("skim", &DEXV2Pair::skim, FunctionTypes::NonPayable, std::vector<std::string>{"to"}),
        std::make_tuple("sync", &DEXV2Pair::sync, FunctionTypes::NonPayable, std::vector<std::string>{})
      );
    }
};

#endif // DEXV2PAIR_H
