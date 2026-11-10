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

add_executable(wire_protocol_parser_harness
    "harnesses/wire_protocol_parser_harness.cpp"
    "${THEMIS_ROOT_DIR}/src/network/websocket_binary_frame_parser.cpp"
)
target_include_directories(wire_protocol_parser_harness PRIVATE
    "${THEMIS_ROOT_DIR}/src/network"
)
set_target_properties(wire_protocol_parser_harness PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${_THEMIS_FUZZ_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${_THEMIS_FUZZ_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${_THEMIS_FUZZ_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${_THEMIS_FUZZ_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${_THEMIS_FUZZ_OUTPUT_DIR}"
)

include(CheckIncludeFileCXX)
check_include_file_cxx("boost/beast/core.hpp" THEMIS_HAS_BOOST_BEAST)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
if(THEMIS_HAS_BOOST_BEAST)
    list(APPEND _THEMIS_STANDALONE_FUZZ_HARNESSES http_parser)
else()
    message(STATUS "Skipping http_parser_harness: boost/beast headers not available")
endif()

add_custom_target(fuzz_targets)
add_dependencies(fuzz_targets wire_protocol_parser_harness)

foreach(_themis_harness IN LISTS _THEMIS_STANDALONE_FUZZ_HARNESSES)
    set(_themis_target "${_themis_harness}_harness")
    add_executable(${_themis_target} "harnesses/${_themis_target}.cpp")
    if(_themis_harness STREQUAL "aql_parser")
        target_sources(${_themis_target} PRIVATE
            "${THEMIS_ROOT_DIR}/src/query/aql_parser.cpp"
            "${THEMIS_ROOT_DIR}/src/query/aql_parser_json.cpp"
            "${THEMIS_ROOT_DIR}/src/utils/error_registry.cpp"
            "${THEMIS_ROOT_DIR}/src/utils/error_contracts.cpp"
            "${THEMIS_ROOT_DIR}/src/utils/logger.cpp"
        )
        target_link_libraries(${_themis_target} PRIVATE fmt::fmt spdlog::spdlog)
    endif()
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

# ─────────────────────────────────────────────────────────────────────────────
# Project-linked harnesses
#
# The GGUF and grammar harnesses use their existing lightweight stubs.
# PostgreSQL importer smoke links the real implementation so corpus inputs
# exercise the production parser.
# ─────────────────────────────────────────────────────────────────────────────

# ── gguf_loader ──────────────────────────────────────────────────────────────
add_executable(gguf_loader_harness
    "harnesses/gguf_loader_harness.cpp"
    "stubs/gguf_loader_stub.cpp"
)
target_include_directories(gguf_loader_harness PRIVATE "${THEMIS_ROOT_DIR}/include")
target_compile_definitions(gguf_loader_harness PRIVATE THEMIS_FUZZ_STUBS=1)
set_target_properties(gguf_loader_harness PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY                "${_THEMIS_FUZZ_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${_THEMIS_FUZZ_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${_THEMIS_FUZZ_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${_THEMIS_FUZZ_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${_THEMIS_FUZZ_OUTPUT_DIR}"
)
add_dependencies(fuzz_targets gguf_loader_harness)
message(STATUS "Fuzz harness configured: gguf_loader_harness (stub build)")

# ── grammar ──────────────────────────────────────────────────────────────────
add_executable(grammar_harness
    "harnesses/grammar_harness.cpp"
    "stubs/grammar_stub.cpp"
)
target_include_directories(grammar_harness PRIVATE "${THEMIS_ROOT_DIR}/include")
target_compile_definitions(grammar_harness PRIVATE THEMIS_FUZZ_STUBS=1)
set_target_properties(grammar_harness PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY                "${_THEMIS_FUZZ_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${_THEMIS_FUZZ_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${_THEMIS_FUZZ_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${_THEMIS_FUZZ_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${_THEMIS_FUZZ_OUTPUT_DIR}"
)
add_dependencies(fuzz_targets grammar_harness)
message(STATUS "Fuzz harness configured: grammar_harness (stub build)")

# ── postgres_importer ─────────────────────────────────────────────────────────
# Requires nlohmann_json (pulled in transitively by the header chain).
find_package(nlohmann_json CONFIG QUIET)
if(NOT nlohmann_json_FOUND)
    find_package(nlohmann_json MODULE QUIET)
endif()

if(nlohmann_json_FOUND AND spdlog_FOUND)
    add_executable(postgres_importer_harness
        "harnesses/postgres_importer_harness.cpp"
        "${THEMIS_ROOT_DIR}/src/importers/postgres_importer.cpp"
        "${THEMIS_ROOT_DIR}/src/importers/conflict_resolver.cpp"
        "${THEMIS_ROOT_DIR}/src/utils/logger.cpp"
        "${THEMIS_ROOT_DIR}/src/utils/error_registry.cpp"
        "${THEMIS_ROOT_DIR}/src/utils/error_contracts.cpp"
    )
    target_include_directories(postgres_importer_harness PRIVATE
        "${THEMIS_ROOT_DIR}/include"
        "${THEMIS_ROOT_DIR}/src"
    )
    target_link_libraries(postgres_importer_harness PRIVATE
        nlohmann_json::nlohmann_json
        spdlog::spdlog
    )
    set_target_properties(postgres_importer_harness PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY                "${_THEMIS_FUZZ_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${_THEMIS_FUZZ_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${_THEMIS_FUZZ_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${_THEMIS_FUZZ_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${_THEMIS_FUZZ_OUTPUT_DIR}"
    )
    add_dependencies(fuzz_targets postgres_importer_harness)
    message(STATUS "Fuzz harness configured: postgres_importer_harness (production parser)")
else()
    message(STATUS "Skipping postgres_importer_harness: requires nlohmann_json and spdlog")
endif()
