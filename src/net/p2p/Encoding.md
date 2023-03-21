# P2P Encoding.

### P2P Handshake

The P2P WebSocket handshake is done through a HTTP request, meaning we can include extra information within that HTTP header.
We encode the following information within the HTTP header:


| Header            | Info                    | 
| ----------------- | ----------------------- |
| X-Node-Id         | Host node ID            | 
| X-Node-Type       | Host node Type          |
| X-Node-ServerPort | Host node Server Port   |

The HTTP encoding has to follow certain rules, such as being UTF-8 encoded (cannot send raw bytes), because of that, X-Node-Id is a hex string, X-Node-Type and X-Node-ServerPort are a string number (e.g. "8080")

---
### P2P::Message

structure that holds any type of P2P Message, it is encoded as following: 


| Variable     | Size      | Info                                        | 
| -------------| --------- | ------------------------------------------- |
| Request Flag | 1 Byte    | Request Type (Answer, request or broadcast) |
| Random ID    | 8 Bytes   | Request ID                                  |
| Command ID   | 2 Bytes   | Command ID                                  |
| Payload      | X Bytes   | Message Command Payload                     |

All hexes displayed in this document are interpreted as bytes

- Request Flag 

Used to tell if the request is a "request" (0), an "answer" (1) to a previous "request", or a "broadcast" (2) request (verify and broadcast towards other nodes of the network)

A Request will always wait and have an respective Answer while broadcast are processed directly, see the difference between functions called by handleAnswer/handleRequest and functions called by handleBroadcast. the randomId of the broadcast request is used to know if the node has previously received that broadcast and if it should broadcast again.

- Random ID

Used to tell different requests appart and to make async requests, during a request, the randomId is calculated with 8 random bytes, during a answer, the randomId is equal to the randomId of the respective request, if broadcast request, RandomID = SafeHash(payload).

- Command ID

Used to tell which command type is encoded in the payload, see more info below

- Payload

Used for payload of the request/answer (if needed)

Given the example  	```0x01adf01827349cad810002123456789abcdef```
we can extract the following values:
- Request Flag: 01
- Random ID: adf01827349cad81
- Command ID: 0002
- Payload: 123456789abcdef

---
## Commands

### Ping

Pings another node.

Ping command ID: \x00\x00

Request Type: Answer or Request only, No broadcast.

Request Payload: Empty

Request example: ```0x00adf01827349cad810000```

Answer Payload: Empty

Answer Example:  ```0x01adf01827349cad810000```


---
### Info (Not yet implemented)

Requests a basic list of information from another node, also sends ours.

Ping Command ID: \x00\x01

Request Type: Answer or Request only, No broadcast

Request Payload: Node version + Epoch + latest nHeight + latest nHash + nConnectedNodes

| Variable    | Size     | Info                             |
| ----------- | -------- | -------------------------------- |
| version     | 8 Bytes  | Node Version                     |
| Epoch       | 8 Bytes  | Timestamp in UNIX microseconds   |
| nHeight     | 8 Bytes  | latest block nHeight             |
| Hash        | 32 Bytes | latest block hash                | 

Request Example: ```0x00adf01827349cad81000100000000000000010005f70436085980000000000000000156aef6ce3d9cefb653ea6a53fc8e810c95268c01428223a8ee267ed2ac9f05d8```

Answer Payload: same as request.

---
### requestNodes

Request a list of connected nodes to a given node.

requestNodes command ID: \x00\x02

Request Type: Answer or Request only, No broadcast.

Request Payload: empty

Request Example: ```0x00adf01827349cad810002```

Answer Payload: Array of address type + address + port (only connections towards servers)

| Variable                | Size          | Info                               |
| ----------------------- | ------------- | ---------------------------------- |
| Node Type (Repeatable)  | 1 Byte        | Node Type (Normal or Discovery)    |
| Node ID (Repetable)     | 32 Bytes      | Node ID                            |
| Type (Repeatable)       | 1 byte        | Address type (v4 or v6)            |  
| Address (Repeatable)    | 4 or 16 Bytes | Address                            |
| Port (Repetable)        | 2 Bytes       | Node Server Port                   |

