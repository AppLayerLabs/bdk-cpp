/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "dexv2pair.h"
#include "dexv2factory.h"

#include "../../../utils/uintconv.h"
#include "../../../utils/strconv.h"

DEXV2Pair::DEXV2Pair(const Address& address, const DB& db
) : DynamicContract(address, db), ERC20(address, db), factory_(this), token0_(this), token1_(this),
  reserve0_(this), reserve1_(this), blockTimestampLast_(this),
  price0CumulativeLast_(this), price1CumulativeLast_(this), kLast_(this)
{
  this->factory_ = Address(db.get(std::string("factory_"), this->getDBPrefix()));
  this->token0_ = Address(db.get(std::string("token0_"), this->getDBPrefix()));
  this->token1_ = Address(db.get(std::string("token1_"), this->getDBPrefix()));
  this->reserve0_ = UintConv::bytesToUint112(db.get(std::string("reserve0_"), this->getDBPrefix()));
  this->reserve1_ = UintConv::bytesToUint112(db.get(std::string("reserve1_"), this->getDBPrefix()));
  this->blockTimestampLast_ = UintConv::bytesToUint32(db.get(std::string("blockTimestampLast_"), this->getDBPrefix()));
  this->price0CumulativeLast_ = UintConv::bytesToUint256(db.get(std::string("price0CumulativeLast_"), this->getDBPrefix()));
  this->price1CumulativeLast_ = UintConv::bytesToUint256(db.get(std::string("price1CumulativeLast_"), this->getDBPrefix()));
  this->kLast_ = UintConv::bytesToUint256(db.get(std::string("kLast_"), this->getDBPrefix()));

  this->factory_.commit();
  this->token0_.commit();
  this->token1_.commit();
  this->reserve0_.commit();
  this->reserve1_.commit();
  this->blockTimestampLast_ .commit();
  this->price0CumulativeLast_.commit();
  this->price1CumulativeLast_.commit();
  this->kLast_.commit();

  this->registerContractFunctions();

  this->factory_.enableRegister();
  this->token0_.enableRegister();
  this->token1_.enableRegister();
  this->reserve0_.enableRegister();
  this->reserve1_.enableRegister();
  this->blockTimestampLast_ .enableRegister();
  this->price0CumulativeLast_.enableRegister();
  this->price1CumulativeLast_.enableRegister();
  this->kLast_.enableRegister();
}

DEXV2Pair::DEXV2Pair(
  const Address& address, const Address& creator, const uint64_t& chainId
) : DynamicContract("DEXV2Pair", address, creator, chainId),
  ERC20("DEXV2Pair", "DEX V2", "DEX-V2", 18, 0, address, creator, chainId),
  factory_(this), token0_(this), token1_(this), reserve0_(this), reserve1_(this),
  blockTimestampLast_(this), price0CumulativeLast_(this), price1CumulativeLast_(this), kLast_(this)
{
  // Explicitly initialize numbers to 0 to avoid junk values on DB load
  this->factory_ = creator;
  this->reserve0_ = 0;
  this->reserve1_ = 0;
  this->blockTimestampLast_ = 0;
  this->price0CumulativeLast_ = 0;
  this->price1CumulativeLast_ = 0;
  this->kLast_ = 0;

  this->factory_.commit();
  this->token0_.commit();
  this->token1_.commit();
  this->reserve0_.commit();
  this->reserve1_.commit();
  this->blockTimestampLast_ .commit();
  this->price0CumulativeLast_.commit();
  this->price1CumulativeLast_.commit();
  this->kLast_.commit();

  this->registerContractFunctions();

  this->factory_.enableRegister();
  this->token0_.enableRegister();
  this->token1_.enableRegister();
  this->reserve0_.enableRegister();
  this->reserve1_.enableRegister();
  this->blockTimestampLast_ .enableRegister();
  this->price0CumulativeLast_.enableRegister();
  this->price1CumulativeLast_.enableRegister();
  this->kLast_.enableRegister();
}

DEXV2Pair::~DEXV2Pair() {};

