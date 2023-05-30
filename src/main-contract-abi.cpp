#include "contract/customcontracts.h"
#include "contract/nativewrapper.h"
#include "utils/contractreflectioninterface.h"


int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " outputFilename" << std::endl;
        return 1;
    }

    std::string outputFilename = argv[1];

    ContractReflectionInterface::writeContractToJson<NativeWrapper>(outputFilename);

    return 0;
}