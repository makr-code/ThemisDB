cmake_minimum_required(VERSION 3.19)

get_filename_component(_THEMIS_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

if(NOT DEFINED OUTPUT_DIR)
    set(OUTPUT_DIR "${_THEMIS_REPO_ROOT}/.cmake-reproducible-build-probe")
endif()

file(REMOVE_RECURSE "${OUTPUT_DIR}")
file(MAKE_DIRECTORY "${OUTPUT_DIR}/src")

file(WRITE "${OUTPUT_DIR}/src/CMakeLists.txt" [=[
cmake_minimum_required(VERSION 3.19)
project(ThemisBuildInfoReproProbe LANGUAGES NONE)
include("]=] "${_THEMIS_REPO_ROOT}/cmake/BuildInfo.cmake" [=[")
]=])

function(_run_configure build_dir epoch require_repro expect_success)
    set(_command
        "${CMAKE_COMMAND}"
        -S "${OUTPUT_DIR}/src"
        -B "${build_dir}"
        "-DTHEMIS_REQUIRE_REPRODUCIBLE_BUILD=${require_repro}")

    if(NOT "${epoch}" STREQUAL "")
        execute_process(
            COMMAND "${CMAKE_COMMAND}" -E env "SOURCE_DATE_EPOCH=${epoch}" ${_command}
            RESULT_VARIABLE _result
            OUTPUT_VARIABLE _stdout
            ERROR_VARIABLE _stderr
        )
    else()
        execute_process(
            COMMAND ${_command}
            RESULT_VARIABLE _result
            OUTPUT_VARIABLE _stdout
            ERROR_VARIABLE _stderr
        )
    endif()

    if(expect_success AND NOT _result EQUAL 0)
        message(FATAL_ERROR
            "Configure failed for '${build_dir}' with exit code ${_result}\n"
            "stdout:\n${_stdout}\n"
            "stderr:\n${_stderr}")
    endif()

    if(NOT expect_success AND _result EQUAL 0)
        message(FATAL_ERROR
            "Configure unexpectedly succeeded for '${build_dir}'.")
    endif()
endfunction()

set(_same_epoch_a "${OUTPUT_DIR}/same-epoch-a")
set(_same_epoch_b "${OUTPUT_DIR}/same-epoch-b")
set(_different_epoch "${OUTPUT_DIR}/different-epoch")
set(_strict_missing_source_date_epoch "${OUTPUT_DIR}/strict-missing-source-date-epoch")
set(_invalid_source_date_epoch "${OUTPUT_DIR}/invalid-source-date-epoch")

_run_configure("${_same_epoch_a}" "1700000000" ON TRUE)
_run_configure("${_same_epoch_b}" "1700000000" ON TRUE)
_run_configure("${_different_epoch}" "1700000100" ON TRUE)
_run_configure("${_strict_missing_source_date_epoch}" "" ON TRUE)
_run_configure("${_invalid_source_date_epoch}" "not-a-timestamp" ON FALSE)

set(_header_same_epoch_a "${_same_epoch_a}/include/updates/build_info.h")
set(_header_same_epoch_b "${_same_epoch_b}/include/updates/build_info.h")
set(_header_different_epoch "${_different_epoch}/include/updates/build_info.h")

foreach(_header IN ITEMS
        "${_header_same_epoch_a}"
        "${_header_same_epoch_b}"
        "${_header_different_epoch}")
    if(NOT EXISTS "${_header}")
        message(FATAL_ERROR "Expected generated header missing: ${_header}")
    endif()
endforeach()

file(SHA256 "${_header_same_epoch_a}" _hash_same_epoch_a)
file(SHA256 "${_header_same_epoch_b}" _hash_same_epoch_b)
file(SHA256 "${_header_different_epoch}" _hash_different_epoch)

if(NOT _hash_same_epoch_a STREQUAL _hash_same_epoch_b)
    message(FATAL_ERROR
        "Identical SOURCE_DATE_EPOCH values produced different build_info.h hashes:\n"
        "  ${_hash_same_epoch_a}\n"
        "  ${_hash_same_epoch_b}")
endif()

if(_hash_same_epoch_a STREQUAL _hash_different_epoch)
    message(FATAL_ERROR
        "Different SOURCE_DATE_EPOCH values produced the same build_info.h hash:\n"
        "  ${_hash_same_epoch_a}")
endif()

message(STATUS "Reproducible build-info check passed")
message(STATUS "same-epoch hash:      ${_hash_same_epoch_a}")
message(STATUS "different-epoch hash: ${_hash_different_epoch}")
