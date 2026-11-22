# multi module tests

file(GLOB MULTI_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS MULTI_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_multi_${_stem}_focused")
    set(_ctest "${_stem}_multi_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    if(_stem STREQUAL "test_multi_gpu_management")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/llm/adapter_load_balancer.cpp
        )
    endif()
    if(_stem STREQUAL "test_multi_step_rag")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/rag/multi_step_rag.cpp
        )
    endif()
    if(_stem STREQUAL "test_multi_gpu_vector_index")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/index/gpu_memory_oversubscription.cpp
            ${THEMIS_ROOT_DIR}/src/index/gpu_vector_index.cpp
            ${THEMIS_ROOT_DIR}/src/index/multi_gpu_vector_index.cpp
        )
        if(THEMIS_ENABLE_GPU AND THEMIS_ENABLE_VECTOR_SEARCH)
            gtest_discover_tests(${_target}
                DISCOVERY_MODE POST_BUILD
                WORKING_DIRECTORY $<TARGET_FILE_DIR:${_target}>
                EXTRA_ARGS --gtest_color=yes
                CONFIGURATIONS Release Debug
            )
        endif()
    endif()
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
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE multi
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS multi
    )
endforeach()