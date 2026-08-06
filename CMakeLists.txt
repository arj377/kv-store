cmake_minimum_required(VERSION 3.16)

project(kv_store)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(gRPC REQUIRED)

include_directories(
    include
    proto
)

add_executable(server
    src/server.cpp
    src/kv_service.cpp
    src/kv_store.cpp
    src/wal.cpp
    src/page.cpp
    src/disk_manager.cpp
    src/lru_replacer.cpp
    src/buffer_pool_manager.cpp
    proto/kv.pb.cc
    proto/kv.grpc.pb.cc
)

target_link_libraries(server
    PRIVATE
    protobuf::libprotobuf
    gRPC::grpc++
    gRPC::grpc
)


add_executable(client
    src/client.cpp

    proto/kv.pb.cc
    proto/kv.grpc.pb.cc
)

target_link_libraries(client
    PRIVATE
    protobuf::libprotobuf
    gRPC::grpc++
    gRPC::grpc
)