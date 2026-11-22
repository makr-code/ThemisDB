# training module tests

file(GLOB TRAINING_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS TRAINING_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_training_${_stem}_focused")
    set(_ctest "${_stem}_training_FocusedTests")
    set(_tier unit)
    if(_stem STREQUAL "test_adalora_training_pipeline")
        set(_tier benchmark)
    endif()

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    set(_training_test_libs ${TEST_LIBS})
    if(_stem STREQUAL "test_incremental_lora_db_training")
        list(REMOVE_ITEM _training_test_libs GTest::gmock gmock::gmock)
    endif()
    target_link_libraries(${_target} PRIVATE
        ${_training_test_libs}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    if(_stem STREQUAL "test_training_convergence" OR
       _stem STREQUAL "test_training_diagnostics_consistency" OR
       _stem STREQUAL "test_training_phase2")
        target_link_libraries(${_target} PRIVATE RocksDB::rocksdb)
    endif()
    if(_stem STREQUAL "test_training_diagnostics_consistency" OR
       _stem STREQUAL "test_training_phase2")
        target_link_libraries(${_target} PRIVATE OpenSSL::Crypto)
    endif()
    if(_stem STREQUAL "test_training_database_optimizer")
        target_link_libraries(${_target} PRIVATE nlohmann_json::nlohmann_json)
    endif()
    if((_stem STREQUAL "test_incremental_lora_db_training" OR
        _stem STREQUAL "test_training_pipeline_db" OR
        _stem STREQUAL "test_adalora_tt_bridge_hardening" OR
        _stem STREQUAL "test_docs_db_training_sample_provider") AND TARGET themis_training)
        target_link_libraries(${_target} PRIVATE themis_training)
    endif()
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE training
        NAME ${_ctest}
        TARGET ${_target}
        TIER ${_tier}
        TIMEOUT 120
        LABELS training
    )
    if(_stem STREQUAL "test_multitask_lora_acceptance_gates")
        gtest_discover_tests(${_target}
            DISCOVERY_MODE POST_BUILD
            WORKING_DIRECTORY $<TARGET_FILE_DIR:${_target}>
            EXTRA_ARGS --gtest_color=yes
        )
    endif()
endforeach()