/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef SDKTESTSUITE_H
#define SDKTESTSUITE_H

/**
 * Helper class for seamlessly managing blockchain components during testing
 * (performing txs, creating and calling contracts).
 */
class SDKTestSuite {
  private:

    // Test listen P2P port number generator needs to be in SDKTestSuite due to createNewEnvironment(),
    //   which selects the port for the caller.
    // This should be used by all tests that open a node listen port, not only SDKTestSuite tests.
    static int p2pListenPortMin_;
    static int p2pListenPortMax_;
    static int p2pListenPortGen_;

  public:

    /// Get next P2P listen port to use in unit tests.
    static int getTestPort() {
      int tries = 1000;
      boost::asio::io_context io_context;
      while (true) {
        if (p2pListenPortGen_ > p2pListenPortMax_) {
          p2pListenPortGen_ = p2pListenPortMin_;
        } else {
          ++p2pListenPortGen_;
        }
        boost::asio::ip::tcp::acceptor acceptor(io_context);
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), p2pListenPortGen_);
        boost::system::error_code ec, ec2;
        acceptor.open(endpoint.protocol(), ec);
        if (!ec) {
          acceptor.bind(endpoint, ec);
          acceptor.close(ec2);
          if (!ec) {
            return p2pListenPortGen_;
          }
        }
        if (--tries <= 0) {
          SLOGFATAL_THROW("Exhausted tries while searching for a free port number");
        }
      }
    }
};

#endif // SDKTESTSUITE_H
