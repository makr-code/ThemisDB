if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: stable_diffusion")

themis_add_standard_benchmark(bench_stable_diffusion_release_gates bench_stable_diffusion_release_gates.cpp)
