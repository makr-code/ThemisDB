include_guard(GLOBAL)

# =============================================================================
# BenchmarkPolicy.cmake
# Canonical CTest registration helper for Google Benchmark targets.
#
# Mirrors the test-side TestPolicy.cmake / RegisterModuleTests.cmake pattern so
# that benchmark smoke runs follow the same structural rules as unit tests:
#   - Only registered when the target actually exists (avoids orphaned entries).
#   - Uses $<TARGET_FILE:...> so CTest resolves the binary path at run-time.
#   - Mandatory labels: module, tier:benchmark, kind.
#   - Explicit timeout and JSON output arguments enforced uniformly.
#   - A unique result file for every CTest name, including duplicate targets.
# =============================================================================

include(CMakeParseArguments)

function(_themis_benchmark_json_string output value)
    string(REPLACE "\\" "\\\\" _value "${value}")
    string(REPLACE "\"" "\\\"" _value "${_value}")
    string(REPLACE "\n" "\\n" _value "${_value}")
    string(REPLACE "\r" "\\r" _value "${_value}")
    string(REPLACE "\t" "\\t" _value "${_value}")
    set(${output} "\"${_value}\"" PARENT_SCOPE)
endfunction()

# -----------------------------------------------------------------------------
# themis_register_benchmark_ctest
#
# Registers a Google Benchmark binary as a CTest smoke entry.
#
# Parameters
#   NAME          Unique CTest test name (required).
#   TARGET        CMake target name of the benchmark executable (required).
#   MODULE        Module/subsystem label, e.g. "geo", "wave5" (required).
#   KIND          Optional kind tag, defaults to "smoke".
#   TIMEOUT       Timeout in seconds, defaults to 120.
#   EXTRA_LABELS  Additional labels appended after the standard set.
#   MIN_TIME      --benchmark_min_time argument value, defaults to "0.1s".
#   EXTRA_ARGS    Additional arguments forwarded to the benchmark binary.
# -----------------------------------------------------------------------------
function(themis_register_benchmark_ctest)
    set(options)
    set(oneValueArgs NAME TARGET MODULE KIND TIMEOUT MIN_TIME)
    set(multiValueArgs EXTRA_LABELS EXTRA_ARGS)
    cmake_parse_arguments(TRB "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT TRB_NAME)
        message(FATAL_ERROR "themis_register_benchmark_ctest requires NAME")
    endif()

    if(NOT TRB_TARGET)
        message(FATAL_ERROR "themis_register_benchmark_ctest requires TARGET")
    endif()

    if(NOT TRB_MODULE)
        message(FATAL_ERROR "themis_register_benchmark_ctest requires MODULE")
    endif()

    if(NOT TRB_KIND)
        set(TRB_KIND "smoke")
    endif()

    if(NOT TRB_TIMEOUT)
        set(TRB_TIMEOUT 120)
    endif()

    if(NOT TRB_MIN_TIME)
        set(TRB_MIN_TIME "0.1s")
    endif()

    string(SHA256 _id "${TRB_NAME}")
    foreach(_field IN ITEMS TRB_NAME TRB_TARGET TRB_MODULE TRB_KIND)
        _themis_benchmark_json_string(_json_${_field} "${${_field}}")
    endforeach()
    if(NOT TARGET "${TRB_TARGET}")
        set_property(GLOBAL APPEND PROPERTY THEMIS_BENCHMARK_MANIFEST_ENTRIES
            "{\"id\":\"${_id}\",\"test\":${_json_TRB_NAME},\"target\":${_json_TRB_TARGET},\"module\":${_json_TRB_MODULE},\"kind\":${_json_TRB_KIND},\"registration_status\":\"not_available\",\"executable\":null,\"result\":null}")
        message(STATUS "[benchmark-policy] Skipping CTest entry '${TRB_NAME}' — target '${TRB_TARGET}' is not available")
        return()
    endif()

    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    foreach(_arg IN LISTS TRB_EXTRA_ARGS)
        if(_arg MATCHES "^--benchmark_out(_format)?(=|$)")
            message(FATAL_ERROR "Benchmark output arguments are owned by BenchmarkPolicy: ${_arg}")
        endif()
    endforeach()

    set(_out_dir "${CMAKE_BINARY_DIR}/bench_results/current")
    string(LENGTH "${TRB_NAME}" _name_length)
    string(TOUPPER "${TRB_NAME}" _upper_name)
    if(TRB_NAME MATCHES "^[A-Za-z0-9_][A-Za-z0-9_.-]*$"
       AND NOT TRB_NAME MATCHES "\\.$"
       AND NOT _upper_name MATCHES "^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(\\.|$)"
       AND _name_length LESS 180)
        set(_result "${_out_dir}/${TRB_NAME}.json")
        string(TOLOWER "${TRB_NAME}" _folded_name)
        string(SHA256 _name_key "${_folded_name}")
        get_property(_existing GLOBAL PROPERTY "THEMIS_BENCHMARK_FILENAME_${_name_key}")
        if(_existing)
            message(FATAL_ERROR "Benchmark result filename collides with ${_existing}: ${TRB_NAME}")
        endif()
        set_property(GLOBAL PROPERTY "THEMIS_BENCHMARK_FILENAME_${_name_key}" "${TRB_NAME}")
    else()
        set(_result "${_out_dir}/.unsafe/${_id}.json")
    endif()
    _themis_benchmark_json_string(_json_executable "$<TARGET_FILE:${TRB_TARGET}>")
    _themis_benchmark_json_string(_json_result "${_result}")
    set_property(GLOBAL APPEND PROPERTY THEMIS_BENCHMARK_MANIFEST_ENTRIES
        "{\"id\":\"${_id}\",\"test\":${_json_TRB_NAME},\"target\":${_json_TRB_TARGET},\"module\":${_json_TRB_MODULE},\"kind\":${_json_TRB_KIND},\"registration_status\":\"registered\",\"executable\":${_json_executable},\"result\":${_json_result}}")

    add_test(
        NAME    "${TRB_NAME}"
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/benchmark_run_manifest.py"
            run --results-dir "${_out_dir}" --id "${_id}" --timeout "${TRB_TIMEOUT}"
            -- "$<TARGET_FILE:${TRB_TARGET}>"
            "--benchmark_out=${_out_dir}/.manifest/${_id}-output"
            "--benchmark_out_format=json"
            "--benchmark_min_time=${TRB_MIN_TIME}"
            ${TRB_EXTRA_ARGS}
    )

    set(_labels
        "module:${TRB_MODULE}"
        "tier:benchmark"
        "kind:${TRB_KIND}"
    )

    if(TRB_EXTRA_LABELS)
        list(APPEND _labels ${TRB_EXTRA_LABELS})
        if("release_critical" IN_LIST TRB_EXTRA_LABELS)
            set_property(GLOBAL APPEND PROPERTY THEMIS_RELEASE_CRITICAL_TARGETS ${TRB_TARGET})
        endif()
        if("wave_a_benchmark_server_llm" IN_LIST TRB_EXTRA_LABELS)
            set_property(GLOBAL APPEND PROPERTY
                THEMIS_WAVE_A_BENCHMARK_SERVER_LLM_TARGETS
                ${TRB_TARGET})
        endif()
        if("wave_a_benchmark_server_query_storage_transaction" IN_LIST TRB_EXTRA_LABELS)
            set_property(GLOBAL APPEND PROPERTY
                THEMIS_WAVE_A_BENCHMARK_SERVER_QUERY_STORAGE_TRANSACTION_TARGETS
                ${TRB_TARGET})
        endif()
        if("wave_a_benchmark_sharding_transaction" IN_LIST TRB_EXTRA_LABELS)
            set_property(GLOBAL APPEND PROPERTY
                THEMIS_WAVE_A_BENCHMARK_SHARDING_TRANSACTION_TARGETS
                ${TRB_TARGET})
        endif()
        if("wave_a_benchmark_search_index_tensor_graph_llm" IN_LIST TRB_EXTRA_LABELS)
            set_property(GLOBAL APPEND PROPERTY
                THEMIS_WAVE_A_BENCHMARK_SEARCH_INDEX_TENSOR_GRAPH_LLM_TARGETS
                ${TRB_TARGET})
        endif()
    endif()

    if(NOT TRB_TIMEOUT MATCHES "^([0-9]+)(\\.[0-9]+)?$")
        message(FATAL_ERROR "Benchmark TIMEOUT must be a non-negative number of seconds")
    endif()
    math(EXPR _ctest_timeout "${CMAKE_MATCH_1} + 16")
    set_tests_properties("${TRB_NAME}" PROPERTIES
        LABELS            "${_labels}"
        TIMEOUT           "${_ctest_timeout}"
        FIXTURES_REQUIRED "themis_google_benchmark_run"
    )
endfunction()

# Add one benchmark-suite setup and one post-processing cleanup test. CTest
# schedules the cleanup after every benchmark requiring the run fixture, even
# when a benchmark fails, and auto-adds fixture setup/cleanup for label subsets.
function(themis_register_benchmark_postprocess_ctest)
    set(options)
    set(oneValueArgs NAME SCRIPT RESULTS_DIR ARCHIVE_DIR REPORT_PATH TIMEOUT)
    cmake_parse_arguments(TRP "${options}" "${oneValueArgs}" "" ${ARGN})

    foreach(_required IN ITEMS NAME SCRIPT RESULTS_DIR ARCHIVE_DIR REPORT_PATH)
        if(NOT TRP_${_required})
            message(FATAL_ERROR "themis_register_benchmark_postprocess_ctest requires ${_required}")
        endif()
    endforeach()

    if(NOT EXISTS "${TRP_SCRIPT}")
        message(FATAL_ERROR "Benchmark postprocessor not found: ${TRP_SCRIPT}")
    endif()

    if(NOT TRP_RESULTS_DIR STREQUAL "${CMAKE_BINARY_DIR}/bench_results/current")
        message(FATAL_ERROR "Benchmark postprocessor RESULTS_DIR must match the benchmark registration directory")
    endif()

    if(NOT TRP_TIMEOUT)
        set(TRP_TIMEOUT 600)
    endif()

    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    get_property(_entries GLOBAL PROPERTY THEMIS_BENCHMARK_MANIFEST_ENTRIES)
    string(JOIN ",\n" _entry_json ${_entries})
    set(_registry "${CMAKE_BINARY_DIR}/bench_results/registry-$<CONFIG>.json")
    file(GENERATE OUTPUT "${_registry}" CONTENT "[${_entry_json}]\n")

    add_test(
        NAME "${TRP_NAME}_prepare"
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/benchmark_run_manifest.py"
            prepare --results-dir "${TRP_RESULTS_DIR}" --archive-dir "${TRP_ARCHIVE_DIR}"
            --registry "${_registry}" --cmake "${CMAKE_COMMAND}"
            --prepare-script "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/PrepareBenchmarkResults.cmake"
    )
    set_tests_properties("${TRP_NAME}_prepare" PROPERTIES
        LABELS "module:benchmark_reporting;tier:benchmark;kind:setup"
        FIXTURES_SETUP "themis_google_benchmark_run"
        TIMEOUT 60
    )

    add_test(
        NAME "${TRP_NAME}"
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/benchmark_run_manifest.py"
            finish --results-dir "${TRP_RESULTS_DIR}" --src-dir "${THEMIS_BENCHMARK_ROOT}/src"
            -- "${Python3_EXECUTABLE}" "${TRP_SCRIPT}"
            --src-dir "${THEMIS_BENCHMARK_ROOT}/src" --results-dir "${TRP_RESULTS_DIR}"
            --output "${TRP_REPORT_PATH}"
    )
    set_tests_properties("${TRP_NAME}" PROPERTIES
        LABELS "module:benchmark_reporting;tier:benchmark;kind:postprocess"
        FIXTURES_CLEANUP "themis_google_benchmark_run"
        TIMEOUT "${TRP_TIMEOUT}"
    )
endfunction()
