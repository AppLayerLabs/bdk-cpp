#include "httpclient.h"
#include <boost/certify/extensions.hpp>
#include <boost/certify/https_verification.hpp>

std::string HTTPClient::fujiRequest(
    std::string reqBody
) {
  std::string result = "";
  using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
  namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
  namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

  // Lock mutex and get information from it.
  std::string url = "api.avax-test.network";
  std::string target = "/ext/bc/C/rpc";
  std::string port = "443";
  // Uncomment to see request details
  //std::cout << "url: " << url << std::endl;
  //std::cout << "target: " << target << std::endl;
  //std::cout << "port: " << port << std::endl;
  //std::cout << "reqBody: " << reqBody << std::endl;

  try {
    // Create context and load certificates into it
    boost::system::error_code ec;
    boost::asio::io_context ioc;
    std::cout << "1" << std::endl;
    ssl::context ctx{ssl::context::sslv23_client};
    std::cout << ":(" << std::endl;
    ctx.set_verify_mode(
      ssl::context::verify_peer | ssl::context::verify_fail_if_no_peer_cert
    );
    std::cout << "2" << std::endl;
    ctx.set_default_verify_paths();
    boost::certify::enable_native_https_server_verification(ctx);

    tcp::resolver resolver{ioc};
    std::cout << "3" << std::endl;
    ssl::stream<tcp::socket> stream{ioc, ctx};
    std::cout << "4" << std::endl;
    // Set SNI Hostname (many hosts need this to handshake successfully)
    boost::certify::sni_hostname(stream, url, ec);
    auto const results = resolver.resolve(url, port);

    // Connect and Handshake
    boost::asio::connect(stream.next_layer(), results.begin(), results.end());
    stream.handshake(ssl::stream_base::client);

    // Set up an HTTP POST/GET request message
    http::request<http::string_body> req{
      http::verb::get, target, 11
    };

      req.set(http::field::host, url);
      req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
      req.set(http::field::content_type, "application/json");
      req.body() = reqBody;
      req.prepare_payload();

    // Send the HTTP request to the remote host
    http::write(stream, req);
    boost::beast::flat_buffer buffer;

    // Declare a container to hold the response
    http::response<http::dynamic_body> res;

    // Receive the HTTP response
    http::read(stream, buffer, res);
    //std::cout << res.base().result() << std::endl;
    // Write only the body answer to output
    std::string body {
      boost::asio::buffers_begin(res.body().data()),
      boost::asio::buffers_end(res.body().data())
    };
    result = body;
    //Utils::logToDebug("API Result ID " + RequestID + " : " + result);
    //std::cout << "REQUEST RESULT: \n" << result << std::endl; // Uncomment for debugging

    stream.shutdown(ec);

    // SSL Connections return stream_truncated when closed.
    // For that reason, we need to treat this as an error.
    if (ec == boost::asio::error::eof || boost::asio::ssl::error::stream_truncated)
      ec.assign(0, ec.category());
    if (ec) {
      std::cout << "2 throwed " << boost::system::system_error{ec}.what() << std::endl;
      throw boost::system::system_error{ec};
    }
  } catch (std::exception const& e) {
    std::cout << "3 throwed " << e.what() << std::endl;
    throw std::string("HTTP Request error: ") + e.what();
  }

  //std::cout << "Return: " << result << std::endl;
  return result;
}



