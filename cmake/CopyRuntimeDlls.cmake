# CopyRuntimeDlls.cmake
# Build-time helper for syncing runtime DLLs into a target executable directory.
#
# The hardened logic resolves the actual import graph for the executable when
# TARGET_FILE is given and falls back to broad bin/vcpkg directory syncing when
# it is not. This avoids stale dependency copies and catches transitive DLLs that
# are not manually listed in the build tree.

if(NOT DEFINED DST_DIR OR "${DST_DIR}" STREQUAL "")
    message(FATAL_ERROR "CopyRuntimeDlls.cmake requires DST_DIR")
endif()

file(MAKE_DIRECTORY "${DST_DIR}")

function(_copy_single_file_if_needed SOURCE_PATH DEST_DIR)
    if(NOT SOURCE_PATH OR "${SOURCE_PATH}" STREQUAL "")
        return()
    endif()

    if(NOT EXISTS "${SOURCE_PATH}")
        return()
    endif()

    get_filename_component(_copy_name "${SOURCE_PATH}" NAME)
    set(_dest_path "${DEST_DIR}/${_copy_name}")

    if(EXISTS "${_dest_path}")
        file(REAL_PATH "${SOURCE_PATH}" _src_real)
        file(REAL_PATH "${_dest_path}" _dst_real)
        if(_src_real STREQUAL _dst_real)
            return()
        endif()
    endif()

    set(_copy_result 1)
    set(_attempt_max 5)
    set(_sleep_secs 1)
    foreach(_attempt RANGE 1 ${_attempt_max})
        execute_process(
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${SOURCE_PATH}" "${DEST_DIR}"
            RESULT_VARIABLE _copy_result
        )
        if(_copy_result EQUAL 0)
            return()
        endif()
        if(_attempt LESS ${_attempt_max})
            execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep ${_sleep_secs})
            math(EXPR _sleep_secs "${_sleep_secs} * 2")
        endif()
    endforeach()

    message(FATAL_ERROR "[CopyRuntimeDlls] Failed to copy dependency after ${_attempt_max} attempts: ${SOURCE_PATH}")
endfunction()

function(_cleanup_zero_byte_themis_dlls TARGET_DIR)
    if(NOT TARGET_DIR OR "${TARGET_DIR}" STREQUAL "")
        return()
    endif()

    if(NOT EXISTS "${TARGET_DIR}")
        return()
    endif()

    file(GLOB _target_dlls "${TARGET_DIR}/themis*.dll")
    foreach(_dll IN LISTS _target_dlls)
        file(SIZE "${_dll}" _dll_size)
        if(_dll_size EQUAL 0)
            file(REMOVE "${_dll}")
            message(STATUS "[CopyRuntimeDlls] Removed zero-byte DLL: ${_dll}")
        endif()
    endforeach()
endfunction()

function(_copy_dlls_if_present SRC_DIR)
    if(NOT SRC_DIR OR "${SRC_DIR}" STREQUAL "")
        return()
    endif()

    if(NOT EXISTS "${SRC_DIR}")
        message(STATUS "[CopyRuntimeDlls] Skip missing source directory: ${SRC_DIR}")
        return()
    endif()

    file(REAL_PATH "${SRC_DIR}" _src_real)
    file(REAL_PATH "${DST_DIR}" _dst_real)
    if(_src_real STREQUAL _dst_real)
        message(STATUS "[CopyRuntimeDlls] Skip self-copy directory: ${SRC_DIR}")
        return()
    endif()

    file(GLOB _dlls "${SRC_DIR}/*.dll")
    foreach(_dll IN LISTS _dlls)
        file(SIZE "${_dll}" _dll_size)
        if(_dll_size EQUAL 0)
            message(WARNING "[CopyRuntimeDlls] Skip zero-byte DLL: ${_dll}")
            continue()
        endif()
        _copy_single_file_if_needed("${_dll}" "${DST_DIR}")
    endforeach()
endfunction()

_cleanup_zero_byte_themis_dlls("${DST_DIR}")

if(DEFINED TARGET_FILE AND NOT "${TARGET_FILE}" STREQUAL "")
    if(NOT EXISTS "${TARGET_FILE}")
        message(FATAL_ERROR "[CopyRuntimeDlls] TARGET_FILE does not exist: ${TARGET_FILE}")
    endif()

    set(_dep_dirs)
    foreach(_dir IN LISTS BIN_DIR VCPKG_BIN_DIR)
        if(_dir AND EXISTS "${_dir}")
            list(APPEND _dep_dirs "${_dir}")
        endif()
    endforeach()

    if(NOT _dep_dirs)
        set(_dep_dirs "${CMAKE_BINARY_DIR}/bin")
    endif()

    list(REMOVE_DUPLICATES _dep_dirs)

    file(GET_RUNTIME_DEPENDENCIES
        EXECUTABLES "${TARGET_FILE}"
        RESOLVED_DEPENDENCIES_VAR _resolved_deps
        UNRESOLVED_DEPENDENCIES_VAR _unresolved_deps
        DIRECTORIES ${_dep_dirs}
        POST_EXCLUDE_REGEXES "^api-ms-win-.*" "^ext-ms-win-.*"
    )

    foreach(_dep IN LISTS _resolved_deps)
        if(EXISTS "${_dep}")
            _copy_single_file_if_needed("${_dep}" "${DST_DIR}")
        endif()
    endforeach()

    foreach(_dep IN LISTS _unresolved_deps)
        message(WARNING "[CopyRuntimeDlls] Unresolved runtime dependency for ${TARGET_FILE}: ${_dep}")
    endforeach()

    return()
endif()

_copy_dlls_if_present("${BIN_DIR}")
_copy_dlls_if_present("${VCPKG_BIN_DIR}")
