# Auto-generated module tests for llama_cpp plugin

file(GLOB LLAMA_CPP_PREFIX_TEST_SOURCES
    "${THEMIS_ROOT_DIR}/src/llama_cpp/tests/test_llama_cpp_*.cpp"
)

foreach(_src IN LISTS LLAMA_CPP_PREFIX_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_llama_cpp_${_stem}_focused")
    set(_ctest "${_stem}_llamacpp_FocusedTests")
    set(_extra_sources)

    if(TARGET ${_target})
        continue()
    endif()

    # Include llama_cpp_plugin implementation and registrar
    list(APPEND _extra_sources
        ${THEMIS_ROOT_DIR}/src/llama_cpp/llama_cpp_plugin.cpp
        ${THEMIS_ROOT_DIR}/src/llama_cpp/llama_cpp_registrar.cpp
        ${THEMIS_ROOT_DIR}/src/llama_cpp/llama_cpp_plugin_validation_gates.cpp
    )

    # RAG assembler provides context assembly used by LlamaCppPlugin::generateRAG()
    if(EXISTS "${THEMIS_ROOT_DIR}/src/rag/rag_context_assembler.cpp")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/rag_context_assembler.cpp
        )
    endif()

    add_executable(${_target}
        "${_src}"
        ${_extra_sources}
    )
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
        ${THEMIS_ROOT_DIR}/tests
    )
    
    # Link GTest and core dependencies
    set(_gmock_link "")
    if(TARGET GTest::gmock)
        set(_gmock_link GTest::gmock)
    elseif(TARGET gmock::gmock)
        set(_gmock_link gmock::gmock)
    endif()
    
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        ${_gmock_link}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    
    # Enable stub mode for test builds to preserve existing test behavior
    # Group D, N6 tests rely on stub echo responses
    target_compile_definitions(${_target} PRIVATE
        THEMIS_TEST_BUILD=1
        THEMIS_LLAMA_CPP_STUB_MODE
    )

    themis_register_module_focused_test(
        MODULE llama_cpp
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS llama_cpp
    )
endforeach()
