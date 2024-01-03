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

/// Struct for a given stop order in the book.
struct StopOrder {
  const uint256_t id_;          ///< Sequential unique ID of the order.
  const uint64_t timestamp_;    ///< The epoch timestamp of the order's creation.
  const Address owner_;         ///< The address that made the order.
  uint256_t amountAsset_;       ///< The amount of the asset the order has to offer (tokenA for bids, tokenB for asks).
  const uint256_t assetPrice_;  ///< The unit price of the asset the order has to offer in WEI of tokenB.
  const uint256_t stopLimit_;   ///< The stop limit price of the order (only for stop limit orders), in WEI.
  const OrderSide side_;        ///< Whether the order originally is a bid or ask.
  const OrderType type_;        ///< Whether the order originally is a market or limit.

  /**
   * Constructor.
   * @param id The order's id.
   * @param timestamp The order's timestamp.
   * @param owner The order's owner address.
   * @param amountAsset The order's amount.
   * @param assetPrice The order's unit price.
   * @param stopLimit The order's stop limit.
   * @param side The order's side (bid or ask).
   * @param type The order's original type (market or limit).
   */
  StopOrder(
    const uint256_t& id, const uint64_t& timestamp, const Address& owner,
    const uint256_t& amountAsset, const uint256_t& assetPrice,
    const uint256_t& stopLimit, const OrderSide& side, const OrderType type
  ) : id_(id), timestamp_(timestamp), owner_(owner), amountAsset_(amountAsset),
    assetPrice_(assetPrice), stopLimit_(stopLimit), side_(side), type_(type)
  {
    if (type != STOPLIMIT && type != STOPMARKET) {
      throw std::runtime_error("Invalid order type for stop order.");
    }
  }

  /// Lesser comparison operator.
  bool operator<(const StopOrder& o) const {
    return (
      this->stopLimit_ < o.stopLimit_ ||
      (this->stopLimit_ == o.stopLimit_ && this->timestamp_ < o.timestamp_)
    );
  }

  /// Higher comparison operator.
  bool operator>(const StopOrder& o) const {
    return (
      this->stopLimit_ > o.stopLimit_ ||
      (this->stopLimit_ == o.stopLimit_ && this->timestamp_ < o.timestamp_)
    );
  }
};

/// Struct for a given order in the book.
struct Order {
  const uint256_t id_;          ///< Sequential unique ID of the order.
  const uint64_t timestamp_;    ///< The epoch timestamp of the order's creation.
  const Address owner_;         ///< The address that made the order.
  uint256_t amountAsset_;       ///< The amount of the asset the order has to offer (tokenA for bids, tokenB for asks).
  const uint256_t assetPrice_;  ///< The unit price of the asset the order has to offer in WEI of tokenB.

  /**
   * Constructor.
   * @param id The order's id.
   * @param timestamp The order's timestamp.
   * @param owner The order's owner address.
   * @param amountAsset The order's amount.
   * @param assetPrice The order's unit price.
   */
  Order(
    const uint256_t& id, const uint64_t& timestamp, const Address& owner,
    const uint256_t& amountAsset, const uint256_t& assetPrice
  ) : id_(id), timestamp_(timestamp), owner_(owner),
     amountAsset_(amountAsset), assetPrice_(assetPrice)
  {}

  Order(const StopOrder& stopOrder, const uint64_t& timestamp)
    : id_(stopOrder.id_), timestamp_(timestamp), owner_(stopOrder.owner_),
    amountAsset_(stopOrder.amountAsset_), assetPrice_(stopOrder.assetPrice_)
  {}

  /// Lesser comparison operator.
  bool operator<(const Order& o) const {
    return (
      this->assetPrice_ < o.assetPrice_ ||
      (this->assetPrice_ == o.assetPrice_ && this->timestamp_ < o.timestamp_)
    );
  }

