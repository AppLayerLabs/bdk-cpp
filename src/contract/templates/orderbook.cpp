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
  const Address& creator, const uint64_t& chainId, const std::unique_ptr<DB> &db
) : DynamicContract(interface, "OrderBook", address, creator, chainId, db), nextOrderID_(this),
  addressAssetA_(this), tickerAssetA_(this), addressAssetB_(this), tickerAssetB_(this), spread_(this)
{
  uint8_t decA = this->callContractViewFunction(addA, &ERC20::decimals);
  uint8_t decB = this->callContractViewFunction(addB, &ERC20::decimals);
  if (decA <= 8 || decB <= 8) throw std::runtime_error("Token decimals must be greater than 8");
  this->nextOrderID_ = 0;
  this->addressAssetA_ = addA;
  this->addressAssetB_ = addB;
  this->tickerAssetA_ = tickerA;
  this->tickerAssetB_ = tickerB;
  this->spread_ = 0;
  this->tickSize_ = Utils::exp10(decB - 4);
  this->lotSize_ = Utils::exp10(decA - 4);
  this->lastPrice_ = 0;
  registerContractFunctions();
}

OrderBook::OrderBook(
  ContractManagerInterface &interface,
  const Address& address,
  const std::unique_ptr<DB> &db
) : DynamicContract(interface, address, db), nextOrderID_(this),
  addressAssetA_(this), tickerAssetA_(this), addressAssetB_(this), tickerAssetB_(this), spread_(this)
{
  this->nextOrderID_ = Utils::bytesToUint256(this->db_->get(std::string("nextOrderID_"), this->getDBPrefix()));
  this->addressAssetA_ = Address(this->db_->get(std::string("addressAssetA_"), this->getDBPrefix()));
  this->addressAssetB_ = Address(this->db_->get(std::string("addressAssetB_"), this->getDBPrefix()));
  this->tickerAssetA_ = Utils::bytesToString(this->db_->get(std::string("tickerAssetA_"), this->getDBPrefix()));
  this->tickerAssetB_ = Utils::bytesToString(this->db_->get(std::string("tickerAssetB_"), this->getDBPrefix()));
  this->spread_ = Utils::bytesToUint256(this->db_->get(std::string("spread_"), this->getDBPrefix()));
  this->tickSize_ = Utils::bytesToUint256(this->db_->get(std::string("tickSize_"), this->getDBPrefix()));
  this->lotSize_ = Utils::bytesToUint256(this->db_->get(std::string("lotSize_"), this->getDBPrefix()));
  this->lastPrice_ = Utils::bytesToUint256(this->db_->get(std::string("lastPrice_"), this->getDBPrefix()));
  registerContractFunctions();
}

OrderBook::~OrderBook() {
  DBBatch b;
  b.push_back(Utils::stringToBytes("nextOrderID_"), Utils::uint256ToBytes(this->nextOrderID_.get()), this->getDBPrefix());
  b.push_back(Utils::stringToBytes("addressAssetA_"), this->addressAssetA_.get().get(), this->getDBPrefix());
  b.push_back(Utils::stringToBytes("addressAssetB_"), this->addressAssetB_.get().get(), this->getDBPrefix());
  b.push_back(Utils::stringToBytes("tickerAssetA_"), Utils::stringToBytes(this->tickerAssetA_.get()), this->getDBPrefix());
  b.push_back(Utils::stringToBytes("tickerAssetB_"), Utils::stringToBytes(this->tickerAssetB_.get()), this->getDBPrefix());
  b.push_back(Utils::stringToBytes("spread_"), Utils::uint256ToBytes(this->spread_.get()), this->getDBPrefix());
  b.push_back(Utils::stringToBytes("tickSize_"), Utils::uint256ToBytes(this->tickSize_.get()), this->getDBPrefix());
  b.push_back(Utils::stringToBytes("lotSize_"), Utils::uint256ToBytes(this->lotSize_.get()), this->getDBPrefix());
  b.push_back(Utils::stringToBytes("lastPrice_"), Utils::uint256ToBytes(this->lastPrice_.get()), this->getDBPrefix());
  this->db_->putBatch(b);
}

// TokenA Decimals 18
// TokenB Decimals 18
// TokenA == Lot Size: 100000000000000 (pow(10, 18-4))
// TokenB == Tick Size: 100000000000000 pow(10, 18-4))
// Current market best ask order:
// 5.6512 TokenA - 9238.2385 TokenB
// Current market best bid order:
// 3.6512 TokenA - 9245.2356
// Total TokenB in the order: 33730.6564112
// lotsAmount = 36512 TokenA
// assetPrice = 92382385 TokenB
// 36512 * 92382385 == 3373065641120
// convertTick(3373065641120) = 3373065641120 * 100000000000000 = 337306564112000000000000000
// 337306564112000000000000000 / 10000
// 33730.656411200000000000

