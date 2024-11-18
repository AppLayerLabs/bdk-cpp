/*
  Copyright (c) [2023] [Sparq Network]

  This software is distributed under the MIT License.
  See the LICENSE.txt file in the project root for more information.
*/

#include "orderbook.h"

OrderBook::OrderBook(const Address& addA, const std::string& tickerA,
                     const Address& addB, const std::string& tickerB,
                     const Address& address, const Address& creator,
                     const uint64_t& chainId)
  : DynamicContract("OrderBook", address, creator, chainId),
    nextOrderID_(this),
    addressAssetA_(this),
    tickerAssetA_(this),
    addressAssetB_(this),
    tickerAssetB_(this),
    spread_(this)
{
  this->nextOrderID_ = 0;
  this->addressAssetA_ = addA;
  this->addressAssetB_ = addB;
  this->tickerAssetA_ = tickerA;
  this->tickerAssetB_ = tickerB;
  this->spread_ = 0;
  this->lastPrice_ = 0;
  this->lotSize_ = 0;
  this->tickSize_ = 0;
  this->nextOrderID_.commit();
  this->addressAssetA_.commit();
  this->addressAssetB_.commit();
  this->tickerAssetA_.commit();
  this->tickerAssetB_.commit();
  this->spread_.commit();
  this->lastPrice_.commit();
  this->lotSize_.commit();
  this->tickSize_.commit();

  this->registerContractFunctions();

  this->nextOrderID_.enableRegister();
  this->addressAssetA_.enableRegister();
  this->addressAssetB_.enableRegister();
  this->tickerAssetA_.enableRegister();
  this->tickerAssetB_.enableRegister();
  this->spread_.enableRegister();
  this->lastPrice_.enableRegister();
  this->lotSize_.enableRegister();
  this->tickSize_.enableRegister();
}

OrderBook::OrderBook(
  const Address& address,
  const DB &db
  ) : DynamicContract(address, db),
      nextOrderID_(this),
      addressAssetA_(this),
      tickerAssetA_(this),
      addressAssetB_(this),
      tickerAssetB_(this),
      spread_(this)
{
  this->nextOrderID_ = Utils::bytesToUint256(db.get(std::string("nextOrderID_"), this->getDBPrefix()));
  this->addressAssetA_ = Address(db.get(std::string("addressAssetA_"), this->getDBPrefix()));
  this->addressAssetB_ = Address(db.get(std::string("addressAssetB_"), this->getDBPrefix()));
  this->tickerAssetA_ = Utils::bytesToString(db.get(std::string("tickerAssetA_"), this->getDBPrefix()));
  this->tickerAssetB_ = Utils::bytesToString(db.get(std::string("tickerAssetB_"), this->getDBPrefix()));
  this->spread_ = Utils::bytesToUint256(db.get(std::string("spread_"), this->getDBPrefix()));
  this->tickSize_ = Utils::bytesToUint256(db.get(std::string("tickSize_"), this->getDBPrefix()));
  this->lotSize_ = Utils::bytesToUint256(db.get(std::string("lotSize_"), this->getDBPrefix()));
  this->lastPrice_ = Utils::bytesToUint256(db.get(std::string("lastPrice_"), this->getDBPrefix()));
  this->registerContractFunctions();
}

inline void OrderBook::insertAskOrder(const Order& askOrder)
{
  this->asks_.insert(std::move(askOrder));
}

inline void OrderBook::insertBidOrder(const Order& bidOrder)
{
  this->bids_.insert(std::move(bidOrder));
}

inline void OrderBook::eraseAskOrder(const Order& askOrder)
{
  this->asks_.erase(askOrder);
}

inline void OrderBook::eraseBidOrder(const Order& bidOrder)
{
  this->bids_.erase(bidOrder);
}

void OrderBook::executeOrder(const Address& askOwner,
                             const Address& bidOwner,
                             uint256_t tokenAmount,
                             uint256_t lotAmount)
{
  this->callContractFunction(this->addressAssetB_.get(), &ERC20::transfer, askOwner, tokenAmount);
  this->callContractFunction(this->addressAssetA_.get(), &ERC20::transfer, bidOwner, this->convertLot(lotAmount));
}

