if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()
message(STATUS "Configuring benchmark module: user_storage_encrypted")

# Phase 5: Release gate benchmarks (error cast, switch dispatch, struct alloc)
themis_add_standard_benchmark(bench_user_storage_encrypted_release_gates bench_user_storage_encrypted_release_gates.cpp)

# Phase 5: Lifecycle benchmarks (mount, unmount, key derivation, key rotation, throughput)
themis_add_standard_benchmark(bench_user_storage_encrypted_lifecycle_gates bench_user_storage_encrypted_lifecycle_gates.cpp)

# Register lifecycle gates as release_critical benchmark smoke entry.
themis_register_benchmark_ctest(
    NAME         bench_user_storage_encrypted_lifecycle_gates
    TARGET       bench_user_storage_encrypted_lifecycle_gates
    MODULE       "user_storage_encrypted"
    TIMEOUT      300
    EXTRA_LABELS release_critical
)

# Wave D: Dedicated operability gates (ES-BM-01..04)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_encrypted_storage_dedicated_gates.cpp")
    if(NOT TARGET bench_encrypted_storage_dedicated_gates)
        themis_add_standard_benchmark(bench_encrypted_storage_dedicated_gates
            bench_encrypted_storage_dedicated_gates.cpp)
    endif()
endif()

