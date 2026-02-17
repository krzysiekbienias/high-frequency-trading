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
