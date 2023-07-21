#ifndef DEXV2ROUTER_H
#define DEXV2ROUTER_H

#include <memory>

#include "../utils/contractreflectioninterface.h"
#include "../utils/db.h"
#include "dynamiccontract.h"
#include "variables/safeaddress.h"
#include "dexv2library.h"


class DEXV2Router02 : public DynamicContract {
  private:
    /// Solidity: address private immutable _factory
    SafeAddress factory_;

    /// Solidity: address private immutable _WETH
    SafeAddress wrappedNative_;

    /// Function for calling the register functions for contracts.
    void registerContractFunctions() override;

    /**
     * Add liquidity to a specific pair.
     * @param tokenA The address of the first token.
     * @param tokenB The address of the second token.
     * @param amountADesired The amount of tokenA to add to the pool.
     * @param amountBDesired The amount of tokenB to add to the pool.
     * @param amountAMin The minimum amount of tokenA to add to the pool.
     * @param amountBMin The minimum amount of tokenB to add to the pool.
     * @return Optimal amount of tokens to add to the pair. (amountA, amountB). Between desired and min.
     * Solidity Counterpart:
     * function _addLiquidity( address tokenA, address tokenB, uint amountADesired, uint amountBDesired, uint amountAMin, uint amountBMin) internal virtual returns (uint amountA, uint amountB)
     */
    std::pair<uint256_t, uint256_t> _addLiquidity(
      const Address& tokenA,
      const Address& tokenB,
      const uint256_t& amountADesired,
      const uint256_t& amountBDesired,
      const uint256_t& amountAMin,
      const uint256_t& amountBMin
    );

    /**
     * Swap tokens for another token.
     * Requires the initial amount to have already been sent to the first pair
     * @param amounts
     * @param path
     * @param to
     * Solidity counterpart:
     * function _swap(uint[] memory amounts, address[] memory path, address _to) internal virtual
     */
    void _swap(const std::vector<uint256_t>& amounts, const std::vector<Address>& path, const Address& _to);

    /**
     * As there is no equivalent to the Solidity Modifier in C++ we have to implement the modifier logic somehow
     * The ensure modifier only checks if a timestamp is higher than current block timestamp
     * ensure() doesn't have to execute the anything after the function call.
     * So we can make it a bool function.
     * @param deadline The timestamp to check against. (SECONDS!!!)
     */
    bool ensure(const uint64_t& deadline);

  public:

    /**
    * ConstructorArguments is a tuple of the contract constructor arguments in the order they appear in the constructor.
    */
    using ConstructorArguments = std::tuple<Address, Address>;

    /**
     * Constructor for loading contract from DB.
     * @param interface Reference to the contract manager interface.
     * @param address The address where the contract will be deployed.
     * @param db Reference to the database object.
    */
    DEXV2Router02(
      ContractManagerInterface& interface,
      const Address& address, const std::unique_ptr<DB>& db
    );

    /**
     * Constructor to be used when creating a new contract.
     * @param factory The address of the factory contract.
     * @param wrappedNative The address of the wrapped native token.
     * @param interface Reference to the contract manager interface.
     * @param address The address where the contract will be deployed.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain where the contract wil be deployed.
     * @param db Reference to the database object.
     */
    DEXV2Router02(
      const Address& factory, const Address& wrappedNative,
      ContractManagerInterface &interface,
      const Address &address, const Address &creator, const uint64_t &chainId,
      const std::unique_ptr<DB> &db
    );

    // Destructor.
    ~DEXV2Router02() override;

    /// Getter for the factory_ variable.
    Address factory() const;

    /// Getter for the wrappedNative_ variable.
    Address wrappedNative() const;

