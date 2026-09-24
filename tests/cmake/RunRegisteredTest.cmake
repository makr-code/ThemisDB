# Runs a registered CTest target and auto-builds it on demand when missing.
# Inputs:
# - TARGET_NAME: CMake target name to build when executable is missing
# - TARGET_FILE: expected executable path
# - BUILD_DIR: CMake build directory
# - BUILD_CONFIG: optional multi-config name (Debug/Release/...)
# - TEST_ARGS: '|' separated list of command-line args

if(NOT DEFINED TARGET_NAME OR "${TARGET_NAME}" STREQUAL "")
    message(FATAL_ERROR "RunRegisteredTest.cmake: TARGET_NAME is required")
endif()

if(NOT DEFINED TARGET_FILE OR "${TARGET_FILE}" STREQUAL "")
    message(FATAL_ERROR "RunRegisteredTest.cmake: TARGET_FILE is required")
endif()

if(NOT DEFINED BUILD_DIR OR "${BUILD_DIR}" STREQUAL "")
    message(FATAL_ERROR "RunRegisteredTest.cmake: BUILD_DIR is required")
endif()

set(_argv)
if(DEFINED TEST_ARGS AND NOT "${TEST_ARGS}" STREQUAL "")
    string(REPLACE "|" ";" _argv "${TEST_ARGS}")
endif()

if(WIN32)
    set(_fallback_include_paths)
    set(_fallback_library_paths)

    set(_themis_host_cxx_compiler "")
    if(DEFINED HOST_CXX_COMPILER AND EXISTS "${HOST_CXX_COMPILER}")
        set(_themis_host_cxx_compiler "${HOST_CXX_COMPILER}")
    else()
        find_program(_themis_cl_exe cl)
        if(_themis_cl_exe AND EXISTS "${_themis_cl_exe}")
            set(_themis_host_cxx_compiler "${_themis_cl_exe}")
        endif()
    endif()

    if(_themis_host_cxx_compiler)
        get_filename_component(_host_bin_dir "${_themis_host_cxx_compiler}" DIRECTORY)
        get_filename_component(_host_bin_host_dir "${_host_bin_dir}" DIRECTORY)
        get_filename_component(_host_bin_root_dir "${_host_bin_host_dir}" DIRECTORY)
        get_filename_component(_host_msvc_root_dir "${_host_bin_root_dir}" DIRECTORY)

        if(EXISTS "${_host_msvc_root_dir}/include")
            list(APPEND _fallback_include_paths "${_host_msvc_root_dir}/include")
        endif()
        if(EXISTS "${_host_msvc_root_dir}/lib/x64")
            list(APPEND _fallback_library_paths "${_host_msvc_root_dir}/lib/x64")
        endif()
    endif()

    if(NOT _fallback_include_paths AND DEFINED ENV{VCToolsInstallDir} AND EXISTS "$ENV{VCToolsInstallDir}/include")
        list(APPEND _fallback_include_paths "$ENV{VCToolsInstallDir}/include")
    endif()
    if(NOT _fallback_library_paths AND DEFINED ENV{VCToolsInstallDir} AND EXISTS "$ENV{VCToolsInstallDir}/lib/x64")
        list(APPEND _fallback_library_paths "$ENV{VCToolsInstallDir}/lib/x64")
    endif()

    if((NOT _fallback_include_paths OR NOT _fallback_library_paths) AND DEFINED ENV{VSINSTALLDIR} AND EXISTS "$ENV{VSINSTALLDIR}/VC/Tools/MSVC")
        file(GLOB _themis_vs_msvc_versions RELATIVE "$ENV{VSINSTALLDIR}/VC/Tools/MSVC" "$ENV{VSINSTALLDIR}/VC/Tools/MSVC/*")
        list(SORT _themis_vs_msvc_versions COMPARE NATURAL ORDER DESCENDING)
        foreach(_themis_vs_msvc_ver IN LISTS _themis_vs_msvc_versions)
            if(NOT _fallback_include_paths AND EXISTS "$ENV{VSINSTALLDIR}/VC/Tools/MSVC/${_themis_vs_msvc_ver}/include")
                list(APPEND _fallback_include_paths "$ENV{VSINSTALLDIR}/VC/Tools/MSVC/${_themis_vs_msvc_ver}/include")
            endif()
            if(NOT _fallback_library_paths AND EXISTS "$ENV{VSINSTALLDIR}/VC/Tools/MSVC/${_themis_vs_msvc_ver}/lib/x64")
                list(APPEND _fallback_library_paths "$ENV{VSINSTALLDIR}/VC/Tools/MSVC/${_themis_vs_msvc_ver}/lib/x64")
            endif()
            if(_fallback_include_paths AND _fallback_library_paths)
                break()
            endif()
        endforeach()
    endif()

    set(_sdk_root "C:/Program Files (x86)/Windows Kits/10")
    if(EXISTS "${_sdk_root}/Include")
        file(GLOB _sdk_versions RELATIVE "${_sdk_root}/Include" "${_sdk_root}/Include/*")
        list(SORT _sdk_versions COMPARE NATURAL ORDER DESCENDING)
        foreach(_ver IN LISTS _sdk_versions)
            if(EXISTS "${_sdk_root}/Include/${_ver}/ucrt" AND EXISTS "${_sdk_root}/Include/${_ver}/um")
                foreach(_inc_sub IN ITEMS ucrt um shared winrt cppwinrt)
                    if(EXISTS "${_sdk_root}/Include/${_ver}/${_inc_sub}")
                        list(APPEND _fallback_include_paths "${_sdk_root}/Include/${_ver}/${_inc_sub}")
                    endif()
                endforeach()

                foreach(_lib_sub IN ITEMS ucrt um)
                    if(EXISTS "${_sdk_root}/Lib/${_ver}/${_lib_sub}/x64")
                        list(APPEND _fallback_library_paths "${_sdk_root}/Lib/${_ver}/${_lib_sub}/x64")
                    endif()
                endforeach()
                break()
            endif()
        endforeach()
    endif()

    if(_fallback_include_paths)
        list(REMOVE_DUPLICATES _fallback_include_paths)
    endif()
    if(_fallback_library_paths)
        list(REMOVE_DUPLICATES _fallback_library_paths)
    endif()
