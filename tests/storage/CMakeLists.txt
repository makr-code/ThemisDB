# Storage focused module tests (flat hierarchy under tests/storage)

add_executable(test_wal_process_crash_worker
    "${CMAKE_CURRENT_LIST_DIR}/wal_process_crash_worker.cpp"
)
target_include_directories(test_wal_process_crash_worker PRIVATE
    ${THEMIS_ROOT_DIR}/include
)
if(TARGET themis_storage)
    target_link_libraries(test_wal_process_crash_worker PRIVATE themis_storage)
else()
    target_link_libraries(test_wal_process_crash_worker PRIVATE themis_core)
endif()
target_link_libraries(test_wal_process_crash_worker PRIVATE
    spdlog::spdlog
    Threads::Threads
)
target_compile_definitions(test_wal_process_crash_worker PRIVATE THEMIS_TEST_BUILD=1)

find_package(Python3 3.10 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    set(_wal_crash_environment
        "PATH=path_list_prepend:${CMAKE_BINARY_DIR}/bin"
    )
    if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
        list(APPEND _wal_crash_environment
            "PATH=path_list_prepend:${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin"
        )
    endif()
    add_test(
        NAME WALProcessCrashRecoveryTests
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_LIST_DIR}/test_wal_process_crash.py"
            --worker "$<TARGET_FILE:test_wal_process_crash_worker>"
            --iterations 50
            --report "${CMAKE_BINARY_DIR}/test-results/wal-process-crash.json"
    )
    set_tests_properties(WALProcessCrashRecoveryTests PROPERTIES
        LABELS "storage;wal;process;crash;integration"
        TIMEOUT 900
        ENVIRONMENT_MODIFICATION "${_wal_crash_environment}"
    )
    add_test(
        NAME WALProcessUnobservedAppendTests
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_LIST_DIR}/test_wal_process_crash.py"
            --worker "$<TARGET_FILE:test_wal_process_crash_worker>"
            --scenario unobserved-append
            --iterations 50
            --report "${CMAKE_BINARY_DIR}/test-results/wal-unobserved-append.json"
    )
    set_tests_properties(WALProcessUnobservedAppendTests PROPERTIES
        LABELS "storage;wal;process;crash;integration"
        TIMEOUT 900
        ENVIRONMENT_MODIFICATION "${_wal_crash_environment}"
    )
else()
    message(WARNING "Python 3.10+ interpreter not found; WAL process-crash tests are not registered")
endif()

file(GLOB STORAGE_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

if(UNIX)
    add_executable(storage_wal_crash_worker
        "${CMAKE_CURRENT_SOURCE_DIR}/wal_crash_worker.cpp"
    )
    target_include_directories(storage_wal_crash_worker PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )
    target_link_libraries(storage_wal_crash_worker PRIVATE
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
endif()

foreach(_src IN LISTS STORAGE_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_storage_${_stem}_focused")
    set(_ctest "${_stem}_StorageFocusedTests")
    set(_labels)

    set(_extra_sources)

    if(_stem STREQUAL "test_tensor_storage_observer")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/graph/tensor_fingerprint_graph.cpp
        )
    elseif(_stem STREQUAL "test_mvcc_chain_pruner")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/storage/mvcc_chain_pruner.cpp
            ${THEMIS_ROOT_DIR}/src/storage/mvcc_store.cpp
            ${THEMIS_ROOT_DIR}/src/temporal/temporal_tier_manager.cpp
            ${THEMIS_ROOT_DIR}/src/temporal/temporal_cold_store.cpp
        )
    elseif(_stem STREQUAL "test_storage_parquet_exporter")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/storage/storage_parquet_exporter.cpp
            ${THEMIS_ROOT_DIR}/src/storage/columnar_format.cpp
        )
    elseif(_stem STREQUAL "test_storage_layout_advisor")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/storage/storage_layout_advisor.cpp
        )
    elseif(_stem STREQUAL "test_storage_phase3_error_handling_focused")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/storage/storage_error_diagnostics.cpp
            ${THEMIS_ROOT_DIR}/src/storage/storage_recovery_fault_handler.cpp
            ${THEMIS_ROOT_DIR}/src/storage/storage_pressure_manager.cpp
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
    if(_stem STREQUAL "test_mvcc_chain_pruner")
        target_link_libraries(${_target} PRIVATE nlohmann_json::nlohmann_json)
        if(TARGET themis_rocksdb_lib)
            target_link_libraries(${_target} PRIVATE themis_rocksdb_lib)
        elseif(TARGET RocksDB::rocksdb)
            target_link_libraries(${_target} PRIVATE RocksDB::rocksdb)
        endif()
        target_compile_definitions(${_target} PRIVATE THEMIS_UNIT_TEST=1)
    endif()
    if(_stem STREQUAL "test_storage_parquet_exporter")
        if(DEFINED THEMIS_LZ4_TARGET)
            target_link_libraries(${_target} PRIVATE ${THEMIS_LZ4_TARGET})
        elseif(TARGET lz4::lz4)
            target_link_libraries(${_target} PRIVATE lz4::lz4)
        endif()
        if(TARGET Snappy::snappy)
            target_link_libraries(${_target} PRIVATE Snappy::snappy)
        endif()
    endif()
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    if(_stem STREQUAL "test_wal_process_crash" AND UNIX)
        add_dependencies(${_target} storage_wal_crash_worker)
        target_compile_definitions(${_target} PRIVATE
            THEMIS_STO07_WAL_CRASH_WORKER="$<TARGET_FILE:storage_wal_crash_worker>"
        )
        list(APPEND _labels storage crash_recovery process_kill)
    elseif(_stem STREQUAL "test_storage_contract_hardening_focused")
        list(APPEND _labels storage contract hardening wave_a release_critical wave_a_flow_server_query_storage_transaction)
    elseif(_stem STREQUAL "test_storage_phase3_error_handling_focused")
        list(APPEND _labels storage error_handling phase3 wave_a release_critical wave_a_flow_server_query_storage_transaction)
    elseif(_stem STREQUAL "test_mvcc_chain_pruner")
        list(APPEND _labels storage mvcc pruning wave_a release_critical wave_a_flow_server_query_storage_transaction)
    endif()

    themis_register_module_focused_test(
        MODULE storage
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS ${_labels}
    )
    if(_stem STREQUAL "test_storage_layout_advisor")
        gtest_discover_tests(${_target}
            DISCOVERY_MODE POST_BUILD
            WORKING_DIRECTORY $<TARGET_FILE_DIR:${_target}>
            EXTRA_ARGS --gtest_color=yes
        )
    endif()
endforeach()

# AUTOGEN PREFIX BLOCK: storage
file(GLOB STORAGE_AUTOGEN_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_storage_*.cpp"
)

foreach(_src IN LISTS STORAGE_AUTOGEN_PREFIX_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_storage_${_stem}_focused")
    set(_ctest "${_stem}_storage_AutoFocusedTests")
    set(_labels storage autogen)

    if(_stem STREQUAL "test_storage_phase3_error_handling_focused")
        list(APPEND _labels error_handling phase3 wave_a release_critical wave_a_flow_server_query_storage_transaction)
    endif()

    if(TARGET ${_target})
        themis_register_module_focused_test(
            MODULE storage
            NAME ${_ctest}
            TARGET ${_target}
            TIER unit
            TIMEOUT 120
            LABELS ${_labels}
        )
    else()
        message(FATAL_ERROR
            "AutoFocused storage test '${_src}' has no canonical target '${_target}'"
        )
    endif()
endforeach()