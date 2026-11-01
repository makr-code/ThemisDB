# Performance focused module tests (flat hierarchy under tests/performance)

file(GLOB PERFORMANCE_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

file(GLOB_RECURSE PERFORMANCE_NESTED_MODULE_TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/phase3/test_*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/phase4/test_*.cpp"
)

list(APPEND PERFORMANCE_MODULE_TEST_SOURCES
    ${PERFORMANCE_NESTED_MODULE_TEST_SOURCES}
)

foreach(_src IN LISTS PERFORMANCE_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    file(RELATIVE_PATH _relative_source "${CMAKE_CURRENT_SOURCE_DIR}" "${_src}")
    if(_relative_source MATCHES "^phase[34]/")
        string(SHA256 _source_hash "${_relative_source}")
        string(SUBSTRING "${_source_hash}" 0 16 _source_hash)
        set(_name_stem "nested_${_stem}_${_source_hash}")
    else()
        set(_name_stem "${_stem}")
    endif()

    set(_target "module_performance_${_name_stem}_focused")
    set(_ctest "${_name_stem}_PerformanceFocusedTests")
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

file(GLOB_RECURSE PERFORMANCE_NESTED_AUTOGEN_PREFIX_TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/phase3/test_performance_*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/phase4/test_performance_*.cpp"
)

list(APPEND PERFORMANCE_AUTOGEN_PREFIX_TEST_SOURCES
    ${PERFORMANCE_NESTED_AUTOGEN_PREFIX_TEST_SOURCES}
)

foreach(_src IN LISTS PERFORMANCE_AUTOGEN_PREFIX_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    file(RELATIVE_PATH _relative_source "${CMAKE_CURRENT_SOURCE_DIR}" "${_src}")
    if(_relative_source MATCHES "^phase[34]/")
        string(SHA256 _source_hash "${_relative_source}")
        string(SUBSTRING "${_source_hash}" 0 16 _source_hash)
        set(_name_stem "nested_${_stem}_${_source_hash}")
    else()
        set(_name_stem "${_stem}")
    endif()

    set(_target "module_performance_${_name_stem}_autofocused")
    set(_ctest "${_name_stem}_performance_AutoFocusedTests")
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