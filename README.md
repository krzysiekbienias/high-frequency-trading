### v.001

### Description

We need to design and implement equity matcher engine which performs buy and sell sides of the order book. The matcher
receives an input stream consisting of buy and sell orders and after receiving a match command, it should attempt to
match any outstanding orders at that point on time. The matcher must ensure that buy order with highest price is matched
with sell order with the lowest price. In case of multiple orders have the same price on given side (buy or sell) the
matcher should pick order on first come first serve based on time stamp. System allows several commands, that consist of
following with description below :

### Commands Description

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

### N Command (new)
N command means that a new order is being requested to be entered into the matching book. The matcher should reject the incoming order on case of duplicate order Id or invalid input fields. The command is described as:  
`N,<OrderID>,<Timestamp>,<Symbol>,<OrderType>,<Side>,<Price>,<Quantity>`

The matcher should output one of the following:
* `<OrderID> - Accept`
* `<OrderID> - Reject - 303 - Invalid order details`

#### Input Command
Commands are stream like following:
```
N,2,00000002,XYZ,L,B,104.53,100 ✅
N,3,00000002,XYZ,L,B,104.53,100.3 ----> Quantity must be integer ❌
```
#### Output
After the new command we should get

`2 - Accept`  
`3 - Reject - 303 - Invalid order details`

### A Command (amend)
Amend means an existing order is being requested to be updated as per details in this command. A valid amend command should have quantity and/or price amended, any other field update should result into error code.

`A,<OrderID>,<Timestamp>,<Symbol>,<OrderType>,<Side>,<Price>,<Quantity>`

In case of quantity amend down, the amended order should not lose existing priority in the matching book. In all other amend requests , the priority of amended order in matching book may vawy according to the latest price and timestamp. Also note that partial amends should be supported. In case of a quantity amend request on a partially executed order, it should be accepted. If the quantity in the amend request is less than or equal to the currently matched quantity then order should be considered closed. In scenario where order is fully matched or already canceled, new amend requests should be rejected The command is described as

* `<OrderID> - AmendAccept`
* `<OrderID> - AmendReject - 101 - Invalid amendement details`
* `<OrderID> - AmendReject - 404 - Order does not exist`

#### Input Command
For example:
```
N,2,00000001,XYZ,L,B,103.53,100 ---> timestamp and price change ✅
N,3,00000002,XYZ,L,B,104.53,100. ----> not allowed to change side ❌
```
#### Output
After the new command we should get

* `<OrderID> - Accept`
* `<OrderID> - Reject - 303 - Invalid order details`