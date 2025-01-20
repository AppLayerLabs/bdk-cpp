/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef STATETEST_HPP
#define STATETEST_HPP

#include "../src/core/state.h"

#include "../src/contract/contracthost.h"


// TestState class
// The only purpose of this class is to be able to allow
// Direct call to internal methods of State class
// That is not necessary in production code.
// This is done to better isolate the code and test it.

class StateTest : public State {
  public:
    // StateTest has the same constructor as State
    StateTest(const DB& db, Storage& storage, P2P::ManagerNormal& p2pManager, const uint64_t& snapshotHeight, const Options& options) :
      State(db, storage, p2pManager, snapshotHeight, options) {};
};

#endif // STATETEST_HPP
