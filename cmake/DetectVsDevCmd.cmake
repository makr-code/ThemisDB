## DetectVsDevCmd.cmake
# Initialize a selected Visual Studio Developer environment inside CMake configure
# This runs the chosen VsDevCmd.bat in a temporary cmd session, captures the
# environment block and applies it to the CMake process (so subsequent compiler
# discovery and configure steps use the same MSVC toolset). No external wrapper
# scripts required; behavior is controlled by the cache variable
# `THEMIS_MSVC_SELECTION` (defaults to `vs2022`).

if(NOT WIN32)
    return()
endif()

if(NOT DEFINED THEMIS_MSVC_SELECTION)
    set(THEMIS_MSVC_SELECTION "vs2022" CACHE STRING "Visual Studio to initialize (vs2022|vs2026-insiders)")
endif()

message(STATUS "Themis: MSVC selection = ${THEMIS_MSVC_SELECTION}")

set(_conflicting_vars
    VSINSTALLDIR
    VCToolsVersion
    VCToolsInstallDir
    VisualStudioVersion
    DevEnvDir
    VS160COMNTOOLS
    VS170COMNTOOLS
    VS180COMNTOOLS
    VSCMD_VER
)

if(THEMIS_MSVC_SELECTION STREQUAL "vs2022")
    set(_candidates
        "${CMAKE_PROGRAM_FILES}/Microsoft Visual Studio/2022/Professional/Common7/Tools/VsDevCmd.bat"
        "${CMAKE_PROGRAM_FILES}/Microsoft Visual Studio/2022/Community/Common7/Tools/VsDevCmd.bat"
        "${CMAKE_PROGRAM_FILES}/Microsoft Visual Studio/2022/Enterprise/Common7/Tools/VsDevCmd.bat"
        "${CMAKE_PROGRAM_FILES}/Microsoft Visual Studio/2022/BuildTools/Common7/Tools/VsDevCmd.bat"
    )
else()
    # allow other named selections (e.g. vs2026-insiders)
    set(_candidates
        "${CMAKE_PROGRAM_FILES}/Microsoft Visual Studio/18/Insiders/Common7/Tools/VsDevCmd.bat"
    )
endif()

set(_vsdevcmd "")
foreach(_p IN LISTS _candidates)
    if(EXISTS "${_p}")
        set(_vsdevcmd "${_p}")
        break()
    endif()
endforeach()

if(NOT _vsdevcmd)
    message(STATUS "No VsDevCmd.bat found for selection ${THEMIS_MSVC_SELECTION}; skipping MSVC env auto-initialization")
    return()
endif()

# Create a temporary bootstrap script to clear conflicting vars and capture `set`
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/themis_tmp")
math(EXPR _rand_seed "${CMAKE_SYSTEM_PROCESS_ID} + ${CMAKE_CACHE_MAJOR_VERSION}")
set(_bootstrap "${CMAKE_BINARY_DIR}/themis_tmp/themis-vsdev-${_rand_seed}.cmd")

set(_lines "@echo off\r\n")
foreach(_v IN LISTS _conflicting_vars)
    list(APPEND _lines "set ${_v}=")
endforeach()
list(APPEND _lines "call \"${_vsdevcmd}\" -arch=x64")
list(APPEND _lines "set")

file(WRITE "${_bootstrap}" "${_lines}")

execute_process(
    COMMAND cmd /c "${_bootstrap}"
    RESULT_VARIABLE _res
    OUTPUT_VARIABLE _out
    ERROR_VARIABLE _err
)

file(REMOVE "${_bootstrap}")

if(NOT _res EQUAL 0)
    message(FATAL_ERROR "Failed to execute VsDevCmd bootstrap: ${_err}")
endif()

# Normalize line endings and split
string(REPLACE "\r\n" "\n" _out_norm "${_out}")
string(REPLACE "\r" "\n" _out_norm "${_out_norm}")
string(REGEX MATCHALL ".*" _lines_list "${_out_norm}")

foreach(_line IN LISTS _lines_list)
    if(_line MATCHES "^([A-Za-z_][A-Za-z0-9_]*)=(.*)$")
        string(REGEX REPLACE "^([A-Za-z_][A-Za-z0-9_]*)=(.*)$" "\\1" _name "${_line}")
        string(REGEX REPLACE "^([A-Za-z_][A-Za-z0-9_]*)=(.*)$" "\\2" _value "${_line}")
        # Trim possible surrounding quotes
        string(REGEX REPLACE "^\"(.*)\"$" "\\1" _value "${_value}")
        # Apply to current CMake process environment
        if(NOT _name STREQUAL "PROMPT" AND NOT _name STREQUAL "ERRORLEVEL")
            set(ENV{${_name}} "${_value}")
        endif()
    endif()
endforeach()

# Validate required variables
set(_required LIB INCLUDE LIBPATH WindowsSdkDir VCToolsInstallDir)
set(_missing "")
foreach(_r IN LISTS _required)
    if("$ENV{${_r}}" STREQUAL "")
        list(APPEND _missing "${_r}")
    endif()
endforeach()

if(_missing)
    message(FATAL_ERROR "MSVC developer environment incomplete after VsDevCmd initialization. Missing: ${_missing}" )
endif()

message(STATUS "Applied Visual Studio environment from: ${_vsdevcmd}")