void DEXV2Pair::registerContractFunctions() {
  registerContract();
  this->registerMemberFunctions(
    std::make_tuple("initialize", &DEXV2Pair::initialize, FunctionTypes::NonPayable, this),
    std::make_tuple("getReserves", &DEXV2Pair::getReserves, FunctionTypes::View, this),
    std::make_tuple("factory", &DEXV2Pair::factory, FunctionTypes::View, this),
    std::make_tuple("token0", &DEXV2Pair::token0, FunctionTypes::View, this),
    std::make_tuple("token1", &DEXV2Pair::token1, FunctionTypes::View, this),
    std::make_tuple("price0CumulativeLast", &DEXV2Pair::price0CumulativeLast, FunctionTypes::View, this),
    std::make_tuple("price1CumulativeLast", &DEXV2Pair::price1CumulativeLast, FunctionTypes::View, this),
    std::make_tuple("kLast", &DEXV2Pair::kLast, FunctionTypes::View, this),
    std::make_tuple("mint", &DEXV2Pair::mint, FunctionTypes::NonPayable, this),
    std::make_tuple("burn", &DEXV2Pair::burn, FunctionTypes::NonPayable, this),
    std::make_tuple("swap", &DEXV2Pair::swap, FunctionTypes::NonPayable, this),
    std::make_tuple("skim", &DEXV2Pair::skim, FunctionTypes::NonPayable, this),
    std::make_tuple("sync", &DEXV2Pair::sync, FunctionTypes::NonPayable, this)
  );
}

void DEXV2Pair::_safeTransfer(const Address& token, const Address& to, const uint256_t& value) {
  this->callContractFunction(token, &ERC20::transfer, to, value);
}

void DEXV2Pair::_update(const uint256_t& balance0, const uint256_t& balance1, const uint256_t& reserve0, const uint256_t& reserve1) {
  // Timestamp is in microseconds, we want in seconds
  auto blockTimestamp = uint32_t(ContractGlobals::getBlockTimestamp() / 1000000);
  if (
    uint32_t timeElapsed = blockTimestamp - this->blockTimestampLast_.get();
    timeElapsed > 0 && reserve0 != 0 && reserve1 != 0
  ) {
    this->price0CumulativeLast_ += uint256_t(UQ112x112::uqdiv(UQ112x112::encode(uint112_t(reserve1)), uint112_t(reserve0))) * timeElapsed;
    this->price1CumulativeLast_ += uint256_t(UQ112x112::uqdiv(UQ112x112::encode(uint112_t(reserve0)), uint112_t(reserve1))) * timeElapsed;
  }
  this->reserve0_ = uint112_t(balance0);
  this->reserve1_ = uint112_t(balance1);
  this->blockTimestampLast_ = blockTimestamp;
}

bool DEXV2Pair::_mintFee(uint112_t reserve0, uint112_t reserve1) {
  Address feeTo = this->callContractViewFunction(this->factory_.get(), &DEXV2Factory::feeTo);
  bool feeOn = feeTo ? true : false;
  uint256_t _kLast = this->kLast_.get();
  if (feeOn && _kLast != 0) {
    uint256_t rootK = boost::multiprecision::sqrt(uint256_t(reserve0) * uint256_t(reserve1));
    uint256_t rootKLast = boost::multiprecision::sqrt(_kLast);
    if (rootK > rootKLast) {
      uint256_t numerator = this->totalSupply_.get() * (rootK - rootKLast);
      uint256_t denominator = rootK * 5 + rootKLast;
      uint256_t liquidity = numerator / denominator;
      if (liquidity > 0) this->mintValue_(feeTo, liquidity);
    }
  } else if (_kLast != 0) {
    this->kLast_ = 0;
  }
  return feeOn;
}

void DEXV2Pair::initialize(const Address& token0, const Address& token1) {
  if (this->factory_ != this->getCaller()) throw DynamicException("DEXV2Pair: FORBIDDEN");
  this->token0_ = token0;
  this->token1_ = token1;
}

std::tuple<uint256_t, uint256_t, uint256_t> DEXV2Pair::getReserves() const {
  return std::make_tuple(this->reserve0_.get(), this->reserve1_.get(), this->blockTimestampLast_.get());
}

Address DEXV2Pair::factory() const { return this->factory_.get(); }

Address DEXV2Pair::token0() const { return this->token0_.get(); }

Address DEXV2Pair::token1() const { return this->token1_.get(); }

uint256_t DEXV2Pair::price0CumulativeLast() const { return this->price0CumulativeLast_.get(); }

