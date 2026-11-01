# governance module tests

file(GLOB GOVERNANCE_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS GOVERNANCE_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_governance_${_stem}_focused")
    set(_ctest "${_stem}_governance_FocusedTests")
    set(_labels governance)

    # Keep both suites quarantined until the canonical project build can
    # configure, build, and run their focused targets. The operational-audit
    # test passes a standalone local build, but the project configure is blocked
    # here by missing RocksDB; versioning/approval additionally depends on the
    # full core dependency graph.
    if(_stem STREQUAL "test_operational_audit_evidence"
       OR _stem STREQUAL "test_policy_versioning_and_approval")
        continue()
    endif()

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")

    if(_stem STREQUAL "test_operational_audit_evidence")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/governance/operational_audit.cpp
        )
        target_link_libraries(${_target} PRIVATE OpenSSL::Crypto)
    endif()

    if(_stem STREQUAL "test_policy_versioning_and_approval")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/governance/policy_change_manager.cpp
            ${THEMIS_ROOT_DIR}/src/governance/policy_approval_workflow.cpp
        )
        target_link_libraries(${_target} PRIVATE fmt::fmt)
    endif()

    # Audit-integrity focused tests require concrete implementation symbols
    # from governance_audit_integrity.cpp.
    if(_stem STREQUAL "test_audit_integrity")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/governance/governance_audit_integrity.cpp
        )
    endif()

    # Compliance framework focused tests require concrete registry/mapping/
    # validator implementation symbols.
    if(_stem STREQUAL "test_compliance_framework")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/governance/compliance_framework.cpp
            ${THEMIS_ROOT_DIR}/src/governance/compliance_mapping.cpp
            ${THEMIS_ROOT_DIR}/src/governance/compliance_validator.cpp
        )
    endif()

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

    if(_stem STREQUAL "test_governance_contract_hardening_focused" OR
       _stem STREQUAL "test_governance_llm_policy_focused" OR
       _stem STREQUAL "test_compliance_framework")
        list(APPEND _labels wave_b wave_b_boundary_policy_control)
    endif()

    themis_register_module_focused_test(
        MODULE governance
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS ${_labels}
    )
endforeach()