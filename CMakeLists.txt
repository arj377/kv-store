cmake_minimum_required(VERSION 3.16)

project(kv_store)
include_directories(include)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(server
    src/server.cpp
    src/kv_store.cpp
    src/wal.cpp
    src/thread_pool.cpp
    src/page.cpp
    src/disk_manager.cpp
    src/lru_replacer.cpp
    src/buffer_pool_manager.cpp
)
add_executable(client
    src/client.cpp
)