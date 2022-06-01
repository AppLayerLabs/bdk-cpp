curl --location --request POST 'localhost:30000/' --header 'Content-Type: application/json' --data-raw '{
 "jsonrpc":"2.0",
 "method":"BridgeFrom",
 "txid":"0xf6a77c53ceaf9c01ee5b76b2ab3eebcdc395c89e39f06be4cabe267909f6ef51",
 "id":1
}'