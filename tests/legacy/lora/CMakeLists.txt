# lora module tests

file(GLOB LORA_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS LORA_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_lora_${_stem}_focused")
    set(_ctest "${_stem}_lora_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    if(_stem STREQUAL "test_lora_adapter_merger")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/training/lora_adapter.cpp
            ${THEMIS_ROOT_DIR}/src/training/lora_adapter_merger.cpp
        )
    endif()
    if(_stem STREQUAL "test_lora_router")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/llm/lora_router.cpp
            ${THEMIS_ROOT_DIR}/src/llm/adapter_load_balancer.cpp
            ${THEMIS_ROOT_DIR}/src/llm/lora_framework/embedding_provider.cpp
        )
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
        MODULE lora
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS lora
    )
endforeach()