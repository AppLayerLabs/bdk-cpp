/*
  Copyright (c) [2023-2024] [Sparq Network]

  This software is distributed under the MIT License.
  See the LICENSE.txt file in the project root for more information.
*/

#ifndef DUMP_H
#define DUMP_H

#include <vector>

/**
 * Abstraction of the dumpable object.
 */
class Dumpable {
public:
  /**
   * Pure virtual function to be implemented.
   * The function should dump implemented by the methods that are dumpable.
   */
  virtual void dump() = 0;
};

/**
 * Dumpable management.
 * Used to store dumpable objects in memory.
 */
class DumpManager {
private:
  std::vector<Dumpable*> dumpable_;
public:
  /**
   * Function that will register (push) a dumpable object
   * to dumplable_ vector.
   * @param dumpable The reference to be added.
   */
  void pushBack(Dumpable* dumpable) { dumpable_.push_back(dumpable); }

  /**
   * Will call all the dump functions from the dumpable objects
   * contained in the dumpable_ vector.
   */
  void dumpAll()
  {
    for (const auto dumpable: dumpable_)
      dumpable->dump();
  }
};

#endif // DUMP
