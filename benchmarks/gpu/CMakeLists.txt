# gpu module benchmarks — Wave D dedicated gates

if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: gpu (Wave D dedicated gates)")

file(GLOB GPU_DEDICATED_BENCH_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/bench_gpu_dedicated_gates.cpp"
)

foreach(_src IN LISTS GPU_DEDICATED_BENCH_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)

    if(TARGET ${_stem})
        continue()
    endif()

    themis_add_standard_benchmark(${_stem} "${_src}")
endforeach()
