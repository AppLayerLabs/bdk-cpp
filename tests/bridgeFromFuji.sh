curl --location --request POST 'localhost:30000/' --header 'Content-Type: application/json' --data-raw '{
 "jsonrpc":"2.0",
 "method":"BridgeFrom",
 "txid":"0x6ecba8a8de65ce3cc2f310f1c16bf9d8caa3156f8ea75dcaf39d2b4ba6310db9",
 "id":1
}'