Order* OrderBook::findMatchAskOrder(const Order& bidOrder)
{
  auto& bidAssetAmount = std::get<3>(bidOrder);
  // do we have any asks orders?
  // bid order was filled already?
  if (this->asks_.empty() || bidAssetAmount == 0) {
    // not found
    return nullptr;
  }
  // get the first ask order
  const Order* askOrder = &(*(this->asks_.begin()));
  auto& askAssetPrice = std::get<4>(*askOrder);
  auto& bidAssetPrice = std::get<4>(bidOrder);
  // we want to buy for the lower price
  // at the limit bid asset price
  if (bidAssetPrice <= askAssetPrice) {
    return const_cast<Order*>(askOrder);
  }
  // not found
  return nullptr;
}

Order* OrderBook::findMatchBidOrder(const Order& askOrder)
{
  auto& askAssetAmount = std::get<3>(askOrder);
  // do we have any asks orders?
  // ask order was filled already?
  if (this->bids_.empty() || askAssetAmount == 0) {
    return nullptr;
  }
  // get the first bid order
  const Order* bidOrder = &(*(this->bids_.begin()));
  // we want to sell for the higher bid price
  // but never not lower to the ask price limit
  auto& bidAssetPrice = std::get<4>(*bidOrder);
  auto& askAssetPrice = std::get<4>(askOrder);
  // we should not sell below the ask price limit
  if (bidAssetPrice >= askAssetPrice) {
    return const_cast<Order*>(bidOrder);
  }
  // not found
  return nullptr;
}

inline uint256_t OrderBook::convertToken(const uint256_t& assetAmount,
                                         const uint256_t& assetPrice) const
{
  return this->convertTick(assetAmount * assetPrice) / this->precision_;
}

void OrderBook::evaluateBidLimitOrder(Order& bidOrder)
{
  Order *matchAskOrder = findMatchAskOrder(bidOrder);
  auto& bidAssetAmount = std::get<3>(bidOrder);
  if (matchAskOrder != nullptr) {
    // get bid order attributes values
    const auto& bidOwner = std::get<2>(bidOrder);
    auto& bidAssetPrice = std::get<4>(bidOrder);
    // aggressive order, will be executed
    // the side effect here will be the transfer from asks order owner
    // to the bid owner order
    // and the askOrder can be altered leaving a remainder in its assetAmount
    // or remove from the asks set in case it's filled
    // if the bidOrder is not filled it will the added to the bid order set
    do {
      // get ask order attributes values
      const auto& askOwner = std::get<2>(*matchAskOrder);
      auto& askAssetAmount = std::get<3>(*matchAskOrder);
      auto& askAssetPrice = std::get<4>(*matchAskOrder);

      // compute the asset and token amount
      uint256_t assetAmount = std::min(askAssetAmount, bidAssetAmount);
      uint256_t tokenAmount = this->convertToken(assetAmount, askAssetPrice);

      // executes the order, transfer the tokens from ask owner to bid owner
      this->executeOrder(askOwner, bidOwner, tokenAmount, assetAmount);

      // update amount information
      bidAssetAmount -= assetAmount;
      askAssetAmount -= assetAmount;

      // erase the ask asset if filled
      if (askAssetAmount == 0) {
        this->eraseAskOrder(*matchAskOrder);
      }
      // find next match ask order
      matchAskOrder = findMatchAskOrder(bidOrder);
    } while (matchAskOrder != nullptr);
  }
  // handle the bid order that was not filled (remainder)
  if (bidAssetAmount > 0) {
    this->insertBidOrder(bidOrder);
  }
}

void OrderBook::evaluateAskLimitOrder(Order& askOrder)
{
  Order *matchBidOrder = findMatchBidOrder(askOrder);
  auto& askAssetAmount = std::get<3>(askOrder);
  if (matchBidOrder != nullptr) {
    const auto& askOwner = std::get<2>(askOrder);
    auto& askAssetPrice = std::get<4>(askOrder);
    do {
      const auto& bidOwner = std::get<2>(*matchBidOrder);
      auto& bidAssetAmount = std::get<3>(*matchBidOrder);
      auto& bidAssetPrice = std::get<4>(*matchBidOrder);
      // compute the asset and token amount
      uint256_t assetAmount = std::min(askAssetAmount, bidAssetAmount);
      uint256_t tokenAmount = this->convertToken(assetAmount, askAssetPrice);
      // update order asset amounts
      askAssetAmount -= assetAmount;
      bidAssetAmount -= assetAmount;
      // erase the bid order filled
      if (bidAssetAmount == 0) {
        this->eraseBidOrder(*matchBidOrder);
      }
      // find next match ask order
      matchBidOrder = findMatchBidOrder(askOrder);
    } while (matchBidOrder != nullptr);
  }
  // handle the bid order that was not filled (remainder)
  if (askAssetAmount > 0) {
    this->insertAskOrder(askOrder);
  }
}