    /**
     * Add liquidity
     * @param tokenA The address of the first token.
     * @param tokenB The address of the second token.
     * @param amountADesired The amount of tokenA to add to the pool.
     * @param amountBDesired The amount of tokenB to add to the pool.
     * @param amountAMin The minimum amount of tokenA to add to the pool.
     * @param amountBMin The minimum amount of tokenB to add to the pool.
     * @param to The address that receives the liquidity tokens.
     * @param deadline The timestamp to check against. (SECONDS!!!)
     * @returns amountA The amount of tokenA that was added to the pool.
     * @returns amountB The amount of tokenB that was added to the pool.
     * @returns liquidity The amount of liquidity tokens minted.
     * Solidity counterpart:
     * function addLiquidity( address tokenA, address tokenB, uint amountADesired, uint amountBDesired, uint amountAMin, uint amountBMin, address to, uint deadline) external virtual override ensure(deadline) returns (uint amountA, uint amountB, uint liquidity)
     */
    BytesEncoded addLiquidity(
      const Address& tokenA,
      const Address& tokenB,
      const uint256_t& amountADesired,
      const uint256_t& amountBDesired,
      const uint256_t& amountAMin,
      const uint256_t& amountBMin,
      const Address& to,
      const uint64_t& deadline
    );

    /**
     * Add liquidity native
     * @param token The address of the token to add liquidity to.
     * @param amountTokenDesired The amount of token to add to the pool.
     * @param amountTokenMin The minimum amount of token to add to the pool.
     * @param amountNativeMin The minimum amount of native to add to the pool.
     * @param to The address that receives the liquidity tokens.
     * @param deadline The timestamp to check against. (SECONDS!!!)
     * @returns amountToken The amount of tokenA that was added to the pool.
     * @returns amountNative The amount of native tokens that was added to the pool.
     * @returns liquidity The amount of liquidity tokens minted.
     */
    BytesEncoded addLiquidityNative(
      const Address& token,
      const uint256_t& amountTokenDesired,
      const uint256_t& amountTokenMin,
      const uint256_t& amountNativeMin,
      const Address& to,
      const uint64_t& deadline
    );

    /**
     * Remove liquidity
     * @param tokenA The address of the first token.
     * @param tokenB The address of the second token.
     * @param liquidity The amount of liquidity tokens to remove.
     * @param amountAMin The minimum amount of tokenA to remove from the pool.
     * @param amountBMin The minimum amount of tokenB to remove from the pool.
     * @param to The address that receives the tokens.
     * @param deadline The timestamp to check against. (SECONDS!!!)
     * @returns amountA The amount of tokenA that was removed from the pool.
     * @returns amountB The amount of tokenB that was removed from the pool.
     * Solidity counterpart:
     * function removeLiquidity( address tokenA, address tokenB, uint liquidity, uint amountAMin, uint amountBMin, address to, uint deadline) public virtual override ensure(deadline) returns (uint amountA, uint amountB)
     */
    BytesEncoded removeLiquidity(
      const Address& tokenA,
      const Address& tokenB,
      const uint256_t& liquidity,
      const uint256_t& amountAMin,
      const uint256_t& amountBMin,
      const Address& to,
      const uint64_t& deadline
    );

    /**
     * Remove liquidity native
     * @param token The address of the token to remove liquidity from.
     * @param liquidity The amount of liquidity tokens to remove.
     * @param amountTokenMin The minimum amount of token to remove from the pool.
     * @param amountNativeMin The minimum amount of native to remove from the pool.
     * @param to The address that receives the tokens.
     * @param deadline The timestamp to check against. (SECONDS!!!)
     * @returns amountToken The amount of token that was removed from the pool.
     * @returns amountNative The amount of native tokens that was removed from the pool.
     * Solidity counterpart:
     * function removeLiquidityNative( address token, uint liquidity, uint amountTokenMin, uint amountNativeMin, address to, uint deadline) public virtual override ensure(deadline) returns (uint amountToken, uint amountNative)
     */
    BytesEncoded removeLiquidityNative(
      const Address& token,
      const uint256_t& liquidity,
      const uint256_t& amountTokenMin,
      const uint256_t& amountNativeMin,
      const Address& to,
      const uint64_t& deadline
    );

