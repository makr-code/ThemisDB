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
    themis_add_standard_benchmark(
        bench_epic3_distributed_tensor_bench_tensor_partial_refit
        bench_tensor_partial_refit.cc
    )
    if(TARGET bench_epic3_distributed_tensor_bench_tensor_partial_refit)
        target_include_directories(
            bench_epic3_distributed_tensor_bench_tensor_partial_refit PRIVATE
            ${THEMIS_ROOT_DIR}
            ${THEMIS_ROOT_DIR}/benchmarks
            ${THEMIS_ROOT_DIR}/src
            ${THEMIS_ROOT_DIR}/src/distributed_tensor/include
        )
        target_sources(
            bench_epic3_distributed_tensor_bench_tensor_partial_refit PRIVATE
            ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/tensor_delta_log.cc
            ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/snapshot_update_worker.cc
            ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/crash_recovery_checkpoint.cc
            ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/distributed_lock_manager.cc
            ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/stale_artifact_detector.cc
            ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/error_recovery_handler.cc
        )
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
