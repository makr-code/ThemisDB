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

# ─────────────────────────────────────────────────────────────────────────────
# Project-linked harnesses
#
# These three harnesses include real project headers (llm/gguf_loader.h,
# llm/grammar.h, importers/postgres_importer.h) rather than inline mocks.
# They are compiled against lightweight fuzz stubs in fuzz/stubs/ that provide
# the required class implementations without external dependencies (spdlog,
# RocksDB) so that the targets build standalone under ENABLE_FUZZING=ON.
#
# postgres_importer additionally requires nlohmann_json (transitively included
# by the header chain) and is only built when nlohmann_json is found.
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

if(nlohmann_json_FOUND)
    add_executable(postgres_importer_harness
        "harnesses/postgres_importer_harness.cpp"
        "stubs/postgres_importer_stub.cpp"
        # conflict_resolver.cpp has no external deps beyond nlohmann_json and
        # is compiled directly to satisfy the ImportConflictResolver link dep.
        "${THEMIS_ROOT_DIR}/src/importers/conflict_resolver.cpp"
    )
    target_include_directories(postgres_importer_harness PRIVATE
        "${THEMIS_ROOT_DIR}/include"
        "${THEMIS_ROOT_DIR}/src"
    )
    target_compile_definitions(postgres_importer_harness PRIVATE THEMIS_FUZZ_STUBS=1)
    target_link_libraries(postgres_importer_harness PRIVATE nlohmann_json::nlohmann_json)
    set_target_properties(postgres_importer_harness PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY                "${_THEMIS_FUZZ_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${_THEMIS_FUZZ_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${_THEMIS_FUZZ_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${_THEMIS_FUZZ_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${_THEMIS_FUZZ_OUTPUT_DIR}"
    )
    add_dependencies(fuzz_targets postgres_importer_harness)
    message(STATUS "Fuzz harness configured: postgres_importer_harness (stub build, nlohmann_json found)")
else()
    message(STATUS "Skipping postgres_importer_harness: nlohmann_json not found "
                   "(install via: vcpkg install nlohmann-json  OR  "
                   "apt-get install nlohmann-json3-dev)")
endif()
