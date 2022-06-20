#ifndef SUBNET_H
#define SUBNET_H


#include "block.h"

#endif // SUBNET_H

// The subnet class acts as being the middleman of every "module" of the subnet
// Every class originating from this, being the gRPC server/client or the inner
// validation status of the system.
// A given sub-module (let's say, the gRPC Server) does a request.
// the gRPC server will call a function on the Subnet class (as it has a reference for it) 
// And then the Subnet class will process the request, this means that the gRPC server cannot access directly
// another sub-module, it has to go through Subnet first.

class Subnet {
  private:

  public:
    void start();
    void stop();
};