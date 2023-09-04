#include "pausable.h"

/// This is done because pausable causes multiple definition errors
namespace Pausable {
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