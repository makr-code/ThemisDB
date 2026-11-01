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

if(TARGET bench_lora_data_paths)
    if(THEMIS_BUILD_MODULAR)
        target_sources(bench_lora_data_paths PRIVATE
            ${THEMIS_ROOT_DIR}/src/aql/aql_lora_finetuner.cpp
        )
    endif()
    if(TARGET themis_training)
        target_link_libraries(bench_lora_data_paths PRIVATE themis_training)
    endif()
    if(TARGET themis_llm)
        target_link_libraries(bench_lora_data_paths PRIVATE themis_llm)
    endif()
endif()
