# training module benchmarks — Wave D dedicated gates

if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: training")

file(GLOB TRAINING_BENCH_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/bench_*.cpp"
)

foreach(_src IN LISTS TRAINING_BENCH_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)

    if(TARGET ${_stem})
        continue()
    endif()

    themis_add_standard_benchmark(${_stem} "${_src}")
endforeach()
