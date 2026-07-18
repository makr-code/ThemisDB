# plugin module tests

file(GLOB PLUGIN_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

if(NOT TARGET themis_plugin_runtime_test_plugin)
    add_library(themis_plugin_runtime_test_plugin SHARED
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime_test_plugin.cpp"
    )
    target_include_directories(themis_plugin_runtime_test_plugin PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(themis_plugin_runtime_test_plugin PRIVATE
        nlohmann_json::nlohmann_json
    )
    target_compile_definitions(themis_plugin_runtime_test_plugin PRIVATE
        THEMIS_PLUGIN_EXPORTS=1
    )
    set_target_properties(themis_plugin_runtime_test_plugin PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    )
endif()

foreach(_src IN LISTS PLUGIN_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_plugin_${_stem}_focused")
    set(_ctest "${_stem}_plugin_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")

    if(_stem STREQUAL "test_plugin_registrar_fail_closed")
        target_sources(${_target} PRIVATE
            "${THEMIS_ROOT_DIR}/src/llama_cpp/llama_cpp_plugin.cpp"
            "${THEMIS_ROOT_DIR}/src/llama_cpp/llama_cpp_registrar.cpp"
            "${THEMIS_ROOT_DIR}/src/rag/rag_context_assembler.cpp"
            "${THEMIS_ROOT_DIR}/src/stable_diffusion/sd_config.cpp"
            "${THEMIS_ROOT_DIR}/src/stable_diffusion/sd_prompt_sanitizer.cpp"
            "${THEMIS_ROOT_DIR}/src/stable_diffusion/sd_plugin.cpp"
            "${THEMIS_ROOT_DIR}/src/stable_diffusion/sd_plugin_registrar.cpp"
            "${THEMIS_ROOT_DIR}/src/whisper/audio_chunk_reader.cpp"
            "${THEMIS_ROOT_DIR}/src/whisper/whisper_config.cpp"
            "${THEMIS_ROOT_DIR}/src/whisper/whisper_transcriber.cpp"
            "${THEMIS_ROOT_DIR}/src/whisper/whisper_plugin.cpp"
            "${THEMIS_ROOT_DIR}/src/whisper/whisper_plugin_registrar.cpp"
        )
    endif()

    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        themis_llm
        themis_content
        themis_query
        themis_graph
        themis_sharding
        themis_ingestion
        themis_storage
        themis_base
        spdlog::spdlog
        fmt::fmt
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE plugin
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS plugin
    )
endforeach()

if(TARGET module_plugin_test_plugin_runtime_loading_focused)
    add_dependencies(module_plugin_test_plugin_runtime_loading_focused
        themis_plugin_runtime_test_plugin
    )
    target_compile_definitions(module_plugin_test_plugin_runtime_loading_focused PRIVATE
        THEMIS_RUNTIME_TEST_PLUGIN_PATH="$<TARGET_FILE:themis_plugin_runtime_test_plugin>"
    )
endif()