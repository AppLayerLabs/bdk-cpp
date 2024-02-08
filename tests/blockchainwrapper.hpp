/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef BLOCKCHAINWRAPPER_H
#define BLOCKCHAINWRAPPER_H

#include "../src/core/storage.h"
#include "../src/core/rdpos.h"
#include "../src/core/state.h"
#include "../src/net/p2p/managernormal.h"
#include "../src/net/http/httpserver.h"
#include "../src/utils/options.h"
#include "../src/utils/db.h"
#include "../src/core/blockchain.h"
#include "../src/utils/utils.h"

/**
 * Simple wrapper struct for management all blockchain related objects
 * Allow direct access to the underlying objects, does not apply any logic or checks.
 * But initialize them properly in the constructor
 */

struct TestBlockchainWrapper {
  const Options options;        ///< Pointer to the options singleton.
  DB db;                  ///< Pointer to the database.
  Storage storage;        ///< Pointer to the blockchain storage.
  State state;            ///< Pointer to the blockchain state.
  rdPoS rdpos;            ///< Pointer to the rdPoS object (consensus).
  P2P::ManagerNormal p2p; ///< Pointer to the P2P connection manager.
  HTTPServer http;        ///< Pointer to the HTTP server.
  explicit TestBlockchainWrapper(const Options& options_) :
    options(options_),
    db(options.getRootPath() + "/db"),
    storage(db, options),
    state(db, storage, rdpos, p2p, options),
    rdpos(db, storage, p2p, options, state),
    p2p(boost::asio::ip::address::from_string("127.0.0.1"), rdpos, options, storage, state),
    http(state, storage, p2p, options) {};
};

#endif // SDKTESTSUITE_H