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
    INCLUDE
    LIB
    LIBPATH
)

set(_themis_vs_roots)
foreach(_root IN ITEMS
    "$ENV{ProgramFiles}"
    "$ENV{ProgramFiles\(x86\)}"
    "$ENV{ProgramW6432}"
    "${CMAKE_PROGRAM_FILES}"
)
    if(_root)
        list(APPEND _themis_vs_roots "${_root}")
    endif()
endforeach()
list(REMOVE_DUPLICATES _themis_vs_roots)

set(_candidates)
if(THEMIS_MSVC_SELECTION STREQUAL "vs2022")
    foreach(_root IN LISTS _themis_vs_roots)
        file(GLOB _glob_candidates
            "${_root}/Microsoft Visual Studio/2022/Professional/Common7/Tools/VsDevCmd.bat"
            "${_root}/Microsoft Visual Studio/2022/Community/Common7/Tools/VsDevCmd.bat"
            "${_root}/Microsoft Visual Studio/2022/Enterprise/Common7/Tools/VsDevCmd.bat"
            "${_root}/Microsoft Visual Studio/2022/BuildTools/Common7/Tools/VsDevCmd.bat"
            "${_root}/Microsoft Visual Studio/*/Common7/Tools/VsDevCmd.bat"
        )
        list(APPEND _candidates ${_glob_candidates})
    endforeach()
else()
    foreach(_root IN LISTS _themis_vs_roots)
        file(GLOB _glob_candidates
            "${_root}/Microsoft Visual Studio/18/Insiders/Common7/Tools/VsDevCmd.bat"
            "${_root}/Microsoft Visual Studio/*/Common7/Tools/VsDevCmd.bat"
        )
        list(APPEND _candidates ${_glob_candidates})
    endforeach()
endif()

list(REMOVE_DUPLICATES _candidates)

set(_vsdevcmd "")
foreach(_p IN LISTS _candidates)
    if(EXISTS "${_p}")
        set(_vsdevcmd "${_p}")
        break()
    endif()
endforeach()

if(NOT _vsdevcmd)
    message(STATUS "No VsDevCmd.bat found for selection ${THEMIS_MSVC_SELECTION}; attempting direct MSVC-SDK environment bootstrap")

    set(_msvc_root "")
    foreach(_root IN LISTS _themis_vs_roots)
        file(GLOB _vc_roots "${_root}/Microsoft Visual Studio/*/*/VC/Tools/MSVC/*")
        if(_vc_roots)
            list(APPEND _msvc_root ${_vc_roots})
        endif()
    endforeach()
    list(REMOVE_DUPLICATES _msvc_root)
    list(SORT _msvc_root ORDER DESCENDING)
    if(_msvc_root)
        list(GET _msvc_root 0 _msvc_root)
        set(ENV{VCToolsInstallDir} "${_msvc_root}")
    endif()

    set(_sdk_root "")
    foreach(_root IN LISTS _themis_vs_roots)
        if(EXISTS "${_root}/Windows Kits/10")
            set(_sdk_root "${_root}/Windows Kits/10")
            break()
        endif()
    endforeach()
    if(_sdk_root)
        set(ENV{WindowsSdkDir} "${_sdk_root}")
    endif()

    if(DEFINED ENV{VCToolsInstallDir} AND EXISTS "$ENV{VCToolsInstallDir}/lib/x64")
        set(_env_libs "$ENV{VCToolsInstallDir}/lib/x64")
    endif()
    if(DEFINED ENV{WindowsSdkDir})
        set(_sdk_version "")
        if(DEFINED ENV{WindowsSDKVersion})
            set(_sdk_version "$ENV{WindowsSDKVersion}")
        elseif(DEFINED ENV{WindowsSDKLibVersion})
            set(_sdk_version "$ENV{WindowsSDKLibVersion}")
        else()
            file(GLOB _sdk_versions "$ENV{WindowsSdkDir}/Include/*")
            foreach(_sdk IN LISTS _sdk_versions)
                get_filename_component(_sdk_name "${_sdk}" NAME)
                if(EXISTS "${_sdk}/ucrt" AND EXISTS "${_sdk}/shared" AND EXISTS "${_sdk}/um")
                    set(_sdk_version "${_sdk_name}")
                    break()
                endif()
            endforeach()
        endif()
        if(_sdk_version)
            string(REGEX REPLACE "[\\/]$" "" _sdk_version "${_sdk_version}")
            list(APPEND _env_libs "$ENV{WindowsSdkDir}/Lib/${_sdk_version}/ucrt/x64")
            list(APPEND _env_libs "$ENV{WindowsSdkDir}/Lib/${_sdk_version}/um/x64")
        endif()
    endif()

    if(_env_libs)
        list(REMOVE_DUPLICATES _env_libs)
        string(REPLACE ";" ";" _env_libs_str "${_env_libs}")
        set(ENV{LIB} "${_env_libs_str}")
        set(ENV{LIBPATH} "${_env_libs_str}")
        message(STATUS "Applied fallback MSVC SDK library paths to LIB/LIBPATH")
    endif()

    set(_required LIB INCLUDE LIBPATH WindowsSdkDir VCToolsInstallDir)
    set(_missing "")
    foreach(_r IN LISTS _required)
        if("$ENV{${_r}}" STREQUAL "")
            list(APPEND _missing "${_r}")
        endif()
    endforeach()

    if(_missing)
        message(STATUS "Fallback MSVC bootstrap did not populate all required vars; missing: ${_missing}")
    endif()

    return()
