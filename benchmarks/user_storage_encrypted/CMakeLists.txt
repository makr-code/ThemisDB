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

