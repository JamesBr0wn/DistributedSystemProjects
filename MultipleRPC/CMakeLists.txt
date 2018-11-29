# GRPC and Protocol Buffers libraries location
list(APPEND CMAKE_PREFIX_PATH "/usr/include/grpc" "/usr/include/grpc++" "/usr/include/google/protobuf")

# Cmake find modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(Protobuf REQUIRED)
find_package(GRPC REQUIRED)

set(PROTOS
        ${CMAKE_CURRENT_SOURCE_DIR}/protos/multiple_rpc.proto
        )

set(PROTO_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/proto-src)
file(MAKE_DIRECTORY ${PROTO_SRC_DIR})
include_directories(${PROTO_SRC_DIR})

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_SRC_DIR} ${PROTOS})
grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${PROTO_SRC_DIR} ${PROTOS})

# Building server
add_executable(multiple_rpc_server
        ${CMAKE_CURRENT_SOURCE_DIR}/multiple_rpc_server.cc
        ${PROTO_SRCS}
        ${GRPC_SRCS}
        )
target_link_libraries(multiple_rpc_server
        gRPC::grpc++_reflection
        protobuf::libprotobuf
        )

# Building client
add_executable(multiple_rpc_client
        ${CMAKE_CURRENT_SOURCE_DIR}/multiple_rpc_client.cc
        ${PROTO_SRCS}
        ${GRPC_SRCS}
        )
target_link_libraries(multiple_rpc_client
        gRPC::grpc++_reflection
        protobuf::libprotobuf
        )
