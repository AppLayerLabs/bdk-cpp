/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "websocketserver.h"

namespace BTVServer {
  bool WebsocketServer::setup() {
    // Setup tells the listener to start listening on the port
    Printer::safePrint("Websocket Server Setup");
    this->listener_.start();
    Printer::safePrint("Websocket Server Setup: DONE");
    return true;
  }

  void WebsocketServer::close() {
    // Close tells the listener to stop listening on the port
    Printer::safePrint("Websocket Server Close");
    this->listener_.close();
    Printer::safePrint("Websocket Server Close: DONE");
  }

}