    /**
     * Swap exact tokens for tokens
     * @param amountIn The amount of token to swap.
     * @param amountOutMin The minimum amount of token to receive.
     * @param path The path of the tokens to swap.
     * @param to The address that receives the tokens.
     * @param deadline The timestamp to check against. (SECONDS!!!)
     * @returns amounts The amount of tokens that was swapped.
     * Solidity counterpart:
     * function swapExactTokensForTokens(uint amountIn, uint amountOutMin, address[] calldata path, address to, uint deadline) external virtual override ensure(deadline) returns (uint[] memory amounts)
     */
    std::vector<uint256_t> swapExactTokensForTokens(
      const uint256_t& amountIn,
      const uint256_t& amountOutMin,
      const std::vector<Address>& path,
      const Address& to,
      const uint64_t& deadline
    );

    /**
     * Swap tokens for exact amount of tokens
     * @param amountOut The amount of token to receive.
     * @param amountInMax The maximum amount of token to swap.
     * @param path The path of the tokens to swap.
     * @param to The address that receives the tokens.
     * @param deadline The timestamp to check against. (SECONDS!!!)
     * @returns amounts The amount of tokens that was swapped.
     * Solidity counterpart:
     * function swapTokensForExactTokens(uint amountOut, uint amountInMax, address[] calldata path, address to, uint deadline) external virtual override ensure(deadline) returns (uint[] memory amounts)
     */
    std::vector<uint256_t> swapTokensForExactTokens(
      const uint256_t& amountOut,
      const uint256_t& amountInMax,
      const std::vector<Address>& path,
      const Address& to,
      const uint64_t& deadline
    );

    /**
     * Swap exact native for tokens
     * @param amountOutMin The minimum amount of token to receive.
     * @param path The path of the tokens to swap.
     * @param to The address that receives the tokens.
     * @param deadline The timestamp to check against. (SECONDS!!!)
     * @returns amounts The amount of tokens that was swapped.
     * Solidity counterpart:
     * function swapExactNativeForTokens(uint amountOutMin, address[] calldata path, address to, uint deadline) external virtual payable override ensure(deadline) returns (uint[] memory amounts)
     */
    std::vector<uint256_t> swapExactNativeForTokens(
      const uint256_t& amountOutMin,
      const std::vector<Address>& path,
      const Address& to,
      const uint64_t& deadline
    );

    /**
     * Swap tokens for exact native
     * @param amountOut The amount of native to receive.
     * @param amountInMax The maximum amount of token to swap.
     * @param path The path of the tokens to swap.
     * @param to The address that receives the tokens.
     * @param deadline The timestamp to check against. (SECONDS!!!)
     * @returns amounts The amount of tokens that was swapped.
     * Solidity counterpart:
     * function swapTokensForExactNative(uint amountOut, uint amountInMax, address[] calldata path, address to, uint deadline) external virtual override ensure(deadline) returns (uint[] memory amounts)
     */
    std::vector<uint256_t> swapTokensForExactNative(
      const uint256_t& amountOut,
      const uint256_t& amountInMax,
      const std::vector<Address>& path,
      const Address& to,
      const uint64_t& deadline
    );

    /**
     * Swap exact tokens for native
     * @param amountIn The amount of token to swap.
     * @param amountOutMin The minimum amount of native to receive.
     * @param path The path of the tokens to swap.
     * @param to The address that receives the tokens.
     * @param deadline The timestamp to check against. (SECONDS!!!)
     * @returns amounts The amount of tokens that was swapped.
     * Solidity counterpart:
     * function swapExactTokensForNative(uint amountIn, uint amountOutMin, address[] calldata path, address to, uint deadline) external virtual override ensure(deadline) returns (uint[] memory amounts)
     */
    std::vector<uint256_t> swapExactTokensForNative(
      const uint256_t& amountIn,
      const uint256_t& amountOutMin,
      const std::vector<Address>& path,
      const Address& to,
      const uint64_t& deadline
    );

