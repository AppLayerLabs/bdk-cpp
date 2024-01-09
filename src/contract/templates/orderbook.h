/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

#include "erc20.h"

#include "../dynamiccontract.h"
#include "../variables/safeaddress.h"
#include "../variables/safemultiset.h"
#include "../variables/safestring.h"
#include "../variables/safeuint.h"

#include "../../utils/strings.h"

// TODO: tests when done

// TODO: trade feed for logging (we need to fully implement events first)

// TODO: OrderBook: getters AND save/load in ctors/dtor for bids, asks and stops (we need to fully implement support for structs first)

/// Enum for identifying order types (market or limit, and the respective stops).
enum OrderType { MARKET, LIMIT, STOPMARKET, STOPLIMIT };

/// Enum for identifying order side (bid or ask).
enum OrderSide { BID, ASK };

/**
 * Tuple for a given stop order in the book.
 * 0 - const uint256_t  - id          - Sequential unique ID of the order.
 * 1 - const uint       - timestamp   - The epoch timestamp of the order's creation.
 * 2 - const Address    - owner       - The address that made the order.
 * 3 - uint256_t        - amountAsset - The amount of the asset the order has to offer (tokenA for bids, tokenB for asks).
 * 4 - const uint256_t  - assetPrice  - The unit price of the asset the order has to offer in WEI of tokenB.
 * 5 - const uint256_t  - stopLimit   - The stop limit price of the order (only for stop limit orders), in WEI.
 * 6 - const OrderSide  - side        - Whether the order originally is a bid or ask.
 * 7 - const OrderType  - type        - Whether the order originally is a market or limit.
 */
using StopOrder = std::tuple<const uint256_t, const uint64_t, const Address, uint256_t, const uint256_t, const uint256_t, const OrderSide, const OrderType>;

/**
 * Lesser comparison operator.
 * @param lhs The left hand side of the comparison.
 * @param rhs The right hand side of the comparison.
 * @return True if lhs < rhs, false otherwise.
 */
