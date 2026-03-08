## Introduction

In the stock trading world, the order book is the place where all active orders (both Buy and Sell) are maintained in
certain priority to match buy and sell orders. Orders (both Buy and Sell Sides) can be of different
types, for this challenge we consider the following order types:

* **Market**: A Market order is an order to buy or sell a symbol that must be matched at the best price on the other
  side
    - Market Buy: A Market Buy order specifies a quantity to purchase and is executed immediately against the lowest
      available sell prices in the order book until the requested quantity is fully filled or no more liquidity is
      available.
    - Market Sell: A Market Sell order specifies a quantity to sell and is executed immediately against the highest
      available buy prices in the order book until the requested quantity is fully filled or no more liquidity is
      available.
* A **Limit** order is an order to buy or sell a symbol at a specified price or better. It may be matched immediately if
  suitable liquidity exists, or it may remain in the order book awaiting future matches.
    - Limit Buy:A Limit Buy order specifies a maximum price and a quantity to purchase. It is matched immediately
      against available sell orders priced at or below the specified limit price, following
      price-time priority.
      Any unfilled quantity remains in the order book at the specified price.
    - Limit Sell:
      A Limit Sell order specifies a minimum price and a quantity to sell.
      It is matched immediately against available buy orders priced at or above the specified limit price, following
      price-time priority.
      Any unfilled quantity remains in the order book at the specified price.

* An **Immediate-or-Cancel (IOC)** order is an order that must be executed immediately, either fully or partially. Any
  portion of the order that cannot be matched immediately is automatically canceled and does not remain in the order
  book.
    - IOC Buy: An IOC Buy order specifies a maximum price and a quantity to purchase. It is matched immediately against
      available sell orders priced at or below the specified limit price.
      Any unfilled quantity is canceled immediately.
    - IOC Sell: An IOC Sell order specifies a minimum price and a quantity to sell. It is matched immediately against
      available buy orders priced at or above the specified limit price. Any unfilled quantity is canceled immediately.

## Instruction

The goal of this project is to design and implement an equity matcher engine which performs buy and sell sides of the
order book. The matcher receives an input stream consisting of buy and sell orders, and after receiving a match command,
it
should attempt to match any outstanding orders at that point in time. The matcher must ensure that the buy order with
the highest price is matched with the sell order with the lowest price. In case of multiple orders have the same price
on the given side (buy or sell), the matcher should pick the order on first come, first served based on time stamp.
System allows several
commands that consist of the following with the description below:

## Commands Description

1. Action
    * N(New)
    * A(Amend)
    * X(cancel)
    * M(Match)
    * Q(Query)

2. OrderID int
3. Timestamp – integer typically milliseconds since epoch
4. Symbol string Varying length string containing only alphabets
5. OrderType
    * M(Market), L(Limit), I(IOC)
6. Side: B(Buy), S(Sell)
7. Price float, with two places of decimal, 0.00 if OrderType is M
8. Quantity: integer.

### New (N) 👶

N command means that a new order is being requested to be entered into the matching book. The matcher should reject the
incoming order in case of duplicate order id or invalid input fields. The command is described as:

#### Input Command

`N,<OrderID>,<Timestamp>,<Symbol>,<OrderType>,<Side>,<Price>,<Quantity>`

#### Output

The matcher should output one of the following:

* `<OrderID> - Accept`
* `<OrderID> - Reject - 303 - Invalid order details`

#### Example

Commands are stream like the following:

```
N,2,00000002,XYZ,L,B,104.53,100 ✅
N,3,00000002,XYZ,L,B,104.53,100.3 ----> Quantity must be integer ❌
```

After the new command we should get

`2 - Accept`  
`3 - Reject - 303 - Invalid order details`

### Amend (A)

Amend means an existing order is being requested to be updated as per details in this command. A valid amend command
should have quantity and/or price amended, any other field update should result in error code.

#### Input Command

`A,<OrderID>,<Timestamp>,<Symbol>,<OrderType>,<Side>,<Price>,<Quantity>`

In case of quantity amended down, the amended order should not lose existing priority in the matching book. In all other
amended requests, the priority of amended order in a matching book may vary according to the latest price and timestamp.
Also note that partial amends should be supported. In case of a quantity amend request on a partially executed order, it
should be accepted. If the quantity in the amended request is less than or equal to the currently matched quantity, then
the order should be considered closed. In a scenario where the order is fully matched or already canceled, new amend
requests
should be rejected The command is described as

#### Output

* `<OrderID> - AmendAccept`
* `<OrderID> - AmendReject - 101 - Invalid amendement details`
* `<OrderID> - AmendReject - 404 - Order does not exist`

