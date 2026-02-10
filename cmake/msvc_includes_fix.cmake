# MSVC Include Path Fix for Subprojects
# This file is automatically included after every project() call via CMAKE_PROJECT_INCLUDE

if(MSVC)
    if(DEFINED ENV{VCToolsInstallDir})
        set(_VC_TOOLS_DIR "$ENV{VCToolsInstallDir}")
        string(REGEX REPLACE "\\\\$" "" _VC_TOOLS_DIR "${_VC_TOOLS_DIR}")
    else()
        set(_VC_TOOLS_DIR "C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Tools/MSVC/14.44.35207")
    endif()
    
    set(_WIN_SDK_VERSION "10.0.22621.0")
    set(_WIN_SDK_ROOT "C:/Program Files (x86)/Windows Kits/10")
    
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
    
    if(NOT PROJECT_NAME STREQUAL "Themis")
        message(STATUS "Applied MSVC includes to project: ${PROJECT_NAME}")
    endif()
endif()
