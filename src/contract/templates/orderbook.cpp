/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "orderbook.h"

OrderBook::OrderBook(
  const Address addA, const std::string& tickerA,
  const Address addB, const std::string& tickerB,
  ContractManagerInterface &interface, const Address& address,
  const Address& creator, const uint64_t& chainId,
  const std::unique_ptr<DB> &db
) : DynamicContract(interface, "OrderBook", address, creator, chainId, db), nextOrderID_(this),
  addressAssetA_(this), tickerAssetA_(this), addressAssetB_(this), tickerAssetB_(this),
  marketPrice_(this), midPrice_(this), spread_(this)
{
  this->nextOrderID_ = 0;
  this->addressAssetA_ = addA;
  this->addressAssetB_ = addB;
  this->tickerAssetA_ = tickerA;
  this->tickerAssetB_ = tickerB;
  this->marketPrice_ = 0;
  this->midPrice_ = 0;
  this->spread_ = 0;
  registerContractFunctions();
}

OrderBook::OrderBook(
  ContractManagerInterface &interface,
  const Address& address,
  const std::unique_ptr<DB> &db
) : DynamicContract(interface, address, db), nextOrderID_(this),
  addressAssetA_(this), tickerAssetA_(this), addressAssetB_(this), tickerAssetB_(this),
  marketPrice_(this), midPrice_(this), spread_(this)
{
  this->nextOrderID_ = Utils::bytesToUint256(this->db_->get(std::string("nextOrderID_"), this->getDBPrefix()));
  this->addressAssetA_ = Address(this->db_->get(std::string("addressAssetA_"), this->getDBPrefix()));
  this->addressAssetB_ = Address(this->db_->get(std::string("addressAssetB_"), this->getDBPrefix()));
  this->tickerAssetA_ = Utils::bytesToString(this->db_->get(std::string("tickerAssetA_"), this->getDBPrefix()));
  this->tickerAssetB_ = Utils::bytesToString(this->db_->get(std::string("tickerAssetB_"), this->getDBPrefix()));
  this->marketPrice_ = Utils::bytesToUint256(this->db_->get(std::string("marketPrice_"), this->getDBPrefix()));
  this->midPrice_ = Utils::bytesToUint256(this->db_->get(std::string("midPrice_"), this->getDBPrefix()));
  this->spread_ = Utils::bytesToUint256(this->db_->get(std::string("spread_"), this->getDBPrefix()));
  registerContractFunctions();
}

OrderBook::~OrderBook() {
  DBBatch b;
  b.push_back(Utils::stringToBytes("nextOrderID_"), Utils::uint256ToBytes(this->nextOrderID_.get()), this->getDBPrefix());
  b.push_back(Utils::stringToBytes("addressAssetA_"), this->addressAssetA_.get().get(), this->getDBPrefix());
  b.push_back(Utils::stringToBytes("addressAssetB_"), this->addressAssetB_.get().get(), this->getDBPrefix());
  b.push_back(Utils::stringToBytes("tickerAssetA_"), Utils::stringToBytes(this->tickerAssetA_.get()), this->getDBPrefix());
  b.push_back(Utils::stringToBytes("tickerAssetB_"), Utils::stringToBytes(this->tickerAssetB_.get()), this->getDBPrefix());
  b.push_back(Utils::stringToBytes("marketPrice_"), Utils::uint256ToBytes(this->marketPrice_.get()), this->getDBPrefix());
  b.push_back(Utils::stringToBytes("midPrice_"), Utils::uint256ToBytes(this->midPrice_.get()), this->getDBPrefix());
  b.push_back(Utils::stringToBytes("spread_"), Utils::uint256ToBytes(this->spread_.get()), this->getDBPrefix());
  this->db_->putBatch(b);
}

