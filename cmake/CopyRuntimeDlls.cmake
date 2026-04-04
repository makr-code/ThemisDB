# CopyRuntimeDlls.cmake
# Build-time helper for syncing runtime DLLs into a test executable directory.

if(NOT DEFINED DST_DIR OR "${DST_DIR}" STREQUAL "")
    message(FATAL_ERROR "CopyRuntimeDlls.cmake requires DST_DIR")
endif()

file(MAKE_DIRECTORY "${DST_DIR}")

function(_copy_dlls_if_present SRC_DIR)
    if(NOT SRC_DIR OR "${SRC_DIR}" STREQUAL "")
        return()
    endif()

    if(NOT EXISTS "${SRC_DIR}")
        message(STATUS "[CopyRuntimeDlls] Skip missing source directory: ${SRC_DIR}")
        return()
    endif()

    file(GLOB _dlls "${SRC_DIR}/*.dll")
    foreach(_dll IN LISTS _dlls)
        execute_process(
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${_dll}" "${DST_DIR}"
            RESULT_VARIABLE _copy_result
        )
        if(NOT _copy_result EQUAL 0)
            message(FATAL_ERROR "[CopyRuntimeDlls] Failed to copy DLL: ${_dll}")
        endif()
    endforeach()
endfunction()

_copy_dlls_if_present("${BIN_DIR}")
_copy_dlls_if_present("${VCPKG_BIN_DIR}")
