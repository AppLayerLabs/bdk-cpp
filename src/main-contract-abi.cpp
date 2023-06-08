#include "contract/customcontracts.h"
#include "contract/erc20.h"
#include "utils/contractreflectioninterface.h"


int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " outputFilename" << std::endl;
        return 1;
    }

    std::string outputFilename = argv[1];

    ContractReflectionInterface::writeContractToJson<ERC20>(outputFilename);

    return 0;
}