void OrderBook::transferToContract(const Address& assetAddress,
                                   const uint256_t &tokenAmount)
{
  this->callContractFunction(assetAddress,
                             &ERC20::transferFrom,
                             this->getCaller(),
                             this->getContractAddress(),
                             tokenAmount);
}

Order* OrderBook::makeOrder(const uint256_t& assetAmount,
                            const uint256_t& assetPrice)
{
  return (new Order(this->nextOrderID_.get(),
                    this->getCurrentTimestamp(),
                    this->getCaller(),
                    assetAmount,
                    assetPrice));
}

void OrderBook::addBidLimitOrder(const uint256_t& assetAmount,
                                 const uint256_t& assetPrice)
{
  // get caller = owner address
  uint256_t assetBalance = \
    this->callContractViewFunction(this->addressAssetB_.get(),
                                   &ERC20::balanceOf,
                                   this->getCaller());
  // convert to token amount
  uint256_t tokenAmount = this->convertToken(assetAmount, assetPrice);

  // verify the asset balance
  if (tokenAmount > assetBalance) {
    throw std::runtime_error("OrderBook::addBidLimitOrder: INSUFFICIENT_BALANCE");
  }
  // transfer token amount to order book contract
  this->callContractFunction(this->addressAssetB_.get(),
                             &ERC20::transferFrom,
                             this->getCaller(),
                             this->getContractAddress(),
                             tokenAmount);
  // evaluate the bid limit order
  this->evaluateBidLimitOrder(*(this->makeOrder(assetAmount, assetPrice)));
  // increment the order id
  this->nextOrderID_++;
}

void OrderBook::addAskLimitOrder(const uint256_t& assetAmount,
                                 const uint256_t& assetPrice)
{
  // set asset balance
  uint256_t assetBalance = \
    this->callContractViewFunction(this->addressAssetA_.get(),
                                   &ERC20::balanceOf,
                                   this->getCaller());
  // convert to lot amount
  uint256_t lotAmount = this->convertLot(assetAmount);
  // verify if lot amount is bigger than user balance
  if (lotAmount > assetBalance) {
    throw std::runtime_error("OrderBook::addAskLimitOrder: INSUFFICIENT_BALANCE");
  }
  // transfer lot amount to order book contract
  this->callContractFunction(this->addressAssetA_.get(),
                             &ERC20::transferFrom,
                             this->getCaller(),
                             this->getContractAddress(),
                             lotAmount);
  // evaluate the the nearly created ask limit order
  this->evaluateAskLimitOrder(*(this->makeOrder(assetAmount, assetPrice)));
  // increment next order id
  this->nextOrderID_++;
}

void OrderBook::updateLastPrice(const uint256_t &price)
{
  this->lastPrice_ = price;
}

void OrderBook::updateSpreadAndMidPrice()
{
  uint256_t bidPrice = std::get<4>(*this->bids_.cbegin());
  uint256_t askPrice = std::get<4>(*this->asks_.cbegin());
  this->spread_ = ((bidPrice >= askPrice) ?     \
                   bidPrice - askPrice : \
                   askPrice - bidPrice);
}

uint64_t OrderBook::getCurrentTimestamp() const
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

std::vector<Order> OrderBook::getBids() const
{
  std::vector<Order> ret;
  for (const auto& bid : this->bids_) {
    ret.push_back(bid);
  }
  return ret;
}

std::vector<Order> OrderBook::getAsks() const
{
  std::vector<Order> ret;
  for (const auto& ask : this->asks_) {
    ret.push_back(ask);
  }
  return ret;
}

std::tuple<std::vector<Order>,
           std::vector<Order>,
           std::vector<StopOrder>>
