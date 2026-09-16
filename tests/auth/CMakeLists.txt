# auth module tests

file(GLOB AUTH_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS AUTH_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_auth_${_stem}_focused")
    set(_ctest "${_stem}_auth_FocusedTests")

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
        MODULE auth
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS auth
    )
endforeach()

# ---------------------------------------------------------------------------
# Wave 4-B hardening suite (explicit registration for wave_b / release_critical)
# ---------------------------------------------------------------------------

if(NOT TARGET module_auth_test_wave4b_auth_hardening2_focused)
    add_executable(module_auth_test_wave4b_auth_hardening2_focused
        "${CMAKE_CURRENT_SOURCE_DIR}/test_wave4b_auth_hardening2.cpp"
    )
    target_include_directories(module_auth_test_wave4b_auth_hardening2_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(module_auth_test_wave4b_auth_hardening2_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(module_auth_test_wave4b_auth_hardening2_focused
        PRIVATE THEMIS_TEST_BUILD=1
    )
    themis_register_module_focused_test(
        MODULE auth
        NAME test_wave4b_auth_hardening2_auth_FocusedTests
        TARGET module_auth_test_wave4b_auth_hardening2_focused
        TIER unit
        TIMEOUT 120
        LABELS auth wave_b release_critical wave_b_boundary_policy_control
    )
endif()

# ---------------------------------------------------------------------------
# Wave 7 — LDAP Connection Pool + Federated Cross-Provider State Sync
# (explicit registration for wave_b / release_critical)
# ---------------------------------------------------------------------------

if(NOT TARGET module_auth_test_wave7_auth_ldap_federated_focused)
    add_executable(module_auth_test_wave7_auth_ldap_federated_focused
        "${CMAKE_CURRENT_SOURCE_DIR}/test_wave7_auth_ldap_federated.cpp"
    )
    target_include_directories(module_auth_test_wave7_auth_ldap_federated_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(module_auth_test_wave7_auth_ldap_federated_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(module_auth_test_wave7_auth_ldap_federated_focused
        PRIVATE THEMIS_TEST_BUILD=1
    )
    themis_register_module_focused_test(
        MODULE auth
        NAME test_wave7_auth_ldap_federated_auth_FocusedTests
        TARGET module_auth_test_wave7_auth_ldap_federated_focused
        TIER unit
        TIMEOUT 120
        LABELS auth wave_b release_critical wave_b_boundary_policy_control
    )
endif()

# ---------------------------------------------------------------------------
# Auth Protocol Matrix Regression suite (release_critical)
# Deterministic cross-protocol regression tests: OIDC+mTLS, SAML+MFA,
# Kerberos+Revocation, LDAP+Federation, API-Key+Rate-Limiting, WebAuthn+Session,
# OAuth+Blacklist, multi-realm trust isolation.
# ---------------------------------------------------------------------------

if(NOT TARGET module_auth_test_auth_protocol_matrix_regression_focused)
    add_executable(module_auth_test_auth_protocol_matrix_regression_focused
        "${CMAKE_CURRENT_SOURCE_DIR}/test_auth_protocol_matrix_regression.cpp"
    )
    target_include_directories(module_auth_test_auth_protocol_matrix_regression_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(module_auth_test_auth_protocol_matrix_regression_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(module_auth_test_auth_protocol_matrix_regression_focused
        PRIVATE THEMIS_TEST_BUILD=1
    )
    themis_register_module_focused_test(
        MODULE auth
        NAME test_auth_protocol_matrix_regression_auth_FocusedTests
        TARGET module_auth_test_auth_protocol_matrix_regression_focused
        TIER unit
        TIMEOUT 120
        LABELS auth release_critical protocol_matrix
    )
endif()

# ---------------------------------------------------------------------------
# Wave 8 hardening suite — explicit release_critical registration
# Covers W8-15 (LDAP pagination retry), W8-16 (token cache LRU/SHA-256),
# W8-17 (pool exhaustion audit trail).
# ---------------------------------------------------------------------------

if(NOT TARGET module_auth_test_wave8_auth_hardening_focused)
    add_executable(module_auth_test_wave8_auth_hardening_focused
        "${CMAKE_CURRENT_SOURCE_DIR}/test_wave8_auth_hardening.cpp"
    )
    target_include_directories(module_auth_test_wave8_auth_hardening_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(module_auth_test_wave8_auth_hardening_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(module_auth_test_wave8_auth_hardening_focused
        PRIVATE THEMIS_TEST_BUILD=1
    )
    themis_register_module_focused_test(
        MODULE auth
        NAME test_wave8_auth_hardening_auth_FocusedTests
        TARGET module_auth_test_wave8_auth_hardening_focused
        TIER unit
        TIMEOUT 120
        LABELS auth wave_8 release_critical
    )
endif()

# ---------------------------------------------------------------------------
# High-cardinality stress suite (Wave D)
# ---------------------------------------------------------------------------

if(NOT TARGET module_auth_test_auth_highcardinality_stress_focused)
    add_executable(module_auth_test_auth_highcardinality_stress_focused
        "${CMAKE_CURRENT_SOURCE_DIR}/test_auth_highcardinality_stress.cpp"
    )
    target_include_directories(module_auth_test_auth_highcardinality_stress_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(module_auth_test_auth_highcardinality_stress_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(module_auth_test_auth_highcardinality_stress_focused
        PRIVATE THEMIS_TEST_BUILD=1
    )
    themis_register_module_focused_test(
        MODULE auth
        NAME test_auth_highcardinality_stress_auth_FocusedTests
        TARGET module_auth_test_auth_highcardinality_stress_focused
        TIER unit
        TIMEOUT 300
        LABELS auth stress high_cardinality wave_d
    )
endif()

# ---------------------------------------------------------------------------
# Long-duration soak test suite (Wave D)
# ---------------------------------------------------------------------------

if(NOT TARGET module_auth_test_auth_soak_focused)
    add_executable(module_auth_test_auth_soak_focused
        "${CMAKE_CURRENT_SOURCE_DIR}/test_auth_soak.cpp"
    )
    target_include_directories(module_auth_test_auth_soak_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(module_auth_test_auth_soak_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(module_auth_test_auth_soak_focused
        PRIVATE THEMIS_TEST_BUILD=1
    )
    themis_register_module_focused_test(
        MODULE auth
        NAME test_auth_soak_auth_FocusedTests
        TARGET module_auth_test_auth_soak_focused
        TIER unit
        TIMEOUT 600
        LABELS auth soak wave_d
    )
endif()