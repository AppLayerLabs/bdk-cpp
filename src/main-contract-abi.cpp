#include "contract/customcontracts.h"
#include "utils/contractreflectioninterface.h"


int main() {
    return ContractReflectionInterface::writeContractsToJson<ERC20, ERC20Wrapper, NativeWrapper>();
}