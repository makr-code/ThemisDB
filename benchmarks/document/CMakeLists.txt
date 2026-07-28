# Document Module Benchmark Suite — CMakeLists.txt
# Registers bench_document_diff_merge, bench_document_round_trip, and
# bench_document_store targets.  Follows the wave7 CMakeLists.txt pattern.

cmake_minimum_required(VERSION 3.20)

# Guard: the parent CMakeLists already checks THEMIS_BUILD_BENCHMARKS before
# including this subdirectory, but an explicit guard is retained here for
# clarity and standalone re-use.
if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

find_package(benchmark QUIET CONFIG)
if(NOT benchmark_FOUND)
    message(WARNING "Document benchmarks skipped: Google Benchmark not found.")
    return()
endif()

# ---------------------------------------------------------------------------
# Helper: add a single document benchmark executable
# ---------------------------------------------------------------------------
function(add_document_benchmark TARGET_NAME SOURCE_FILE)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE}")
        return()
    endif()
    if(TARGET ${TARGET_NAME})
        return()
    endif()

    add_executable(${TARGET_NAME} ${SOURCE_FILE})

    target_include_directories(${TARGET_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/benchmarks
    )

    target_link_libraries(${TARGET_NAME} PRIVATE
        benchmark::benchmark_main
        themis_core
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(${TARGET_NAME} PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(${TARGET_NAME})

    install(TARGETS ${TARGET_NAME}
        RUNTIME DESTINATION bin/benchmarks/document
        COMPONENT benchmarks
    )

    message(STATUS "  [Document] Registered: ${TARGET_NAME}")
endfunction()

# ---------------------------------------------------------------------------
# DDM: Document Diff/Merge — DDM-BM-01..08 + GATE-DOC-01..02
# ---------------------------------------------------------------------------
add_document_benchmark(
    bench_document_diff_merge
    bench_document_diff_merge.cpp
)

# ---------------------------------------------------------------------------
# RTP: Round-Trip Editor — RTP-BM-01..08 + GATE-DOC-03..04
# ---------------------------------------------------------------------------
add_document_benchmark(
    bench_document_round_trip
    bench_document_round_trip.cpp
)

# ---------------------------------------------------------------------------
# DST: Document Store — DST-BM-01..08 + GATE-DOC-05..06
# ---------------------------------------------------------------------------
add_document_benchmark(
    bench_document_store
    bench_document_store.cpp
)
