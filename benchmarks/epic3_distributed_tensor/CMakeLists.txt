if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: epic3_distributed_tensor")

set(EPIC3_DISTRIBUTED_TENSOR_BENCHMARKS
    placement_strategy_bench.cc
    integrity_verification_bench.cc
    recovery_rebuild_bench.cc
    distributed_retrieval_bench.cc
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_tensor_partial_refit.cc")
    message(STATUS "Adding EPIC3: Phase B Partial Refit Benchmark (BP-01..10)")
    themis_add_standard_benchmark(
        module_epic3_distributed_tensor_bench_partial_refit_focused
        bench_tensor_partial_refit.cc
    )
    if(TARGET module_epic3_distributed_tensor_bench_partial_refit_focused)
        target_include_directories(
            module_epic3_distributed_tensor_bench_partial_refit_focused PRIVATE
            ${THEMIS_ROOT_DIR}
            ${THEMIS_ROOT_DIR}/benchmarks
            ${THEMIS_ROOT_DIR}/src
            ${THEMIS_ROOT_DIR}/src/distributed_tensor/include
        )
        target_sources(
            module_epic3_distributed_tensor_bench_partial_refit_focused PRIVATE
            ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/tensor_delta_log.cc
            ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/snapshot_update_worker.cc
            ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/crash_recovery_checkpoint.cc
            ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/distributed_lock_manager.cc
            ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/stale_artifact_detector.cc
            ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/error_recovery_handler.cc
        )
        target_link_libraries(module_epic3_distributed_tensor_bench_partial_refit_focused
            PRIVATE Threads::Threads)
        target_compile_definitions(module_epic3_distributed_tensor_bench_partial_refit_focused
            PRIVATE THEMIS_TEST_BUILD=1)
        message(STATUS "  Phase B Partial Refit Benchmark: patch path (p99 <= 5ms), partial refit (p99 <= 30ms), rebuild (p99 <= 100ms) (BP-01..10)")
    endif()
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_tensor_summary_first.cc")
    message(STATUS "Adding EPIC3: Phase C Summary-First Routing Benchmark (BSF-01..12)")
    themis_add_standard_benchmark(
        module_epic3_distributed_tensor_bench_summary_first_focused
        bench_tensor_summary_first.cc
    )
    if(TARGET module_epic3_distributed_tensor_bench_summary_first_focused)
        target_include_directories(
            module_epic3_distributed_tensor_bench_summary_first_focused PRIVATE
            ${THEMIS_ROOT_DIR}
            ${THEMIS_ROOT_DIR}/benchmarks
            ${THEMIS_ROOT_DIR}/src
            ${THEMIS_ROOT_DIR}/src/distributed_tensor/include
        )
        if(TARGET themis_distributed_tensor)
            target_link_libraries(module_epic3_distributed_tensor_bench_summary_first_focused 
                PRIVATE themis_distributed_tensor)
        else()
            target_sources(module_epic3_distributed_tensor_bench_summary_first_focused PRIVATE
                ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/shard_summary_coordinator.cc
                ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/manifest_store.cc
            )
        endif()
        target_link_libraries(module_epic3_distributed_tensor_bench_summary_first_focused
            PRIVATE Threads::Threads)
        target_compile_definitions(module_epic3_distributed_tensor_bench_summary_first_focused
            PRIVATE THEMIS_TEST_BUILD=1)
        message(STATUS "  Phase C Summary-First Benchmark: routing, consensus, fetch latency (BSF-01..12)")
    endif()
endif()

foreach(_src IN LISTS EPIC3_DISTRIBUTED_TENSOR_BENCHMARKS)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "bench_epic3_distributed_tensor_${_stem}")
    themis_add_standard_benchmark(${_target} "${_src}")
    if(TARGET ${_target})
        target_include_directories(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/benchmarks
        )
        if(TARGET themis_distributed_tensor)
            target_link_libraries(${_target} PRIVATE themis_distributed_tensor)
        endif()
    endif()
endforeach()
