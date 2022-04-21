curl --location --request POST 'localhost:30000/' --header 'Content-Type: application/json' --data-raw '{
 "jsonrpc":"2.0",
 "method":"createNewToken",
 "params":{
        "name" : "SUBNET TOKEN",
        "symbol" : "SBTK",
        "decimals" : 18,
        "totalSupply" : "10000000000000000000000",
        "address" : "0x010101010101010101010174657374746f6b656e", 
        "balances": [
          {
            "address" : "0xe6a2d1ef7d7129d2a422af0a725629a0a1fbdec4",
            "value" : "10000000000000000000000"
          }
        ],
        "allowances" : []
    },
 "id":1
}'