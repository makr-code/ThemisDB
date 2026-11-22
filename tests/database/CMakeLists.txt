# database module tests

file(GLOB DATABASE_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS DATABASE_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_database_${_stem}_focused")
    set(_ctest "${_stem}_database_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    if(_stem STREQUAL "test_database_maintenance_orchestrator")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/maintenance/database_maintenance_orchestrator.cpp
            ${THEMIS_ROOT_DIR}/src/maintenance/maintenance_schedule_store.cpp
            ${THEMIS_ROOT_DIR}/src/maintenance/maintenance_registry.cpp
            ${THEMIS_ROOT_DIR}/src/server/maintenance_api_handler.cpp
        )
    elseif(_stem STREQUAL "test_database_domain_auto_labeler")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/training/database_domain_auto_labeler.cpp
        )
    endif()
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    if(_stem STREQUAL "test_database_maintenance_orchestrator")
        target_link_libraries(${_target} PRIVATE nlohmann_json::nlohmann_json)
    endif()
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)
    if(_stem STREQUAL "test_database_maintenance_orchestrator")
        target_compile_definitions(${_target} PRIVATE THEMIS_ENABLE_HTTP_SERVER=1)
    endif()

    themis_register_module_focused_test(
        MODULE database
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS database
    )
    if(_stem STREQUAL "test_database_maintenance_orchestrator")
        add_test(NAME DatabaseMaintenanceOrchestratorFocusedTests
            COMMAND $<TARGET_FILE:${_target}>
        )
        set_tests_properties(DatabaseMaintenanceOrchestratorFocusedTests PROPERTIES
            LABELS "maintenance;orchestrator;schedule;crud;health;window;metrics;focused;unit"
            TIMEOUT 120
        )
    endif()
    if(_stem STREQUAL "test_database_domain_auto_labeler")
        gtest_discover_tests(${_target}
            DISCOVERY_MODE POST_BUILD
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/cmake/tests"
            EXTRA_ARGS --gtest_color=yes
        )
    endif()
endforeach()