void OrderBook::newBidOrder(
  const Address owner, const uint256_t amountAssetA, const uint256_t amountAssetB, bool stop
) {
  // Determine order type automatically by the difference between order price and market price
  OrderType type;
  if (amountAssetB >= this->marketPrice_.get()) {
    type = (stop) ? OrderType::STOP : OrderType::MARKET;
  } else if (amountAssetB < this->marketPrice_.get()) {
    type = (stop) ? OrderType::STOPLIMIT : OrderType::LIMIT;
  }

  // Build the order and decide what to do with it
  Order bid(this->nextOrderID_.get(), type, this->getCurrentTimestamp(), owner, amountAssetA, amountAssetB);
  switch (bid.type_) {
    case OrderType::MARKET: this->processMarketBidOrder(bid); break;
    case OrderType::LIMIT: this->bids_.insert(std::move(bid)); break;
    case OrderType::STOP:
    case OrderType::STOPLIMIT: this->stopBids_.insert(std::move(bid)); break;
  }

  // Update the order book accordingly
  this->nextOrderID_++;
  this->clearFilledOrders();
  this->triggerStopOrders();
  this->updateSpreadAndMidPrice();
}

void OrderBook::newAskOrder(
  const Address owner, const uint256_t amountAssetA, const uint256_t amountAssetB, bool stop
) {
  // Determine order type automatically by the difference between order price and market price
  OrderType type;
  if (amountAssetB <= this->marketPrice_.get()) {
    type = (stop) ? OrderType::STOP : OrderType::MARKET;
  } else if (amountAssetB > this->marketPrice_.get()) {
    type = (stop) ? OrderType::STOPLIMIT : OrderType::LIMIT;
  }

  // Build the order and decide what to do with it
  Order ask(this->nextOrderID_.get(), type, this->getCurrentTimestamp(), owner, amountAssetA, amountAssetB);
  switch (ask.type_) {
    case OrderType::MARKET: this->processMarketAskOrder(ask); break;
    case OrderType::LIMIT: this->asks_.insert(std::move(ask)); break;
    case OrderType::STOP:
    case OrderType::STOPLIMIT: this->stopAsks_.insert(std::move(ask)); break;
  }

  // Update the order book accordingly
  this->nextOrderID_++;
  this->clearFilledOrders();
  this->triggerStopOrders();
  this->updateSpreadAndMidPrice();
}

void OrderBook::cancelBidOrder(const uint256_t id) {
  this->bids_.erase_if([&id, this](const Order& o) {
    return o.id_ == id && o.owner_ == this->getCaller();
  });
  this->stopBids_.erase_if([&id, this](const Order& o) {
    return o.id_ == id && o.owner_ == this->getCaller();
  });
  this->updateSpreadAndMidPrice();
}

void OrderBook::cancelAskOrder(const uint256_t id) {
  this->asks_.erase_if([&id, this](const Order& o) {
    return o.id_ == id && o.owner_ == this->getCaller();
  });
  this->stopAsks_.erase_if([&id, this](const Order& o) {
    return o.id_ == id && o.owner_ == this->getCaller();
  });
  this->updateSpreadAndMidPrice();
}

void OrderBook::processMarketBidOrder(Order& b) {
  // Keep executing orders until bid quant is zeroed or bid price is hit
  for (auto it = this->asks_.begin(); it != this->asks_.end(); it++) {
    Order a = *it;
    if (b.amountAssetA_ == 0 || a.amountAssetB_ > b.amountAssetB_) break;
    if (a.amountAssetA_ == 0) continue;
    if (this->executeTrade(b, a)) {
      // Replace pre-exec order with post-exec order and trigger any stop orders if they exist
      it = this->asks_.erase(it);
      this->asks_.emplace_hint(it, a);
    }
  }
  // If bid still has quant, convert to limit
  if (b.amountAssetA_ > 0) { b.type_ = OrderType::LIMIT; this->bids_.insert(std::move(b)); }
}

void OrderBook::processMarketAskOrder(Order& a) {
  // Keep executing orders until ask quant is zeroed or ask price is hit
  for (auto it = this->bids_.begin(); it != this->bids_.end(); it++) {
    Order b = *it;
    if (a.amountAssetA_ == 0 || b.amountAssetB_ < a.amountAssetB_) break;
    if (b.amountAssetA_ == 0) continue;
    if (this->executeTrade(b, a)) {
      // Replace pre-exec order with post-exec order and trigger any stop orders if they exist
      it = this->bids_.erase(it);
      this->bids_.emplace_hint(it, a);
    }
  }
  // If ask still has quant, convert to limit
  if (a.amountAssetA_ > 0) { a.type_ = OrderType::LIMIT; this->asks_.insert(std::move(a)); }
}

