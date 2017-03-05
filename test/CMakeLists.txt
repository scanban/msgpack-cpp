find_package(Threads REQUIRED)
include(ExternalProject)

if (ENABLE_TRAVIS)
    ExternalProject_Add(
            gtest
            GIT_REPOSITORY https://github.com/google/googletest.git
            PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
            # Disable install step
            INSTALL_COMMAND ""
    )
else ()
    ExternalProject_Add(
            gtest
            URL https://github.com/google/googletest/archive/master.zip
            PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
            # Disable install step
            INSTALL_COMMAND ""
    )
endif ()
# Get GTest source and binary directories from CMake project
ExternalProject_Get_Property(gtest source_dir binary_dir)

# Create a libgtest target to be used as a dependency by test programs
add_library(libgtest IMPORTED STATIC GLOBAL)
add_dependencies(libgtest gtest)

# Set libgtest properties
set_target_properties(libgtest PROPERTIES
        "IMPORTED_LOCATION" "${binary_dir}/googlemock/gtest/libgtest.a"
        "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
        )

# Create a libgmock target to be used as a dependency by test programs
add_library(libgmock IMPORTED STATIC GLOBAL)
add_dependencies(libgmock gtest)

# Set libgmock properties
set_target_properties(libgmock PROPERTIES
        "IMPORTED_LOCATION" "${binary_dir}/googlemock/libgmock.a"
        "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
        )

# I couldn't make it work with INTERFACE_INCLUDE_DIRECTORIES
include_directories("${source_dir}/googletest/include"
        "${source_dir}/googlemock/include")

set(GTEST_LIBRARIES libgtest libgmock)

set(TEST_PROGRAMS msgpack_test)
set(INCLUDES ../packer.h ../unpacker.h ../platform.h)

foreach (source_file ${TEST_PROGRAMS})
    get_filename_component(test_name ${source_file} NAME)
    add_executable(
            ${test_name}
            ${source_file}
            ${INCLUDES}
    )
    target_link_libraries(${test_name}
            ${GTEST_LIBRARIES}
            ${CMAKE_THREAD_LIBS_INIT}
            )
    target_include_directories(${test_name} PUBLIC ${CMAKE_SOURCE_DIR})
    add_test(${test_name} ${test_name})
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        set_property(TARGET ${source_file_we} APPEND_STRING PROPERTY COMPILE_FLAGS "-Wall -Wextra -Werror -g -O3 ")
    endif ()
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set_property(TARGET ${source_file_we} APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-mismatched-tags")
    endif ()

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        if (CMAKE_CXX_FLAGS MATCHES "/W[0-4] /WX")
            string(REGEX REPLACE "/W[0-4]" "/W3 /WX" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        else ()
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
        endif ()
    endif ()
endforeach ()
