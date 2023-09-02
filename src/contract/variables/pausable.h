#ifndef PAUSABLE_H
#define PAUSABLE_H

#include "safebool.h"

/**
 * Simple namespace for allowing the same functionallity as OpenZeppelin Pausable contract
 * Which effectively allows for pausing and unpausing of the contract
 * In Solidity, it takes advantage of the modifier pattern
 * Where this doesn't exist on C++, for that the logic is as follows:
 * The Contract has a SafeBool variable of type PausableActor
 * And uses the pause_contract(), unpause_contract(), requireNotPaused(), requirePaused() and isPaused functions
 * Passing the SafeBool variable as the first argument
 */

namespace Pausable {
  struct PausableActor {
    SafeBool paused_;
    PausableActor(DynamicContract *contract) : paused_(contract) {}
  };

  void pause(PausableActor& actor) {
    actor.paused_ = true;
  }

  void unpause(PausableActor& actor) {
    actor.paused_ = false;
  }

  bool isPaused(PausableActor& actor) {
    return actor.paused_.get();
  }

  void requireNotPaused(PausableActor& actor) {
    if (actor.paused_) {
      throw std::runtime_error("Pausable: Contract is paused");
    }
  }

  void requirePaused(PausableActor& actor) {
    if (!actor.paused_) {
      throw std::runtime_error("Pausable: Contract is not paused");
    }
  }
}




#endif PAUSABLE_H