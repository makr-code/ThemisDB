# MSVC Include Path Fix for Subprojects
# This file is automatically included after every project() call via CMAKE_PROJECT_INCLUDE

if(MSVC)
    set(_themis_program_files_x86 "$ENV{ProgramFiles\(x86\)}")
    if(NOT _themis_program_files_x86)
        set(_themis_program_files_x86 "$ENV{ProgramFiles}")
    endif()

    set(_themis_vs_search_roots)
    foreach(_themis_vs_root IN ITEMS "$ENV{ProgramFiles\(x86\)}" "$ENV{ProgramFiles}" "$ENV{ProgramW6432}")
        if(_themis_vs_root)
            list(APPEND _themis_vs_search_roots "${_themis_vs_root}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES _themis_vs_search_roots)

    if(DEFINED ENV{VCToolsInstallDir})
        set(_VC_TOOLS_DIR "$ENV{VCToolsInstallDir}")
        string(REGEX REPLACE "\\\\$" "" _VC_TOOLS_DIR "${_VC_TOOLS_DIR}")
    else()
        set(_VC_TOOLS_DIR "")
    endif()

    if(NOT _VC_TOOLS_DIR)
        set(_themis_msvc_tool_roots)
        foreach(_themis_vs_root IN LISTS _themis_vs_search_roots)
            file(GLOB _themis_msvc_tool_roots_glob
                "${_themis_vs_root}/Microsoft Visual Studio/*/*/VC/Tools/MSVC/*")
            list(APPEND _themis_msvc_tool_roots ${_themis_msvc_tool_roots_glob})
        endforeach()
        list(REMOVE_DUPLICATES _themis_msvc_tool_roots)
        list(SORT _themis_msvc_tool_roots ORDER DESCENDING)
        list(LENGTH _themis_msvc_tool_roots _themis_msvc_tool_roots_count)
        if(_themis_msvc_tool_roots_count GREATER 0)
            list(GET _themis_msvc_tool_roots 0 _VC_TOOLS_DIR)
        endif()
    endif()
    
    set(_WIN_SDK_VERSION "$ENV{WindowsSDKVersion}")
    if(NOT _WIN_SDK_VERSION)
        set(_WIN_SDK_VERSION "$ENV{WindowsSDKLibVersion}")
    endif()
    set(_WIN_SDK_ROOT "$ENV{WindowsSdkDir}")
    if(_WIN_SDK_ROOT)
        string(REGEX REPLACE "[\\/]$" "" _WIN_SDK_ROOT "${_WIN_SDK_ROOT}")
    endif()

    if(NOT _WIN_SDK_ROOT AND _themis_program_files_x86)
        set(_WIN_SDK_ROOT "${_themis_program_files_x86}/Windows Kits/10")
    endif()

    if(_WIN_SDK_VERSION)
        string(REGEX REPLACE "[\\/]$" "" _WIN_SDK_VERSION "${_WIN_SDK_VERSION}")
    endif()

    if(_WIN_SDK_ROOT AND NOT _WIN_SDK_VERSION)
        file(GLOB _themis_windows_sdk_versions "${_WIN_SDK_ROOT}/Include/*")
        set(_themis_windows_sdk_version_names)
        foreach(_themis_sdk_dir IN LISTS _themis_windows_sdk_versions)
            get_filename_component(_themis_sdk_ver "${_themis_sdk_dir}" NAME)
            if(EXISTS "${_themis_sdk_dir}/ucrt" AND EXISTS "${_themis_sdk_dir}/shared" AND EXISTS "${_themis_sdk_dir}/um")
                list(APPEND _themis_windows_sdk_version_names "${_themis_sdk_ver}")
            endif()
        endforeach()
        list(SORT _themis_windows_sdk_version_names ORDER DESCENDING)
        list(LENGTH _themis_windows_sdk_version_names _themis_windows_sdk_version_count)
        if(_themis_windows_sdk_version_count GREATER 0)
            list(GET _themis_windows_sdk_version_names 0 _WIN_SDK_VERSION)
        endif()
    endif()
    
    if(_VC_TOOLS_DIR AND _WIN_SDK_ROOT AND _WIN_SDK_VERSION)
        # Apply globally after each project() call
        # DO NOT use SYSTEM - it marks them as low-priority external includes
        # and MSVC headers won't find each other (e.g., yvals.h can't find crtdbg.h)
        include_directories(BEFORE
            "${_VC_TOOLS_DIR}/include"
            "${_WIN_SDK_ROOT}/Include/${_WIN_SDK_VERSION}/ucrt"
            "${_WIN_SDK_ROOT}/Include/${_WIN_SDK_VERSION}/shared"
            "${_WIN_SDK_ROOT}/Include/${_WIN_SDK_VERSION}/um"
        )
        
        # Also add link directories for every project
        link_directories(
            "${_VC_TOOLS_DIR}/lib/x64"
            "${_WIN_SDK_ROOT}/Lib/${_WIN_SDK_VERSION}/ucrt/x64"
            "${_WIN_SDK_ROOT}/Lib/${_WIN_SDK_VERSION}/um/x64"
        )
    endif()
    
    if(NOT PROJECT_NAME STREQUAL "Themis")
        message(STATUS "Applied MSVC includes to project: ${PROJECT_NAME}")
    endif()
endif()