void OrderBook::processLimitBidOrder(Order& b, const OrderType& type) {
  if (type != OrderType::LIMIT && type != OrderType::STOPLIMIT) {
    throw std::runtime_error("OrderBook::processLimitBidOrder: INVALID_ORDER_TYPE");
  }
  uint256_t previousMarketPrice = this->lastPrice_.get();
  uint256_t currentMarketPrice = previousMarketPrice;

  for (auto it = this->asks_.begin(); it != this->asks_.end();) {
    if (b.amountAsset_ == 0) break; // If all lots are filled, stop processing
    Order a = *it;
    if (a.assetPrice_ > b.assetPrice_) break; // If ask price > bid price, stop processing

    // Sell the token from the ask order to the bid order.
    uint256_t lotsAmount = std::min(a.amountAsset_, b.amountAsset_); // How many lots the bid order can buy from this ask order.
    uint256_t tokenBAmount = lotsAmount * a.assetPrice_;
    currentMarketPrice = a.assetPrice_;
    tokenBAmount = this->convertTick(tokenBAmount) / this->precision_;

    // Send the ticks to the ask order owner, and the lots to the bid owner order,
    // respectively, then update the order amounts accordingly.
    this->callContractFunction(this->addressAssetB_.get(), &ERC20::transfer, a.owner_, tokenBAmount);
    this->callContractFunction(this->addressAssetA_.get(), &ERC20::transfer, b.owner_, this->convertLot(lotsAmount));
    b.amountAsset_ -= lotsAmount;
    a.amountAsset_ -= lotsAmount;

    // Replace pre-exec order with post-exec. Iterator is on the next element
    it = this->asks_.erase(it);
    if (a.amountAsset_ > 0) this->asks_.emplace_hint(it, a);
  }

  // Add the bid order to the order book if it still has lots to buy.
  if (type != OrderType::MARKET && b.amountAsset_ > 0) this->bids_.insert(b);

  // When a stop limit order reaches it stop price, it becomes a limit order.
  // The problem with that is that trigger*StopOrder functions are inside a
  // loop checking every order and calling back process*Order functions,
  // which in turn call trigger*StopOrder functions again.
  // This causes the stop limit order to be processed up to undefined times.
  if (currentMarketPrice != previousMarketPrice) {
    this->updateLastPrice(currentMarketPrice);
    // If the best ask price changed we can trigger stop orders.
    if (type != OrderType::STOPLIMIT) this->triggerStopOrders(currentMarketPrice, previousMarketPrice);
  }
}

void OrderBook::processLimitAskOrder(Order& a, const OrderType& type) {
  if (type != OrderType::LIMIT && type != OrderType::STOPLIMIT) {
    throw std::runtime_error("OrderBook::processLimitAskOrder: INVALID_ORDER_TYPE");
  }
  uint256_t previousMarketPrice = this->bids_.begin()->assetPrice_;
  uint256_t currentMarketPrice = previousMarketPrice;

  for (auto it = this->bids_.begin(); it != this->bids_.end();) {
    if (a.amountAsset_ == 0) break; // If all lots are filled, stop processing
    Order b = *it;
    if (b.assetPrice_ < a.assetPrice_) break; // If bid price < ask price, stop processing

    // Sell the token from the ask order to the bid order.
    uint256_t lotsAmount = std::min(a.amountAsset_, b.amountAsset_); // How many lots the bid order can buy from this ask order.
    uint256_t tokenBAmount = lotsAmount * b.assetPrice_;
    currentMarketPrice = b.assetPrice_;
    tokenBAmount = this->convertTick(tokenBAmount) / this->precision_;

    // Send the ticks to the ask order owner, and the lots to the bid owner order,
    // respectively, then update the order amounts accordingly.
    this->callContractFunction(this->addressAssetB_.get(), &ERC20::transfer, a.owner_, tokenBAmount);
    this->callContractFunction(this->addressAssetA_.get(), &ERC20::transfer, b.owner_, this->convertLot(lotsAmount));
    a.amountAsset_ -= lotsAmount;
    b.amountAsset_ -= lotsAmount;

    // Replace pre-exec order with post-exec. Iterator is on the next element
    it = this->bids_.erase(it);
    if (b.amountAsset_ > 0) this->bids_.emplace_hint(it, b);
  }

  // Add the ask order to the order book if it still has lots to sell.
  if (type != OrderType::MARKET && a.amountAsset_ > 0) this->asks_.insert(a);

  // When a stop limit order reaches it stop price, it becomes a limit order.
  // The problem with that is that trigger*StopOrder functions are inside a loop checking every order
  // And calling back process*Order functions, which in turn call trigger*StopOrder functions again.
  // This causes the stop limit order to be processed up to undefined times.
  if (currentMarketPrice != previousMarketPrice) {
    this->updateLastPrice(currentMarketPrice);
    // If the best ask price changed we can trigger stop orders.
    if (type != OrderType::STOPLIMIT) this->triggerStopOrders(currentMarketPrice, previousMarketPrice);
  }
}

