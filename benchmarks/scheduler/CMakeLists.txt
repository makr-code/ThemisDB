if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()
message(STATUS "Configuring benchmark module: scheduler")

file(GLOB SCHEDULER_BENCH_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/bench_*.cpp"
)

foreach(_src IN LISTS SCHEDULER_BENCH_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)

    if(TARGET ${_stem})
        continue()
    endif()

    themis_add_standard_benchmark(${_stem} "${_src}")
endforeach()

# Smoke only the benchmark that exercises production TaskScheduler APIs.
# Timing remains informational until an owner, stable hardware, baseline, and
# acceptance threshold are agreed.
if(THEMIS_BUILD_TESTS AND TARGET bench_task_scheduler)
    themis_register_benchmark_ctest(
        NAME scheduler_task_scheduler_benchmark_smoke
        TARGET bench_task_scheduler
        MODULE scheduler
        TIMEOUT 60
        MIN_TIME "0.01s"
        EXTRA_ARGS "--benchmark_filter=TaskSchedulerBenchFixture/ExecuteTaskNow"
    )
endif()