  /// Higher comparison operator.
  bool operator>(const Order& o) const {
    return (
      this->assetPrice_ > o.assetPrice_ ||
      (this->assetPrice_ == o.assetPrice_ && this->timestamp_ < o.timestamp_)
    );
  }
};

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
      ContractReflectionInterface::registerContract<
        OrderBook, const Address, const std::string&, const Address, const std::string&,
        ContractManagerInterface&, const Address&, const Address&, const uint64_t&,
        const std::unique_ptr<DB>&
      >(
        std::vector<std::string>{
          "nextOrderID_", "addressAssetA_", "addressAssetB_", "tickerAssetA_", "tickerAssetB_",
          "spread_", "tickSize_", "lotSize_", "lastPrice_", "precision_", "bids_", "asks_", "stops_"
        },
        std::make_tuple("getNextOrderID", &OrderBook::getNextOrderID, "view", std::vector<std::string>{}),
        std::make_tuple("getAddressAssetA", &OrderBook::getAddressAssetA, "view", std::vector<std::string>{}),
        std::make_tuple("getAddressAssetB", &OrderBook::getAddressAssetB, "view", std::vector<std::string>{}),
        std::make_tuple("getTickerAssetA", &OrderBook::getTickerAssetA, "view", std::vector<std::string>{}),
        std::make_tuple("getTickerAssetB", &OrderBook::getTickerAssetB, "view", std::vector<std::string>{}),
        std::make_tuple("getSpread", &OrderBook::getSpread, "view", std::vector<std::string>{}),
        std::make_tuple("getTickSize", &OrderBook::getTickSize, "view", std::vector<std::string>{}),
        std::make_tuple("getLotSize", &OrderBook::getLotSize, "view", std::vector<std::string>{}),
        std::make_tuple("getLastPrice", &OrderBook::getLastPrice, "view", std::vector<std::string>{}),
        std::make_tuple("getPrecision", &OrderBook::getPrecision, "view", std::vector<std::string>{}),
        std::make_tuple("newLimitBidOrder", &OrderBook::newLimitBidOrder, "nonpayable", std::vector<std::string>{"amountAsset", "assetPrice"}),
        std::make_tuple("newLimitAskOrder", &OrderBook::newLimitAskOrder, "nonpayable", std::vector<std::string>{"amountAsset", "assetPrice"}),
        std::make_tuple("newMarketBuyOrder", &OrderBook::newMarketBuyOrder, "nonpayable", std::vector<std::string>{"amountTokenB"}),
        std::make_tuple("newMarketSellOrder", &OrderBook::newMarketSellOrder, "nonpayable", std::vector<std::string>{"amountAsset"}),
        std::make_tuple("newStopLimitBidOrder", &OrderBook::newStopLimitBidOrder, "nonpayable", std::vector<std::string>{"amountAsset", "assetPrice", "stopLimit"}),
        std::make_tuple("newStopLimitAskOrder", &OrderBook::newStopLimitAskOrder, "nonpayable", std::vector<std::string>{"amountAsset", "assetPrice", "stopLimit"}),
        std::make_tuple("newStopMarketBuyOrder", &OrderBook::newStopMarketBuyOrder, "nonpayable", std::vector<std::string>{"amountTokenB", "stopLimit"}),
        std::make_tuple("newStopMarketSellOrder", &OrderBook::newStopMarketSellOrder, "nonpayable", std::vector<std::string>{"amountAsset", "stopLimit"}),
        std::make_tuple("cancelLimitBidOrder", &OrderBook::cancelLimitBidOrder, "nonpayable", std::vector<std::string>{"id"}),
        std::make_tuple("cancelLimitAskOrder", &OrderBook::cancelLimitAskOrder, "nonpayable", std::vector<std::string>{"id"}),
        std::make_tuple("cancelMarketBuyOrder", &OrderBook::cancelMarketBuyOrder, "nonpayable", std::vector<std::string>{"id"}),
        std::make_tuple("cancelMarketSellOrder", &OrderBook::cancelMarketSellOrder, "nonpayable", std::vector<std::string>{"id"})
      );
    }
};

#endif  // ORDERBOOK_H
