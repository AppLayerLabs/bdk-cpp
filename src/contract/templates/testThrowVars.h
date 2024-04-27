#ifndef TESTTHROWVARS_H
#define TESTTHROWVARS_H

#include "../dynamiccontract.h"
#include "../variables/safestring.h"

class TestThrowVars : public DynamicContract {
  private:
    SafeString var1_;
    SafeString var2_;
    SafeString var3_;
    void registerContractFunctions() override;

  public:
    using ConstructorArguments = std::tuple<const std::string&, const std::string&, const std::string&>;

    TestThrowVars(
      const std::string& var1, const std::string& var2, const std::string& var3,
      const Address& address,
      const Address& creator, const uint64_t& chainId
    );

    TestThrowVars(
      const Address& address, const DB& db
    );

    ~TestThrowVars() override;

    static void registerContract() {
      ContractReflectionInterface::registerContractMethods<
        TestThrowVars, const std::string&, const std::string&, const std::string&,
        const Address&, const Address&, const uint64_t&, DB&
      >(
        std::vector<std::string>{"var1_", "var2_", "var3_"}
      );
    }

    /// Dump method
    DBBatch dump() const override;
};

#endif  // TESTTHROWVARS_H