// TokenA Decimals 18
// TokenB Decimals 18
// TokenA == Lot Size: 100000000000000 (pow(10, 18-4))
// TokenB == Tick Size: 100000000000000 pow(10, 18-4))
// Current market best ask order:
// 5.6512 TokenA - 9238.2385 TokenB
// Buy order 20000 TokenB at market price: (should receive 2.1649 tokenA).
// amountAsset_ = 20000 0000 /// Remember, we are dealing with "Lots of ticks"
// assetPrice_ = 92382385
// 200000000 * 10000 / 92382385 = /// Max precision of 4 decimals due to lot size.
// 21649 lots (amount of tokenA to transfer)
// 21649 * 92382385 == 1999986252865
// 1999986252865 * 100000000000000 == 199998625286500000000000000
// 199998625286500000000000000 / 10000
// 19999862528650000000000
// 19999.86 2528 6500 0000 0000

void OrderBook::processMarketBuyOrder(Order& b, const OrderType& type) {
  if (type != OrderType::MARKET && type != OrderType::STOPMARKET) {
    throw std::runtime_error("OrderBook::processMarketBuyOrder: INVALID_ORDER_TYPE");
  }
  uint256_t previousMarketPrice = this->lastPrice_.get();
  uint256_t currentMarketPrice = previousMarketPrice;
  uint256_t remainingAssetB = this->convertTick(b.amountAsset_); // Amount of tokenB to spend

  for (auto it = this->asks_.begin(); it != this->asks_.end() && (remainingAssetB > 0 || remainingAssetB < this->tickSize_.get());) {
    // Calculate the amount of tokenA that can be bought with the remaining tokenB
    Order a = *it;
    uint256_t tokenAAmount = remainingAssetB * 10000 / a.assetPrice_; // This value is in lots
    tokenAAmount = std::min(tokenAAmount, a.amountAsset_); // Make sure that the ask order has enough lots
    uint256_t spendableAssetB = this->convertTick(tokenAAmount * a.assetPrice_) / this->precision_; // Value now in token min units
    remainingAssetB -= spendableAssetB; // Update remaining amount of tokenB

    // Execute the trade and update the ask order accordingly
    if (type != OrderType::STOPMARKET) {
      this->callContractFunction(this->addressAssetA_.get(), &ERC20::transferFrom, a.owner_, b.owner_, this->convertLot(tokenAAmount));
    } else {
      this->callContractFunction(this->addressAssetA_.get(), &ERC20::transfer, b.owner_, this->convertLot(tokenAAmount));
    }
    this->callContractFunction(this->addressAssetA_.get(), &ERC20::transfer, b.owner_, this->convertLot(tokenAAmount));
    a.amountAsset_ -= tokenAAmount;
    currentMarketPrice = a.assetPrice_;

    // Erase or update the ask order in the order book
    it = this->asks_.erase(it);
    if (a.amountAsset_ > 0) {
      this->asks_.emplace_hint(it, a);
      break; // We don't need to loop again, partially executed.
    }
  }

  if (currentMarketPrice != previousMarketPrice) {
    this->updateLastPrice(currentMarketPrice);
    if (type != OrderType::STOPMARKET) {
      // If the best ask price changed we can trigger stop orders.
      this->triggerStopOrders(currentMarketPrice, previousMarketPrice);
    }
  }

  // TODO: Return not executed from stop orders (or add to total fee)
}

// TokenA Decimals 18
// TokenB Decimals 18
// TokenA == Lot Size: 100000000000000 (pow(10, 18-4))
// TokenB == Tick Size: 100000000000000 pow(10, 18-4))
// Current market best bid order:
// 5125.6512 TokenA - 9238.2385 TokenB
// 51256512 lots for 92382385 tokenB each
// Sell 4500 TokenA at market price: (should receive 41572073.25 tokenB).
// 45000000 lots
// 45000000 * 92382385 = 4157207325000000
// convertTick(4157207325000000) = 415720732500000000000000000000
// 415720732500000000000000000000 / 10000
// 41572073250000000000000000
// 415720732500.00 0000 0000 0000 0000

