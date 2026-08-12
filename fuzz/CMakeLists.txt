set(_THEMIS_FUZZ_OUTPUT_DIR "${CMAKE_BINARY_DIR}/fuzz/bin")
file(MAKE_DIRECTORY "${_THEMIS_FUZZ_OUTPUT_DIR}")

set(_THEMIS_STANDALONE_FUZZ_HARNESSES
    aql_parser
    jwt_rbac_config
    ldap_dn
    pii_redaction
    security_input_validator
    security_policy_engine
)

include(CheckIncludeFileCXX)
check_include_file_cxx("boost/beast/core.hpp" THEMIS_HAS_BOOST_BEAST)
if(THEMIS_HAS_BOOST_BEAST)
    list(APPEND _THEMIS_STANDALONE_FUZZ_HARNESSES http_parser)
else()
    message(STATUS "Skipping http_parser_harness: boost/beast headers not available")
endif()

add_custom_target(fuzz_targets)

foreach(_themis_harness IN LISTS _THEMIS_STANDALONE_FUZZ_HARNESSES)
    set(_themis_target "${_themis_harness}_harness")
    add_executable(${_themis_target} "harnesses/${_themis_target}.cpp")
    target_include_directories(${_themis_target} PRIVATE "${THEMIS_ROOT_DIR}/include")
    set_target_properties(${_themis_target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${_THEMIS_FUZZ_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${_THEMIS_FUZZ_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${_THEMIS_FUZZ_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${_THEMIS_FUZZ_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${_THEMIS_FUZZ_OUTPUT_DIR}"
    )
    add_dependencies(fuzz_targets ${_themis_target})
endforeach()
