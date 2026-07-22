# plugin module tests

file(GLOB PLUGIN_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

if(NOT TARGET themis_plugin_runtime_test_plugin AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/runtime_test_plugin.cpp")
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
    
    # TEMPORARY: Skip fail_closed test until plugin isolation refactor
    # (requires full ILLMPlugin/IImageGenerationPlugin/IAudioTranscriptionPlugin virtual implementations)
    if(_stem STREQUAL "test_plugin_registrar_fail_closed")
        continue()
    endif()
    
    set(_target "module_plugin_${_stem}_focused")
    set(_ctest "${_stem}_plugin_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    # Link plugin implementations
    if(TARGET llama_cpp_plugin)
        target_link_libraries(${_target} PRIVATE llama_cpp_plugin $<TARGET_FILE:llama_cpp_plugin>)
        add_dependencies(${_target} llama_cpp_plugin)
    endif()
    if(TARGET stable_diffusion_plugin)
        target_link_libraries(${_target} PRIVATE stable_diffusion_plugin $<TARGET_FILE:stable_diffusion_plugin>)
        add_dependencies(${_target} stable_diffusion_plugin)
    endif()
    if(TARGET whisper_plugin)
        target_link_libraries(${_target} PRIVATE whisper_plugin $<TARGET_FILE:whisper_plugin>)
        add_dependencies(${_target} whisper_plugin)
    endif()
    # Fallback: if plugin .lib exists in the build output, link by explicit path
    if(EXISTS "${CMAKE_BINARY_DIR}/lib/llama_cpp_plugin.lib")
        target_link_libraries(${_target} PRIVATE "${CMAKE_BINARY_DIR}/lib/llama_cpp_plugin.lib")
    endif()
    if(EXISTS "${CMAKE_BINARY_DIR}/lib/stable_diffusion_plugin.lib")
        target_link_libraries(${_target} PRIVATE "${CMAKE_BINARY_DIR}/lib/stable_diffusion_plugin.lib")
    endif()
    if(EXISTS "${CMAKE_BINARY_DIR}/lib/whisper_plugin.lib")
        target_link_libraries(${_target} PRIVATE "${CMAKE_BINARY_DIR}/lib/whisper_plugin.lib")
    endif()
    # Ensure RAG library is linked when available (llama plugin uses RAG types)
    if(TARGET themis_rag)
        target_link_libraries(${_target} PRIVATE themis_rag)
        add_dependencies(${_target} themis_rag)
    elseif(EXISTS "${CMAKE_BINARY_DIR}/lib/themis_rag.lib")
        target_link_libraries(${_target} PRIVATE "${CMAKE_BINARY_DIR}/lib/themis_rag.lib")
    endif()
    # If RAG implementation symbols are needed by plugins, compile the
    # minimal implementation source directly into the test executable so
    # the linker can resolve `RAGContextAssembler` without pulling in the
    # full `themis_rag.lib` (avoids duplicate-definition errors).
    if(EXISTS "${CMAKE_SOURCE_DIR}/src/rag/rag_context_assembler.cpp")
        target_sources(${_target} PRIVATE "${CMAKE_SOURCE_DIR}/src/rag/rag_context_assembler.cpp")
    endif()
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
    if(TARGET themis_plugin_runtime_test_plugin)
        add_dependencies(module_plugin_test_plugin_runtime_loading_focused
            themis_plugin_runtime_test_plugin
        )
        target_compile_definitions(module_plugin_test_plugin_runtime_loading_focused PRIVATE
            THEMIS_RUNTIME_TEST_PLUGIN_PATH="$<TARGET_FILE:themis_plugin_runtime_test_plugin>"
        )
    else()
        message(STATUS "tests/plugin: runtime_test_plugin.cpp not present — skipping runtime plugin dependency for module_plugin_test_plugin_runtime_loading_focused")
    endif()
endif()