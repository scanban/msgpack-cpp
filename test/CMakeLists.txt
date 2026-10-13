include(FetchContent)

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        release-1.12.1
        GIT_SHALLOW    TRUE
)
# use the same (shared) C runtime as the tests on MSVC and keep gtest out of `make install`
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(GTEST_LIBRARIES GTest::gtest GTest::gmock)

set(TEST_PROGRAMS msgpack_test.cpp review_unpacker_test.cpp review_packer_test.cpp)
set(INCLUDES ../packer.h ../unpacker.h ../msgpack-platform.h)

foreach (source_file ${TEST_PROGRAMS})
    get_filename_component(test_name ${source_file} NAME_WE)
    add_executable(
            ${test_name}
            ${source_file}
            ${INCLUDES}
    )
    target_link_libraries(${test_name} ${GTEST_LIBRARIES})
    target_include_directories(${test_name} PUBLIC ${CMAKE_SOURCE_DIR})
    add_test(${test_name} ${test_name})
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${test_name} PRIVATE -Wall -Wextra -Werror)
    endif ()
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${test_name} PRIVATE -Wno-mismatched-tags)
    endif ()
    if (MSVC)
        target_compile_options(${test_name} PRIVATE /W3 /WX)
    endif ()
endforeach ()
