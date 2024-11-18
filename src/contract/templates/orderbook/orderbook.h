/*
  Copyright (c) [2023] [Sparq Network]

  This software is distributed under the MIT License.
  See the LICENSE.txt file in the project root for more information.
*/

#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../erc20.h"

#include "../../dynamiccontract.h"
#include "../../variables/safeuint.h"
#include "../../variables/safestring.h"
#include "../../variables/safeaddress.h"
#include "../../variables/safemultiset.h"

#include "../../../utils/db.h"
#include "../../../utils/utils.h"
#include "../../../utils/strings.h"

/// Enum for identifying order types (market or limit, and the respective stops).
enum OrderType { MARKET, LIMIT, STOPMARKET, STOPLIMIT };

// better names?
// enum OrderType { MARKET_ORDER, \
LIMIT_ORDER, \
STOP_MARKET_ORDER, \
STOP_LIMIT_ORDER };

/// Enum for identifying order side (bid or ask).
enum OrderSide { BID, ASK };
/**
 * Tuple for a given stop order in the book.
 * 0 - const uint256_t  - id          - Sequential unique ID of the order.
 * 1 - const uint       - timestamp   - The epoch timestamp of the order's creation.
 * 2 - const Address    - owner       - The address that made the order.
 * 3 - uint256_t        - assetAmount - The amount of the asset the order has to offer (tokenA for bids, tokenB for asks).
 * 4 - const uint256_t  - assetPrice  - The unit price of the asset the order has to offer in WEI of tokenB.
 * 5 - const uint256_t  - stopLimit   - The stop limit price of the order (only for stop limit orders), in WEI.
 * 6 - const OrderSide  - side        - Whether the order originally is a bid or ask.
 * 7 - const OrderType  - type        - Whether the order originally is a market or limit.
 */
using StopOrder = std::tuple<const uint256_t,
                             const uint64_t,
                             const Address,
                             uint256_t,
                             const uint256_t,
                             const uint256_t,
                             const OrderSide,
                             const OrderType>;

/**
 * Lesser comparison operator.
 * @param lhs The left hand side of the comparison.
 * @param rhs The right hand side of the comparison.
 * @return True if lhs < rhs, false otherwise.
 */
inline bool operator<(const StopOrder& lhs, const StopOrder& rhs) {
  const auto& lhs_stopLimit = std::get<5>(lhs);
  const auto& rhs_stopLimit = std::get<5>(rhs);
  const auto& lhs_timestamp = std::get<1>(lhs);
  const auto& rhs_timestamp = std::get<1>(rhs);
  return (lhs_stopLimit < rhs_stopLimit) ||
    (lhs_stopLimit == rhs_stopLimit && lhs_timestamp < rhs_timestamp);
}

/**
 * Higher comparison operator.
 * @param lhs The left hand side of the comparison.
 * @param rhs The right hand side of the comparison.
 * @return True if lhs > rhs, false otherwise.
 */
inline bool operator>(const StopOrder& lhs, const StopOrder& rhs) {
  const auto& lhs_stopLimit = std::get<5>(lhs);
  const auto& rhs_stopLimit = std::get<5>(rhs);
  const auto& lhs_timestamp = std::get<1>(lhs);
  const auto& rhs_timestamp = std::get<1>(rhs);
  return (lhs_stopLimit > rhs_stopLimit) ||
    (lhs_stopLimit == rhs_stopLimit && lhs_timestamp < rhs_timestamp);
}

/**
 * Tuple for a given order in the book.
 * 0 - const uint256_t  - id          - Sequential unique ID of the order.
 * 1 - const uint       - timestamp   - The epoch timestamp of the order's creation.
 * 2 - const Address    - owner       - The address that made the order.
 * 3 - uint256_t        - assetAmount - The amount of the asset the order has to offer (tokenA for bids, tokenB for asks).
 * 4 - const uint256_t  - assetPrice  - The unit price of the asset the order has to offer in WEI of tokenB.
 */
using Order = std::tuple<const uint256_t,
                         const uint64_t,
                         const Address,
                         uint256_t,
                         const uint256_t>;

/**
 * Lesser comparison operator.
 * @param lhs The left hand side of the comparison.
 * @param rhs The right hand side of the comparison.
 * @return True if lhs < rhs, false otherwise.
 */
inline bool operator<(const Order& lhs, const Order& rhs) {
  const auto& lhs_assetPrice = std::get<4>(lhs);
  const auto& rhs_assetPrice = std::get<4>(rhs);
  const auto& lhs_timestamp = std::get<1>(lhs);
  const auto& rhs_timestamp = std::get<1>(rhs);
  return (lhs_assetPrice < rhs_assetPrice) ||
    (lhs_assetPrice == rhs_assetPrice && lhs_timestamp < rhs_timestamp);
}

/**
 * Higher comparison operator.
 * @param lhs The left hand side of the comparison.
 * @param rhs The right hand side of the comparison.
 * @return True if lhs > rhs, false otherwise.
 */