bool operator<(const StopOrder& lhs, const StopOrder& rhs) {
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
bool operator>(const StopOrder& lhs, const StopOrder& rhs) {
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
 * 3 - uint256_t        - amountAsset - The amount of the asset the order has to offer (tokenA for bids, tokenB for asks).
 * 4 - const uint256_t  - assetPrice  - The unit price of the asset the order has to offer in WEI of tokenB.
 */
using Order = std::tuple<const uint256_t, const uint64_t, const Address, uint256_t, const uint256_t>;

/**
 * Lesser comparison operator.
 * @param lhs The left hand side of the comparison.
 * @param rhs The right hand side of the comparison.
 * @return True if lhs < rhs, false otherwise.
 */
bool operator<(const Order& lhs, const Order& rhs) {
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
bool operator>(const Order& lhs, const Order& rhs) {
  const auto& lhs_assetPrice = std::get<4>(lhs);
  const auto& rhs_assetPrice = std::get<4>(rhs);
  const auto& lhs_timestamp = std::get<1>(lhs);
  const auto& rhs_timestamp = std::get<1>(rhs);
  return (lhs_assetPrice > rhs_assetPrice) ||
         (lhs_assetPrice == rhs_assetPrice && lhs_timestamp < rhs_timestamp);
}

Order orderFromStopOrder(const StopOrder& stopOrder, const uint64_t& timestamp) {
  return std::make_tuple(
    std::get<0>(stopOrder), timestamp, std::get<2>(stopOrder),
    std::get<3>(stopOrder), std::get<4>(stopOrder)
  );
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
    SafeUint256_t tickSize_;                              ///< The tick size of the order book (minimum difference between price levels). Should be pow(10, AssetB_.decimals() - 4), tokens MUST have at least 8 decimals.
    SafeUint256_t lotSize_;                               ///< The lot size of the order book (minimum difference between order amounts). Should be pow(10, AssetA_.decimals() - 4), tokens MUST have at least 8 decimals.
    SafeUint256_t lastPrice_;                             ///< The last price of the pair.
    const uint256_t precision_ = 10000;                   ///< Equivalent to 10^4, difference between tick/lot size to the actual token value
    SafeMultiSet<Order, std::greater<Order>> bids_;       ///< List of currently active bids, from highest to lowest price.
    SafeMultiSet<Order, std::less<Order>> asks_;          ///< List of currently active asks, from lowest to highest price.
    SafeMultiSet<StopOrder, std::less<StopOrder>> stops_; ///< List of stop orders, from lower to highest stop price.

    /**
     * Process a newly created bid limit order, executing it if the price
     * matches with an ask order, or adding it to the bids list if there are
     * no matching ask orders or if the order was partially executed.
     * Should always be called when wanting to create a new bid order on the market.
     * @param b The order to be executed and/or added to the bids list.
     * @param type The type of the order (should only be LIMIT or STOPLIMIT).
     */
    void processLimitBidOrder(Order& b, const OrderType& type);

    /**
     * Process a newly created ask limit order, executing it if the price
     * matches with a bid order, or adding it to the asks list if there are
     * no matching bid orders or if the order was partially executed.
     * Should always be called when wanting to create a new ask order on the market.
     * @param b The order to be executed and/or added to the asks list.
     * @param type The type of the order (should only be LIMIT or STOPLIMIT).
     */
    void processLimitAskOrder(Order& b, const OrderType& type);

    /**
     * Process a newly created market bid order, executing it if the price
     * matches with an ask order at any price possible, or dropping it off
     * entirely if the order is not executed or partially executed.
     * ATTENTION: Remember that amountAsset_ is ticks of tokenB to buy, NOT
     * LOTS OF TOKENA, there is no pricing.
     * Also, this function requires us to move from the buying user to the
     * selling user instead of transfering from the contract.
     * @param a The order to be executed.
     * @param type The type of the order (should only be MARKET or STOPMARKET).
     */
    void processMarketBuyOrder(Order& a, const OrderType& type);

    /**
     * Process a newly created market ask order, executing it if the price
     * matches with a bid order at any price possible, or dropping it off
     * entirely if the order is not executed or partially executed.
     * ATTENTION: Remember that amountAsset_ is ticks of tokenB to buy, NOT
     * LOTS OF TOKENA, there is no pricing.
     * Also, we need to be aware of returning the tokens to the user if the
     * order is partially executed.
     * @param a The order to be executed.
     * @param type The type of the order (should only be MARKET or STOPMARKET).
     */
    void processMarketSellOrder(Order& a, const OrderType& type);

    /**
     * Trigger stored stop orders based on the given market prices.
     * @param currentMarketPrice The current market price (after a bid/ask process).
     * @param previousMarketPrice The previous market price (before a bid/ask process).
     */
    void triggerStopOrders(const uint256_t& currentMarketPrice, const uint256_t& previousMarketPrice);

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
    inline uint256_t convertLot(const uint256_t& value) const { return value * lotSize_.get(); }

    /**
     * Convert from tick size to token amount.
     * @param value The value to convert.
     */
    inline uint256_t convertTick(const uint256_t& value) const { return value * tickSize_.get(); }

    /// Call the register functions for the contract.
    void registerContractFunctions() override;

  public:
    using ConstructorArguments = std::tuple<
      const Address, const std::string&, const Address, const std::string&
    >;
    /**
     * Constructor from scratch.
     * @param addA The address of the pair's first asset.
     * @param tickerA The ticker of the pair's first asset.
     * @param addB The address of the pair's second asset.
     * @param tickerB The ticker of the pair's second asset.
     * @param interface The interface to the contract manager.
     * @param address The address of the contract.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain ID.
     * @param db The database to use.
     */
    OrderBook(
      const Address addA, const std::string& tickerA,
      const Address addB, const std::string& tickerB,
      ContractManagerInterface &interface, const Address& address,
      const Address& creator, const uint64_t& chainId, const std::unique_ptr<DB> &db
    );

    /**
     * Constructor from load. Load contract from database.
     * @param interface The interface to the contract manager.
     * @param address The address of the contract.
     * @param db The database to use.
     */
    OrderBook(
      ContractManagerInterface &interface, const Address& address, const std::unique_ptr<DB> &db
    );

    /// Destructor.
    ~OrderBook() override;

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
     * Create a new limit bid order.
     * @param amountAsset The amount of the asset the order wants to buy (in lots).
     * @param assetPrice The amount of the asset the order wants to pay (in ticks).
     */
    void newLimitBidOrder(const uint256_t& amountAsset, const uint256_t& assetPrice);

    /**
     * Create a new limit ask order.
     * @param amountAsset The amount of the asset the order wants to sell (in lots).
     * @param assetPrice The amount of the asset the order wants to receive (in ticks).
     */
    void newLimitAskOrder(const uint256_t& amountAsset, const uint256_t& assetPrice);

    /**
     * Create a new market buy order.
     * ATTENTION: Differently from other functions, this function takes
     * amountAsset as tokenB instead of tokenA. It uses processMarketBuyOrder()
     * to buy tokens at any price possible.
     * @param amountTokenB The amount of the ticks the order wants to buy (in tick sizes).
     */
    void newMarketBuyOrder(const uint256_t& amountTokenB);

    /**
     * Create a new market sell order.
     * @param amountAsset The amount of the asset the order wants to sell.
     */
    void newMarketSellOrder(const uint256_t& amountAsset);

    /**
     * Create a new stop limit bid order.
     * @param amountAsset The amount of the asset the order wants to buy.
     * @param assetPrice The amount of the asset the order wants to pay.
     * @param stopLimit The stop limit price of the order.
     */
    void newStopLimitBidOrder(
      const uint256_t& amountAsset, const uint256_t& assetPrice, const uint256_t& stopLimit
    );

    /**
     * Create a new stop limit ask order.
     * @param amountAsset The amount of the asset the order wants to sell.
     * @param assetPrice The amount of the asset the order wants to receive.
     * @param stopLimit The stop limit price of the order.
     */
    void newStopLimitAskOrder(
      const uint256_t& amountAsset, const uint256_t& assetPrice, const uint256_t& stopLimit
    );

    /**
     * Create a new stop market buy order.
     * ATTENTION: Differently from other functions, this function takes
     * amountAsset as tokenB instead of tokenA.
     * @param amountTokenB The amount of the ticks the order wants to buy (in tick sizes).
     * @param stopLimit The stop limit price of the order.
     */
    void newStopMarketBuyOrder(const uint256_t& amountTokenB, const uint256_t& stopLimit);

    /**
     * Create a new stop market sell order.
     * @param amountAsset The amount of the asset the order wants to sell (in lots).
     * @param stopLimit The stop limit price of the order.
     */
    void newStopMarketSellOrder(const uint256_t& amountAsset, const uint256_t& stopLimit);

    /**
     * Cancel a previously made limit bid order.
     * Only the owner of the order can cancel it.
     * @param id The ID of the order to cancel.
     */
    void cancelLimitBidOrder(const uint256_t& id);

    /**
     * Cancel a previously made limit ask order.
     * Only the owner of the order can cancel it.
     * @param id The ID of the order to cancel.
     */
    void cancelLimitAskOrder(const uint256_t& id);

    /**
     * Cancel a previously made market buy order. Only for stop functions (for obvious reasons).
     * Only the owner of the order can cancel it.
     * @param id The ID of the order to cancel.
     */
    void cancelMarketBuyOrder(const uint256_t& id);

    /**
     * Cancel a previously made market sell order. Only for stop functions (for obvious reasons).
     * Only the owner of the order can cancel it.
     * @param id The ID of the order to cancel.
     */
    void cancelMarketSellOrder(const uint256_t& id);

    /// Register the contract structure.
    static void registerContract() {
      ContractReflectionInterface::registerContractMethods<
        OrderBook, const Address, const std::string&, const Address, const std::string&,
        ContractManagerInterface&, const Address&, const Address&, const uint64_t&,
        const std::unique_ptr<DB>&
      >(
        std::vector<std::string>{
          "nextOrderID_", "addressAssetA_", "addressAssetB_", "tickerAssetA_", "tickerAssetB_",
          "spread_", "tickSize_", "lotSize_", "lastPrice_", "precision_", "bids_", "asks_", "stops_"
        },
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
        std::make_tuple("newLimitBidOrder", &OrderBook::newLimitBidOrder, FunctionTypes::NonPayable, std::vector<std::string>{"amountAsset", "assetPrice"}),
        std::make_tuple("newLimitAskOrder", &OrderBook::newLimitAskOrder, FunctionTypes::NonPayable, std::vector<std::string>{"amountAsset", "assetPrice"}),
        std::make_tuple("newMarketBuyOrder", &OrderBook::newMarketBuyOrder, FunctionTypes::NonPayable, std::vector<std::string>{"amountTokenB"}),
        std::make_tuple("newMarketSellOrder", &OrderBook::newMarketSellOrder, FunctionTypes::NonPayable, std::vector<std::string>{"amountAsset"}),
        std::make_tuple("newStopLimitBidOrder", &OrderBook::newStopLimitBidOrder, FunctionTypes::NonPayable, std::vector<std::string>{"amountAsset", "assetPrice", "stopLimit"}),
        std::make_tuple("newStopLimitAskOrder", &OrderBook::newStopLimitAskOrder, FunctionTypes::NonPayable, std::vector<std::string>{"amountAsset", "assetPrice", "stopLimit"}),
        std::make_tuple("newStopMarketBuyOrder", &OrderBook::newStopMarketBuyOrder, FunctionTypes::NonPayable, std::vector<std::string>{"amountTokenB", "stopLimit"}),
        std::make_tuple("newStopMarketSellOrder", &OrderBook::newStopMarketSellOrder, FunctionTypes::NonPayable, std::vector<std::string>{"amountAsset", "stopLimit"}),
        std::make_tuple("cancelLimitBidOrder", &OrderBook::cancelLimitBidOrder, FunctionTypes::NonPayable, std::vector<std::string>{"id"}),
        std::make_tuple("cancelLimitAskOrder", &OrderBook::cancelLimitAskOrder, FunctionTypes::NonPayable, std::vector<std::string>{"id"}),
        std::make_tuple("cancelMarketBuyOrder", &OrderBook::cancelMarketBuyOrder, FunctionTypes::NonPayable, std::vector<std::string>{"id"}),
        std::make_tuple("cancelMarketSellOrder", &OrderBook::cancelMarketSellOrder, FunctionTypes::NonPayable, std::vector<std::string>{"id"})
      );
    }
};

#endif  // ORDERBOOK_H