endif()

# Create a temporary bootstrap script to clear conflicting vars and capture `set`
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/themis_tmp")
set(_rand_seed 0)
if(DEFINED CMAKE_SYSTEM_PROCESS_ID)
    math(EXPR _rand_seed "${CMAKE_SYSTEM_PROCESS_ID} + 1")
endif()
if(_rand_seed EQUAL 0)
    set(_rand_seed 1)
endif()
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
    message(WARNING "MSVC developer environment incomplete after VsDevCmd initialization. Missing: ${_missing}. Applying direct fallback library paths for sub-builds.")

    set(_fallback_root "")
    foreach(_root IN ITEMS "$ENV{ProgramFiles}" "$ENV{ProgramFiles\(x86\)}" "$ENV{ProgramW6432}" "$ENV{ProgramFiles\(x86\)}")
        if(_root AND EXISTS "${_root}/Microsoft Visual Studio")
            set(_fallback_root "${_root}")
            break()
        endif()
    endforeach()

    if(_fallback_root)
        file(GLOB _fallback_vc_roots "${_fallback_root}/Microsoft Visual Studio/*/*/VC/Tools/MSVC/*")
        list(SORT _fallback_vc_roots ORDER DESCENDING)
        if(_fallback_vc_roots)
            list(GET _fallback_vc_roots 0 _fallback_vc_root)
            set(ENV{VCToolsInstallDir} "${_fallback_vc_root}")
        endif()
        if(EXISTS "${_fallback_root}/Windows Kits/10")
            set(ENV{WindowsSdkDir} "${_fallback_root}/Windows Kits/10")
        endif()
    endif()

    if(DEFINED ENV{VCToolsInstallDir} AND EXISTS "$ENV{VCToolsInstallDir}/lib/x64")
        set(ENV{LIB} "$ENV{VCToolsInstallDir}/lib/x64")
        if(DEFINED ENV{WindowsSdkDir})
            set(_sdk_version "$ENV{WindowsSDKVersion}")
            if(NOT _sdk_version)
                set(_sdk_version "$ENV{WindowsSDKLibVersion}")
            endif()
            if(NOT _sdk_version)
                file(GLOB _sdk_versions "$ENV{WindowsSdkDir}/Include/*")
                foreach(_sdk IN LISTS _sdk_versions)
                    get_filename_component(_sdk_name "${_sdk}" NAME)
                    if(EXISTS "${_sdk}/ucrt" AND EXISTS "${_sdk}/shared" AND EXISTS "${_sdk}/um")
                        set(_sdk_version "${_sdk_name}")
                        break()
                    endif()
                endforeach()
            endif()
            if(_sdk_version)
                string(REGEX REPLACE "[\\/]$" "" _sdk_version "${_sdk_version}")
                set(ENV{LIB} "$ENV{VCToolsInstallDir}/lib/x64;$ENV{WindowsSdkDir}/Lib/${_sdk_version}/ucrt/x64;$ENV{WindowsSdkDir}/Lib/${_sdk_version}/um/x64")
                set(ENV{LIBPATH} "$ENV{LIB}")
            endif()
        endif()
    endif()
endif()

message(STATUS "Applied Visual Studio environment from: ${_vsdevcmd}")