#### Example

```
N,2,00000001,XYZ,L,B,103.53,100 ---> timestamp and price change ✅
N,3,00000002,XYZ,L,B,104.53,100. ----> not allowed to change side ❌
```

After the new command we should get

* `2 - Accept`
* `2 - Reject - 303 - Invalid order details`

### Cancel (X)

The X (Cancel) command means an existing order is being requested to be canceled. Also note that partial cancels should
be supported. In scenarios where an order is fully matched or already canceled, new cancel requests should be rejected.
The
command is described as :

#### Input

* `X,<OrderID>,<Timestamp>`

#### Output

* `<OrderID> - CancelAccept`
* `<OrderID> - CancelReject - 101 - Invalid amendement details`

#### Example

```
X,1,0000001
X,2,0000002
X,2,0000002
```

```
1 - CancelAccept
2 - CancelAccept
2 - CancelReject - 404 - Orderd does not exist
```

### Match (M)

The M (Match) command means that the existing orders in the matching book should be matched now. The `Symbol` is an
optional parameter if specified would mean matching should be done only for the specified symbol else it should be done
for all symbols in alphabetical order the command is described by one of the following formats:

#### Input

* `M,<Timestamp>`
* `M,<Timestamp>,<Symbol>`  
  The matching result has three components which must be seperated by a pipe (`|`):
* `Symbol` This represents the matched symbol
* `MatchedBuy` This represents the information of matched buy. It should have:  
  `<OrderID>,<OrderType>,<Buy Qty Matched>,<Buy Price Matched>`
* `MatchedSell` This represents the information of matched sell. It should have:  
  `<Sell Price Matched>,<Sell Qty Matched>,<OrderType>,<OrderID>`

The matcher should output all resulting in the following format:  
`<Symbol>|<MatchedBuy>|<MatchedSell>`

If case of no matches, there shouldn't be any output. For example, consider the following commands

#### Example

```
N,1,0000001,ALN,L,B,60.90,100
N,11,0000002,XYZ,L,B,60.90,200
N,110,0000003,XYZ,L,S,60.90,100
N,112,0000003,XYZ,L,S,60.90,120
N,10,0000006,ALN,L,S,60.90,100
M,00010
M,00010,ALN
```

The matcher should output the following:

```
ALN|1,L,100,60.90|60.90,100,L,10
XYZ|11,L,100,60.90|60.90,100,L,110
XYZ|11,L,100,60.90|60.90,100,L,112
```

Note that, there is no output for the match command `M,00010,ALN` becasue after the match command `M,00010` there are no
buy or sell orders to be matched

### Query(Q)

The Query(Q) is a request to print the state of a matching book. `Symbol` and `Timestamp` are optional parameters. If
only symbol is specified would mean the state of the matching book should be printed for the given symbol. Of only timestamp
is specified would mean the state of the matching books should be printed. if no optional parameters are specified, then
matcher should print the state of the matching books as of now for all symbols in alphabetical order. The command is
described by one of the following formats:
* `Q`
* `Q,<Symbol>`
* `Q,<Timestamp>`
* `Q,<Timestamp>,<Symbol>`
* `Q,<Symbol>,<Timestamp>`

The state of a matching book should be described by the following three components which must be seperated by pipe (`|`)

* `Symbol` This represents the matched symbol
* `BuyState` This represents the information of matched buy. It should have:  
  `<OrderID>,<OrderType>,<Buy Qty>,<Buy Price>` format. If there are no corresponding buy orders this field should be blank
* `SellState` This represents the information of matched sell. It should have:  
  `<Sell Price>,<Sell Qty>,<OrderType>,<OrderID>` If there are no corresponding buy orders this field should be blank
The matcher should output the top five buy and sell entries as the state of the matching book for the given available symbols in the following format

`<Symbol>|<MatchedBuy>|<SellState>`
In case of Market orders, price should be pronted as $0.00$

#### Example 

```
N,1,00000001,ALN,L,B,60.90,100
N,13,00000002,ALN,L,B,60.90,100
N,10,00000003,ALN,L,S,60.90,100
N,12,00000004,ALN,L,S,60.90,100
N,11,00000005,ALB,L,S,60.90,100
N,14,00000006,ALB,L,S,62.90,101
N,16,00000007,ALB,L,S,63.90,102
N,18,00000008,ALB,L,S,64.90,103
N,20,00000009,ALB,L,S,65.90,104
Q,00000003
Q,ALB,
Q,ALN,00000002
Q,00000002,ALN
Q
```

