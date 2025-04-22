if (BUILD_BTVSERVER)
    add_library(btv_server_lib STATIC
            ${CMAKE_SOURCE_DIR}/src/bins/btv-server/src/utils.h
            ${CMAKE_SOURCE_DIR}/src/bins/btv-server/src/httpclient.h
            ${CMAKE_SOURCE_DIR}/src/bins/btv-server/src/manager.h
            ${CMAKE_SOURCE_DIR}/src/bins/btv-server/src/socketlistener.h
            ${CMAKE_SOURCE_DIR}/src/bins/btv-server/src/websocketsession.h
            ${CMAKE_SOURCE_DIR}/src/bins/btv-server/src/websocketserver.h
            ${CMAKE_SOURCE_DIR}/src/bins/btv-server/src/utils.cpp
            ${CMAKE_SOURCE_DIR}/src/bins/btv-server/src/httpclient.cpp
            ${CMAKE_SOURCE_DIR}/src/bins/btv-server/src/manager.cpp
            ${CMAKE_SOURCE_DIR}/src/bins/btv-server/src/socketlistener.cpp
            ${CMAKE_SOURCE_DIR}/src/bins/btv-server/src/websocketsession.cpp
            ${CMAKE_SOURCE_DIR}/src/bins/btv-server/src/websocketserver.cpp
    )

    target_include_directories(btv_server_lib PRIVATE
            ${CMAKE_SOURCE_DIR}/include ${OPENSSL_INCLUDE_DIR} bdk_lib
            ${ETHASH_INCLUDE_DIR} ${KECCAK_INCLUDE_DIR}
            ${SPEEDB_INCLUDE_DIR} ${SECP256K1_INCLUDE_DIR}
    )

    target_link_libraries(btv_server_lib PRIVATE bdk_lib
            ${CRYPTOPP_LIBRARIES} ${SCRYPT_LIBRARY} ${SECP256K1_LIBRARY}
            ${ETHASH_LIBRARY} ${KECCAK_LIBRARY} ${SPEEDB_LIBRARY}
            ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} bdk_lib
    )

    # Compile and link the btv-server executable
    add_executable(btv-server "main.cpp")

    add_dependencies(btv-server bdk_lib btv_server_lib)

    target_include_directories(btv-server PRIVATE
            bdk_lib btv_server_lib ${OPENSSL_INCLUDE_DIR}
            ${ETHASH_INCLUDE_DIR} ${KECCAK_INCLUDE_DIR}
            ${SPEEDB_INCLUDE_DIR} ${SECP256K1_INCLUDE_DIR}
    )

    target_link_libraries(btv-server
            bdk_lib btv_server_lib
            ${SPEEDB_LIBRARY} ${SNAPPY_LIBRARY} ${Boost_LIBRARIES}
            ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES} ${SECP256K1_LIBRARY}
            ${ETHASH_LIBRARY} ${KECCAK_LIBRARY}
    )

endif ()