uint256_t DEXV2Pair::price1CumulativeLast() const { return this->price1CumulativeLast_.get(); }

uint256_t DEXV2Pair::kLast() const { return this->kLast_.get(); }

uint256_t DEXV2Pair::mint(const Address& to) {
  ReentrancyGuard reentrancyGuard(this->reentrancyLock_);
  uint256_t liquidity = 0;
  uint256_t balance0 = this->callContractViewFunction(this->token0_.get(), &ERC20::balanceOf, this->getContractAddress());
  uint256_t balance1 = this->callContractViewFunction(this->token1_.get(), &ERC20::balanceOf, this->getContractAddress());
  uint256_t amount0 = balance0 - this->reserve0_.get();
  uint256_t amount1 = balance1 - this->reserve1_.get();

  bool feeOn = this->_mintFee(this->reserve0_.get(), this->reserve1_.get());
  if (uint256_t totalSupply = this->totalSupply_.get(); totalSupply == 0) {
    // Permanently lock the first MINIMUM_LIQUIDITY tokens
    liquidity = boost::multiprecision::sqrt(amount0 * amount1) - MINIMUM_LIQUIDITY;
    this->mintValue_(Address(Hex::toBytes("0x0000000000000000000000000000000000000000")), MINIMUM_LIQUIDITY);
  } else {
    liquidity = std::min(amount0 * totalSupply / this->reserve0_.get(), amount1 * totalSupply / this->reserve1_.get());
  }

  if (liquidity == 0) throw DynamicException("DEXV2Pair: INSUFFICIENT_LIQUIDITY_MINTED");
  this->mintValue_(to, liquidity);
  this->_update(balance0, balance1, this->reserve0_.get(), this->reserve1_.get());
  if (feeOn) this->kLast_ = uint256_t(this->reserve0_.get()) * uint256_t(this->reserve1_.get());
  return liquidity;
}

std::tuple<uint256_t, uint256_t> DEXV2Pair::burn(const Address& to) {
  ReentrancyGuard reentrancyGuard(this->reentrancyLock_);
  uint256_t balance0 = this->callContractViewFunction(
    this->token0_.get(), &ERC20::balanceOf, this->getContractAddress()
  );
  uint256_t balance1 = this->callContractViewFunction(
    this->token1_.get(), &ERC20::balanceOf, this->getContractAddress()
  );
  uint256_t liquidity = this->balanceOf(this->getContractAddress());

  bool feeOn = this->_mintFee(this->reserve0_.get(), this->reserve1_.get());
  uint256_t totalSupply = this->totalSupply_.get();
  uint256_t amount0 = liquidity * balance0 / totalSupply;
  uint256_t amount1 = liquidity * balance1 / totalSupply;
  if (amount0 == 0 || amount1 == 0) throw DynamicException("DEXV2Pair: INSUFFICIENT_LIQUIDITY_BURNED");
  this->burnValue_(this->getContractAddress(), liquidity);
  this->_safeTransfer(this->token0_.get(), to, amount0);
  this->_safeTransfer(this->token1_.get(), to, amount1);
  balance0 = this->callContractViewFunction(this->token0_.get(), &ERC20::balanceOf, this->getContractAddress());
  balance1 = this->callContractViewFunction(this->token1_.get(), &ERC20::balanceOf, this->getContractAddress());
  this->_update(balance0, balance1, this->reserve0_.get(), this->reserve1_.get());
  if (feeOn) kLast_ = uint256_t(this->reserve0_.get()) * uint256_t(this->reserve1_.get());
  return std::make_tuple(amount0, amount1);
}

