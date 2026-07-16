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
