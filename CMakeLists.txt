cmake_minimum_required(VERSION 3.16)

project(kv_store)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(server
    src/server.cpp
    src/kv_store.cpp
    src/ThreadPool.cpp
)

add_executable(client
    src/client.cpp
)