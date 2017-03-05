include(ExternalProject)

ExternalProject_Add(
        hayai
        URL https://github.com/nickbruun/hayai/archive/v1.0.1.zip
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/hayai
        # Disable install step
        INSTALL_COMMAND ""
)

# Get hayai source and binary directories from CMake project
ExternalProject_Get_Property(hayai source_dir binary_dir)

# Create a hayai_main target to be used as a dependency by test programs
add_library(hayai_main IMPORTED STATIC GLOBAL)

# Set hayai_main properties
set_target_properties(hayai_main PROPERTIES
        "IMPORTED_LOCATION" "${binary_dir}/src/libhayai_main.a")

set(BENCHMARK_PROGRAMS msgpack_benchmark)
set(INCLUDES ../packer.h ../unpacker.h ../platform.h)
include_directories(${source_dir}/src)

foreach (source_file ${BENCHMARK_PROGRAMS})
    get_filename_component(bench_name ${source_file} NAME)
    add_executable(
            ${bench_name}
            ${source_file}
            ${INCLUDES}
    )
    add_dependencies(${bench_name} hayai)
    target_link_libraries(${bench_name} hayai_main)
    target_include_directories(${bench_name} PUBLIC ${CMAKE_SOURCE_DIR})
endforeach ()
