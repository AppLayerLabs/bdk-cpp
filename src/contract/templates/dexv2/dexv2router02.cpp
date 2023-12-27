/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "dexv2router02.h"
#include "dexv2factory.h"
#include "dexv2pair.h"
#include "../nativewrapper.h"
#include <sys/types.h>

DEXV2Router02::DEXV2Router02(
  ContractManagerInterface &interface, const Address &address, const std::unique_ptr<DB> &db
) : DynamicContract(interface, address, db), factory_(this), wrappedNative_(this)
{
  this->factory_ = Address(this->db_->get(Utils::stringToBytes("factory_"), this->getDBPrefix()));
  this->wrappedNative_ = Address(this->db_->get(Utils::stringToBytes("wrappedNative_"), this->getDBPrefix()));
  this->factory_.commit();
  this->wrappedNative_.commit();
  this->registerContractFunctions();
}

DEXV2Router02::DEXV2Router02(
  const Address& factory, const Address& nativeWrapper,
  ContractManagerInterface &interface,
  const Address &address, const Address &creator, const uint64_t &chainId,
  const std::unique_ptr<DB> &db
) : DynamicContract(interface, "DEXV2Router02", address, creator, chainId, db),
  factory_(this), wrappedNative_(this)
{
  this->factory_ = factory;
  this->wrappedNative_ = nativeWrapper;
  this->factory_.commit();
  this->wrappedNative_.commit();
  this->registerContractFunctions();
}

DEXV2Router02::~DEXV2Router02() {
  DBBatch batchOperations;
  batchOperations.push_back(
    Utils::stringToBytes("factory_"), this->factory_.get().view_const(), this->getDBPrefix()
  );
  batchOperations.push_back(
    Utils::stringToBytes("wrappedNative_"), this->wrappedNative_.get().view_const(), this->getDBPrefix()
  );
  this->db_->putBatch(batchOperations);
}

void DEXV2Router02::registerContractFunctions() {
  registerContract();
  this->registerMemberFunction("factory", &DEXV2Router02::factory, this);
  this->registerMemberFunction("wrappedNative", &DEXV2Router02::wrappedNative, this);
  this->registerMemberFunction("addLiquidity", &DEXV2Router02::addLiquidity, this);
  this->registerMemberFunction("addLiquidityNative", &DEXV2Router02::addLiquidityNative, this);
  this->registerMemberFunction("removeLiquidity", &DEXV2Router02::removeLiquidity, this);
  this->registerMemberFunction("removeLiquidityNative", &DEXV2Router02::removeLiquidityNative, this);
  this->registerMemberFunction("swapExactTokensForTokens", &DEXV2Router02::swapExactTokensForTokens, this);
  this->registerMemberFunction("swapTokensForExactTokens", &DEXV2Router02::swapTokensForExactTokens, this);
  this->registerMemberFunction("swapExactNativeForTokens", &DEXV2Router02::swapExactNativeForTokens, this);
  this->registerMemberFunction("swapTokensForExactNative", &DEXV2Router02::swapTokensForExactNative, this);
  this->registerMemberFunction("swapExactTokensForNative", &DEXV2Router02::swapExactTokensForNative, this);
  this->registerMemberFunction("swapNativeForExactTokens", &DEXV2Router02::swapNativeForExactTokens, this);
}

std::pair<uint256_t, uint256_t> DEXV2Router02::_addLiquidity(
  const Address& tokenA,
  const Address& tokenB,
  const uint256_t& amountADesired,
  const uint256_t& amountBDesired,
  const uint256_t& amountAMin,
  const uint256_t& amountBMin
) {
  uint256_t amountA = 0;
  uint256_t amountB = 0;
  auto pairAddress = this->callContractViewFunction(
    this->factory_.get(), &DEXV2Factory::getPair, tokenA, tokenB
  );
  if (!pairAddress) {
    Utils::safePrint("_addLiquidity: contract doesn't exist!");
    pairAddress = this->callContractFunction(
      this->factory_.get(), &DEXV2Factory::createPair, tokenA, tokenB
    );
  } else {
    Utils::safePrint("_addLiquidity: contract exists!");
  }
  auto reserves = this->callContractViewFunction(pairAddress.get(), &DEXV2Pair::getReservess);
  const auto& [reserveA, reserveB] = reserves;
  if (reserveA == 0 && reserveB == 0) {
    amountA = amountADesired;
    amountB = amountBDesired;
  } else {
    uint256_t amountBoptimal = DEXV2Library::quote(amountADesired, reserveA, reserveB);
    if (amountBoptimal <= amountBDesired) {
      if (amountBoptimal < amountBMin) throw std::runtime_error(
        "DEXV2Router02::_addLiquidity: INSUFFICIENT_B_AMOUNT"
      );
      amountA = amountADesired;
      amountB = amountBoptimal;
    } else {
      uint256_t amountAoptimal = DEXV2Library::quote(amountBDesired, reserveB, reserveA);
      if (amountAoptimal <= amountADesired) {
        if (amountAoptimal < amountAMin) throw std::runtime_error(
          "DEXV2Router02::_addLiquidity: INSUFFICIENT_A_AMOUNT"
        );
        amountA = amountAoptimal;
        amountB = amountBDesired;
      } else {
        throw std::runtime_error("DEXV2Router02::_addLiquidity: INSUFFICIENT_A_AMOUNT");
      }
    }
  }

  return {amountA, amountB};
}

