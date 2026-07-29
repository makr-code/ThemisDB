# Security focused module tests (flat hierarchy under tests/security)

file(GLOB SECURITY_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS SECURITY_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_security_${_stem}_focused")
    set(_ctest "${_stem}_SecurityFocusedTests")

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

    if(_stem STREQUAL "test_aql_safety_validator")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/query/aql_safety_validator.cpp
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
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_security_${_stem}_autofocused")
    set(_ctest "${_stem}_security_AutoFocusedTests")

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