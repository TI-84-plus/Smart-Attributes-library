cmake_minimum_required(VERSION 3.10)
project(nvme_scan_example)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)

add_executable(SMART main.cpp)

target_link_libraries(SMART ${LIBNVME_LIBRARIES})