void DEXV2Router02::_swap(
  const std::vector<uint256_t>& amounts, const std::vector<Address>& path, const Address& _to
) {
  for (size_t i = 0; i < path.size() - 1; i++) {
    auto input = path[i];
    auto output = path[i + 1];
    auto pairAddress = this->callContractViewFunction(
      this->factory_.get(), &DEXV2Factory::getPair, input, output
    );
    if (!pairAddress) throw std::runtime_error("DEXV2Router02::_swap: PAIR_NOT_FOUND");
    auto token0 = DEXV2Library::sortTokens(input, output).first;
    uint256_t amountOut = amounts[i + 1];
    uint256_t amount0Out, amount1Out;
    if (input == token0) {
      amount0Out = uint256_t(0);
      amount1Out = amountOut;
    } else {
      amount0Out = amountOut;
      amount1Out = uint256_t(0);
    }
    auto to = (i < path.size() - 2) ? this->callContractViewFunction(
      this->factory_.get(), &DEXV2Factory::getPair, output, path[i + 2]
    ) : _to;
    this->callContractFunction(
      pairAddress.get(), &DEXV2Pair::swap, amount0Out, amount1Out, to
    );
  }
}

bool DEXV2Router02::ensure(const uint256_t& deadline) {
  if (deadline < this->getBlockTimestamp()) throw std::runtime_error("DEXV2Router02::ensure: EXPIRED");
  return true;
}

Address DEXV2Router02::factory() const { return this->factory_.get(); }

Address DEXV2Router02::wrappedNative() const { return this->wrappedNative_.get(); }

std::tuple<uint256_t, uint256_t, uint256_t> DEXV2Router02::addLiquidity(
  const Address& tokenA,
  const Address& tokenB,
  const uint256_t& amountADesired,
  const uint256_t& amountBDesired,
  const uint256_t& amountAMin,
  const uint256_t& amountBMin,
  const Address& to,
  const uint256_t& deadline
) {
  this->ensure(deadline);
  auto [amountA, amountB] = this->_addLiquidity(
    tokenA, tokenB, amountADesired, amountBDesired, amountAMin, amountBMin
  );
  auto pair = DEXV2Library::pairFor(this->interface_, this->factory_.get(), tokenA, tokenB);
  this->callContractFunction(tokenA, &ERC20::transferFrom, this->getCaller(), pair, amountA);
  this->callContractFunction(tokenB, &ERC20::transferFrom, this->getCaller(), pair, amountB);
  auto liquidity = this->callContractFunction(pair, &DEXV2Pair::mint, to);
  return std::make_tuple(amountA, amountB, liquidity);
}

std::tuple<uint256_t, uint256_t, uint256_t> DEXV2Router02::addLiquidityNative(
  const Address& token,
  const uint256_t& amountTokenDesired,
  const uint256_t& amountTokenMin,
  const uint256_t& amountNativeMin,
  const Address& to,
  const uint256_t& deadline
) {
  this->ensure(deadline);
  auto [amountToken, amountNative] = this->_addLiquidity(
    token, this->wrappedNative_.get(), amountTokenDesired,
    amountNativeMin, amountTokenMin, amountNativeMin
  );
  auto pair = DEXV2Library::pairFor(this->interface_, this->factory_.get(), token, this->wrappedNative_.get());
  this->callContractFunction(token, &ERC20::transferFrom, this->getCaller(), pair, amountToken);
  this->callContractFunction(amountNative, this->wrappedNative_.get(), &NativeWrapper::deposit);
  this->callContractFunction(this->wrappedNative_.get(), &ERC20::transfer, pair, amountNative);
  auto liquidity = this->callContractFunction(pair, &DEXV2Pair::mint, to);
  // Refund dust Native, if any.
  if (this->getValue() > amountNative) this->sendTokens(
    this->getCaller(), this->getValue() - amountNative
  );
  return std::make_tuple(amountToken, amountNative, liquidity);
}

