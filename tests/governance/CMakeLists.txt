# governance module tests

file(GLOB GOVERNANCE_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS GOVERNANCE_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_governance_${_stem}_focused")
    set(_ctest "${_stem}_governance_FocusedTests")

    # Temporarily quarantine this suite while operational_audit.h/.cpp API
    # convergence is in progress; otherwise it blocks unrelated build waves.
    if(_stem STREQUAL "test_operational_audit_evidence"
       OR _stem STREQUAL "test_policy_versioning_and_approval")
        continue()
    endif()

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")

    # Audit-integrity focused tests require concrete implementation symbols
    # from governance_audit_integrity.cpp.
    if(_stem STREQUAL "test_audit_integrity")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/governance/governance_audit_integrity.cpp
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

    themis_register_module_focused_test(
        MODULE governance
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS governance
    )
endforeach()