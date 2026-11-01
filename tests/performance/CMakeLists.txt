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
    get_filename_component(_src_real "${_src}" REALPATH)
    set(_registered_targets "")
    get_property(_performance_dir_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    foreach(_performance_tgt IN LISTS _performance_dir_targets)
        if(_performance_tgt STREQUAL "themis_tests")
            continue()
        endif()
        get_target_property(_performance_tgt_type ${_performance_tgt} TYPE)
        if(NOT _performance_tgt_type STREQUAL "EXECUTABLE")
            continue()
        endif()
        get_target_property(_performance_tgt_sources ${_performance_tgt} SOURCES)
        foreach(_performance_tgt_source IN LISTS _performance_tgt_sources)
            if(_performance_tgt_source MATCHES "\\$<")
                continue()
            endif()
            if(NOT IS_ABSOLUTE "${_performance_tgt_source}")
                get_filename_component(
                    _performance_tgt_source
                    "${_performance_tgt_source}"
                    ABSOLUTE
                    BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
                )
            endif()
            get_filename_component(_performance_tgt_source_real "${_performance_tgt_source}" REALPATH)
            if(_performance_tgt_source_real STREQUAL _src_real)
                list(APPEND _registered_targets "${_performance_tgt}")
                break()
            endif()
        endforeach()
    endforeach()

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

    list(LENGTH _registered_targets _registered_target_count)
    if(_registered_target_count GREATER 1)
        message(FATAL_ERROR
            "Performance test source ${_src} is already compiled by multiple executable targets: "
            "${_registered_targets}"
        )
    elseif(_registered_target_count EQUAL 1)
        list(GET _registered_targets 0 _registered_target)
        themis_register_module_focused_test(
            MODULE performance
            NAME ${_name_stem}_performance_AutoFocusedTests
            TARGET ${_registered_target}
            TIER unit
            TIMEOUT 120
            LABELS performance autogen
        )
        continue()
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