The matcher should print the following:


### What data structure to choose for holding state of orders in the  book
To store orders in our book reasonable are the following choices:

| Structure           | Match Speed                  | Cancel/Amend | FIFO   | Complexity | Real-World Viability     |
|---------------------|------------------------------|--------------|--------|------------|--------------------------|
| std::priority_queue | 🟢 Fast (O(1))               | 🔴 No        | 🔴 No  | 🟢 Simple  | ❌ Unrealistic            |
| std::multiset       | 🟡 Good (O(log n))           | 🟢 Yes       | 🟢 Yes | 🟡 Medium  | ✅ Suitable for many      |
| map<price, deque>   | 🟢 Good (log n price levels) | 🟢 Yes       | 🟢 Yes | 🔵 Complex | ✅✅ Best for real trading 

We decided to take `map<prive,deque>` It is the solid choise becasue:

* Best bid / best ask in $O(1)$ via `begin()` (because the map is always sorted).
* Add / remove a price level in $O(log P)$ where `P` = number of distinct price levels.
* FIFO at each price is naturally handled by a deque (or list-like structure).

It seems to be the best trade off between complexity and performance.

### Performance Considerations 🚩

Even with a `map<Price, deque<Order>>` structure, we may encounter performance pitfalls during the matching phase.

The potential risk ⚠️ arises when we need to perform matching for a specific symbol.

Price-cross-matching itself is efficient because the best bid and the best ask can be accessed directly via
`map.begin()` ($O(1)$ for top-of-book access).

However, when matching per symbol, the situation becomes more complex.😔
To find the best order for a given symbol, we must additionally traverse through price levels and potentially scan the
entire deque at each level until we find a matching symbol.

In the worst case, this may significantly increase time complexity, potentially leading to near $O(n²)$ behavior during
heavy matching scenarios.

### Future Improvement 💉🚀

A more scalable solution would be to redesign the order book structure to maintain:

```
unordered_map<Symbol, BookForSymbol>
```

where each `BookForSymbol` maintains its own:

```
buyBook  -> map<Price, deque<Order>>
sellBook -> map<Price, deque<Order>>
```

This would efficiently restore the top of the book access per symbol and eliminate the need for repeated scans across
unrelated symbols.

For the time being (MVP version) we intentionally stick with the primary:

```
map<Price, deque<Order>>
```

## Manual run (dev_main) + sample command streams

This repo contains sample command streams you can use to simulate incoming requests without a full CLI layer that will
be implemented later.

#### Sample data

Command files live under:

- `testing_commands/` (e.g. `testing_commands/commands_part1.txt`)

## Build & run

If you build with CLion / CMake into `cmake-build-debug`, run:

```bash
./cmake-build-debug/dev_main testing_commands/commands_part1.txt
./cmake-build-debug/dev_main testing_commands/commands_for_matching.txt
```

## Example

Consider the following state before we decide to perform match

```
=== ORDER BOOK DUMP ===
BUY (highest -> lowest)
  price=104.65 | count=1
    Order{id=20, ts=9, sym=XYZ, type=I, side=B, price=104.65, qty=200}
  price=104.60 | count=2
    Order{id=1, ts=1, sym=XYZ, type=L, side=B, price=104.60, qty=100}
    Order{id=2, ts=7, sym=XYZ, type=L, side=B, price=104.60, qty=30}
SELL (lowest -> highest)
  price=104.60 | count=1
    Order{id=12, ts=6, sym=XYZ, type=L, side=S, price=104.60, qty=90}
  price=104.65 | count=1
    Order{id=11, ts=5, sym=XYZ, type=L, side=S, price=104.65, qty=60}
  price=104.70 | count=1
    Order{id=10, ts=4, sym=XYZ, type=L, side=S, price=104.70, qty=80}
```

After run match command we get:

```
========================
XYZ|20,I,90,10460|10460,90,L,12
XYZ|20,I,60,10465|10465,60,L,11
========================
```

And then new state of the book is following

```
== ORDER BOOK DUMP ===
BUY (highest -> lowest)
  price=104.65 | count=1
    Order{id=20, ts=9, sym=XYZ, type=I, side=B, price=104.65, qty=50}
  price=104.60 | count=2
    Order{id=1, ts=1, sym=XYZ, type=L, side=B, price=104.60, qty=100}
    Order{id=2, ts=7, sym=XYZ, type=L, side=B, price=104.60, qty=30}
SELL (lowest -> highest)
  price=104.70 | count=1
    Order{id=10, ts=4, sym=XYZ, type=L, side=S, price=104.70, qty=80}
========================
```
