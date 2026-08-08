if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()
message(STATUS "Configuring benchmark module: user_storage_encrypted")

# Phase 5: Release gate benchmarks (error cast, switch dispatch, struct alloc)
themis_add_standard_benchmark(bench_user_storage_encrypted_release_gates bench_user_storage_encrypted_release_gates.cpp)

# Phase 5: Lifecycle benchmarks (mount, unmount, key derivation, key rotation, throughput)
themis_add_standard_benchmark(bench_user_storage_encrypted_lifecycle_gates bench_user_storage_encrypted_lifecycle_gates.cpp)

# Set release_critical label for lifecycle gates
add_test(NAME bench_user_storage_encrypted_lifecycle_gates
         COMMAND bench_user_storage_encrypted_lifecycle_gates)
set_tests_properties(bench_user_storage_encrypted_lifecycle_gates
  PROPERTIES
    LABELS "user_storage_encrypted;release_critical;performance"
    TIMEOUT 300
)