void OrderBook::processMarketSellOrder(Order& a, const OrderType& type) {
  if (type != OrderType::MARKET && type != OrderType::STOPMARKET) {
    throw std::runtime_error("OrderBook::processMarketSellOrder: INVALID_ORDER_TYPE");
  }
  uint256_t previousMarketPrice = this->lastPrice_.get();
  uint256_t currentMarketPrice = previousMarketPrice;
  uint256_t remainingLotsToSell = a.amountAsset_;

  for (auto it = this->bids_.begin(); it != this->bids_.end() && (remainingLotsToSell > 0);) {
    // Check how many lots the order can execute
    Order b = *it;
    uint256_t lotsToSell = std::min(remainingLotsToSell, b.amountAsset_);

    // Calculate the amount of tokenB that can be bought with the amount of lots sold
    uint256_t tokenBAmount = lotsToSell * b.assetPrice_;
    tokenBAmount = this->convertTick(tokenBAmount) / this->precision_;

    // Execute the trade and update the bid order accordingly
    this->callContractFunction(this->addressAssetB_.get(), &ERC20::transfer, a.owner_, tokenBAmount);
    this->callContractFunction(this->addressAssetA_.get(), &ERC20::transfer, b.owner_, this->convertLot(lotsToSell));
    b.amountAsset_ -= lotsToSell;
    remainingLotsToSell -= lotsToSell;
    currentMarketPrice = b.assetPrice_;

    // Erase or update the bid order in the order book
    it = this->bids_.erase(it);
    if (b.amountAsset_ > 0) {
      this->bids_.emplace_hint(it, b);
      break; // We don't need to loop again, partially executed.
    }
  }

  if (currentMarketPrice != previousMarketPrice) {
    this->updateLastPrice(currentMarketPrice);
    if (type != OrderType::STOPMARKET) {
      // If the best ask price changed we can trigger stop orders.
      this->triggerStopOrders(currentMarketPrice, previousMarketPrice);
    }
  }
}

// TokenA Decimals 18
// TokenB Decimals 18
// TokenA == Lot Size: 100000000000000 (pow(10, 18-4))
// TokenB == Tick Size: 100000000000000 pow(10, 18-4))
// 1.6493 TokenA  --- 121389 TokenB each 1 BTC
// Meaning I need to send 200206.8777 TokenB to buy 1.6493 TokenA
// 16493 amountAsset --- 1213890000 assetPrice
// 16493 * 1213890000 == 20020687770000
// 20020687770000
// convertTick (20020687770000) == 20020687770000 * 100000000000000 = 2002068777000000000000000000
// 2002068777000000000000000000 / 10000
// 200206.8777 0000 0000 0000 00

// TokenA Decimals 18
// TokenB Decimals 18
// TokenA == Lot Size: 100000000000000 (pow(10, 18-4))
// TokenB == Tick Size: 100000000000000 pow(10, 18-4))
// 1.6493 TokenA  --- 65345.5987 TokenB each 1 BTC
// Meaning I need to send 107774.49593591 TokenB to buy 200206.8777 TokenA
// 16493 amountAsset --- 653455987 assetPrice
// 16493 * 653455987 == 10777449593591
// 10777449593591
// ConvertTick(10777449593591) === 1077744959359100000000000000
// 1077744959359100000000000000 / 10000
// 107774.4959 3591 0000 0000 00

// TokenA Decimals 18
// TokenB Decimals 8
// TokenA == Lot Size: 100000000000000 (pow(10, 18-4))
// TokenB == Tick Size: 10000 pow(10, 8-4))
// 1.6493 TokenA  --- 65345.5987 TokenB each 1 BTC
// Meaning I need to send 107774.49593591 TokenB to buy 200206.8777 TokenA
// 16493 amountAsset --- 653455987 assetPrice
// 16493 * 653455987 == 10777449593591
// 10777449593591
// ConvertTick(10777449593591) === 107774495935910000
// 107774495935910000 / 10000
// 107774.49593591 (amount of tokenB to transfer...)

void OrderBook::newLimitBidOrder(
  const uint256_t& amountAsset, const uint256_t& assetPrice
) {
  uint256_t amountAssetB = amountAsset * assetPrice;
  amountAssetB = this->convertTick(amountAssetB) / this->precision_;

  // Make sure user has enough tokens to buy amountAsset lots
  uint256_t userBalance = this->callContractViewFunction(
    this->addressAssetB_.get(), &ERC20::balanceOf, this->getCaller()
  );
  if (amountAssetB > userBalance) throw std::runtime_error(
    "OrderBook::newLimitBidOrder: INSUFFICIENT_BALANCE"
  );
  this->callContractFunction(
    this->addressAssetB_.get(), &ERC20::transferFrom, this->getCaller(),
    this->getContractAddress(), amountAssetB
  );

  // Build the order and decide what to do with it
  Order bid(this->nextOrderID_.get(), this->getCurrentTimestamp(), this->getCaller(), amountAsset, assetPrice);
  this->processLimitBidOrder(bid, OrderType::LIMIT);
  this->nextOrderID_++;
}

