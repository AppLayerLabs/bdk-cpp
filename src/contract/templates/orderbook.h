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

/**
 * TODO:
 * - tests when done
 * - trade book? do it with events or a separate class/struct?
 * - check later if orders should implement TIF (Time-In-Force - GTC, DAY, FOK, IOC, etc.) - GTC is the default
 * - logic for automatically cancelling orders if/when necessary (probly after TIF is implemented?)
 * - what exactly do we need to save/load in the database? do we let id, orders, prices, spread, etc. be wiped and start from a fresh slate every time?
 */

enum OrderType { MARKET, LIMIT, STOPMARKET, STOPLIMIT };
enum OrderSide { BID, ASK };
/// Struct for a given stop order in the book.
struct StopOrder {
  const uint256_t id_;            ///< Sequential unique ID of the order.
  const uint64_t timestamp_;      ///< The epoch timestamp of the order's creation.
  const Address owner_;           ///< The address that made the order.
  uint256_t amountAsset_;   ///< The amount of the asset the order has to offer (tokenA for bids, tokenB for asks).
  const uint256_t assetPrice_;    ///< The price of the asset the order has to offer in WEI of tokenB.
  const uint256_t stopLimit_;     ///< The stop limit price of the order (only for stop limit orders), in WEI.
  const OrderSide side_;
  const OrderType type_;

  /// Constructor.
  StopOrder(
    const uint256_t& id, const uint64_t& timestamp,
    const Address& owner, const uint256_t& amountAsset, const uint256_t& assetPrice, const uint256_t& stopLimit, const OrderSide& side, const OrderType type
  ) : id_(id), timestamp_(timestamp), owner_(owner),
      amountAsset_(amountAsset), assetPrice_(assetPrice), stopLimit_(stopLimit), side_(side), type_(type)
  {
    if (type != STOPLIMIT && type != STOPMARKET) {
      throw std::runtime_error("Invalid order type for stop order.");
    }
  }

  /// Lesser comparison operator.
  bool operator<(const StopOrder& o) const {
    return (this->stopLimit_ < o.stopLimit_ ||
            (this->stopLimit_ == o.stopLimit_ && this->timestamp_ < o.timestamp_)
    );
  }

  /// Higher comparison operator.
  bool operator>(const StopOrder& o) const {
    return (this->stopLimit_ > o.stopLimit_ ||
            (this->stopLimit_ == o.stopLimit_ && this->timestamp_ < o.timestamp_)
    );
  }
};

/// Struct for a given order in the book.
struct Order {
  const uint256_t id_;            ///< Sequential unique ID of the order.
  const uint64_t timestamp_;      ///< The epoch timestamp of the order's creation.
  const Address owner_;           ///< The address that made the order.
  uint256_t amountAsset_;   ///< The amount of the asset the order has to offer (tokenA for bids, tokenB for asks).
  const uint256_t assetPrice_;    ///< The price of the asset the order has to offer in WEI of tokenB.
  /// Constructor.
  Order(
    const uint256_t& id, const uint64_t& timestamp,
    const Address& owner, const uint256_t& amountAsset, const uint256_t& assetPrice
  ) : id_(id), timestamp_(timestamp), owner_(owner),
     amountAsset_(amountAsset), assetPrice_(assetPrice)
  {}

  Order(const StopOrder& stopOrder, const uint64_t& timestamp) : id_(stopOrder.id_), timestamp_(timestamp), owner_(stopOrder.owner_),
    amountAsset_(stopOrder.amountAsset_), assetPrice_(stopOrder.assetPrice_)
  {}

  /// Lesser comparison operator.
  bool operator<(const Order& o) const {
    return (this->assetPrice_ < o.assetPrice_ ||
      (this->assetPrice_ == o.assetPrice_ && this->timestamp_ < o.timestamp_)
    );
  }

