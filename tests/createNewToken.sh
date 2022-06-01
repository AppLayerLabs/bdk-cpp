curl --location --request POST 'localhost:30000/' --header 'Content-Type: application/json' --data-raw '{
 "jsonrpc":"2.0",
 "method":"createNewToken",
 "params":{
        "name" : "SUBNET TOKEN",
        "symbol" : "SBTK",
        "decimals" : 18,
        "totalSupply" : "10000000000000000000000",
        "address" : "0x96dd1f16dc8a5d2d21040dd018d9d6b90039a4ac", 
        "balances": [
          {
            "address" : "0x798333f07163eb62d1e22cc2df1acfe597567882",
            "value" : "10000000000000000000000"
          }
        ],
        "allowances" : []
    },
 "id":1
}'