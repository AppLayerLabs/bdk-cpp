#include "main.h"

// This stop function is a workaround to allow
// our subnet to be stopped without the need of
// AvalancheGo letting it know

// As of 05/07/22, the AvalancheGo daemon
// Simply kills the subnet, not letting it know through the grpcserver
// Or any other mean...

std::unique_ptr<Subnet> subnet;

void shutdown_handler(int sig) {
    subnet->stop();
}

int main() {
    
    std::signal(SIGINT, SIG_IGN);
    std::signal(SIGTERM, shutdown_handler);
    subnet = std::make_unique<Subnet>();
    subnet->start();

    return 0;
}
