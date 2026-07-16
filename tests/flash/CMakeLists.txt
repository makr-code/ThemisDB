# flash module tests

file(GLOB FLASH_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS FLASH_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_flash_${_stem}_focused")
    set(_ctest "${_stem}_flash_FocusedTests")
    set(_extra_sources)
    set(_extra_compile_options)

    if(_stem STREQUAL "test_flash_attention_correctness")
        list(APPEND _extra_sources
            "${THEMIS_ROOT_DIR}/src/llm/attention/flash_attention.cpp"
            "${THEMIS_ROOT_DIR}/src/llm/attention/kv_cache_manager.cpp"
        )
        if(MSVC)
            list(APPEND _extra_compile_options /UTHEMIS_ENABLE_VULKAN)
        endif()
    endif()

    if(_stem STREQUAL "test_flash_lora")
        if(MSVC)
            list(APPEND _extra_compile_options /UTHEMIS_ENABLE_VULKAN)
        endif()
    endif()

    if(TARGET ${_target})
        continue()
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
    if(_extra_compile_options)
        target_compile_options(${_target} PRIVATE ${_extra_compile_options})
    endif()

    themis_register_module_focused_test(
        MODULE flash
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS flash
    )
endforeach()