# CopyRuntimeDlls.cmake
# Build-time helper for syncing runtime DLLs into a test executable directory.

if(NOT DEFINED DST_DIR OR "${DST_DIR}" STREQUAL "")
    message(FATAL_ERROR "CopyRuntimeDlls.cmake requires DST_DIR")
endif()

file(MAKE_DIRECTORY "${DST_DIR}")

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

        get_filename_component(_dll_name "${_dll}" NAME)
        if(EXISTS "${DST_DIR}/${_dll_name}")
            file(REAL_PATH "${_dll}" _dll_real)
            file(REAL_PATH "${DST_DIR}/${_dll_name}" _dst_dll_real)
            if(_dll_real STREQUAL _dst_dll_real)
                message(STATUS "[CopyRuntimeDlls] Skip self-copy DLL: ${_dll}")
                continue()
            endif()
        endif()

        # Retry copy up to 5 times with exponential backoff to mitigate
        # transient file-locks on Windows (antivirus, parallel linker, etc.).
        set(_copy_result 1)
        set(_attempt_max 5)
        set(_sleep_secs 1)
        foreach(_attempt RANGE 1 ${_attempt_max})
            execute_process(
                COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${_dll}" "${DST_DIR}"
                RESULT_VARIABLE _copy_result
            )
            if(_copy_result EQUAL 0)
                break()
            endif()
            if(_attempt LESS ${_attempt_max})
                message(STATUS "[CopyRuntimeDlls] Copy failed (attempt ${_attempt}), retrying in ${_sleep_secs}s: ${_dll}")
                execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep ${_sleep_secs})
                math(EXPR _sleep_secs "${_sleep_secs} * 2")
            else()
                message(FATAL_ERROR "[CopyRuntimeDlls] Failed to copy DLL after ${_attempt_max} attempts: ${_dll}")
            endif()
        endforeach()
    endforeach()
endfunction()

_cleanup_zero_byte_themis_dlls("${DST_DIR}")

_copy_dlls_if_present("${BIN_DIR}")
_copy_dlls_if_present("${VCPKG_BIN_DIR}")
