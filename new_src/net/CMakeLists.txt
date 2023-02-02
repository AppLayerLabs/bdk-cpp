
if(BUILD_AVALANCHEGO)
  set(NET_HEADERS
    ${CMAKE_SOURCE_DIR}/new_src/net/grpcclient.h
    ${CMAKE_SOURCE_DIR}/new_src/net/grpcserver.h
    ${CMAKE_SOURCE_DIR}/new_src/net/httpserver.h
    ${CMAKE_SOURCE_DIR}/new_src/net/p2pclient.h
    ${CMAKE_SOURCE_DIR}/new_src/net/p2pencoding.h
    ${CMAKE_SOURCE_DIR}/new_src/net/p2pmanager.h
    ${CMAKE_SOURCE_DIR}/new_src/net/p2pserver.h
    PARENT_SCOPE
  )

  set(NET_SOURCES
    ""
    PARENT_SCOPE
  )
else()
  set(NET_HEADERS
    ${CMAKE_SOURCE_DIR}/new_src/net/httpserver.h
    ${CMAKE_SOURCE_DIR}/new_src/net/p2pclient.h
    ${CMAKE_SOURCE_DIR}/new_src/net/p2pencoding.h
    ${CMAKE_SOURCE_DIR}/new_src/net/p2pmanager.h
    ${CMAKE_SOURCE_DIR}/new_src/net/p2pserver.h
    ${CMAKE_SOURCE_DIR}/new_src/net/PARENT_SCOPE
  )

  set(NET_SOURCES
    ""
    PARENT_SCOPE
  )
endif()