  /// Higher comparison operator.
  bool operator>(const Order& o) const {
    return (this->assetPrice_ > o.assetPrice_ ||
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
    SafeUint256_t tickSize_;                              ///< The tick size of the order book. Should be pow(10, AssetB_.decimals() - 4), Tokens MUST have at least 8 decimals.
    SafeUint256_t lotSize_;                               ///< The lot size of the order book. Should be pow(10, AssetA_.decimals() - 4), Tokens MUST have at least 8 decimals
    SafeUint256_t lastPrice_;                             ///< The last price of the pair.
    const uint256_t precision_ = 10000;                   ///< Equivalent to 10^4, difference between tick/lot size to the actual token value
    SafeMultiSet<Order, std::greater<Order>> bids_;       ///< List of currently active bids, from highest to lowest price.
    SafeMultiSet<Order, std::less<Order>> asks_;          ///< List of currently active asks, from lowest to highest price.
    SafeMultiSet<StopOrder, std::less<StopOrder>> stops_; ///< List of stop orders, from lower to highest stop price.

    /**
     * Should always be called when wanting to create a new bid order on the market
     * @param b the Order to be executed and/or added to bids_
     * @param type the type of the order (MARKET, LIMIT, STOPMARKET, STOPLIMIT) (should only be LIMIT or STOPLIMIT)
     * This function will process a newly created bid order
     * Executing it if the prices match with an ask order
     * Adding it to the bids_ list if there are no matching ask orders or if the order was partially executed.
     */
    void processLimitBidOrder(Order& b, const OrderType& type);

    /**
     * Should always be called when wanting to create a new ask order on the market
     * @param a the Order to be executed and/or added to asks_
     * @param type the type of the order (MARKET, LIMIT, STOPMARKET, STOPLIMIT) (should only be LIMIT or STOPLIMIT)
     * This function will process a newly created ask order
     * Executing it if the prices match with a bid order
     * Adding it to the asks_ list if there are no matching bid orders or if the order was partially executed.
     */
    void processLimitAskOrder(Order& b, const OrderType& type);

    /**
     * Should always be called when wanting to process a new buy order on the market
     * @param a the Order to be executed
     * @param type the type of the order (MARKET, LIMIT, STOPMARKET, STOPLIMIT) (should only be MARKET or STOPMARKET)
     * This function will process a newly created buy order
     * Executing it until the order is fully executed or there are no more
     * ATTENTION: Remember that ammountAsset_ is ticks of tokenB to buy NOT LOTS OF TOKENA, there is no pricing.
     * Also this function requires us to move from the buying user to the selling user instead of transfering from the contract.
     * This function will buy tokens at any price possible.
     */
    void processMarketBuyOrder(Order& a, const OrderType& type);

    /**
     * Should always be called when wanting to process a new sell order on the market
     * @param a the Order to be executed
     * @param type the type of the order (MARKET, LIMIT, STOPMARKET, STOPLIMIT) (should only be MARKET or STOPMARKET)
     * This function will process a newly created sell order
     * Executing it until the order is fully executed or there are no more
     * ATTENTION: Remember that ammountAsset_ is lots of tokensA to sell, there is no pricing.
     * We need to be aware of returning the tokens to the user if the order is partially executed.
     * This function will sell tokens at any price possible.
     */
    void processMarketSellOrder(Order& a, const OrderType& type);

    /// Trigger stored stop orders.
    void triggerStopOrders(const uint256_t& currentMarketPrice, const uint256_t& previousMarketPrice);

    /// Update the last price of the pair.
    void updateLastPrice(const uint256_t& price);

    /// Update the current spread and mid price.
    void updateSpreadAndMidPrice();

    /// Get the current epoch timestamp, in milliseconds.
    uint64_t getCurrentTimestamp() const;

    /// Call the register functions for the contract.
    void registerContractFunctions() override;

    /// Converter from lot size to token amount.
    uint256_t convertLot(const uint256_t& value) const;

    /// Converter from ticket size to token amount.
    uint256_t convertTick(const uint256_t& value) const;

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
      const Address& creator, const uint64_t& chainId,
      const std::unique_ptr<DB> &db
    );

    /**
     * Constructor from load. Load contract from database.
     * @param interface The interface to the contract manager.
     * @param address The address of the contract.
     * @param db The database to use.
     */
    OrderBook(
      ContractManagerInterface &interface,
      const Address& address,
      const std::unique_ptr<DB> &db
    );

    /// Destructor.
    ~OrderBook() override;

    /// Getter for `spread_`.
    uint256_t getSpread() const { return this->spread_.get(); }

    /**
     * Create a new limit bid order.
     * @param amountAsset The amount of the asset the order wants to buy. (in lots)
     * @param assetPrice The amount of the asset the order wants to pay.  (in ticks)
     */
    void newLimitBidOrder(
      const uint256_t& amountAsset,
      const uint256_t& assetPrice
    );

    /**
     * Create a new limit ask order.
     * @param amountAsset The amount of the asset the order wants to sell.   (in lots)
     * @param assetPrice The amount of the asset the order wants to receive. (in ticks)
     */
    void newLimitAskOrder(
      const uint256_t& amountAsset,
      const uint256_t& assetPrice
    );

    /**
     * Create a new market buy order.
     * @param amountTokenB The amount of the ticks the order wants to buy (in tickSizes).
     * ATTENTION: Differently from other functions, this function takes amountAsset as tokenB instead of tokenA
     * It uses the processMarketBidOrder function to buy tokens at any price possible.
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
      const uint256_t& amountAsset,
      const uint256_t& assetPrice,
      const uint256_t& stopLimit
    );

    /**
     * Create a new stop limit ask order.
     * @param amountAsset The amount of the asset the order wants to sell.
     * @param assetPrice The amount of the asset the order wants to receive.
     * @param stopLimit The stop limit price of the order.
     */
    void newStopLimitAskOrder(
      const uint256_t& amountAsset,
      const uint256_t& assetPrice,
      const uint256_t& stopLimit
    );

    /**
     * Create a new stop market buy order.
     * @param amountTokenB The amount of the ticks the order wants to buy (in tickSizes).
     * @param stopLimit The stop limit price of the order.
     * ATTENTION: Differently from other functions, this function takes amountAsset as tokenB instead of tokenA
     */
    void newStopMarketBuyOrder(
      const uint256_t& amountTokenB,
      const uint256_t& stopLimit
    );

    /**
     * Create a new stop market sell order.
     * @param amountAsset The amount of the asset the order wants to sell. (in lots)
     * @param stopLimit The stop limit price of the order.
     */
    void newStopMarketSellOrder(
      const uint256_t& amountAsset,
      const uint256_t& stopLimit
    );

    /**
     * Cancel a previously made bid order.
     * Only the owner of the order can cancel it.
     * @param id The ID of the order to cancel.
     */
    void cancelLimitBidOrder(const uint256_t& id);

    /**
     * Cancel a previously made ask order.
     * Only the owner of the order can cancel it.
     * @param id The ID of the order to cancel.
     */
    void cancelLimitAskOrder(const uint256_t& id);

    /**
     * Cancel a previously made market buy order.
     * Only for stop functions (for obvious reasons)
     * Only the owner of the order can cancel it.
     * @param id The ID of the order to cancel.
     */
    void cancelMarketBuyOrder(const uint256_t& id);

    /**
     * Cancel a previously made market sell order.
     * Only for stop functions (for obvious reasons)
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
          "bids_", "asks_", "spread_"
        },
        std::make_tuple("getSpread", &OrderBook::getSpread, "", std::vector<std::string>{}),
        std::make_tuple("newLimitBidOrder", &OrderBook::newLimitBidOrder, "", std::vector<std::string>{"owner", "amountAsset", "assetPrice"}),
        std::make_tuple("newLimitAskOrder", &OrderBook::newLimitAskOrder, "", std::vector<std::string>{"owner", "amountAsset", "assetPrice"}),
        std::make_tuple("cancelBidOrder", &OrderBook::cancelLimitBidOrder, "", std::vector<std::string>{"id"}),
        std::make_tuple("cancelAskOrder", &OrderBook::cancelLimitAskOrder, "", std::vector<std::string>{"id"})
      );
    }
};

#endif  // ORDERBOOK_H