bool OrderBook::executeTrade(Order& bid, Order& ask) {
  // Check both orders' quantities and prices to see how much they can be fulfilled.
  // Lowest quant/price on whichever side has the priority.
  // TODO: check if this calculation is actually right - should it be whole (just price) or per unit (quant * price)?
  uint256_t quant = (bid.amountAssetA_ <= ask.amountAssetA_) ? bid.amountAssetA_ : ask.amountAssetA_;
  uint256_t price = (bid.amountAssetB_ <= ask.amountAssetB_) ? bid.amountAssetB_ : ask.amountAssetB_;

  // Attempt the trade (bid before ask)
  try {
    this->callContractFunction(this->addressAssetB_.get(), &ERC20::transferFrom, bid.owner_, ask.owner_, quant);
    this->callContractFunction(this->addressAssetA_.get(), &ERC20::transferFrom, ask.owner_, bid.owner_, price);
    bid.amountAssetA_ -= quant;
    ask.amountAssetA_ -= quant;
    this->setMarketPrice(price);
    return true;
  } catch (std::exception &e) {
    return false;
  }
}

void OrderBook::clearFilledOrders() {
  this->bids_.erase_if([](const Order& o) { return o.amountAssetA_ == 0; });
  this->asks_.erase_if([](const Order& o) { return o.amountAssetA_ == 0; });
}

void OrderBook::triggerStopOrders() {
  for (auto it = this->stopBids_.begin(); it != this->stopBids_.end(); it++) {
    if (it->amountAssetB_ >= this->marketPrice_.get()) {
      Order bid = this->stopBids_.extract(it).value();
      if (bid.type_ == OrderType::STOP) {
        bid.type_ = OrderType::MARKET;
        this->processMarketBidOrder(bid);
      } else if (bid.type_ == OrderType::STOPLIMIT) {
        bid.type_ = OrderType::LIMIT;
        this->bids_.insert(std::move(bid));
      }
    }
  }
  for (auto it = this->stopAsks_.begin(); it != this->stopAsks_.end(); it++) {
    if (it->amountAssetB_ <= this->marketPrice_.get()) {
      Order ask = this->stopAsks_.extract(it).value();
      if (ask.type_ == OrderType::STOP) {
        ask.type_ = OrderType::MARKET;
        this->processMarketAskOrder(ask);
      } else if (ask.type_ == OrderType::STOPLIMIT) {
        ask.type_ = OrderType::LIMIT;
        this->asks_.insert(std::move(ask));
      }
    }
  }
}

void OrderBook::updateSpreadAndMidPrice() {
  uint256_t bidPrice = this->bids_.cbegin()->amountAssetB_;
  uint256_t askPrice = this->asks_.cbegin()->amountAssetB_;
  this->spread_ = (bidPrice >= askPrice) ? bidPrice - askPrice : askPrice - bidPrice;
  this->midPrice_ = (bidPrice + askPrice) / 2;
}

uint64_t OrderBook::getCurrentTimestamp() const {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::system_clock::now().time_since_epoch()
  ).count();
}

void OrderBook::registerContractFunctions() {
  registerContract();
  this->registerMemberFunction("getMarketPrice", &OrderBook::getMarketPrice, this);
  this->registerMemberFunction("getMidPrice", &OrderBook::getMidPrice, this);
  this->registerMemberFunction("getSpread", &OrderBook::getSpread, this);
  this->registerMemberFunction("newBidOrder", &OrderBook::newBidOrder, this);
  this->registerMemberFunction("newAskOrder", &OrderBook::newAskOrder, this);
  this->registerMemberFunction("cancelBidOrder", &OrderBook::cancelBidOrder, this);
  this->registerMemberFunction("cancelAskOrder", &OrderBook::cancelAskOrder, this);
}

