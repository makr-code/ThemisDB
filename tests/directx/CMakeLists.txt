# Auto-generated module tests for prefix 'directx'

file(GLOB DIRECTX_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_directx_*.cpp"
)

foreach(_src IN LISTS DIRECTX_PREFIX_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_directx_${_stem}_focused")
    set(_ctest "${_stem}_directx_FocusedTests")
    set(_extra_sources)

    if(TARGET ${_target})
        continue()
    endif()

    if(_stem STREQUAL "test_directx_backend")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/acceleration/directx_backend_full.cpp
            ${THEMIS_ROOT_DIR}/src/llm/lora_framework/directx_context.cpp
            ${THEMIS_ROOT_DIR}/src/llm/lora_framework/directx_buffer.cpp
            ${THEMIS_ROOT_DIR}/src/llm/lora_framework/directx_descriptors.cpp
            ${THEMIS_ROOT_DIR}/src/llm/lora_framework/directx_pipeline.cpp
            ${THEMIS_ROOT_DIR}/src/llm/lora_framework/directx_shader.cpp
            ${THEMIS_ROOT_DIR}/src/llm/lora_framework/kernels/directx_kernels.cpp
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
        MODULE directx
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS directx
    )
endforeach()