Example, node has connections to servers 127.0.0.1:8080 and 127.0.0.1:8081 and gets requested with a requestNodes, it will encode an address as following, Address = ``` Bytes(NodeType) + Bytes(NodeID) + "0x00"/"0x01" depending on address type + Bytes(address) + Bytes(port)```, the total encoded string will be ```Answer + RandomID + Command ID + Address¹ + Address²```

Normal Node at 127.0.0.1:8080, Node ID = 71fb09a7e9dc4fd2a85797bc92080d49ead60ebaa2b562d817bdfb31bc258134, encoded ```0x0071fb09a7e9dc4fd2a85797bc92080d49ead60ebaa2b562d817bdfb31bc258134007f0000011f90```


Discovery Node 127.0.0.1:8081, Node ID = 28fbed1f738503d325acfede76f1d96f0f7d2d4ca7148170d824b9f00c420a5a, Encoded ```0x0128fbed1f738503d325acfede76f1d96f0f7d2d4ca7148170d824b9f00c420a5a007f0000011f91```

Total encoded string = ```0x00adf01827349cad8100020071fb09a7e9dc4fd2a85797bc92080d49ead60ebaa2b562d817bdfb31bc258134007f0000011f900128fbed1f738503d325acfede76f1d96f0f7d2d4ca7148170d824b9f00c420a5a007f0000011f91```


---
### RequestValidatorTxs

Request TxValidator memory pool of another node.

requestNodes command ID: \x00\x03

Request Type: Answer or Request only, No broadcast.

Request Payload: empty

Request Example: ```0x00adf01827349cad810003```

Answer Payload: nTxCount + [ 4 Bytes tx size + N Bytes Tx data ]\x00\x04

| Variable                | Size          | Info                               |
| ----------------------- | ------------- | ---------------------------------- |
| nTx                     | 4 Byte        | Tx Counter                         |
| TxSize (Repeatable)     | 4 Bytes       | Tx Size                            |
| TxData (Repeatable)     | N byte        | Transaction RLP encoded data       |  

Example, node has two transactions within mempool with the following rlp
```f86ba4cfffe74621e0caa14dfcb67a6f263391f4a6ad957ace256f3e706b6da07517a63295165701823f44a08cfdf3826149ca0317eaf4f5832c867a4f5050e3e70d635323947d61a4f35618a07dbe86e6a8cef509c7c6174cb8c703ddd8cb695511d72043630d99888ff2ba20```
and ```f86ba4cfffe74621e0caa14dfcb67a6f263391f4a6ad957ace256f3e706b6da07517a63295165701823f44a01545c0c89ad5fda9e4c6ef65f264ef575fa2edebef29d577f88d365ff9d28357a00d9ed64e1675315477aca44908148b9b572c7de45d420398193dcfc2d430d158```

To encode this, the node has to encode the number of transactions (0x00000002), and then append the transactions size and txdata in a ordered manner.

The result Message would be ```0x01adf01827349cad810003000000020000006df86ba4cfffe74621e0caa14dfcb67a6f263391f4a6ad957ace256f3e706b6da07517a63295165701823f44a08cfdf3826149ca0317eaf4f5832c867a4f5050e3e70d635323947d61a4f35618a07dbe86e6a8cef509c7c6174cb8c703ddd8cb695511d72043630d99888ff2ba200000006df86ba4cfffe74621e0caa14dfcb67a6f263391f4a6ad957ace256f3e706b6da07517a63295165701823f44a01545c0c89ad5fda9e4c6ef65f264ef575fa2edebef29d577f88d365ff9d28357a00d9ed64e1675315477aca44908148b9b572c7de45d420398193dcfc2d430d158```

---
### BroadcastValidatorTx

Broadcast a given TxValidator to all know nodes connected to us, they should validate and rebroadcast

BroadcastValidatorTx command ID: \x00\x04

Request Type: Broadcast Only

Payload: TxRLP

Example:

```0x02adf01827349cad810004f86ba4cfffe74621e0caa14dfcb67a6f263391f4a6ad957ace256f3e706b6da07517a63295165701823f44a01545c0c89ad5fda9e4c6ef65f264ef575fa2edebef29d577f88d365ff9d28357a00d9ed64e1675315477aca44908148b9b572c7de45d420398193dcfc2d430d158```


