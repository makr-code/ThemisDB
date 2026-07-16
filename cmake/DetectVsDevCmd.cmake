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

set(THEMIS_MSVC_ENV_INIT_MODE "AUTO" CACHE STRING "MSVC env init mode (AUTO|ON|OFF)")
set_property(CACHE THEMIS_MSVC_ENV_INIT_MODE PROPERTY STRINGS AUTO ON OFF)

if(THEMIS_MSVC_ENV_INIT_MODE STREQUAL "OFF")
    message(STATUS "Themis: MSVC env initialization disabled (THEMIS_MSVC_ENV_INIT_MODE=OFF)")
    return()
endif()

# Only initialize VS env when MSVC is selected (or compiler is not explicitly set yet).
set(_compiler_hint "")
if(DEFINED CMAKE_CXX_COMPILER AND NOT "${CMAKE_CXX_COMPILER}" STREQUAL "")
    set(_compiler_hint "${CMAKE_CXX_COMPILER}")
elseif(DEFINED CMAKE_C_COMPILER AND NOT "${CMAKE_C_COMPILER}" STREQUAL "")
    set(_compiler_hint "${CMAKE_C_COMPILER}")
elseif(DEFINED ENV{CXX} AND NOT "$ENV{CXX}" STREQUAL "")
    set(_compiler_hint "$ENV{CXX}")
elseif(DEFINED ENV{CC} AND NOT "$ENV{CC}" STREQUAL "")
    set(_compiler_hint "$ENV{CC}")
endif()

set(_msvc_selected TRUE)
if(NOT "${_compiler_hint}" STREQUAL "")
    string(TOLOWER "${_compiler_hint}" _compiler_hint_lc)
    if(_compiler_hint_lc MATCHES "clang-cl|clang\\+\\+|clang|gcc|g\\+\\+")
        set(_msvc_selected FALSE)
    endif()
endif()

if(THEMIS_MSVC_ENV_INIT_MODE STREQUAL "AUTO" AND NOT _msvc_selected)
    message(STATUS "Themis: skipping VS env initialization (non-MSVC compiler selected: ${_compiler_hint})")
    return()
endif()

message(STATUS "Themis: MSVC selection = ${THEMIS_MSVC_SELECTION}")

function(_themis_capture_env _script_lines _out_var _res_var _err_var)
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/themis_tmp")
    string(RANDOM LENGTH 8 ALPHABET 0123456789abcdef _suffix)
    set(_bootstrap "${CMAKE_BINARY_DIR}/themis_tmp/capture_vs_env_${_suffix}.cmd")
    file(WRITE "${_bootstrap}" "${_script_lines}")

    execute_process(
        COMMAND cmd /d /c "${_bootstrap}"
        RESULT_VARIABLE _res
        OUTPUT_VARIABLE _out
        ERROR_VARIABLE _err
    )

    file(REMOVE "${_bootstrap}")
    string(REPLACE ";" "\\;" _out_escaped "${_out}")
    string(REPLACE ";" "\\;" _err_escaped "${_err}")
    set(${_out_var} "${_out_escaped}" PARENT_SCOPE)
    set(${_res_var} "${_res}" PARENT_SCOPE)
    set(${_err_var} "${_err_escaped}" PARENT_SCOPE)
endfunction()

