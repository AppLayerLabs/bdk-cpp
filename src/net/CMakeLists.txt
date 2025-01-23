set(NET_HEADERS
  ${CMAKE_SOURCE_DIR}/src/net/abci/abciserver.h
  ${CMAKE_SOURCE_DIR}/src/net/abci/abcinetserver.h
  ${CMAKE_SOURCE_DIR}/src/net/abci/abcinetsession.h
  ${CMAKE_SOURCE_DIR}/src/net/http/httpclient.h
  ${CMAKE_SOURCE_DIR}/src/net/http/httpparser.h
  ${CMAKE_SOURCE_DIR}/src/net/http/httpsession.h
  ${CMAKE_SOURCE_DIR}/src/net/http/httplistener.h
  ${CMAKE_SOURCE_DIR}/src/net/http/httpserver.h
  PARENT_SCOPE
)

set(NET_SOURCES
  ${CMAKE_SOURCE_DIR}/src/net/abci/abciserver.cpp
  ${CMAKE_SOURCE_DIR}/src/net/abci/abcinetserver.cpp
  ${CMAKE_SOURCE_DIR}/src/net/abci/abcinetsession.cpp
  ${CMAKE_SOURCE_DIR}/src/net/http/httpclient.cpp
  ${CMAKE_SOURCE_DIR}/src/net/http/httpparser.cpp
  ${CMAKE_SOURCE_DIR}/src/net/http/httpsession.cpp
  ${CMAKE_SOURCE_DIR}/src/net/http/httplistener.cpp
  ${CMAKE_SOURCE_DIR}/src/net/http/httpserver.cpp
  ${CMAKE_SOURCE_DIR}/src/net/http/jsonrpc/call.cpp
  ${CMAKE_SOURCE_DIR}/src/net/http/jsonrpc/parser.cpp
  ${CMAKE_SOURCE_DIR}/src/net/http/jsonrpc/blocktag.cpp
  PARENT_SCOPE
)

