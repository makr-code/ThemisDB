# llm_streaming module tests — Wave D stress coverage

file(GLOB LLM_STREAMING_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS LLM_STREAMING_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_llm_streaming_${_stem}_focused")
    set(_ctest "${_stem}_LLMStreamingFocusedTests")

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
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    set(_labels llm_streaming)
    if(_stem STREQUAL "test_llm_streaming_highcardinality_stress")
        list(APPEND _labels wave_d stress not_release_critical)
    endif()

    themis_register_module_focused_test(
        MODULE llm_streaming
        NAME   ${_ctest}
        TARGET ${_target}
        TIER   stress
        TIMEOUT 300
        LABELS ${_labels}
    )
endforeach()