inline bool operator>(const Order& lhs, const Order& rhs) {
  const auto& lhs_assetPrice = std::get<4>(lhs);
  const auto& rhs_assetPrice = std::get<4>(rhs);
  const auto& lhs_timestamp = std::get<1>(lhs);
  const auto& rhs_timestamp = std::get<1>(rhs);
  return (lhs_assetPrice > rhs_assetPrice) ||
    (lhs_assetPrice == rhs_assetPrice && lhs_timestamp < rhs_timestamp);
}

inline Order orderFromStopOrder(const StopOrder& stopOrder, const uint64_t& timestamp)
{
  return std::make_tuple(std::get<0>(stopOrder),
                         timestamp,
                         std::get<2>(stopOrder),
                         std::get<3>(stopOrder),
                         std::get<4>(stopOrder));
}

/// Contract template for a given exchange pair order book.
class OrderBook : public DynamicContract {
private:
  SafeUint256_t nextOrderID_;                           ///< Counter for the next order ID.
  SafeAddress addressAssetA_;                           ///< Address of the first asset of the pair. HAS TO BE AN ERC20 TOKEN.
  SafeAddress addressAssetB_;                           ///< Address of the second asset of the pair. HAS TO BE AN ERC20 TOKEN.
  SafeString tickerAssetA_;                             ///< Ticker of the first asset of the pair.
  SafeString tickerAssetB_;                             ///< Ticker of the second asset of the pair.
  SafeUint256_t spread_;                                ///< Current market spread.
  SafeUint256_t tickSize_;                              ///< The tick size of the order book (minimum difference between price levels). \
                                                        Should be pow(10, AssetB_.decimals() - 4), tokens MUST have at least 8 decimals.
  SafeUint256_t lotSize_;                               ///< The lot size of the order book (minimum difference between order amounts). \
                                                        Should be pow(10, AssetA_.decimals() - 4), tokens MUST have at least 8 decimals.
  SafeUint256_t lastPrice_;                             ///< The last price of the pair.
  const uint256_t precision_ = 10000;                   ///< Equivalent to 10^4, difference between tick/lot size to the actual token value

  SafeMultiSet<Order, std::greater<Order>> bids_;       ///< List of currently active bids, from highest to lowest price.
  SafeMultiSet<Order, std::less<Order>> asks_;          ///< List of currently active asks, from lowest to highest price.
  SafeMultiSet<StopOrder, std::less<StopOrder>> stops_; ///< List of stop orders, from lower to highest stop price.

  /**
   * <Description>
   * @param  Param Description
   * @param  ...
   * @return Return Value Description
   */
  void transferToContract(const Address& from, const uint256_t &tickAmount);

  /**
   * <Description>
   * @param  Param Description
   * @param  ...
   * @return Return Value Description
   */
  Order* makeOrder(const uint256_t& assetAmount,
                   const uint256_t& assetPrice);

  /**
   * <Description>
   * @param  Param Description
   * @param  ...
   * @return Return Value Description
   */
  void executeOrder(const Address& askOwner,
                    const Address& bidOwner,
                    uint256_t tokenAmount,
                    uint256_t assetAmount);

  /**
   * <Description>
   * @param  Param Description
   * @param  ...
   * @return Return Value Description
   */
  inline void insertAskOrder(const Order& askOrder);

  /**
   * <Description>
   * @param  Param Description
   * @param  ...
   * @return Return Value Description
   */
  inline void insertBidOrder(const Order& bidOrder);

  /**
   * <Description>
   * @param  Param Description
   * @param  ...
   * @return Return Value Description
   */
  inline void eraseAskOrder(const Order& askOrder);

  /**
   * <Description>
   * @param  Param Description
   * @param  ...
   * @return Return Value Description
   */
  inline void eraseBidOrder(const Order& bidOrder);

  /**
   * <Description>
   * @param  Param Description
   * @param  ...
   * @return Return Value Description
   */
  void evaluateBidLimitOrder(Order& bidOrder);

  /**
   * <Description>
   * @param  Param Description
   * @param  ...
   * @return Return Value Description
   */
  void evaluateAskLimitOrder(Order& askOrder);

  /**
   * <Description>
   * @param  Param Description
   * @param  ...
   * @return Return Value Description
   */
  Order* findMatchAskOrder(const Order& bidOrder);

  /**
   * <Description>
   * @param  Param Description
   * @param  ...
   * @return Return Value Description
   */
  Order* findMatchBidOrder(const Order& askOrder);

  /**
   * Update the last price of the pair.
   * @param price The new last price.
   */
  void updateLastPrice(const uint256_t& price);

  /// Update the current spread and mid price based on top bid and top ask prices.
  void updateSpreadAndMidPrice();

  /// Get the current epoch timestamp, in milliseconds.
  uint64_t getCurrentTimestamp() const;

  /**
   * Convert from lot size to token amount.
   * @param value The value to convert.
   */
  inline uint256_t convertLot(const uint256_t& value) const {
    return value * lotSize_.get();
  }

  /**
   * <Description>
   * @param  Param Description
   * @param  ...
   * @return Return Value Description
   */
  inline uint256_t convertToken(const uint256_t& assetAmount,
                                const uint256_t& assetPrice) const;

