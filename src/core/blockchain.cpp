#include "blockchain.h"



Blockchain::Blockchain(std::string blockchainPath) :
    options(std::make_unique<Options>(Options::fromFile(blockchainPath))),
    db(std::make_unique<DB>(blockchainPath + "/database")),
    storage(std::make_unique<Storage>(db, options)),
    rdpos(std::make_unique<rdPoS>(db, storage, p2p, options)),
    state(std::make_unique<State>(db, storage, rdpos, p2p)),
    p2p(std::make_unique<P2P::ManagerNormal>(boost::asio::ip::address::from_string("127.0.0.1"), rdpos, options, storage)),
    http(std::make_unique<HTTPServer>(state, storage, p2p, options)) {

}


void Blockchain::start() {
    p2p->startServer();
    http->start();
}