std::tuple<uint256_t, uint256_t> DEXV2Router02::removeLiquidity(
  const Address& tokenA,
  const Address& tokenB,
  const uint256_t& liquidity,
  const uint256_t& amountAMin,
  const uint256_t& amountBMin,
  const Address& to,
  const uint256_t& deadline
) {
  this->ensure(deadline);
  auto pair = DEXV2Library::pairFor(this->interface_, this->factory_.get(), tokenA, tokenB);
  this->callContractFunction(pair, &ERC20::transferFrom, this->getCaller(), pair, liquidity);
  auto burnBytes = this->callContractFunction(pair, &DEXV2Pair::burn, to);
  auto amount0 = std::get<0>(burnBytes);
  auto amount1 = std::get<1>(burnBytes);
  auto amountA = tokenA == DEXV2Library::sortTokens(tokenA, tokenB).first ? amount0 : amount1;
  auto amountB = tokenA == DEXV2Library::sortTokens(tokenA, tokenB).first ? amount1 : amount0;
  if (amountA < amountAMin) throw std::runtime_error("DEXV2Router02::removeLiquidity: INSUFFICIENT_A_AMOUNT");
  if (amountB < amountBMin) throw std::runtime_error("DEXV2Router02::removeLiquidity: INSUFFICIENT_B_AMOUNT");
  return std::make_tuple(amountA, amountB);
}

std::tuple<uint256_t, uint256_t> DEXV2Router02::removeLiquidityNative(
  const Address& token,
  const uint256_t& liquidity,
  const uint256_t& amountTokenMin,
  const uint256_t& amountNativeMin,
  const Address& to,
  const uint256_t& deadline
) {
  this->ensure(deadline);
  auto amounts = this->removeLiquidity(
      token, this->wrappedNative_.get(), liquidity, amountTokenMin,
      amountNativeMin, this->getContractAddress(), deadline
    );
  auto amountToken = std::get<0>(amounts);
  auto amountNative = std::get<1>(amounts);
  this->callContractFunction(token, &ERC20::transfer, to, amountToken);
  this->callContractFunction(this->wrappedNative_.get(), &NativeWrapper::withdraw, amountNative);
  this->sendTokens(this->getCaller(), amountNative);
  return std::make_tuple(amountToken, amountNative);
}

std::vector<uint256_t> DEXV2Router02::swapExactTokensForTokens(
  const uint256_t& amountIn,
  const uint256_t& amountOutMin,
  const std::vector<Address>& path,
  const Address& to,
  const uint256_t& deadline
) {
  this->ensure(deadline);
  auto amounts = DEXV2Library::getAmountsOut(this->interface_, this->factory_.get(), amountIn, path);
  auto amountOut = amounts.back();
  if (amountOut < amountOutMin) throw std::runtime_error(
    "DEXV2Router02::swapExactTokensForTokens: INSUFFICIENT_OUTPUT_AMOUNT"
  );
  auto pair = DEXV2Library::pairFor(this->interface_, this->factory_.get(), path[0], path[1]);
  this->callContractFunction(path.front(), &ERC20::transferFrom, this->getCaller(), pair, amounts[0]);
  this->_swap(amounts, path, to);
  return amounts;
}

std::vector<uint256_t> DEXV2Router02::swapTokensForExactTokens(
  const uint256_t& amountOut,
  const uint256_t& amountInMax,
  const std::vector<Address>& path,
  const Address& to,
  const uint256_t& deadline
) {
  this->ensure(deadline);
  auto amounts = DEXV2Library::getAmountsIn(this->interface_, this->factory_.get(), amountOut, path);
  auto amountIn = amounts.front();
  if (amountIn > amountInMax) throw std::runtime_error(
    "DEXV2Router02::swapTokensForExactTokens: EXCESSIVE_INPUT_AMOUNT"
  );
  auto pair = DEXV2Library::pairFor(this->interface_, this->factory_.get(), path[0], path[1]);
  this->callContractFunction(path.front(), &ERC20::transferFrom, this->getCaller(), pair, amountIn);
  this->_swap(amounts, path, to);
  return amounts;
}

