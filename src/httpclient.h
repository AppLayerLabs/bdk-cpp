#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <future>
#include <string>
#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include "json.hpp"

using json = nlohmann::ordered_json;

// Struct for a JSON request.
typedef struct Request {
  uint64_t id;
  std::string jsonrpc;
  std::string method;
  json params;
} Request;  

namespace HTTPClient {

    std::string fujiRequest(std::string reqBody);;

    std::string buildRequest(Request req);

    std::string getNonce(std::string address);

    std::string getGasFees();

    std::string submitTransaction(std::string txid);

}


#endif // HTTP_CLIENT_H