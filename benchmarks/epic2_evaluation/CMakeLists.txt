if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: epic2_evaluation")

set(EPIC2_EVALUATION_BENCHMARKS
    benchmark_matrix_bench.cc
    planner_decision_bench.cc
    storage_strategy_bench.cc
)

foreach(_src IN LISTS EPIC2_EVALUATION_BENCHMARKS)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "bench_epic2_evaluation_${_stem}")
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