// TokenA Decimals 8
// TokenB Decimals 18
// TokenA == Lot Size: 10000 (pow(10, 18-4))
// TokenB == Tick Size: 100000000000000 pow(10, 18-4))
// 1.6493 TokenA  --- 65345.5987 TokenB each 1 BTC
// Meaning I need to send 107774.49593591 TokenB to buy 200206.8777 TokenA
// 16493 amountAsset --- 653455987 assetPrice
// convertLot(16493) == 16493 * 10000 = 164930000

void OrderBook::newLimitAskOrder(
  const uint256_t& amountAsset, const uint256_t& assetPrice
) {
  // Make sure user has enough lots to sell
  uint256_t userBalance = this->callContractViewFunction(this->addressAssetA_.get(), &ERC20::balanceOf, this->getCaller());
  if (this->convertLot(amountAsset) > userBalance) throw std::runtime_error(
    "OrderBook::newLimitAskOrder: INSUFFICIENT_BALANCE"
  );
  this->callContractFunction(
    this->addressAssetA_.get(), &ERC20::transferFrom, this->getCaller(),
    this->getContractAddress(), this->convertLot(amountAsset)
  );

  // Build the order and decide what to do with it
  Order ask(this->nextOrderID_.get(), this->getCurrentTimestamp(), this->getCaller(), amountAsset, assetPrice);
  this->processLimitAskOrder(ask, OrderType::LIMIT);
  this->nextOrderID_++;
}

void OrderBook::newMarketBuyOrder(const uint256_t& amountAsset) {
  // Make sure user has enough tokens to sell
  uint256_t userBalance = this->callContractViewFunction(
    this->addressAssetB_.get(), &ERC20::balanceOf, this->getCaller()
  );
  if (this->convertTick(amountAsset) > userBalance) throw std::runtime_error(
    "OrderBook::newMarketBuyOrder: INSUFFICIENT_BALANCE"
  );

  // Build the order and decide what to do with it
  Order bid(this->nextOrderID_.get(), this->getCurrentTimestamp(), this->getCaller(), amountAsset, 0);
  this->processMarketBuyOrder(bid, OrderType::MARKET);
  this->nextOrderID_++;
}

void OrderBook::newMarketSellOrder(const uint256_t& amountAsset) {
  // Make sure that the caller has enough tokens to sell.
  uint256_t userBalance = this->callContractViewFunction(
    this->addressAssetA_.get(), &ERC20::balanceOf, this->getCaller()
  );
  if (this->convertLot(amountAsset) > userBalance) throw std::runtime_error(
    "OrderBook::newMarketSellOrder: INSUFFICIENT_BALANCE"
  );

  // Transfer the tokens to the contract, build the order and decide what to do with it
  this->callContractFunction(
    this->addressAssetA_.get(), &ERC20::transferFrom, this->getCaller(),
    this->getContractAddress(), this->convertLot(amountAsset)
  );
  Order ask(this->nextOrderID_.get(), this->getCurrentTimestamp(), this->getCaller(), amountAsset, 0);
  this->processLimitAskOrder(ask, OrderType::MARKET);
  this->nextOrderID_++;
}

void OrderBook::newStopLimitBidOrder(
  const uint256_t& amountAsset, const uint256_t& assetPrice, const uint256_t& stopLimit
) {
  uint256_t amountAssetB = amountAsset * assetPrice;
  amountAssetB = this->convertTick(amountAssetB) / this->precision_;

  // Make sure user has enough tokens to buy amountAsset lots
  uint256_t userBalance = this->callContractViewFunction(
    this->addressAssetB_.get(), &ERC20::balanceOf, this->getCaller()
  );
  if (amountAssetB > userBalance) throw std::runtime_error(
    "OrderBook::newLimitBidOrder: INSUFFICIENT_BALANCE"
  );
  this->callContractFunction(
    this->addressAssetB_.get(), &ERC20::transferFrom, this->getCaller(),
    this->getContractAddress(), amountAssetB
  );

  // Build the order and decide what to do with it
  StopOrder bid(
    this->nextOrderID_.get(), this->getCurrentTimestamp(), this->getCaller(),
    amountAsset, assetPrice, stopLimit, OrderSide::BID, OrderType::STOPLIMIT
  );
  this->stops_.insert(bid);
  this->nextOrderID_++;
}

void OrderBook::newStopLimitAskOrder(
  const uint256_t& amountAsset, const uint256_t& assetPrice, const uint256_t& stopLimit
) {
  // Make sure user has enough lots to sell
  uint256_t userBalance = this->callContractViewFunction(
    this->addressAssetA_.get(), &ERC20::balanceOf, this->getCaller()
  );
  if (this->convertLot(amountAsset) > userBalance) throw std::runtime_error(
    "OrderBook::newLimitAskOrder: INSUFFICIENT_BALANCE"
  );
  this->callContractFunction(
    this->addressAssetA_.get(), &ERC20::transferFrom, this->getCaller(),
    this->getContractAddress(), this->convertLot(amountAsset)
  );

  // Build the order and decide what to do with it
  StopOrder ask(
    this->nextOrderID_.get(), this->getCurrentTimestamp(), this->getCaller(),
    amountAsset, assetPrice, stopLimit, OrderSide::ASK, OrderType::STOPLIMIT
  );
  this->stops_.insert(ask);
  this->nextOrderID_++;
}

