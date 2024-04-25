/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef BLOCKCHAINWRAPPER_H
#define BLOCKCHAINWRAPPER_H

#include "../src/core/storage.h"
#include "../src/core/state.h"
#include "../src/net/p2p/managernormal.h"
#include "../src/net/http/httpserver.h"
#include "../src/utils/options.h"
#include "../src/utils/db.h"
#include "../src/core/blockchain.h"
#include "../src/utils/utils.h"

/**
 * Simple wrapper struct for management of all blockchain related objects.
 * Initializes them properly in the constructor, allowing direct access to the
 * underlying objects, but does not apply any logic or checks.
 */
struct TestBlockchainWrapper {
  const Options options;   ///< Options singleton.
  DB db;                   ///< Database.
  Storage storage;         ///< Blockchain storage.
  State state;             ///< Blockchain state.
  P2P::ManagerNormal p2p;  ///< P2P connection manager.
  HTTPServer http;         ///< HTTP server.

  /**
   * Constructor.
   * @param options_ Reference to the Options singleton.
   */
  explicit TestBlockchainWrapper(const Options& options_) :
    options(options_),
    db(options.getRootPath() + "/db"),
    storage(db, options_),
    state(db, storage, p2p, options),
    p2p(boost::asio::ip::address::from_string("127.0.0.1"), options, storage, state),
    http(state, storage, p2p, options) {};

  /// Destructor.
  ~TestBlockchainWrapper() {
    state.rdposStopWorker();
    p2p.stopDiscovery();
    p2p.stop();
    http.stop();
  }
};

#endif // BLOCKCHAINWRAPPER_H
