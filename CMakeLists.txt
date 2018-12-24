cmake_minimum_required(VERSION 3.10)
project(DistributedSystemProjects)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)

add_subdirectory(MultipleRPC MultipleRPC)
add_subdirectory(DistributedFileSystem DistributedFileSystem)