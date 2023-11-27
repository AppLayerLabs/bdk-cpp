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

/// Enum for order types.
enum OrderType { MARKET, LIMIT, STOP, STOPLIMIT };

/// Struct for a given order in the book.
struct Order {
  uint256_t id_;            ///< Sequential unique ID of the order.
  OrderType type_;          ///< The type of the order.
  uint64_t timestamp_;      ///< The epoch timestamp of the order's creation.
  Address owner_;           ///< The address that made the order.
  uint256_t amountAssetA_;  ///< The amount of the asset the order has to offer (buy for bids, sell for asks).
  uint256_t amountAssetB_;  ///< The amount of the asset the order wants (pay for bids, receive for asks).

  /// Constructor.
  Order(
    const uint256_t id, const OrderType type, const uint64_t timestamp,
    const Address owner, const uint256_t amountAssetA, const uint256_t amountAssetB
  ) : id_(id), type_(type), timestamp_(timestamp), owner_(owner),
    amountAssetA_(amountAssetA), amountAssetB_(amountAssetB)
  {}

  /// Lesser comparison operator.
  bool operator<(const Order& o) const {
    return (this->amountAssetB_ < o.amountAssetB_ ||
      (this->amountAssetB_ == o.amountAssetB_ && this->timestamp_ < o.timestamp_)
    );
  }

  /// Higher comparison operator.
  bool operator>(const Order& o) const {
    return (this->amountAssetB_ > o.amountAssetB_ ||
      (this->amountAssetB_ == o.amountAssetB_ && this->timestamp_ < o.timestamp_)
    );
  }
};

/// Contract template for a given exchange pair order book.
class OrderBook : public DynamicContract {
  private:
    SafeUint256_t nextOrderID_;                         ///< Counter for the next order ID.
    SafeAddress addressAssetA_;                         ///< Address of the first asset of the pair. HAS TO BE AN ERC20 TOKEN.
    SafeAddress addressAssetB_;                         ///< Address of the second asset of the pair. HAS TO BE AN ERC20 TOKEN.
    SafeString tickerAssetA_;                           ///< Ticker of the first asset of the pair.
    SafeString tickerAssetB_;                           ///< Ticker of the second asset of the pair.
    SafeUint256_t marketPrice_;                         ///< Current market price.
    SafeUint256_t midPrice_;                            ///< Current mid price.
    SafeUint256_t spread_;                              ///< Current market spread.
    SafeMultiSet<Order, std::greater<Order>> bids_;     ///< List of currently active bids, from highest to lowest price.
    SafeMultiSet<Order, std::less<Order>> asks_;        ///< List of currently active asks, from lowest to highest price.
    SafeMultiSet<Order, std::greater<Order>> stopBids_; ///< List of issued stop bids, from highest to lowest price.
    SafeMultiSet<Order, std::less<Order>> stopAsks_;    ///< List of issued stop asks, from lowest to highest price.

    /// Setter for `marketPrice_`.
    void setMarketPrice(uint256_t marketPrice) { this->marketPrice_ = marketPrice; }

    /**
     * Process a given market bid order.
     * @param b The bid order to process.
     */
    void processMarketBidOrder(Order& b);

    /**
     * Process a given market bid order.
     * @param a The ask order to process.
     */
    void processMarketAskOrder(Order& a);

    /**
     * Execute a trade between two orders.
     * @param bid The bid order that will trade.
     * @param ask The ask order that will trade.
     * @return `true` if the trade was successful, `false` otherwise.
     */
    bool executeTrade(Order& bid, Order& ask);

    /// Clear filled orders from both lists.
    void clearFilledOrders();

    /// Trigger stored stop orders from both lists.
    void triggerStopOrders();

    /// Update the current spread and mid price.
    void updateSpreadAndMidPrice();

    /// Get the current epoch timestamp, in milliseconds.
    uint64_t getCurrentTimestamp() const;

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

    /// Getter for `marketPrice_`.
    uint256_t getMarketPrice() const { return this->marketPrice_.get(); }

    /// Getter for `midPrice_`.
    uint256_t getMidPrice() const { return this->midPrice_.get(); }

    /// Getter for `spread_`.
    uint256_t getSpread() const { return this->spread_.get(); }

    /**
     * Create a new bid order. Order type is automatically inferred by its price.
     * @param owner The address that wants to buy.
     * @param amountAssetA The amount of the asset the order wants to buy.
     * @param amountAssetB The amount of the asset the order wants to pay.
     * @param stop (optional) If `true`, treats the order as a stop order. Defaults to `false`.
     */
    void newBidOrder(
      const Address owner, const uint256_t amountAssetA,
      const uint256_t amountAssetB, bool stop = false
    );

    /**
     * Create a new ask order Order type is automatically inferred by its price.
     * @param owner The address that wants to sell.
     * @param amountAssetA The amount of the asset the order wants to sell.
     * @param amountAssetB The amount of the asset the order wants to receive.
     * @param stop (optional) If `true`, treats the order as a stop order. Defaults to `false`.
     */
    void newAskOrder(
      const Address owner, const uint256_t amountAssetA,
      const uint256_t amountAssetB, bool stop = false
    );

    /**
     * Cancel a previously made bid order.
     * Only the owner of the order can cancel it.
     * @param id The ID of the order to cancel.
     */
    void cancelBidOrder(const uint256_t id);

    /**
     * Cancel a previously made ask order.
     * Only the owner of the order can cancel it.
     * @param id The ID of the order to cancel.
     */
    void cancelAskOrder(const uint256_t id);

    /// Register the contract structure.
    static void registerContract() {
      ContractReflectionInterface::registerContract<
        OrderBook, const Address, const std::string&, const Address, const std::string&,
        ContractManagerInterface&, const Address&, const Address&, const uint64_t&,
        const std::unique_ptr<DB>&
      >(
        std::vector<std::string>{
          "nextOrderID_", "addressAssetA_", "addressAssetB_", "tickerAssetA_", "tickerAssetB_",
          "bids_", "asks_", "marketPrice_", "midPrice_", "spread_"
        },
        std::make_tuple("getMarketPrice", &OrderBook::getMarketPrice, "", std::vector<std::string>{}),
        std::make_tuple("getMidPrice", &OrderBook::getMidPrice, "", std::vector<std::string>{}),
        std::make_tuple("getSpread", &OrderBook::getSpread, "", std::vector<std::string>{}),
        std::make_tuple("newBidOrder", &OrderBook::newBidOrder, "", std::vector<std::string>{"owner", "amountAssetA", "amountAssetB"}),
        std::make_tuple("newAskOrder", &OrderBook::newAskOrder, "", std::vector<std::string>{"owner", "amountAssetA", "amountAssetB"}),
        std::make_tuple("cancelBidOrder", &OrderBook::cancelBidOrder, "", std::vector<std::string>{"id"}),
        std::make_tuple("cancelAskOrder", &OrderBook::cancelAskOrder, "", std::vector<std::string>{"id"})
      );
    }
};

#endif  // ORDERBOOK_H
