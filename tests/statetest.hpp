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