void OrderBook::newStopMarketBuyOrder(const uint256_t& amountTokenB, const uint256_t& stopLimit) {
  // Make sure user has enough tokens to sell
  uint256_t userBalance = this->callContractViewFunction(
    this->addressAssetB_.get(), &ERC20::balanceOf, this->getCaller()
  );
  if (this->convertTick(amountTokenB) > userBalance) throw std::runtime_error(
    "OrderBook::newMarketBuyOrder: INSUFFICIENT_BALANCE"
  );

  // Transfer the tokens to the contract, build the order and decide what to do with it
  this->callContractFunction(
    this->addressAssetB_.get(), &ERC20::transferFrom, this->getCaller(),
    this->getContractAddress(), this->convertTick(amountTokenB)
  );
  StopOrder bid(
    this->nextOrderID_.get(), this->getCurrentTimestamp(), this->getCaller(),
    amountTokenB, 0, stopLimit, OrderSide::BID, OrderType::STOPMARKET
  );
  this->stops_.insert(bid);
  this->nextOrderID_++;
}

void OrderBook::newStopMarketSellOrder(const uint256_t& amountAsset, const uint256_t& stopLimit) {
 // Make sure that the caller has enough tokens to sell.
  uint256_t userBalance = this->callContractViewFunction(
    this->addressAssetA_.get(), &ERC20::balanceOf, this->getCaller()
  );
  if (this->convertLot(amountAsset) > userBalance) throw std::runtime_error(
    "OrderBook::newMarketSellOrder: INSUFFICIENT_BALANCE"
  );

  // Transfer the tokens to the contract, build the order and decide what to do with it
  this->callContractFunction(
    this->addressAssetA_.get(), &ERC20::transferFrom, this->getCaller(),
    this->getContractAddress(), this->convertLot(amountAsset)
  );
  StopOrder ask(
    this->nextOrderID_.get(), this->getCurrentTimestamp(), this->getCaller(),
    amountAsset, 0, stopLimit, OrderSide::ASK, OrderType::STOPMARKET
  );
  this->stops_.insert(ask);
  this->nextOrderID_++;
}

void OrderBook::cancelLimitBidOrder(const uint256_t& id) {
  // Additional action for regular bid orders
  auto limitBidAction = [&id, this](const Order& o) {
    if (o.id_ == id) {
      if (o.owner_ != this->getCaller()) throw std::runtime_error(
        "OrderBook::cancelLimitBidOrder: INVALID_OWNER"
      );
      // Return the tokens to the owner.
      // Bid order has sent lots * pricePerLot to contract
      // Example bid order of 56.1235 TokenA for 20.5235 TokenB each.
      // Should return 56.1235 * 20.5235 = 1151.85065225 TokenB
      // 561235 * 205235 == 115185065225
      // convertTick(115185065225) == 115185065225 * 100000000000000 =
      // 11518506522500000000000000 / precision
      // 11518506522500000000000000 / 10000
      // 1151850652250000000000
      // 1151.85 0652 2500 000 00000
      uint256_t amountAssetB = o.amountAsset_ * o.assetPrice_;
      amountAssetB = this->convertTick(amountAssetB) / this->precision_;
      this->callContractFunction(this->addressAssetB_.get(), &ERC20::transfer, o.owner_, amountAssetB);
      return true; // Erase the order
    }
    return false;
  };

  // Additional action for stop bid orders
  auto limitStopAction = [&id, this](const StopOrder& o) {
    if (o.id_ == id) {
      if (o.owner_ != this->getCaller()) {
        throw std::runtime_error("OrderBook::cancelLimitBidOrder: INVALID_OWNER");
      }
      if(o.type_ != OrderType::STOPLIMIT) {
        throw std::runtime_error("OrderBook::cancelLimitBidOrder: INVALID_ORDER_TYPE");
      }
      if (o.side_ != OrderSide::BID) {
        throw std::runtime_error("OrderBook::cancelLimitBidOrder: INVALID_ORDER_SIDE");
      }
      // Return the tokens to the owner.
      uint256_t amountAssetB = o.amountAsset_ * o.assetPrice_;
      amountAssetB = this->convertTick(amountAssetB) / this->precision_;
      this->callContractFunction(this->addressAssetB_.get(), &ERC20::transfer, o.owner_, amountAssetB);
      return true; // Erase the order
    }
    return false;
  };

  this->bids_.erase_if(limitBidAction);
  this->stops_.erase_if(limitStopAction);
  this->updateSpreadAndMidPrice();
}

