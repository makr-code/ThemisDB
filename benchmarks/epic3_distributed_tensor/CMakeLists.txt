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