    /**
     * Swap native for exact tokens
     * @param amountOut The amount of token to receive.
     * @param amountInMax The maximum amount of native to swap.
     * @param path The path of the tokens to swap.
     * @param to The address that receives the tokens.
     * @param deadline The timestamp to check against. (SECONDS!!!)
     * @returns amounts The amount of tokens that was swapped.
     * Solidity counterpart:
     * function swapNativeForExactTokens(uint amountOut, uint amountInMax, address[] calldata path, address to, uint deadline) external virtual payable override ensure(deadline) returns (uint[] memory amounts)
     */
    std::vector<uint256_t> swapNativeForExactTokens(
      const uint256_t& amountOut,
      const uint256_t& amountInMax,
      const std::vector<Address>& path,
      const Address& to,
      const uint64_t& deadline
    );

    static void registerContract() {
      ContractReflectionInterface::registerContract<
        DEXV2Router02, const Address &, const Address &, ContractManagerInterface &,
        const Address &, const Address &, const uint64_t &,
        const std::unique_ptr<DB> &
      >(
        std::vector<std::string>{"factory", "wrappedNative"},
        std::make_tuple("factory", &DEXV2Router02::factory, "view", std::vector<std::string>{}),
        std::make_tuple("wrappedNative", &DEXV2Router02::wrappedNative, "view", std::vector<std::string>{}),
        std::make_tuple("addLiquidity", &DEXV2Router02::addLiquidity, "nonpayable",
                        std::vector<std::string>{"tokenA", "tokenB", "amountADesired", "amountBDesired", "amountAMin", "amountBMin", "to", "deadline"}),
        std::make_tuple("addLiquidityNative", &DEXV2Router02::addLiquidityNative, "payable",
                        std::vector<std::string>{"token", "amountTokenDesired", "amountTokenMin", "amountNativeMin", "to", "deadline"}),
        std::make_tuple("removeLiquidity", &DEXV2Router02::removeLiquidity, "nonpayable",
                        std::vector<std::string>{"tokenA", "tokenB", "liquidity", "amountAMin", "amountBMin", "to", "deadline"}),
        std::make_tuple("removeLiquidityNative", &DEXV2Router02::removeLiquidityNative, "payable",
                        std::vector<std::string>{"token", "liquidity", "amountTokenMin", "amountNativeMin", "to", "deadline"}),
        std::make_tuple("swapExactTokensForTokens", &DEXV2Router02::swapExactTokensForTokens, "nonpayable",
                        std::vector<std::string>{"amountIn", "amountOutMin", "path", "to", "deadline"}),
        std::make_tuple("swapTokensForExactTokens", &DEXV2Router02::swapTokensForExactTokens, "nonpayable",
                        std::vector<std::string>{"amountOut", "amountInMax", "path", "to", "deadline"}),
        std::make_tuple("swapExactNativeForTokens", &DEXV2Router02::swapExactNativeForTokens, "payable",
                        std::vector<std::string>{"amountOutMin", "path", "to", "deadline"}),
        std::make_tuple("swapTokensForExactNative", &DEXV2Router02::swapTokensForExactNative, "payable",
                        std::vector<std::string>{"amountIn", "amountOutMin", "path", "to", "deadline"}),
        std::make_tuple("swapExactTokensForNative", &DEXV2Router02::swapExactTokensForNative, "payable",
                        std::vector<std::string>{"amountIn", "amountOutMin", "path", "to", "deadline"}),
        std::make_tuple("swapNativeForExactTokens", &DEXV2Router02::swapNativeForExactTokens, "payable",
                        std::vector<std::string>{"amountOut", "amountInMax", "path", "to", "deadline"})
      );
    }
};

#endif // DEXV2ROUTER_H