void OrderBook::cancelLimitAskOrder(const uint256_t& id) {
  auto limitAskAction = [&id, this](const Order& o) {
    if (o.id_ == id) {
      if (o.owner_ != this->getCaller()) {
        throw std::runtime_error("OrderBook::cancelLimitBidOrder: INVALID_OWNER");
      }
      // Return the tokens to the owner.
      this->callContractFunction(
        this->addressAssetA_.get(), &ERC20::transfer, o.owner_, this->convertLot(o.amountAsset_)
      );
      return true; // Erase the order
    }
    return false;
  };

  // Additional action for stop bid orders
  auto limitStopAction = [&id, this](const StopOrder& o) {
    if (o.id_ == id) {
      if (o.owner_ != this->getCaller()) {
        throw std::runtime_error("OrderBook::cancelLimitBidOrder: INVALID_OWNER");
      }
      if(o.type_ != OrderType::STOPLIMIT) {
        throw std::runtime_error("OrderBook::cancelLimitBidOrder: INVALID_ORDER_TYPE");
      }
      if (o.side_ != OrderSide::ASK) {
        throw std::runtime_error("OrderBook::cancelLimitBidOrder: INVALID_ORDER_SIDE");
      }
      // Return the tokens to the owner.
      this->callContractFunction(this->addressAssetA_.get(), &ERC20::transfer, o.owner_, this->convertLot(o.amountAsset_));
      return true; // Erase the order
    }
    return false;
  };

  this->asks_.erase_if(limitAskAction);
  this->stops_.erase_if(limitStopAction);
  this->updateSpreadAndMidPrice();
}

void OrderBook::triggerStopOrders(const uint256_t& currentMarketPrice, const uint256_t& previousMarketPrice) {
  // Trigger stop bid orders
  std::vector<StopOrder> stopsToTrigger;
  bool priceIncreasing = currentMarketPrice > previousMarketPrice;
  if (priceIncreasing) {
    // If the price is increasing, start from the lowest stop price
    for (auto bidIt = stops_.begin(); bidIt != stops_.end();) {
      StopOrder stopBid = *bidIt;
      // Check if the stop condition is met for bid orders
      if (
        (previousMarketPrice > stopBid.stopLimit_ && currentMarketPrice <= stopBid.stopLimit_) ||
        (previousMarketPrice < stopBid.stopLimit_ && currentMarketPrice >= stopBid.stopLimit_)
      ) {
        // Remove the stop order from the list
        bidIt = stops_.erase(bidIt);
        stopsToTrigger.push_back(stopBid);
      } else bidIt++;
    }
  } else {
    for (auto bidIt = stops_.rbegin(); bidIt != stops_.rend();) {
      StopOrder stopBid = *bidIt;
      // Check if the stop condition is met for bid orders
      if (
        (previousMarketPrice > stopBid.stopLimit_ && currentMarketPrice <= stopBid.stopLimit_) ||
        (previousMarketPrice < stopBid.stopLimit_ && currentMarketPrice >= stopBid.stopLimit_)
      ) {
        // Convert reverse iterator to the corresponding forward iterator
        auto itToDel = std::next(bidIt).base();
        stopsToTrigger.push_back(stopBid);
        stops_.erase(itToDel); // Erase using the forward iterator
        bidIt = std::make_reverse_iterator(itToDel); // Update bidIt to continue iteration
      } else bidIt++;
    }
  }

  // Keep track of price changes to trigger stop orders
  uint256_t newPreviousMarketPrice = this->lastPrice_.get();
  for (const auto& stop : stopsToTrigger) {
    // Process the stop order as a limit order
    Order order(stop, this->getBlockTimestamp());
    if (stop.side_ == OrderSide::BID && stop.type_ == OrderType::STOPMARKET) {
      this->processMarketBuyOrder(order, OrderType::STOPMARKET);
    } else if (stop.side_ == OrderSide::BID && stop.type_ == OrderType::STOPLIMIT) {
      this->processLimitBidOrder(order, OrderType::STOPLIMIT);
    } else if (stop.side_ == OrderSide::ASK && stop.type_ == OrderType::STOPMARKET) {
      this->processMarketSellOrder(order, OrderType::STOPMARKET);
    } else if (stop.side_ == OrderSide::ASK && stop.type_ == OrderType::STOPLIMIT) {
      this->processLimitAskOrder(order, OrderType::STOPLIMIT);
    }
  }

  // If the price changed, trigger stop orders again
  uint256_t newCurrentMarketPrice = this->lastPrice_.get();
  if (newCurrentMarketPrice != newPreviousMarketPrice) {
    this->triggerStopOrders(newCurrentMarketPrice, newPreviousMarketPrice);
  }
}