endif()

if((NOT DEFINED ENV{INCLUDE} OR "$ENV{INCLUDE}" STREQUAL "") AND
   DEFINED HOST_INCLUDE_PATHS AND NOT "${HOST_INCLUDE_PATHS}" STREQUAL "")
    set(ENV{INCLUDE} "${HOST_INCLUDE_PATHS}")
endif()

if((NOT DEFINED ENV{INCLUDE} OR "$ENV{INCLUDE}" STREQUAL "") AND _fallback_include_paths)
    set(ENV{INCLUDE} "${_fallback_include_paths}")
endif()

if((NOT DEFINED ENV{LIB} OR "$ENV{LIB}" STREQUAL "") AND
   DEFINED HOST_LIBRARY_PATHS AND NOT "${HOST_LIBRARY_PATHS}" STREQUAL "")
    set(ENV{LIB} "${HOST_LIBRARY_PATHS}")
endif()

if((NOT DEFINED ENV{LIB} OR "$ENV{LIB}" STREQUAL "") AND _fallback_library_paths)
    set(ENV{LIB} "${_fallback_library_paths}")
endif()

if((NOT DEFINED ENV{LIBPATH} OR "$ENV{LIBPATH}" STREQUAL "") AND
   DEFINED HOST_LIBRARY_PATHS AND NOT "${HOST_LIBRARY_PATHS}" STREQUAL "")
    set(ENV{LIBPATH} "${HOST_LIBRARY_PATHS}")
endif()

if((NOT DEFINED ENV{LIBPATH} OR "$ENV{LIBPATH}" STREQUAL "") AND _fallback_library_paths)
    set(ENV{LIBPATH} "${_fallback_library_paths}")
endif()

if(NOT EXISTS "${TARGET_FILE}")
    message(STATUS "[ctest-policy] Missing executable for test target '${TARGET_NAME}', building on demand")

    set(_build_cmd ${CMAKE_COMMAND} --build "${BUILD_DIR}" --target "${TARGET_NAME}")
    if(DEFINED BUILD_CONFIG AND NOT "${BUILD_CONFIG}" STREQUAL "")
        list(APPEND _build_cmd --config "${BUILD_CONFIG}")
    endif()

    execute_process(
        COMMAND ${_build_cmd}
        RESULT_VARIABLE _build_rc
    )

    if(NOT _build_rc EQUAL 0)
        message(FATAL_ERROR "[ctest-policy] Auto-build failed for target '${TARGET_NAME}' (rc=${_build_rc})")
    endif()

    if(NOT EXISTS "${TARGET_FILE}")
        message(FATAL_ERROR "[ctest-policy] Built target '${TARGET_NAME}' but executable still missing at '${TARGET_FILE}'")
    endif()
endif()

execute_process(
    COMMAND "${TARGET_FILE}" ${_argv}
    RESULT_VARIABLE _test_rc
)

if(NOT _test_rc EQUAL 0)
    message(FATAL_ERROR "[ctest-policy] Test target '${TARGET_NAME}' failed with exit code ${_test_rc}")
endif()
