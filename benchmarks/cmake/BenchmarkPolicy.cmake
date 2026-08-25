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
# =============================================================================

include(CMakeParseArguments)

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

    if(NOT TARGET "${TRB_TARGET}")
        message(STATUS
            "[benchmark-policy] Skipping CTest entry '${TRB_NAME}' — "
            "target '${TRB_TARGET}' is not available"
        )
        return()
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

    # Ensure output directory exists at configure time so the benchmark can
    # write its JSON report without failing on a missing directory.
    set(_out_dir "${CMAKE_BINARY_DIR}/bench_results")
    file(MAKE_DIRECTORY "${_out_dir}")

    add_test(
        NAME    "${TRB_NAME}"
        COMMAND $<TARGET_FILE:${TRB_TARGET}>
            "--benchmark_out=${_out_dir}/${TRB_TARGET}.json"
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
    endif()

    set_tests_properties("${TRB_NAME}" PROPERTIES
        LABELS  "${_labels}"
        TIMEOUT "${TRB_TIMEOUT}"
    )
endfunction()
