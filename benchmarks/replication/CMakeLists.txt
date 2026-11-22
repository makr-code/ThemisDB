# replication module benchmarks

if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: replication")

file(GLOB REPLICATION_BENCH_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/bench_*.cpp"
)

foreach(_src IN LISTS REPLICATION_BENCH_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)

    if(TARGET ${_stem})
        continue()
    endif()

    if(_stem STREQUAL "bench_replication_dedicated_gates")
        # Standalone local-stub diagnostic; intentionally not a Google Benchmark target.
        add_executable(${_stem} "${_src}")
        target_link_libraries(${_stem} PRIVATE Threads::Threads)
    else()
        themis_add_standard_benchmark(${_stem} "${_src}")
    endif()
endforeach()
