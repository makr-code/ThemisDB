# Audit scope: Wave 10 governance compliance test registration.
# This keeps Block D gate tests discoverable in both the repository and the
# active CMake configure while avoiding leaked build state across reconfigures.

function(themis_register_governance_test target source)
    if(TARGET ${target})
        return()
    endif()

    if(NOT EXISTS "${source}")
        return()
    endif()

    add_executable(${target} "${source}")
    target_include_directories(${target} PRIVATE
        "${THEMIS_ROOT_DIR}/include"
        "${THEMIS_ROOT_DIR}/src"
        "${THEMIS_ROOT_DIR}/tests"
    )
    target_link_libraries(${target} PRIVATE ${TEST_LIBS})
    target_compile_definitions(${target} PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME ${target}Tests COMMAND $<TARGET_FILE:${target}>)
endfunction()

themis_register_governance_test(test_wave_exit_criteria "${THEMIS_ROOT_DIR}/tests/audit/test_wave_exit_criteria.cpp")
themis_register_governance_test(test_edition_compliance "${THEMIS_ROOT_DIR}/tests/audit/test_edition_compliance.cpp")
themis_register_governance_test(test_roadmap_alignment "${THEMIS_ROOT_DIR}/tests/audit/test_roadmap_alignment.cpp")
