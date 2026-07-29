# benchmarks/geo/CMakeLists.txt
# Auto-glob all bench_*.cpp files and register them as standard benchmarks.
# This CMakeLists.txt is intentionally simple and follows the pattern used by
# other benchmark modules in this repository.

if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

if(NOT TARGET benchmark::benchmark)
    message(STATUS "[benchmarks/geo] Google Benchmark not found — skipping geo bench targets")
    return()
endif()

message(STATUS "[benchmarks/geo] Configuring geo benchmark targets")

file(GLOB GEO_BENCH_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/bench_*.cpp")

foreach(_src IN LISTS GEO_BENCH_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "bench_geo_${_stem}")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")

    target_include_directories(${_target} PRIVATE
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/src"
    )

    target_link_libraries(${_target} PRIVATE
        themis_core
        benchmark::benchmark
        benchmark::benchmark_main
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(${_target} PRIVATE THEMIS_BENCHMARK_BUILD=1)
    target_compile_features(${_target} PRIVATE cxx_std_17)

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        themis_apply_benchmark_release_flags(${_target})
    endif()

    install(TARGETS ${_target}
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    add_test(
        NAME "${_target}"
        COMMAND "${_target}"
            --benchmark_out=${CMAKE_BINARY_DIR}/bench_results/${_target}.json
            --benchmark_out_format=json
            --benchmark_min_time=0.1s
    )
    set_tests_properties("${_target}" PROPERTIES
        LABELS   "geo;benchmark"
        TIMEOUT  120
    )

    message(STATUS "[benchmarks/geo] Registered ${_target}")
endforeach()
