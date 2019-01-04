# GRPC and Protocol Buffers libraries location
list(APPEND CMAKE_PREFIX_PATH "/usr/include/grpc" "/usr/include/grpcpp" "/usr/include/grpc++" "/usr/include/google/protobuf")

# Cmake find modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(Protobuf REQUIRED)
find_package(GRPC REQUIRED)

set(PROTOS
        ${CMAKE_CURRENT_SOURCE_DIR}/protos/DistributedFileSystem.proto
        )

set(PROTO_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/proto-src)
file(MAKE_DIRECTORY ${PROTO_SRC_DIR})
include_directories(${PROTO_SRC_DIR})

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_SRC_DIR} ${PROTOS})
grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${PROTO_SRC_DIR} ${PROTOS})

find_package(OpenSSL REQUIRED)

# Building server
add_executable(NameServer
        ${CMAKE_CURRENT_SOURCE_DIR}/cpp/NameServer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/cpp/NameServer.h
        ${PROTO_SRCS}
        ${GRPC_SRCS}
        )
target_link_libraries(NameServer
        gRPC::grpc++_reflection
        protobuf::libprotobuf
        OpenSSL::SSL
        )
add_executable(DataServer
        ${CMAKE_CURRENT_SOURCE_DIR}/cpp/DataServer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/cpp/DataServer.h
        ${PROTO_SRCS}
        ${GRPC_SRCS}
        )
target_link_libraries(DataServer
        gRPC::grpc++_reflection
        protobuf::libprotobuf
        OpenSSL::SSL
        )

# Building client
add_executable(TestClient
        ${CMAKE_CURRENT_SOURCE_DIR}/cpp/TestClient.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/cpp/Client.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/cpp/Client.h
        ${PROTO_SRCS}
        ${GRPC_SRCS}
        cpp/TestClient.cpp)
target_link_libraries(TestClient
        gRPC::grpc++_reflection
        protobuf::libprotobuf
        OpenSSL::SSL
        )
