include_guard(GLOBAL)

include(CMakeParseArguments)

# Registers a CTest only if the referenced binary target exists.
# This avoids stale CTest entries when focused targets are conditionally built.
function(themis_register_test_target)
    set(options)
    set(oneValueArgs NAME TARGET WORKING_DIRECTORY TIMEOUT)
    set(multiValueArgs LABELS ENVIRONMENT ARGS)
    cmake_parse_arguments(TRT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT TRT_NAME)
        message(FATAL_ERROR "themis_register_test_target requires NAME")
    endif()

    if(NOT TRT_TARGET)
        message(FATAL_ERROR "themis_register_test_target requires TARGET")
    endif()

    if(NOT TARGET "${TRT_TARGET}")
        message(STATUS "[ctest-policy] Skipping '${TRT_NAME}' because target '${TRT_TARGET}' is not available")
        return()
    endif()

    add_test(
        NAME ${TRT_NAME}
        COMMAND $<TARGET_FILE:${TRT_TARGET}> ${TRT_ARGS}
    )

    if(TRT_WORKING_DIRECTORY)
        set_tests_properties(${TRT_NAME} PROPERTIES WORKING_DIRECTORY "${TRT_WORKING_DIRECTORY}")
    endif()

    if(TRT_TIMEOUT)
        set_tests_properties(${TRT_NAME} PROPERTIES TIMEOUT "${TRT_TIMEOUT}")
    endif()

    if(TRT_LABELS)
        set_tests_properties(${TRT_NAME} PROPERTIES LABELS "${TRT_LABELS}")
    endif()

    if(TRT_ENVIRONMENT)
        set_tests_properties(${TRT_NAME} PROPERTIES ENVIRONMENT "${TRT_ENVIRONMENT}")
    endif()
endfunction()

# Guards legacy root-level test registration blocks whose add_executable()
# command was intentionally removed during modularization. The helper keeps
# configure-time status explicit and prevents orphaned target commands from
# breaking CMake/CTest when a stale root registration is encountered.
function(themis_allow_orphaned_root_test_target OUT_VAR)
    set(options)
    set(oneValueArgs TARGET)
    cmake_parse_arguments(TAORT "${options}" "${oneValueArgs}" "" ${ARGN})

    if(NOT OUT_VAR)
        message(FATAL_ERROR "themis_allow_orphaned_root_test_target requires OUT_VAR")
    endif()

    if(NOT TAORT_TARGET)
        message(FATAL_ERROR "themis_allow_orphaned_root_test_target requires TARGET")
    endif()

    if(TARGET "${TAORT_TARGET}")
        set(${OUT_VAR} TRUE PARENT_SCOPE)
        return()
    endif()

    message(STATUS
        "[ctest-policy] Skipping orphaned root test registration for missing target "
        "'${TAORT_TARGET}'. Register the test from its module CMakeLists.txt or "
        "restore the executable target before adding CTest metadata."
    )
    set(${OUT_VAR} FALSE PARENT_SCOPE)
endfunction()

# Adds target sources only when the files exist. This is used for root-level
# re-include blocks that selectively restore tests previously excluded by broad
# glob filters. Missing files are reported as status messages instead of
# breaking CMake generation with stale paths.
function(themis_target_sources_if_exist TARGET VISIBILITY)
    if(NOT TARGET "${TARGET}")
        message(FATAL_ERROR "themis_target_sources_if_exist requires an existing TARGET")
    endif()

    if(NOT VISIBILITY)
        message(FATAL_ERROR "themis_target_sources_if_exist requires VISIBILITY")
    endif()

    set(_existing_sources)
    foreach(_source IN LISTS ARGN)
        if(EXISTS "${_source}")
            list(APPEND _existing_sources "${_source}")
        else()
            message(STATUS
                "[ctest-policy] Skipping missing source '${_source}' for target "
                "'${TARGET}'. Update the re-include path or remove the stale entry."
            )
        endif()
    endforeach()

    if(_existing_sources)
        target_sources(${TARGET} ${VISIBILITY} ${_existing_sources})
    endif()
endfunction()
