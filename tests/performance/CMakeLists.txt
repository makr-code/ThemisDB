# Performance focused module tests (flat hierarchy under tests/performance)

file(GLOB PERFORMANCE_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS PERFORMANCE_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_performance_${_stem}_focused")
    set(_ctest "${_stem}_PerformanceFocusedTests")
    set(_extra_sources)

    if(_stem MATCHES "^test_rabitq")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/performance/rabitq.cpp
        )
    endif()

    if(_stem STREQUAL "test_cicada")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/performance/cicada.cpp
        )
    endif()

    if(_stem STREQUAL "test_performance_hardware_accelerator")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/performance/hardware_accelerator.cpp
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
        MODULE performance
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
    )
endforeach()

# AUTOGEN PREFIX BLOCK: performance
file(GLOB PERFORMANCE_AUTOGEN_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_performance_*.cpp"
)

foreach(_src IN LISTS PERFORMANCE_AUTOGEN_PREFIX_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_performance_${_stem}_autofocused")
    set(_ctest "${_stem}_performance_AutoFocusedTests")
    set(_extra_sources)

    if(_stem STREQUAL "test_performance_hardware_accelerator")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/performance/hardware_accelerator.cpp
        )
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

    themis_register_module_focused_test(
        MODULE performance
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS performance autogen
    )
endforeach()