# acceleration module benchmarks — Wave D dedicated gates

if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: acceleration (Wave D gates)")

file(GLOB ACCELERATION_DEDICATED_BENCH_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/bench_acceleration_dedicated_gates.cpp"
)

foreach(_src IN LISTS ACCELERATION_DEDICATED_BENCH_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)

    if(TARGET ${_stem})
        continue()
    endif()

    themis_add_standard_benchmark(${_stem} "${_src}")
endforeach()