  /**
   * Convert from tick size to token amount.
   * @param value The value to convert.
   */
  inline uint256_t convertTick(const uint256_t& value) const {
    return value * tickSize_.get();
  }

  /// Call the register functions for the contract.
  void registerContractFunctions() override;

  /// Dump method
  DBBatch dump() const override;

public:
  using ConstructorArguments = std::tuple<
    const Address&, const std::string&, const Address&, const std::string&
    >;
  /**
   * Constructor from scratch.
   * @param addA The address of the pair's first asset.
   * @param tickerA The ticker of the pair's first asset.
   * @param addB The address of the pair's second asset.
   * @param tickerB The ticker of the pair's second asset.
   * @param address The address of the contract.
   * @param creator The address of the creator of the contract.
   * @param chainId The chain ID.
   */
  OrderBook(const Address& addA, const std::string& tickerA,
            const Address& addB, const std::string& tickerB,
            const Address& address, const Address& creator,
            const uint64_t& chainId);

  /**
   * Constructor from load. Load contract from database.
   * @param address The address of the contract.
   * @param db The database to use.
   */
  OrderBook(const Address& address, const DB &db);

  /// Getter for `nextOrderID_`.
  uint256_t getNextOrderID() const { return this->nextOrderID_.get(); }

  /// Getter for `addressAssetA_`.
  Address getAddressAssetA() const { return this->addressAssetA_.get(); }

  /// Getter for `addressAssetB_`.
  Address getAddressAssetB() const { return this->addressAssetB_.get(); }

  /// Getter for `tickerAssetA_`.
  std::string getTickerAssetA() const { return this->tickerAssetA_.get(); }

  /// Getter for `tickerAssetB_`.
  std::string getTickerAssetB() const { return this->tickerAssetB_.get(); }

  /// Getter for `spread_`.
  uint256_t getSpread() const { return this->spread_.get(); }

  /// Getter for `tickSize_`.
  uint256_t getTickSize() const { return this->tickSize_.get(); }

  /// Getter for `lotSize_`.
  uint256_t getLotSize() const { return this->lotSize_.get(); }

  /// Getter for `lastPrice_`.
  uint256_t getLastPrice() const { return this->lastPrice_.get(); }

  /// Getter for `precision_`.
  uint256_t getPrecision() const { return this->precision_; }

  /**
   * Getter for all bids.
   * @return A vector with all bids.
   */
  std::vector<Order> getBids() const;

  /**
   * Getter for all asks.
   * @return A vector with all asks.
   */
  std::vector<Order> getAsks() const;

  /**
   * Getter for all users orders
   * @param user The user to get orders from.
   * @return A tuple with a vector of bids, a vector of asks and a vector of stops.
   */
  std::tuple<std::vector<Order>,
             std::vector<Order>,
             std::vector<StopOrder>> getUserOrders(const Address& user) const;

  /**
   * <Description>
   * @param  Param Description
   * @param  ...
   */
  void addBidLimitOrder(const uint256_t& assetAmount,
                        const uint256_t& assetPrice);

  /**
   * <Description>
   * @param  Param Description
   * @param  ...
   */
  void addAskLimitOrder(const uint256_t& assetAmount,
                        const uint256_t& assetPrice);

  /**
   * <Description>
   */
  void setDecimals();

  /// Register the contract structure.
  static void registerContract() {
    ContractReflectionInterface::registerContractMethods<
      OrderBook,
      const Address,
      const std::string&,
      const Address,
      const std::string&,
      const Address&,
      const Address&,
      const uint64_t&,
      DB&
      >(
        std::vector<std::string>{ "addA", "tickerA", "addB", "tickerB"},
        std::make_tuple("getNextOrderID", &OrderBook::getNextOrderID, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("getAddressAssetA", &OrderBook::getAddressAssetA, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("getAddressAssetB", &OrderBook::getAddressAssetB, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("getTickerAssetA", &OrderBook::getTickerAssetA, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("getTickerAssetB", &OrderBook::getTickerAssetB, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("getSpread", &OrderBook::getSpread, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("getTickSize", &OrderBook::getTickSize, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("getLotSize", &OrderBook::getLotSize, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("getLastPrice", &OrderBook::getLastPrice, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("getPrecision", &OrderBook::getPrecision, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("getBids", &OrderBook::getBids, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("getAsks", &OrderBook::getAsks, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("getUserOrders", &OrderBook::getUserOrders, FunctionTypes::View, std::vector<std::string>{"user"}),
        std::make_tuple("addBidLimitOrder", &OrderBook::addBidLimitOrder, FunctionTypes::NonPayable, std::vector<std::string>{"assetAmount", "assetPrice"}),
        std::make_tuple("addAskLimitOrder", &OrderBook::addAskLimitOrder, FunctionTypes::NonPayable, std::vector<std::string>{"assetAmount", "assetPrice"}),
        std::make_tuple("setDecimals", &OrderBook::setDecimals, FunctionTypes::NonPayable, std::vector<std::string>{})
        );
  }
};

#endif  // ORDERBOOK_H
