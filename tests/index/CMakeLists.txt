# Index focused module tests (flat hierarchy under tests/index)

file(GLOB INDEX_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS INDEX_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_index_${_stem}_focused")
    set(_ctest "${_stem}_IndexFocusedTests")

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
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE index
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
    )
endforeach()

# AUTOGEN PREFIX BLOCK: index
file(GLOB INDEX_AUTOGEN_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_index_*.cpp"
)

foreach(_src IN LISTS INDEX_AUTOGEN_PREFIX_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_index_${_stem}_autofocused")
    set(_ctest "${_stem}_index_AutoFocusedTests")

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
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)
    
    # Phase 5: GPU tests require CUDA linkage if available
    if(_stem MATCHES "gpu_memory" AND THEMIS_CUDA_ENABLED)
        target_link_libraries(${_target} PRIVATE CUDA::cudart)
        target_compile_definitions(${_target} PRIVATE THEMIS_CUDA_ENABLED=1)
    endif()

    themis_register_module_focused_test(
        MODULE index
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS index autogen
    )
endforeach()

# Phase 5: Explicit GPU memory safety test with CUDA support
if(THEMIS_CUDA_ENABLED AND TARGET module_index_test_index_gpu_memory_safety_autofocused)
    target_compile_definitions(module_index_test_index_gpu_memory_safety_autofocused
        PRIVATE THEMIS_CUDA_ENABLED=1
    )
endif()