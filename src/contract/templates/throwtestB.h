/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef THROWTESTB_H
#define THROWTESTB_H

#include "../dynamiccontract.h"
#include "../../utils/utils.h"
#include "throwtestC.h"

class ThrowTestB : public DynamicContract {
  private:
    SafeUint8_t num_;
    void registerContractFunctions() override;

  public:
    using ConstructorArguments = std::tuple<>;

    ThrowTestB(ContractManagerInterface &interface, const Address& address,
      const Address& creator, const uint64_t& chainId, const std::unique_ptr<DB> &db
    );

    ThrowTestB(ContractManagerInterface &interface, const Address& address, const std::unique_ptr<DB> &db);

    // Destructor.
    ~ThrowTestB() override;

    uint8_t getNumB() const;
    void setNumB(const uint8_t& valB, const Address& addC, const uint8_t& valC);

    static void registerContract() {
      ContractReflectionInterface::registerContract<
        ThrowTestB, ContractManagerInterface&, const Address&, const Address&, const uint64_t&, const std::unique_ptr<DB>&
      >(
        std::vector<std::string>{},
        std::make_tuple("getNumB", &ThrowTestB::getNumB, "view", std::vector<std::string>{}),
        std::make_tuple("setNumB", &ThrowTestB::setNumB, "nonpayable", std::vector<std::string>{"valB", "addC", "valC"})
      );
    }
};

#endif  // THROWTESTB_H
