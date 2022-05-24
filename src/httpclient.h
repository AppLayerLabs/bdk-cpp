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

// For convenience.

namespace HTTPClient {

    std::string fujiRequest(std::string reqBody);;

}


#endif // HTTP_CLIENT_H