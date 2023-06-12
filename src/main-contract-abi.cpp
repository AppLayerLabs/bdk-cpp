#include "contract/customcontracts.h"

int main() {
    return ContractReflectionInterface::writeContractsToJson<ERC20, ERC20Wrapper, NativeWrapper>();
}