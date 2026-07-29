cmake_minimum_required(VERSION 3.16)
project(chimera LANGUAGES CXX)

option(CHIMERA_BUILD_TESTS "Build Chimera unit tests" ON)

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ------- MSVC specific configuration copied from Themis CompilerOptions.cmake
option(CHIMERA_DISABLE_SECURITY_HARDENING "Disable compiler security hardening flags for Chimera" OFF)

if(MSVC)
  set(_chimera_program_files_x86 "$ENV{ProgramFiles(\x86)}")
  if(NOT _chimera_program_files_x86)
    set(_chimera_program_files_x86 "$ENV{ProgramFiles}")
  endif()

  set(_chimera_vs_search_roots)
  foreach(_chimera_vs_root IN ITEMS "$ENV{ProgramFiles(\x86)}" "$ENV{ProgramFiles}" "$ENV{ProgramW6432}")
    if(_chimera_vs_root)
      list(APPEND _chimera_vs_search_roots "${_chimera_vs_root}")
    endif()
  endforeach()
  list(REMOVE_DUPLICATES _chimera_vs_search_roots)

  if(DEFINED ENV{VCToolsInstallDir})
    set(_CHIMERA_VC_TOOLS_DIR "$ENV{VCToolsInstallDir}")
    string(REGEX REPLACE "\\\\$" "" _CHIMERA_VC_TOOLS_DIR "${_CHIMERA_VC_TOOLS_DIR}")
  else()
    set(_CHIMERA_VC_TOOLS_DIR "")
  endif()

  if(NOT _CHIMERA_VC_TOOLS_DIR)
    set(_chimera_msvc_tool_roots)
    foreach(_chimera_vs_root IN LISTS _chimera_vs_search_roots)
      file(GLOB _chimera_msvc_tool_roots_glob
        "${_chimera_vs_root}/Microsoft Visual Studio/*/*/VC/Tools/MSVC/*")
      list(APPEND _chimera_msvc_tool_roots ${_chimera_msvc_tool_roots_glob})
    endforeach()
    list(REMOVE_DUPLICATES _chimera_msvc_tool_roots)
    list(SORT _chimera_msvc_tool_roots ORDER DESCENDING)
    list(LENGTH _chimera_msvc_tool_roots _chimera_msvc_tool_roots_count)
    if(_chimera_msvc_tool_roots_count GREATER 0)
      list(GET _chimera_msvc_tool_roots 0 _CHIMERA_VC_TOOLS_DIR)
    endif()
  endif()

  set(_CHIMERA_WIN_SDK_VERSION "$ENV{WindowsSDKVersion}")
  if(NOT _CHIMERA_WIN_SDK_VERSION)
    set(_CHIMERA_WIN_SDK_VERSION "$ENV{WindowsSDKLibVersion}")
  endif()
  set(_CHIMERA_WIN_SDK_ROOT "$ENV{WindowsSdkDir}")
  if(_CHIMERA_WIN_SDK_ROOT)
    string(REGEX REPLACE "[\\\/]$" "" _CHIMERA_WIN_SDK_ROOT "${_CHIMERA_WIN_SDK_ROOT}")
  endif()

  if(NOT _CHIMERA_WIN_SDK_ROOT AND _chimera_program_files_x86)
    set(_CHIMERA_WIN_SDK_ROOT "${_chimera_program_files_x86}/Windows Kits/10")
  endif()

  if(_CHIMERA_WIN_SDK_VERSION)
    string(REGEX REPLACE "[\\\/]$" "" _CHIMERA_WIN_SDK_VERSION "${_CHIMERA_WIN_SDK_VERSION}")
  endif()

  if(_CHIMERA_WIN_SDK_ROOT AND NOT _CHIMERA_WIN_SDK_VERSION)
    file(GLOB _chimera_windows_sdk_versions "${_CHIMERA_WIN_SDK_ROOT}/Include/*")
    set(_chimera_windows_sdk_version_names)
    foreach(_chimera_sdk_dir IN LISTS _chimera_windows_sdk_versions)
      get_filename_component(_chimera_sdk_ver "${_chimera_sdk_dir}" NAME)
      if(EXISTS "${_chimera_sdk_dir}/ucrt" AND EXISTS "${_chimera_sdk_dir}/shared" AND EXISTS "${_chimera_sdk_dir}/um")
        list(APPEND _chimera_windows_sdk_version_names "${_chimera_sdk_ver}")
      endif()
    endforeach()
    list(SORT _chimera_windows_sdk_version_names ORDER DESCENDING)
    list(LENGTH _chimera_windows_sdk_version_names _chimera_windows_sdk_version_count)
    if(_chimera_windows_sdk_version_count GREATER 0)
      list(GET _chimera_windows_sdk_version_names 0 _CHIMERA_WIN_SDK_VERSION)
    endif()
  endif()

  if(_CHIMERA_VC_TOOLS_DIR AND _CHIMERA_WIN_SDK_ROOT AND _CHIMERA_WIN_SDK_VERSION)
    include_directories(BEFORE
      "${_CHIMERA_VC_TOOLS_DIR}/include"
      "${_CHIMERA_WIN_SDK_ROOT}/Include/${_CHIMERA_WIN_SDK_VERSION}/ucrt"
      "${_CHIMERA_WIN_SDK_ROOT}/Include/${_CHIMERA_WIN_SDK_VERSION}/shared"
      "${_CHIMERA_WIN_SDK_ROOT}/Include/${_CHIMERA_WIN_SDK_VERSION}/um"
    )
    link_directories(
      "${_CHIMERA_VC_TOOLS_DIR}/lib/x64"
      "${_CHIMERA_WIN_SDK_ROOT}/Lib/${_CHIMERA_WIN_SDK_VERSION}/ucrt/x64"
      "${_CHIMERA_WIN_SDK_ROOT}/Lib/${_CHIMERA_WIN_SDK_VERSION}/um/x64"
    )
  endif()

  # MSVC runtime selection: prefer DLL runtime unless static triplet detected via vcpkg
  set(_chimera_use_static_crt OFF)
  if(DEFINED VCPKG_TARGET_TRIPLET AND VCPKG_TARGET_TRIPLET MATCHES "static")
    set(_chimera_use_static_crt ON)
  endif()

  if(_chimera_use_static_crt)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "MSVC Runtime" FORCE)
  else()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" CACHE STRING "MSVC Runtime" FORCE)
  endif()

  # MSVC compile options similar to Themis
  add_compile_options(
    /W4
    /WX-
    /fp:precise
    /Gy
    /permissive-
    /EHsc
    /w14018
  )

  # Security hardening flags for Release builds
  if(NOT CHIMERA_DISABLE_SECURITY_HARDENING)
    add_compile_options(
      $<$<CONFIG:Release>:/GS>
      $<$<CONFIG:Release>:/sdl>
      $<$<CONFIG:Release>:/guard:cf>
    )
    add_link_options(
      $<$<CONFIG:Release>:/GUARD:CF>
      $<$<CONFIG:Release>:/NXCOMPAT>
      $<$<CONFIG:Release>:/DYNAMICBASE>
    )
  endif()
endif()


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Sources
file(GLOB_RECURSE CHIMERA_SOURCES CONFIGURE_DEPENDS src/chimera/*.cpp)
add_library(chimera_core STATIC ${CHIMERA_SOURCES})
target_include_directories(chimera_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(CHIMERA_BUILD_TESTS)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/release-1.14.0.zip
  )
  FetchContent_MakeAvailable(googletest)

  enable_testing()

  file(GLOB CHIMERA_TEST_SOURCES CONFIGURE_DEPENDS tests/chimera/*_test.cpp tests/chimera/test_*.cpp)
  foreach(test_src IN LISTS CHIMERA_TEST_SOURCES)
    get_filename_component(test_name ${test_src} NAME_WE)
    string(REPLACE "." "_" test_name_safe ${test_name})
    add_executable(${test_name_safe} ${test_src})
    target_include_directories(${test_name_safe} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(${test_name_safe} PRIVATE chimera_core gtest_main)
    add_test(NAME ${test_name_safe} COMMAND ${test_name_safe})
  endforeach()
endif()

install(TARGETS chimera_core EXPORT chimeraTargets DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
