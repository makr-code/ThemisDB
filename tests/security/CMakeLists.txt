# Security focused module tests (flat hierarchy under tests/security)

file(GLOB SECURITY_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS SECURITY_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_security_${_stem}_focused")
    set(_ctest "${_stem}_SecurityFocusedTests")

    add_executable(${_target} "${_src}")
    if(_stem STREQUAL "test_hsm_security_checker")
        set(_security_test_gmock_link "")
        if(TARGET GTest::gmock)
            set(_security_test_gmock_link GTest::gmock)
        elseif(TARGET gmock::gmock)
            set(_security_test_gmock_link gmock::gmock)
        endif()
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/tests/test_gtest_main.cpp
        )
        set(_security_test_gtest_libs GTest::gtest ${_security_test_gmock_link})
    else()
        set(_security_test_gtest_libs ${TEST_LIBS})
    endif()
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(${_target} PRIVATE
        ${_security_test_gtest_libs}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    if(_stem STREQUAL "test_aql_safety_validator")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/query/aql_safety_validator.cpp
        )
    endif()
    if(_stem STREQUAL "test_audit_log_store")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/security/audit_log_store.cpp
        )
    endif()
    if(_stem STREQUAL "test_key_rotation_manager")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/security/key_rotation_manager.cpp
        )
    endif()
    if(_stem STREQUAL "test_kdf_argon2_bridge")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/user_storage_encrypted/key_derivation_service.cpp
        )
    endif()
    if(_stem STREQUAL "test_oauth_token_manager")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/ingestion/oauth_token_manager.cpp
        )
    endif()
    if(_stem STREQUAL "test_sprint7_batchc_safe_iterator_modules")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/network/wire_protocol.cpp
            ${THEMIS_ROOT_DIR}/src/query/query_executor.cpp
            ${THEMIS_ROOT_DIR}/src/analytics/aggregation.cpp
            ${THEMIS_ROOT_DIR}/src/analytics/time_series.cpp
            ${THEMIS_ROOT_DIR}/src/cache/eviction.cpp
            ${THEMIS_ROOT_DIR}/src/graph/adjacency_list.cpp
        )
    endif()
    if(_stem STREQUAL "test_stub_279_302_remediation")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/transaction/distributed_transaction_manager.cpp
            ${THEMIS_ROOT_DIR}/src/server/voice_api_handler.cpp
        )
    endif()

    themis_register_module_focused_test(
        MODULE security
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
    )
endforeach()

# AUTOGEN PREFIX BLOCK: security
file(GLOB SECURITY_AUTOGEN_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_security_*.cpp"
)

foreach(_src IN LISTS SECURITY_AUTOGEN_PREFIX_TEST_SOURCES)
    get_filename_component(_src_real "${_src}" REALPATH)
    set(_registered_targets "")
    get_property(_security_dir_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    foreach(_security_tgt IN LISTS _security_dir_targets)
        if(_security_tgt STREQUAL "themis_tests")
            continue()
        endif()
        get_target_property(_security_tgt_type ${_security_tgt} TYPE)
        if(NOT _security_tgt_type STREQUAL "EXECUTABLE")
            continue()
        endif()
        get_target_property(_security_tgt_sources ${_security_tgt} SOURCES)
        foreach(_security_tgt_source IN LISTS _security_tgt_sources)
            if(_security_tgt_source MATCHES "\\$<")
                continue()
            endif()
            if(NOT IS_ABSOLUTE "${_security_tgt_source}")
                get_filename_component(
                    _security_tgt_source
                    "${_security_tgt_source}"
                    ABSOLUTE
                    BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
                )
            endif()
            get_filename_component(_security_tgt_source_real "${_security_tgt_source}" REALPATH)
            if(_security_tgt_source_real STREQUAL _src_real)
                list(APPEND _registered_targets "${_security_tgt}")
                break()
            endif()
        endforeach()
    endforeach()

    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_security_${_stem}_autofocused")
    set(_ctest "${_stem}_security_AutoFocusedTests")

    list(LENGTH _registered_targets _registered_target_count)
    if(_registered_target_count GREATER 1)
        message(FATAL_ERROR
            "Security test source ${_src} is already compiled by multiple executable targets: "
            "${_registered_targets}"
        )
    elseif(_registered_target_count EQUAL 1)
        list(GET _registered_targets 0 _registered_target)
        themis_register_module_focused_test(
            MODULE security
            NAME ${_ctest}
            TARGET ${_registered_target}
            TIER unit
            TIMEOUT 120
            LABELS security autogen
        )
        continue()
    endif()

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

    themis_register_module_focused_test(
        MODULE security
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS security autogen
    )
endforeach()