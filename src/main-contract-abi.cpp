#include "utils/jsonabi.h"

int main() {
    return JsonAbi::writeContractsToJson<ContractTypes>();
}