std::vector<uint256_t> DEXV2Router02::swapExactNativeForTokens(
  const uint256_t& amountOutMin,
  const std::vector<Address>& path,
  const Address& to,
  const uint256_t& deadline
) {
  this->ensure(deadline);
  if (path[0] != this->wrappedNative_.get()) throw std::runtime_error(
    "DEXV2Router02::swapExactNativeForTokens: INVALID_PATH"
  );
  auto amounts = DEXV2Library::getAmountsOut(this->interface_, this->factory_.get(), this->getValue(), path);
  auto amountOut = amounts.back();
  if (amountOut < amountOutMin) throw std::runtime_error(
    "DEXV2Router02::swapExactNativeForTokens: INSUFFICIENT_OUTPUT_AMOUNT"
  );
  this->callContractFunction(amounts[0], this->wrappedNative_.get(), &NativeWrapper::deposit);
  auto pair = DEXV2Library::pairFor(this->interface_, this->factory_.get(), path[0], path[1]);
  this->callContractFunction(this->wrappedNative_.get(), &ERC20::transfer, pair, amounts[0]);
  this->_swap(amounts, path, to);
  return amounts;
}

std::vector<uint256_t> DEXV2Router02::swapTokensForExactNative(
  const uint256_t& amountOut,
  const uint256_t& amountInMax,
  const std::vector<Address>& path,
  const Address& to,
  const uint256_t& deadline
) {
  this->ensure(deadline);
  if (path.back() != this->wrappedNative_.get()) throw std::runtime_error(
    "DEXV2Router02::swapTokensForExactNative: INVALID_PATH"
  );
  auto amounts = DEXV2Library::getAmountsIn(this->interface_, this->factory_.get(), amountOut, path);
  auto amountIn = amounts.front();
  if (amountIn > amountInMax) throw std::runtime_error(
    "DEXV2Router02::swapTokensForExactNative: EXCESSIVE_INPUT_AMOUNT"
  );
  auto pair = DEXV2Library::pairFor(this->interface_, this->factory_.get(), path[0], path[1]);
  this->callContractFunction(path.front(), &ERC20::transferFrom, this->getCaller(), pair, amountIn);
  this->_swap(amounts, path, this->getContractAddress());
  this->callContractFunction(this->wrappedNative_.get(), &NativeWrapper::withdraw, amountOut);
  this->sendTokens(to, amountOut);
  return amounts;
}

std::vector<uint256_t> DEXV2Router02::swapExactTokensForNative(
  const uint256_t& amountIn,
  const uint256_t& amountOutMin,
  const std::vector<Address>& path,
  const Address& to,
  const uint256_t& deadline
) {
  this->ensure(deadline);
  if (path.back() != this->wrappedNative_.get()) throw std::runtime_error(
    "DEXV2Router02::swapExactTokensForNative: INVALID_PATH"
  );
  auto amounts = DEXV2Library::getAmountsOut(this->interface_, this->factory_.get(), amountIn, path);
  auto amountOut = amounts.back();
  if (amountOut < amountOutMin) throw std::runtime_error(
    "DEXV2Router02::swapExactTokensForNative: INSUFFICIENT_OUTPUT_AMOUNT"
  );
  auto pair = DEXV2Library::pairFor(this->interface_, this->factory_.get(), path[0], path[1]);
  this->callContractFunction(path.front(), &ERC20::transferFrom, this->getCaller(), pair, amounts[0]);
  this->_swap(amounts, path, this->getContractAddress());
  this->callContractFunction(this->wrappedNative_.get(), &NativeWrapper::withdraw, amountOut);
  this->sendTokens(to, amountOut);
  return amounts;
}

std::vector<uint256_t> DEXV2Router02::swapNativeForExactTokens(
  const uint256_t& amountOut,
  const uint256_t& amountInMax,
  const std::vector<Address>& path,
  const Address& to,
  const uint256_t& deadline
) {
  this->ensure(deadline);
  if (path[0] != this->wrappedNative_.get()) throw std::runtime_error(
    "DEXV2Router02::swapNativeForExactTokens: INVALID_PATH"
  );
  auto amounts = DEXV2Library::getAmountsIn(this->interface_, this->factory_.get(), amountOut, path);
  auto amountIn = amounts.front();
  if (amountIn > amountInMax) throw std::runtime_error(
    "DEXV2Router02::swapNativeForExactTokens: EXCESSIVE_INPUT_AMOUNT"
  );
  this->callContractFunction(amounts[0], this->wrappedNative_.get(), &NativeWrapper::deposit);
  auto pair = DEXV2Library::pairFor(this->interface_, this->factory_.get(), path[0], path[1]);
  this->callContractFunction(this->wrappedNative_.get(), &ERC20::transfer, pair, amounts[0]);
  this->_swap(amounts, path, to);
  return amounts;
}