set(_conflicting_vars
    LIB
    INCLUDE
    LIBPATH
    WindowsSdkDir
    UniversalCRTSdkDir
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

set(_roots "")

if(THEMIS_MSVC_SELECTION STREQUAL "vs2022")
    cmake_host_system_information(RESULT _pf QUERY WINDOWS_REGISTRY "HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion;ProgramFilesDir")
    cmake_host_system_information(RESULT _pf86 QUERY WINDOWS_REGISTRY "HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion;ProgramFilesDir (x86)")

    if(DEFINED ENV{ProgramW6432} AND NOT "$ENV{ProgramW6432}" STREQUAL "")
        list(APPEND _roots "$ENV{ProgramW6432}")
    endif()
    if(DEFINED ENV{ProgramFiles} AND NOT "$ENV{ProgramFiles}" STREQUAL "")
        list(APPEND _roots "$ENV{ProgramFiles}")
    endif()
    if(_pf)
        list(APPEND _roots "${_pf}")
    endif()
    if(_pf86)
        list(APPEND _roots "${_pf86}")
    endif()
    if(CMAKE_PROGRAM_FILES)
        list(APPEND _roots "${CMAKE_PROGRAM_FILES}")
    endif()

    list(REMOVE_DUPLICATES _roots)

    set(_candidates "")
    foreach(_root IN LISTS _roots)
        list(APPEND _candidates
            "${_root}/Microsoft Visual Studio/2022/Professional/Common7/Tools/VsDevCmd.bat"
            "${_root}/Microsoft Visual Studio/2022/Community/Common7/Tools/VsDevCmd.bat"
            "${_root}/Microsoft Visual Studio/2022/Enterprise/Common7/Tools/VsDevCmd.bat"
            "${_root}/Microsoft Visual Studio/2022/BuildTools/Common7/Tools/VsDevCmd.bat"
        )
    endforeach()
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
    set(_vswhere "")
    if(_pf86)
        set(_vswhere "${_pf86}/Microsoft Visual Studio/Installer/vswhere.exe")
    endif()
    if(NOT EXISTS "${_vswhere}" AND _pf)
        set(_vswhere "${_pf}/Microsoft Visual Studio/Installer/vswhere.exe")
    endif()

    if(EXISTS "${_vswhere}")
        execute_process(
            COMMAND "${_vswhere}" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
            RESULT_VARIABLE _vswhere_res
            OUTPUT_VARIABLE _vs_install_path
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(_vswhere_res EQUAL 0 AND NOT "${_vs_install_path}" STREQUAL "")
            set(_vswhere_candidate "${_vs_install_path}/Common7/Tools/VsDevCmd.bat")
            if(EXISTS "${_vswhere_candidate}")
                set(_vsdevcmd "${_vswhere_candidate}")
            endif()
        endif()
    endif()
endif()

set(_out "")
set(_res 1)
set(_err "")
set(_init_source "")

# Resolve vcvarsall path up-front (preferred over VsDevCmd for deterministic compiler env).
set(_vcvarsall "")
if(_vsdevcmd)
    get_filename_component(_vs_root "${_vsdevcmd}" DIRECTORY)  # .../Common7/Tools
    get_filename_component(_vs_root "${_vs_root}" DIRECTORY)   # .../Common7
    get_filename_component(_vs_root "${_vs_root}" DIRECTORY)   # .../<Edition>
    set(_candidate "${_vs_root}/VC/Auxiliary/Build/vcvarsall.bat")
    if(EXISTS "${_candidate}")
        set(_vcvarsall "${_candidate}")
    endif()
endif()

if(NOT _vcvarsall)
    foreach(_root IN LISTS _roots)
        foreach(_edition IN ITEMS Professional Community Enterprise BuildTools)
            set(_candidate "${_root}/Microsoft Visual Studio/2022/${_edition}/VC/Auxiliary/Build/vcvarsall.bat")
            if(EXISTS "${_candidate}")
                set(_vcvarsall "${_candidate}")
                break()
            endif()
        endforeach()
        if(_vcvarsall)
            break()
        endif()
    endforeach()
endif()

if(_vcvarsall)
    # Preferred: vcvarsall produces deterministic compiler/linker env for cl.exe.
    set(_script "@echo off\r\n")
    string(APPEND _script "setlocal EnableDelayedExpansion\r\n")
    foreach(_v IN LISTS _conflicting_vars)
        string(APPEND _script "set ${_v}=\r\n")
    endforeach()
    string(APPEND _script "call \"${_vcvarsall}\" x64 >nul\r\n")
    string(APPEND _script "if errorlevel 1 exit /b 1\r\n")
    string(APPEND _script "echo THEMIS_ENV_LIB=!LIB!\r\n")
    string(APPEND _script "echo THEMIS_ENV_INCLUDE=!INCLUDE!\r\n")
    string(APPEND _script "echo THEMIS_ENV_LIBPATH=!LIBPATH!\r\n")
    string(APPEND _script "echo THEMIS_ENV_WINDOWSSDKDIR=!WindowsSdkDir!\r\n")
    string(APPEND _script "echo THEMIS_ENV_VCTOOLSINSTALLDIR=!VCToolsInstallDir!\r\n")
    string(APPEND _script "exit /b 0\r\n")
    _themis_capture_env("${_script}" _out _res _err)
    if(_res EQUAL 0)
        set(_init_source "${_vcvarsall}")
        message(STATUS "Applied Visual Studio environment via vcvarsall: ${_vcvarsall}")
    endif()
endif()

if((NOT _res EQUAL 0) AND _vsdevcmd)
    # Fallback: VsDevCmd.
        set(_script "@echo off\r\n")
        string(APPEND _script "setlocal EnableDelayedExpansion\r\n")
        foreach(_v IN LISTS _conflicting_vars)
            string(APPEND _script "set ${_v}=\r\n")
        endforeach()
        string(APPEND _script "call \"${_vsdevcmd}\" -arch=x64 >nul\r\n")
        string(APPEND _script "if errorlevel 1 exit /b 1\r\n")
        string(APPEND _script "echo THEMIS_ENV_LIB=!LIB!\r\n")
        string(APPEND _script "echo THEMIS_ENV_INCLUDE=!INCLUDE!\r\n")
        string(APPEND _script "echo THEMIS_ENV_LIBPATH=!LIBPATH!\r\n")
        string(APPEND _script "echo THEMIS_ENV_WINDOWSSDKDIR=!WindowsSdkDir!\r\n")
        string(APPEND _script "echo THEMIS_ENV_VCTOOLSINSTALLDIR=!VCToolsInstallDir!\r\n")
        string(APPEND _script "exit /b 0\r\n")
        _themis_capture_env("${_script}" _out _res _err)
        if(_res EQUAL 0)
            set(_init_source "${_vsdevcmd}")
            message(STATUS "Applied Visual Studio environment via VsDevCmd fallback: ${_vsdevcmd}")
        endif()
endif()

if(NOT _res EQUAL 0)
    message(WARNING "Could not initialize Visual Studio environment automatically; continuing with current process environment. Details: ${_err}")
    return()
endif()

# Normalize line endings and extract captured values line-safe.
# Important: do not use greedy multiline matching here, otherwise LIB may
# accidentally include subsequent THEMIS_ENV_* lines.
string(REPLACE "\r\n" "\n" _out_norm "${_out}")
string(REPLACE "\r" "\n" _out_norm "${_out_norm}")

set(_captured_LIB "")
set(_captured_INCLUDE "")
set(_captured_LIBPATH "")
set(_captured_WINDOWSSDKDIR "")
set(_captured_VCTOOLSINSTALLDIR "")

if(_out_norm MATCHES "(^|\n)THEMIS_ENV_LIB=([^\n]*)")
    set(_captured_LIB "${CMAKE_MATCH_2}")
endif()
if(_out_norm MATCHES "(^|\n)THEMIS_ENV_INCLUDE=([^\n]*)")
    set(_captured_INCLUDE "${CMAKE_MATCH_2}")
endif()
if(_out_norm MATCHES "(^|\n)THEMIS_ENV_LIBPATH=([^\n]*)")
    set(_captured_LIBPATH "${CMAKE_MATCH_2}")
endif()
if(_out_norm MATCHES "(^|\n)THEMIS_ENV_WINDOWSSDKDIR=([^\n]*)")
    set(_captured_WINDOWSSDKDIR "${CMAKE_MATCH_2}")
endif()
if(_out_norm MATCHES "(^|\n)THEMIS_ENV_VCTOOLSINSTALLDIR=([^\n]*)")
    set(_captured_VCTOOLSINSTALLDIR "${CMAKE_MATCH_2}")
endif()

set(_capture_complete TRUE)
if("${_captured_LIB}" STREQUAL "")
    set(_capture_complete FALSE)
endif()
if("${_captured_INCLUDE}" STREQUAL "")
    set(_capture_complete FALSE)
endif()
if("${_captured_LIBPATH}" STREQUAL "")
    set(_capture_complete FALSE)
endif()

if(_capture_complete)
    string(REPLACE "\\;" ";" _captured_LIB "${_captured_LIB}")
    string(REPLACE "\\;" ";" _captured_INCLUDE "${_captured_INCLUDE}")
    string(REPLACE "\\;" ";" _captured_LIBPATH "${_captured_LIBPATH}")
    
    # Store as CMake variables (safer than env vars for sub-projects)
    # These are NOT set in ENV to prevent ExternalProject from writing
    # unescaped Windows paths into cache files (which causes CMake syntax errors)
    set(THEMIS_MSVC_LIB "${_captured_LIB}" CACHE STRING "MSVC library paths" FORCE)
    set(THEMIS_MSVC_INCLUDE "${_captured_INCLUDE}" CACHE STRING "MSVC include paths" FORCE)
    set(THEMIS_MSVC_LIBPATH "${_captured_LIBPATH}" CACHE STRING "MSVC library paths (LIBPATH)" FORCE)
    set(THEMIS_WINDOWS_SDK_DIR "${_captured_WINDOWSSDKDIR}" CACHE STRING "Windows SDK directory" FORCE)
    set(THEMIS_VC_TOOLS_INSTALL_DIR "${_captured_VCTOOLSINSTALLDIR}" CACHE STRING "VC Tools install directory" FORCE)
    
    # ALSO set in ENV for processes that need them (e.g., linker)

    set(ENV{LIB} "${_captured_LIB}")
    set(ENV{INCLUDE} "${_captured_INCLUDE}")
    set(ENV{LIBPATH} "${_captured_LIBPATH}")

    if(NOT "${_captured_WINDOWSSDKDIR}" STREQUAL "")
        set(ENV{WindowsSdkDir} "${_captured_WINDOWSSDKDIR}")
    endif()
    if(NOT "${_captured_VCTOOLSINSTALLDIR}" STREQUAL "")
        set(ENV{VCToolsInstallDir} "${_captured_VCTOOLSINSTALLDIR}")
    endif()
endif()

# Validate required variables
set(_required LIB INCLUDE LIBPATH)
set(_missing "")
foreach(_r IN LISTS _required)
    if("$ENV{${_r}}" STREQUAL "")
        list(APPEND _missing "${_r}")
    endif()
endforeach()

if(_missing)
    # Deterministic fallback: synthesize core MSVC/Windows SDK paths when capture is partial.
    # This keeps all Windows+MSVC presets operational even if batch env capture is flaky.
    set(_fallback_vs_root "")
    if(DEFINED _vcvarsall AND NOT "${_vcvarsall}" STREQUAL "")
        get_filename_component(_fallback_vs_root "${_vcvarsall}" DIRECTORY) # .../VC/Auxiliary/Build
        get_filename_component(_fallback_vs_root "${_fallback_vs_root}" DIRECTORY) # .../VC/Auxiliary
        get_filename_component(_fallback_vs_root "${_fallback_vs_root}" DIRECTORY) # .../VC
        get_filename_component(_fallback_vs_root "${_fallback_vs_root}" DIRECTORY) # .../<Edition>
    elseif(DEFINED _vsdevcmd AND NOT "${_vsdevcmd}" STREQUAL "")
        get_filename_component(_fallback_vs_root "${_vsdevcmd}" DIRECTORY) # .../Common7/Tools
        get_filename_component(_fallback_vs_root "${_fallback_vs_root}" DIRECTORY) # .../Common7
        get_filename_component(_fallback_vs_root "${_fallback_vs_root}" DIRECTORY) # .../<Edition>
    endif()

    set(_fallback_msvc_tools "")
    if(NOT "${_fallback_vs_root}" STREQUAL "")
        file(GLOB _msvc_tool_versions "${_fallback_vs_root}/VC/Tools/MSVC/*")
        if(_msvc_tool_versions)
            list(SORT _msvc_tool_versions)
            list(GET _msvc_tool_versions -1 _fallback_msvc_tools)
        endif()
    endif()

    set(_fallback_kits_root "")
    if(DEFINED _pf86 AND NOT "${_pf86}" STREQUAL "")
        set(_fallback_kits_root "${_pf86}/Windows Kits/10")
    elseif(DEFINED _pf AND NOT "${_pf}" STREQUAL "")
        set(_fallback_kits_root "${_pf}/Windows Kits/10")
    endif()

    # Additional fallbacks when registry lookups are unavailable.
    if("${_fallback_kits_root}" STREQUAL "")
        if(DEFINED ENV{WindowsSdkDir} AND NOT "$ENV{WindowsSdkDir}" STREQUAL "")
            set(_fallback_kits_root "$ENV{WindowsSdkDir}")
        elseif(EXISTS "C:/Program Files (x86)/Windows Kits/10")
            set(_fallback_kits_root "C:/Program Files (x86)/Windows Kits/10")
        elseif(EXISTS "C:/Program Files/Windows Kits/10")
            set(_fallback_kits_root "C:/Program Files/Windows Kits/10")
        endif()
    endif()

    # Normalize trailing slash for stable path joins.
    if(NOT "${_fallback_kits_root}" STREQUAL "")
        string(REGEX REPLACE "[\\/]+$" "" _fallback_kits_root "${_fallback_kits_root}")
    endif()

    set(_fallback_sdk_ver "")
    if(NOT "${_fallback_kits_root}" STREQUAL "")
        file(GLOB _sdk_lib_versions "${_fallback_kits_root}/lib/*")
        if(_sdk_lib_versions)
            list(SORT _sdk_lib_versions)
            list(GET _sdk_lib_versions -1 _fallback_sdk_ver)
            get_filename_component(_fallback_sdk_ver "${_fallback_sdk_ver}" NAME)
        endif()
    endif()

    if("${_captured_LIB}" STREQUAL "" AND NOT "${_fallback_msvc_tools}" STREQUAL "" AND NOT "${_fallback_kits_root}" STREQUAL "" AND NOT "${_fallback_sdk_ver}" STREQUAL "")
        set(_captured_LIB
            "${_fallback_msvc_tools}/ATLMFC/lib/x64;${_fallback_msvc_tools}/lib/x64;${_fallback_kits_root}/lib/${_fallback_sdk_ver}/ucrt/x64;${_fallback_kits_root}/lib/${_fallback_sdk_ver}/um/x64")
    endif()

    if("${_captured_INCLUDE}" STREQUAL "" AND NOT "${_fallback_msvc_tools}" STREQUAL "" AND NOT "${_fallback_kits_root}" STREQUAL "" AND NOT "${_fallback_sdk_ver}" STREQUAL "")
        set(_captured_INCLUDE
            "${_fallback_msvc_tools}/include;${_fallback_msvc_tools}/ATLMFC/include;${_fallback_vs_root}/VC/Auxiliary/VS/include;${_fallback_kits_root}/include/${_fallback_sdk_ver}/ucrt;${_fallback_kits_root}/include/${_fallback_sdk_ver}/um;${_fallback_kits_root}/include/${_fallback_sdk_ver}/shared;${_fallback_kits_root}/include/${_fallback_sdk_ver}/winrt;${_fallback_kits_root}/include/${_fallback_sdk_ver}/cppwinrt")
    endif()

    if("${_captured_LIBPATH}" STREQUAL "" AND NOT "${_fallback_msvc_tools}" STREQUAL "" AND NOT "${_fallback_kits_root}" STREQUAL "" AND NOT "${_fallback_sdk_ver}" STREQUAL "")
        set(_captured_LIBPATH
            "${_fallback_msvc_tools}/ATLMFC/lib/x64;${_fallback_msvc_tools}/lib/x64;${_fallback_msvc_tools}/lib/x86/store/references;${_fallback_kits_root}/UnionMetadata/${_fallback_sdk_ver};${_fallback_kits_root}/References/${_fallback_sdk_ver}")
    endif()

    if("${_captured_WINDOWSSDKDIR}" STREQUAL "" AND NOT "${_fallback_kits_root}" STREQUAL "")
        set(_captured_WINDOWSSDKDIR "${_fallback_kits_root}/")
    endif()

    if("${_captured_VCTOOLSINSTALLDIR}" STREQUAL "" AND NOT "${_fallback_msvc_tools}" STREQUAL "")
        set(_captured_VCTOOLSINSTALLDIR "${_fallback_msvc_tools}/")
    endif()

    # Apply synthesized values (or captured ones) to environment, then re-check.
    if(NOT "${_captured_LIB}" STREQUAL "")
        string(REPLACE "\\;" ";" _captured_LIB "${_captured_LIB}")
        set(ENV{LIB} "${_captured_LIB}")
    endif()
    if(NOT "${_captured_INCLUDE}" STREQUAL "")
        string(REPLACE "\\;" ";" _captured_INCLUDE "${_captured_INCLUDE}")
        set(ENV{INCLUDE} "${_captured_INCLUDE}")
    endif()
    if(NOT "${_captured_LIBPATH}" STREQUAL "")
        string(REPLACE "\\;" ";" _captured_LIBPATH "${_captured_LIBPATH}")
        set(ENV{LIBPATH} "${_captured_LIBPATH}")
    endif()
    if(NOT "${_captured_WINDOWSSDKDIR}" STREQUAL "")
        set(ENV{WindowsSdkDir} "${_captured_WINDOWSSDKDIR}")
    endif()
    if(NOT "${_captured_VCTOOLSINSTALLDIR}" STREQUAL "")
        set(ENV{VCToolsInstallDir} "${_captured_VCTOOLSINSTALLDIR}")
    endif()

    # Recompute missing after fallback application.
    set(_missing "")
    foreach(_r IN LISTS _required)
        if("$ENV{${_r}}" STREQUAL "")
            list(APPEND _missing "${_r}")
        endif()
    endforeach()

    string(LENGTH "${_captured_LIB}" _len_lib)
    string(LENGTH "${_captured_INCLUDE}" _len_include)
    string(LENGTH "${_captured_LIBPATH}" _len_libpath)
    string(LENGTH "${_captured_WINDOWSSDKDIR}" _len_windowssdk)
    string(LENGTH "${_captured_VCTOOLSINSTALLDIR}" _len_vctools)
    if(THEMIS_MSVC_ENV_INIT_MODE STREQUAL "ON")
        message(FATAL_ERROR "MSVC developer environment is incomplete after auto-initialization (missing: ${_missing}). Captured lengths: LIB=${_len_lib}, INCLUDE=${_len_include}, LIBPATH=${_len_libpath}, WindowsSdkDir=${_len_windowssdk}, VCToolsInstallDir=${_len_vctools}.")
    endif()
    if(_missing)
        message(WARNING "MSVC developer environment appears incomplete after auto-initialization (missing: ${_missing}). Captured lengths: LIB=${_len_lib}, INCLUDE=${_len_include}, LIBPATH=${_len_libpath}, WindowsSdkDir=${_len_windowssdk}, VCToolsInstallDir=${_len_vctools}. Fallback context: VS_ROOT='${_fallback_vs_root}', MSVC_TOOLS='${_fallback_msvc_tools}', KITS_ROOT='${_fallback_kits_root}', SDK_VER='${_fallback_sdk_ver}'. Continuing with current process environment.")
        return()
    endif()

    message(STATUS "MSVC environment fallback synthesis applied successfully.")
endif()

if(NOT "${_init_source}" STREQUAL "")
    message(STATUS "Applied Visual Studio environment from: ${_init_source}")
else()
    message(STATUS "Applied Visual Studio environment")
endif()
