#ifndef _ABCIHANDLER_H_
#define _ABCIHANDLER_H_

#include "cometbft/abci/v1/types.pb.h"

/**
 * The implementor of ABCIHandler is the actual handler of ABCI requests
 * received from cometbft. Client code should implement this interface
 * and give an instance of this implementor via the ABCIServer constructor.
 */
class ABCIHandler {
  public:
    virtual void echo(const cometbft::abci::v1::EchoRequest& req, cometbft::abci::v1::EchoResponse* res) = 0;

    // FIXME: specify all callbacks here
};

#endif