void OrderBook::cancelMarketBuyOrder(const uint256_t& id) {
  // Additional action for stop bid orders
  auto stopMarketBuyAction = [&id, this](const StopOrder& o) {
    if (o.id_ == id) {
      if (o.owner_ != this->getCaller()) throw std::runtime_error(
        "OrderBook::cancelMarketBuyOrder: INVALID_OWNER"
      );
      if (o.type_ != OrderType::STOPMARKET) throw std::runtime_error(
        "OrderBook::cancelMarketBuyOrder: INVALID_ORDER_TYPE"
      );
      // Return the tokens to the owner. Remember that on buy market orders amountAsset_ is in tokenB units.
      this->callContractFunction(
        this->addressAssetB_.get(), &ERC20::transfer, o.owner_, this->convertTick(o.amountAsset_)
      );
      return true;
    }
    return false;
  };
  this->stops_.erase_if(stopMarketBuyAction);
}

void OrderBook::cancelMarketSellOrder(const uint256_t &id) {
  // Additional action for stop bid orders
  auto stopMarketSellAction = [&id, this](const StopOrder &o) {
    if (o.id_ == id) {
      if (o.owner_ != this->getCaller()) throw std::runtime_error(
        "OrderBook::cancelMarketSellOrder: INVALID_OWNER"
      );
      if (o.type_ != OrderType::STOPMARKET) throw std::runtime_error(
        "OrderBook::cancelMarketSellOrder: INVALID_ORDER_TYPE"
      );
      // Return the tokens to the owner. Remember that on sell market orders amountAsset_ is in tokenA lots.
      this->callContractFunction(
        this->addressAssetA_.get(), &ERC20::transfer, o.owner_, this->convertLot(o.amountAsset_)
      );
      return true;
    }
    return false;
  };
  this->stops_.erase_if(stopMarketSellAction);
}

void OrderBook::updateLastPrice(const uint256_t &price) { this->lastPrice_ = price; }

void OrderBook::updateSpreadAndMidPrice() {
  uint256_t bidPrice = this->bids_.cbegin()->assetPrice_;
  uint256_t askPrice = this->asks_.cbegin()->assetPrice_;
  this->spread_ = (bidPrice >= askPrice) ? bidPrice - askPrice : askPrice - bidPrice;
}

uint64_t OrderBook::getCurrentTimestamp() const {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::system_clock::now().time_since_epoch()
  ).count();
}

void OrderBook::registerContractFunctions() {
  registerContract();
  this->registerMemberFunction("getNextOrderID", &OrderBook::getNextOrderID, this);
  this->registerMemberFunction("getAddressAssetA", &OrderBook::getAddressAssetA, this);
  this->registerMemberFunction("getAddressAssetB", &OrderBook::getAddressAssetB, this);
  this->registerMemberFunction("getTickerAssetA", &OrderBook::getTickerAssetA, this);
  this->registerMemberFunction("getTickerAssetB", &OrderBook::getTickerAssetB, this);
  this->registerMemberFunction("getSpread", &OrderBook::getSpread, this);
  this->registerMemberFunction("getTickSize", &OrderBook::getTickSize, this);
  this->registerMemberFunction("getLotSize", &OrderBook::getLotSize, this);
  this->registerMemberFunction("getLastPrice", &OrderBook::getLastPrice, this);
  this->registerMemberFunction("getPrecision", &OrderBook::getPrecision, this);
  this->registerMemberFunction("newLimitBidOrder", &OrderBook::newLimitBidOrder, this);
  this->registerMemberFunction("newLimitAskOrder", &OrderBook::newLimitAskOrder, this);
  this->registerMemberFunction("newMarketBuyOrder", &OrderBook::newMarketBuyOrder, this);
  this->registerMemberFunction("newMarketSellOrder", &OrderBook::newMarketSellOrder, this);
  this->registerMemberFunction("newStopLimitBidOrder", &OrderBook::newStopLimitBidOrder, this);
  this->registerMemberFunction("newStopLimitAskOrder", &OrderBook::newStopLimitAskOrder, this);
  this->registerMemberFunction("newStopMarketBuyOrder", &OrderBook::newStopMarketBuyOrder, this);
  this->registerMemberFunction("newStopMarketSellOrder", &OrderBook::newStopMarketSellOrder, this);
  this->registerMemberFunction("cancelLimitBidOrder", &OrderBook::cancelLimitBidOrder, this);
  this->registerMemberFunction("cancelLimitAskOrder", &OrderBook::cancelLimitAskOrder, this);
  this->registerMemberFunction("cancelMarketBuyOrder", &OrderBook::cancelMarketBuyOrder, this);
  this->registerMemberFunction("cancelMarketSellOrder", &OrderBook::cancelMarketSellOrder, this);
}

