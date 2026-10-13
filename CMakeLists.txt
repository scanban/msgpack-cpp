cmake_minimum_required(VERSION 3.14...3.29)
project(msgpack VERSION 0.0.1 LANGUAGES C CXX)

option(ENABLE_TESTING "ENABLE_TESTING" OFF)
option(ENABLE_BENCHMARKING "ENABLE_BENCHMARKING" OFF)
option(ENABLE_TRAVIS "ENABLE_TRAVIS" OFF)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif ()

set(INCLUDE_FILES unpacker.h packer.h msgpack-platform.h)
configure_file("msgpack-cpp.pc.in" "msgpack-cpp.pc" @ONLY)

if (ENABLE_TESTING)
    enable_testing()
    add_subdirectory(test)
endif ()

if (ENABLE_BENCHMARKING)
    add_subdirectory(benchmark)
endif ()

install(FILES ${INCLUDE_FILES} DESTINATION include)
install(FILES "${CMAKE_BINARY_DIR}/msgpack-cpp.pc" DESTINATION lib/pkgconfig)