# OrderBook Contract

The OrderBook contract is a contract that allows users to place orders to buy or sell a token. The contract will match orders and execute them atomically. It follows the rules of a limit orderbook market. See the link for more information:
https://www.machow.ski/posts/2021-07-18-introduction-to-limit-order-books/

Supported order types: 

- Buy Market
- Sell Market
- Buy Limit
- Sell Limit
- Stop Buy Limit
- Stop Sell Limit

Buy/Sell market will execute the order until it is filled at any price, while Buy/Sell limit will execute the order at a specific price or better, possibly creating putting the Order into the book if not filled

## Concepts

All OrderBook contracts mandate that tokens within the contract possess a 
minimum of 8 decimal places. This requirement stems from the method the
contract employs to determine order prices.

In every contract, the A Token is designated as the "lot" token, while
the B Token functions as the "tick" token. The contract invariably 
calculates order prices using the value of the B Token. For instance,
if DAI is chosen as token A and ETH as token B, the price of each order
will be expressed in ETH, with lots quantified in DAI.

The sizes of ticks and lots are derived from the decimal count of the 
tokens, adhering to the formula 10^(tokenDecimals - 4). As an example,
pairing a BTC token (8 decimals) with a WETH Token (18 decimals) results 
in a lot size for BTC of 10,000 WEI, and a tick size for WETH of 100,000,000,000,000 WEI.

A "lot" signifies the minimum quantity of tokens permissible for trade.
In a contract pairing BTC (8 decimals) with WETH (18 decimals), the lot
size is set at 10000 WEI, indicating the smallest tradable
BTC unit is 0.0001 BTC, equivalent to 10000 WEI.

"Ticks" represent the minimal price variation allowable between orders.
In a BTC (8 decimals) and WETH (18 decimals) pairing, the tick size is
10000000000000 WEI, denoting the smallest permissible price fluctuation is 0.00001 WETH.

The Order struct incorporates a field named amountAsset, utilized to
store the quantity of lots available in the order, irrespective of its side.

Functions related to order creation operate in terms of lots/ticks.
Therefore, it is necessary to convert the token amounts into lots/ticks, 
based on the respective lot/tick size, before initiating the function.

Each order generated is assigned a unique, sequentially incrementing ID,
starting from 1. StopLimit orders, upon activation (converted to a standard
Order and executed), retain the ID of the original order.

Orders within the contracts are classified as either Bid or Ask, indicating 
whether the order is intended for buying or selling.

The contract has a precision value, which tells the difference between tick/lot size to the actual token value
## Custom Data Structures

### struct: Order

```cpp
  0 - const uint256_t  - id          - Sequential unique ID of the order.
  1 - const uint       - timestamp   - The epoch timestamp of the order's creation.
  2 - const Address    - owner       - The address that made the order.
  3 - uint256_t        - amountAsset - The unit price of the asset the order has to offer in ticks of tokenB.
  4 - const uint256_t  - assetPrice  - The unit price of the asset the order has to offer in WEI of tokenB.
```

### Struct: StopOrder

```cpp
 0 - const uint256_t  - id          - Sequential unique ID of the order.
 1 - const uint       - timestamp   - The epoch timestamp of the order's creation.
 2 - const Address    - owner       - The address that made the order.
 3 - uint256_t        - amountAsset - The amount of the lots the order has to offer.
 4 - const uint256_t  - assetPrice  - The unit price of the asset the order has to offer in ticks of tokenB.
 5 - const uint256_t  - stopLimit   - The stop limit price of the order (only for stop limit orders), in WEI.
 6 - const OrderSide  - side        - Whether the order originally is a bid or ask.
 7 - const OrderType  - type        - Whether the order originally is a market or limit.
```

### Enum: OrderType
    
```cpp
0 - Market
1 - Limit
2 - Stop Limit
```

### Enum: OrderSide

```cpp
 0 - Bid
 1 - Ask
```

## Contract Functions:

```cpp
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
    std::tuple<std::vector<Order>, std::vector<Order>, std::vector<StopOrder>> getUserOrders(const Address& user) const;    
```

