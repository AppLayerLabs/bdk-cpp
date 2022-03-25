#message InitializeRequest {
#    uint32 networkID = 1;
#    bytes subnetID = 2;
#    bytes chainID = 3;
#    bytes nodeID = 4;
#    bytes xChainID = 5;
#    bytes avaxAssetID = 6;
#    bytes genesisBytes = 7;
#    bytes upgradeBytes = 8;
#    bytes configBytes = 9;
#
#    repeated VersionedDBServer dbServers = 10;
#    uint32 engineServer = 11;
#    uint32 keystoreServer = 12;
#    uint32 sharedMemoryServer = 13;
#    uint32 bcLookupServer = 14;
#    uint32 snLookupServer = 15;
#    uint32 appSenderServer = 16;
#}
#message VersionedDBServer {
#    uint32 dbServer = 1;
#    string version = 2;
#}

grpcurl -plaintext -d \
'{                                
  "networkID" : "1",
  "subnetID" : "0xaa",               
  "chainID" : "",
  "nodeID": "0xaa",
  "xChainID" : "0xaa",
  "avaxAssetID" : "0xaa",
  "genesisBytes" : "0xaa",
  "upgradeBytes" : "0xaa",
  "configBytes" : "0xaa",
  "dbServers" : [ 
    { 
      "dbServer" : "1", 
      "version" : "1"
    }
  ],
  "engineServer" : "1",
  "keystoreServer" : "1", 
  "sharedMemoryServer" : "1",
  "bcLookupServer" : "1",
  "snLookupServer" : "1",
  "appSenderServer" : "1"      
}' 127.0.0.1:50051 vmproto.VM.Initialize