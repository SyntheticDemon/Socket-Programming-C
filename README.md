# Socket-Programming-C
Simple Socket Programming done in C
-> make ()
#entry points
```
./buyer <port> <Chosen_Name>
```

```
./seller <port> <Chosen_Name>

```

## Seller
Setup new Sale using
States can be of 3 types for this to work correctly -> Open,Negotiation,Closed

```Send_Sale <port>,<Sale_Name>,<Initial_State>```

```Accept <Sale_Name>```

```Reject <Sale_Name>```

## Buyer
See all sale suggestions

``` list ```  


Negotiate with seller 
``` negotiate <port> <message> ```