void DEXV2Pair::swap(const uint256_t& amount0Out, const uint256_t& amount1Out, const Address& to) {
  ReentrancyGuard reentrancyGuard(this->reentrancyLock_);
  if (amount0Out == 0 && amount1Out == 0) throw DynamicException("DEXV2Pair: INSUFFICIENT_OUTPUT_AMOUNT");
  if (reserve0_ <= uint112_t(amount0Out) && reserve1_ <= uint112_t(amount1Out)) throw DynamicException("DEXV2Pair: INSUFFICIENT_LIQUIDITY");
  if (token0_ == to || token1_ == to) throw DynamicException("DEXV2Pair: INVALID_TO");
  if (amount0Out > 0) this->_safeTransfer(this->token0_.get(), to, amount0Out);
  if (amount1Out > 0) this->_safeTransfer(this->token1_.get(), to, amount1Out);
  uint256_t balance0 = this->callContractViewFunction(this->token0_.get(), &ERC20::balanceOf, this->getContractAddress());
  uint256_t balance1 = this->callContractViewFunction(this->token1_.get(), &ERC20::balanceOf, this->getContractAddress());
  uint256_t amount0In = balance0 > this->reserve0_.get() - uint112_t(amount0Out) ? balance0 - this->reserve0_.get() + uint112_t(amount0Out) : 0;
  uint256_t amount1In = balance1 > this->reserve1_.get() - uint112_t(amount1Out) ? balance1 - this->reserve1_.get() + uint112_t(amount1Out) : 0;
  if (amount0In == 0 && amount1In == 0) throw DynamicException("DEXV2Pair: INSUFFICIENT_INPUT_AMOUNT");
  uint256_t balance0Adjusted = balance0 * 1000 - amount0In * 3;
  uint256_t balance1Adjusted = balance1 * 1000 - amount1In * 3;
  if (
    uint256_t balTotal = balance0Adjusted * balance1Adjusted;
    balTotal < uint256_t(this->reserve0_.get()) * this->reserve1_.get() * 1000 * 1000
  ) {
    throw DynamicException("DEXV2Pair: K");
  }
  this->_update(balance0, balance1, this->reserve0_.get(), this->reserve1_.get());
}

void DEXV2Pair::skim(const Address &to) {
  ReentrancyGuard reentrancyGuard(this->reentrancyLock_);
  this->_safeTransfer(this->token0_.get(), to, this->callContractViewFunction(
    this->token0_.get(), &ERC20::balanceOf, this->getContractAddress()
  ) - this->reserve0_.get());
  this->_safeTransfer(this->token1_.get(), to, this->callContractViewFunction(
    this->token1_.get(), &ERC20::balanceOf, this->getContractAddress()
  ) - this->reserve1_.get());
}

void DEXV2Pair::sync() {
  ReentrancyGuard reentrancyGuard(this->reentrancyLock_);
  this->_update(
    this->callContractViewFunction(this->token0_.get(), &ERC20::balanceOf, this->getContractAddress()),
    this->callContractViewFunction(this->token1_.get(), &ERC20::balanceOf, this->getContractAddress()),
    this->reserve0_.get(), this->reserve1_.get()
  );
}


DBBatch DEXV2Pair::dump() const {
  // We have to dump the tokens as well
  DBBatch dbBatch = BaseContract::dump();
  DBBatch erc20Batch = ERC20::dump();
  for (const auto& dbItem : erc20Batch.getPuts()) dbBatch.push_back(dbItem);
  for (const auto& dbItem : erc20Batch.getDels()) dbBatch.delete_key(dbItem);

  dbBatch.push_back(StrConv::stringToBytes("factory_"), this->factory_.get().view(), this->getDBPrefix());
  dbBatch.push_back(StrConv::stringToBytes("token0_"), this->token0_.get().view(), this->getDBPrefix());
  dbBatch.push_back(StrConv::stringToBytes("token1_"), this->token1_.get().view(), this->getDBPrefix());
  dbBatch.push_back(StrConv::stringToBytes("reserve0_"), UintConv::uint112ToBytes(this->reserve0_.get()), this->getDBPrefix());
  dbBatch.push_back(StrConv::stringToBytes("reserve1_"), UintConv::uint112ToBytes(this->reserve1_.get()), this->getDBPrefix());
  dbBatch.push_back(StrConv::stringToBytes("blockTimestampLast_"), UintConv::uint32ToBytes(this->blockTimestampLast_.get()), this->getDBPrefix());
  dbBatch.push_back(StrConv::stringToBytes("price0CumulativeLast_"), UintConv::uint256ToBytes(this->price0CumulativeLast_.get()), this->getDBPrefix());
  dbBatch.push_back(StrConv::stringToBytes("price1CumulativeLast_"), UintConv::uint256ToBytes(this->price1CumulativeLast_.get()), this->getDBPrefix());
  dbBatch.push_back(StrConv::stringToBytes("kLast_"), UintConv::uint256ToBytes(this->kLast_.get()), this->getDBPrefix());
  return dbBatch;
}

