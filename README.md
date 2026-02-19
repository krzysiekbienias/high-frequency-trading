## Introduction

In the stock trading world, the order book is the place where all active orders (both Buy and Sell) are maintained in
certain priority to match buy and sell orders. Orders (both Buy and Sell Sides) can be of different
types, for this challenge we consider the following order types:

* **Market**: A Market order is an order to buy or sell a symbol that must be matched at best price on the other side
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
order
book. The matcher receives an input stream consisting of buy and sell orders, and after receiving a match command, it
should attempt to match any outstanding orders at that point in time. The matcher must ensure that buy order with
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
3. Tiemestamp - integer typically milliseconds since epoch
4. Symbol string Varying length string containing only alphabets
5. OrderType
    * M(Market), L(Limit), I(IOC)
6. Side: B(Buy), S(Sell)
7. Price float, with two places of decimal, 0.00 if OrderType is M
8. Quantity: integer.

### New (N)

N command means that a new order is being requested to be entered into the matching book. The matcher should reject the
incoming order on case of duplicate order Id or invalid input fields. The command is described as:

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
should have quantity and/or price amended, any other field update should result into error code.

#### Input Command

`A,<OrderID>,<Timestamp>,<Symbol>,<OrderType>,<Side>,<Price>,<Quantity>`

In case of quantity amend down, the amended order should not lose existing priority in the matching book. In all other
amend requests , the priority of amended order in matching book may vawy according to the latest price and timestamp.
Also note that partial amends should be supported. In case of a quantity amend request on a partially executed order, it
should be accepted. If the quantity in the amend request is less than or equal to the currently matched quantity then
order should be considered closed. In scenario where order is fully matched or already canceled, new amend requests
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
be supported. In scenarios where order is fully matched or already canceled, new cancel requests should be rejected. The
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
```