OrderBook::getUserOrders(const Address& user) const
{
  std::tuple<std::vector<Order>, std::vector<Order>, std::vector<StopOrder>> ret;
  auto& bids = std::get<0>(ret);
  auto& asks = std::get<1>(ret);
  auto& stops = std::get<2>(ret);
  for (const auto& ask : this->asks_) {
    const auto& askAddress = std::get<2>(ask);
    if (askAddress == user) asks.push_back(ask);
  }
  for (const auto& bid : this->bids_) {
    const auto& bidAddress = std::get<2>(bid);
    if (bidAddress == user) bids.push_back(bid);
  }
  for (const auto& stop : this->stops_) {
    const auto& stopAddress = std::get<2>(stop);
    if (stopAddress == user) stops.push_back(stop);
  }
  return ret;
}

void OrderBook::setDecimals()
{
  uint8_t decA = this->callContractViewFunction(this->addressAssetA_.get(), &ERC20::decimals);
  uint8_t decB = this->callContractViewFunction(this->addressAssetB_.get(), &ERC20::decimals);

  if (decA <= 8 || decB <= 8) {
    throw std::runtime_error("Token decimals must be greater than 8");
  }
  this->lotSize_ = Utils::exp10<uint256_t>(decA - 4);
  this->tickSize_ = Utils::exp10<uint256_t>(decB - 4);
}

void OrderBook::registerContractFunctions()
{
  registerContract();
  this->registerMemberFunction("getNextOrderID", &OrderBook::getNextOrderID, FunctionTypes::View, this);
  this->registerMemberFunction("getAddressAssetA", &OrderBook::getAddressAssetA, FunctionTypes::View, this);
  this->registerMemberFunction("getAddressAssetB", &OrderBook::getAddressAssetB, FunctionTypes::View, this);
  this->registerMemberFunction("getTickerAssetA", &OrderBook::getTickerAssetA, FunctionTypes::View, this);
  this->registerMemberFunction("getTickerAssetB", &OrderBook::getTickerAssetB, FunctionTypes::View, this);
  this->registerMemberFunction("getSpread", &OrderBook::getSpread, FunctionTypes::View, this);
  this->registerMemberFunction("getTickSize", &OrderBook::getTickSize, FunctionTypes::View, this);
  this->registerMemberFunction("getLotSize", &OrderBook::getLotSize, FunctionTypes::View, this);
  this->registerMemberFunction("getLastPrice", &OrderBook::getLastPrice, FunctionTypes::View, this);
  this->registerMemberFunction("getPrecision", &OrderBook::getPrecision, FunctionTypes::View, this);
  this->registerMemberFunction("getBids", &OrderBook::getBids, FunctionTypes::View, this);
  this->registerMemberFunction("getAsks", &OrderBook::getAsks, FunctionTypes::View, this);
  this->registerMemberFunction("getUserOrders", &OrderBook::getUserOrders, FunctionTypes::View, this);
  this->registerMemberFunction("addBidLimitOrder", &OrderBook::addBidLimitOrder, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("addAskLimitOrder", &OrderBook::addAskLimitOrder, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("setDecimals", &OrderBook::setDecimals, FunctionTypes::NonPayable, this);
}

DBBatch OrderBook::dump() const
{
  DBBatch b = BaseContract::dump();

  b.push_back(Utils::stringToBytes("nextOrderID_"), Utils::uint256ToBytes(this->nextOrderID_.get()), this->getDBPrefix());
  b.push_back(Utils::stringToBytes("addressAssetA_"), this->addressAssetA_.get().view(), this->getDBPrefix());
  b.push_back(Utils::stringToBytes("addressAssetB_"), this->addressAssetB_.get().view(), this->getDBPrefix());
  b.push_back(Utils::stringToBytes("tickerAssetA_"), Utils::stringToBytes(this->tickerAssetA_.get()), this->getDBPrefix());
  b.push_back(Utils::stringToBytes("tickerAssetB_"), Utils::stringToBytes(this->tickerAssetB_.get()), this->getDBPrefix());
  b.push_back(Utils::stringToBytes("spread_"), Utils::uint256ToBytes(this->spread_.get()), this->getDBPrefix());
  b.push_back(Utils::stringToBytes("tickSize_"), Utils::uint256ToBytes(this->tickSize_.get()), this->getDBPrefix());
  b.push_back(Utils::stringToBytes("lotSize_"), Utils::uint256ToBytes(this->lotSize_.get()), this->getDBPrefix());
  b.push_back(Utils::stringToBytes("lastPrice_"), Utils::uint256ToBytes(this->lastPrice_.get()), this->getDBPrefix());

  return b;
}

