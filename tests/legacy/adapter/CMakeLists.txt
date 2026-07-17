# Auto-generated module tests for prefix 'adapter'

file(GLOB ADAPTER_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_adapter_*.cpp"
)

foreach(_src IN LISTS ADAPTER_PREFIX_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_adapter_${_stem}_focused")
    set(_ctest "${_stem}_adapter_FocusedTests")
    set(_extra_sources)

    if(TARGET ${_target})
        continue()
    endif()

    if(_stem STREQUAL "test_adapter_sync")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/llm/lora_framework/adapter_sync_manager.cpp
            ${THEMIS_ROOT_DIR}/src/llm/lora_framework/adapter_consistency_checker.cpp
            ${THEMIS_ROOT_DIR}/src/llm/lora_framework/lora_storage_service.cpp
            ${THEMIS_ROOT_DIR}/src/sharding/shard_topology.cpp
        )
    endif()

    add_executable(${_target}
        "${_src}"
        ${_extra_sources}
    )
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
        MODULE adapter
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS adapter
    )
endforeach()