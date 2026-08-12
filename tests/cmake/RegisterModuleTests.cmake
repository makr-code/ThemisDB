include_guard(GLOBAL)

include(${CMAKE_CURRENT_LIST_DIR}/TestPolicy.cmake)

# Standardized module-level CTest registration.
# Adds mandatory labels to support modular presets and cleanup reporting.
function(themis_register_module_test)
    set(options)
    set(oneValueArgs MODULE NAME TARGET TIER KIND TIMEOUT WORKING_DIRECTORY)
    set(multiValueArgs LABELS ARGS ENVIRONMENT)
    cmake_parse_arguments(TRM "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT TRM_MODULE)
        message(FATAL_ERROR "themis_register_module_test requires MODULE")
    endif()

    if(NOT TRM_NAME)
        message(FATAL_ERROR "themis_register_module_test requires NAME")
    endif()

    if(NOT TRM_TARGET)
        message(FATAL_ERROR "themis_register_module_test requires TARGET")
    endif()

    if(NOT TRM_TIER)
        set(TRM_TIER "unit")
    endif()

    if(NOT TRM_KIND)
        set(TRM_KIND "standard")
    endif()

    set(_module_labels
        "module:${TRM_MODULE}"
        "tier:${TRM_TIER}"
        "kind:${TRM_KIND}"
    )

    if(TRM_LABELS)
        list(APPEND _module_labels ${TRM_LABELS})
        if("release_critical" IN_LIST TRM_LABELS)
            set_property(GLOBAL APPEND PROPERTY THEMIS_RELEASE_CRITICAL_TARGETS ${TRM_TARGET})
        endif()
    endif()

    themis_register_test_target(
        NAME ${TRM_NAME}
        TARGET ${TRM_TARGET}
        WORKING_DIRECTORY "${TRM_WORKING_DIRECTORY}"
        TIMEOUT "${TRM_TIMEOUT}"
        LABELS "${_module_labels}"
        ENVIRONMENT "${TRM_ENVIRONMENT}"
        ARGS ${TRM_ARGS}
    )
endfunction()

# Convenience helper for focused tests with a consistent label footprint.
function(themis_register_module_focused_test)
    themis_register_module_test(${ARGN} KIND focused LABELS focus)
endfunction()
