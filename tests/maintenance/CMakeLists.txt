cmake_minimum_required(VERSION 3.20)
# Placeholder module test directory for mirrored src structure.

# Phase 4 contract-hardening focused tests.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_maintenance_contract_hardening_focused.cpp")
    set(_target "module_maintenance_test_maintenance_contract_hardening_focused_focused")
    if(NOT TARGET ${_target})
        add_executable(${_target}
            test_maintenance_contract_hardening_focused.cpp
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
            MODULE maintenance
            NAME   test_maintenance_contract_hardening_focused_maintenance_FocusedTests
            TARGET ${_target}
            TIER   unit
            TIMEOUT 120
            LABELS  maintenance
        )
    endif()
endif()

# Phase 2/3 hardening focused tests (MTN-09..MTN-16).
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_maintenance_hardening_phase23_focused.cpp")
    set(_target23 "module_maintenance_test_maintenance_hardening_phase23_focused")
    if(NOT TARGET ${_target23})
        add_executable(${_target23}
            test_maintenance_hardening_phase23_focused.cpp
        )
        target_include_directories(${_target23} PRIVATE
            ${THEMIS_ROOT_DIR}/include
            ${THEMIS_ROOT_DIR}/src
        )
        target_link_libraries(${_target23} PRIVATE
            ${TEST_LIBS}
            themis_core
            spdlog::spdlog
            Threads::Threads
        )
        target_compile_definitions(${_target23} PRIVATE THEMIS_TEST_BUILD=1)
        themis_register_module_focused_test(
            MODULE maintenance
            NAME   test_maintenance_hardening_phase23_focused_maintenance_FocusedTests
            TARGET ${_target23}
            TIER   unit
            TIMEOUT 120
            LABELS  maintenance
        )
    endif()
endif()

# Phase 2 churn-hardening focused tests (MTN-17..MTN-20).
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_maintenance_churn_hardening_focused.cpp")
    set(_target_churn "module_maintenance_test_maintenance_churn_hardening_focused")
    if(NOT TARGET ${_target_churn})
        add_executable(${_target_churn}
            test_maintenance_churn_hardening_focused.cpp
        )
        target_include_directories(${_target_churn} PRIVATE
            ${THEMIS_ROOT_DIR}/include
            ${THEMIS_ROOT_DIR}/src
        )
        target_link_libraries(${_target_churn} PRIVATE
            ${TEST_LIBS}
            themis_core
            spdlog::spdlog
            Threads::Threads
        )
        target_compile_definitions(${_target_churn} PRIVATE THEMIS_TEST_BUILD=1)
        themis_register_module_focused_test(
            MODULE maintenance
            NAME   test_maintenance_churn_hardening_focused_maintenance_FocusedTests
            TARGET ${_target_churn}
            TIER   unit
            TIMEOUT 120
            LABELS  maintenance
        )
    endif()
endif()

# Phase 3 stress/persistence focused tests (MTN-21..MTN-28).
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_maintenance_stress_focused.cpp")
    set(_target_stress "module_maintenance_test_maintenance_stress_focused")
    if(NOT TARGET ${_target_stress})
        add_executable(${_target_stress}
            test_maintenance_stress_focused.cpp
        )
        target_include_directories(${_target_stress} PRIVATE
            ${THEMIS_ROOT_DIR}/include
            ${THEMIS_ROOT_DIR}/src
        )
        target_link_libraries(${_target_stress} PRIVATE
            ${TEST_LIBS}
            themis_core
            spdlog::spdlog
            Threads::Threads
        )
        target_compile_definitions(${_target_stress} PRIVATE THEMIS_TEST_BUILD=1)
        themis_register_module_focused_test(
            MODULE maintenance
            NAME   test_maintenance_stress_focused_maintenance_FocusedTests
            TARGET ${_target_stress}
            TIER   unit
            TIMEOUT 180
            LABELS  maintenance
        )
    endif()
endif()

# Phase 4 diagnostics focused tests (MTN-29..MTN-32).
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_maintenance_diagnostics_focused.cpp")
    set(_target_diag "module_maintenance_test_maintenance_diagnostics_focused")
    if(NOT TARGET ${_target_diag})
        add_executable(${_target_diag}
            test_maintenance_diagnostics_focused.cpp
        )
        target_include_directories(${_target_diag} PRIVATE
            ${THEMIS_ROOT_DIR}/include
            ${THEMIS_ROOT_DIR}/src
        )
        target_link_libraries(${_target_diag} PRIVATE
            ${TEST_LIBS}
            themis_core
            spdlog::spdlog
            Threads::Threads
        )
        target_compile_definitions(${_target_diag} PRIVATE THEMIS_TEST_BUILD=1)
        themis_register_module_focused_test(
            MODULE maintenance
            NAME   test_maintenance_diagnostics_focused_maintenance_FocusedTests
            TARGET ${_target_diag}
            TIER   unit
            TIMEOUT 120
            LABELS  maintenance
        )
    endif()
endif()

# Phase 6 endurance test (MTN-ENDURANCE-01).
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_maintenance_endurance_focused.cpp")
    set(_target_endurance "module_maintenance_test_maintenance_endurance_focused")
    if(NOT TARGET ${_target_endurance})
        add_executable(${_target_endurance}
            test_maintenance_endurance_focused.cpp
        )
        target_include_directories(${_target_endurance} PRIVATE
            ${THEMIS_ROOT_DIR}/include
            ${THEMIS_ROOT_DIR}/src
        )
        target_link_libraries(${_target_endurance} PRIVATE
            ${TEST_LIBS}
            themis_core
            spdlog::spdlog
            Threads::Threads
        )
        target_compile_definitions(${_target_endurance} PRIVATE THEMIS_TEST_BUILD=1)
        themis_register_module_focused_test(
            MODULE maintenance
            NAME   test_maintenance_endurance_focused_maintenance_FocusedTests
            TARGET ${_target_endurance}
            TIER   integration
            TIMEOUT 120
            LABELS  maintenance endurance
        )
    endif()
endif()
