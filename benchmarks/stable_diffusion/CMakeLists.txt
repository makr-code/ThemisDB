if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: stable_diffusion")

themis_add_standard_benchmark(bench_stable_diffusion_release_gates bench_stable_diffusion_release_gates.cpp)

if(TARGET stable_diffusion_plugin)
    target_link_libraries(bench_stable_diffusion_release_gates PRIVATE stable_diffusion_plugin)
else()
    target_sources(bench_stable_diffusion_release_gates PRIVATE
        ${THEMIS_ROOT_DIR}/src/stable_diffusion/sd_config.cpp
        ${THEMIS_ROOT_DIR}/src/stable_diffusion/sd_plugin.cpp
        ${THEMIS_ROOT_DIR}/src/stable_diffusion/sd_prompt_sanitizer.cpp
    )
endif()
