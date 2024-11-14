#include "abcinetserver.h"

#include "abcinetsession.h"

// ???
#include "abcihandler.h"

#include "../../utils/logger.h"

ABCINetServer::ABCINetServer(ABCIHandler* handler, asio::io_context& io_context, const std::string& socket_path)
    : handler_(handler), io_context_(io_context), acceptor_(io_context, stream_protocol::endpoint(socket_path)), failed_(false)
{
    // Do not call do_accept() here
}

void ABCINetServer::start() {
    do_accept();
}

void ABCINetServer::do_accept() {
    auto self(shared_from_this());
    acceptor_.async_accept([this, self](boost::system::error_code ec, stream_protocol::socket socket) {
        if (!ec) {
            auto session = std::make_shared<ABCINetSession>(handler_, std::move(socket), self);
            sessions_.insert(session);
            session->start();
        } else {
            // Notify failure
            notify_failure("Error accepting connection: " + ec.message());
        }
        if (!failed_) {
            do_accept();
        }
    });
}

void ABCINetServer::notify_failure(const std::string& reason) {
    if (!failed_) {
        failed_ = true;
        reason_ = reason;
        // Close all active sessions
        for (auto& weak_session : sessions_) {
            if (auto session = weak_session.lock()) {
                //std::cout << "Closing one session" << std::endl;
                session->close();
            }
        }
        // Stop accepting new connections
        //std::cout << "Closing acceptor" << std::endl;
        GLOGXTRACE("Before acceptor close");
        acceptor_.close();
        GLOGXTRACE("After acceptor close");
        //std::cout << "Acceptor closed" << std::endl;
        // Stop the io_context
        //no need to do this: once the acceptor is closed, the io context run calls all exit
        //  since there is no more work -- no connections/sessions and no way to get more sessions.
        //std::cout << "Stopping io_context" << std::endl;
        //io_context_.stop();
    }
}

bool ABCINetServer::failed() const {
    return failed_;
}

const std::string& ABCINetServer::reason() const {
    return reason_;
}
