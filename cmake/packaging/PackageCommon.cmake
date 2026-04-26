# ThemisDB Packaging Common Helpers

include_guard(GLOBAL)

# Shared defaults (can be overridden by parent scope before include)
set(_themis_legacy_package_store "${CMAKE_SOURCE_DIR}/vcpkg_packages")
set(_themis_default_package_store "${CMAKE_SOURCE_DIR}/releases")

if(NOT DEFINED THEMIS_PACKAGE_STORE)
    set(THEMIS_PACKAGE_STORE "${_themis_default_package_store}" CACHE PATH "Directory for generated packages")
elseif(THEMIS_PACKAGE_STORE STREQUAL "${_themis_legacy_package_store}")
    # Auto-migrate only the historical default path; keep explicit custom stores untouched.
    set(THEMIS_PACKAGE_STORE "${_themis_default_package_store}" CACHE PATH "Directory for generated packages" FORCE)
endif()

if(NOT DEFINED THEMIS_VCPKG_ROOT)
    set(THEMIS_VCPKG_ROOT "${CMAKE_SOURCE_DIR}/vcpkg" CACHE PATH "vcpkg installation directory")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(THEMIS_BUILD_CONFIG "debug")
else()
    set(THEMIS_BUILD_CONFIG "release")
endif()

if(NOT DEFINED THEMIS_ZIP_INCLUDE_DEVELOPMENT)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(THEMIS_ZIP_INCLUDE_DEVELOPMENT ON CACHE BOOL "Include development headers/libs in ZIP packages")
    else()
        set(THEMIS_ZIP_INCLUDE_DEVELOPMENT OFF CACHE BOOL "Include development headers/libs in ZIP packages")
    endif()
endif()

set(THEMIS_PACKAGE_DIR_WINDOWS_DEBUG "${THEMIS_PACKAGE_STORE}/x64-windows/debug")
set(THEMIS_PACKAGE_DIR_WINDOWS_RELEASE "${THEMIS_PACKAGE_STORE}/x64-windows/release")
set(THEMIS_PACKAGE_DIR_LINUX_DEBUG "${THEMIS_PACKAGE_STORE}/x64-linux/debug")
set(THEMIS_PACKAGE_DIR_LINUX_RELEASE "${THEMIS_PACKAGE_STORE}/x64-linux/release")

# Tool discovery
find_program(THEMIS_CPACK_EXECUTABLE cpack)
find_program(THEMIS_7Z_EXECUTABLE 7z)
find_program(THEMIS_TAR_EXECUTABLE tar)
find_program(THEMIS_GZIP_EXECUTABLE gzip)
find_program(THEMIS_NSIS_EXECUTABLE makensis)
find_program(THEMIS_INNO_EXECUTABLE iscc)
find_program(THEMIS_WIX_CANDLE_EXECUTABLE candle)
find_program(THEMIS_WIX_LIGHT_EXECUTABLE light)
find_program(THEMIS_WIX_EXECUTABLE wix)

set(THEMIS_HAS_ZIP_TOOLS OFF)
if(THEMIS_CPACK_EXECUTABLE OR THEMIS_7Z_EXECUTABLE)
    set(THEMIS_HAS_ZIP_TOOLS ON)
endif()

set(THEMIS_HAS_MSI_TOOLS OFF)
if(WIN32)
    if(THEMIS_CPACK_EXECUTABLE AND (THEMIS_WIX_EXECUTABLE OR (THEMIS_WIX_CANDLE_EXECUTABLE AND THEMIS_WIX_LIGHT_EXECUTABLE)))
        set(THEMIS_HAS_MSI_TOOLS ON)
    endif()
endif()

set(THEMIS_HAS_INSTALLER_TOOLS OFF)
if(WIN32)
    if(THEMIS_NSIS_EXECUTABLE OR THEMIS_INNO_EXECUTABLE OR THEMIS_HAS_MSI_TOOLS)
        set(THEMIS_HAS_INSTALLER_TOOLS ON)
    endif()
endif()

set(THEMIS_HAS_TGZ_TOOLS OFF)
if(THEMIS_CPACK_EXECUTABLE OR (THEMIS_TAR_EXECUTABLE AND THEMIS_GZIP_EXECUTABLE))
    set(THEMIS_HAS_TGZ_TOOLS ON)
endif()

if(NOT TARGET package-tools-report)
    add_custom_target(package-tools-report
        COMMAND ${CMAKE_COMMAND} -E echo "Packaging tool status"
        COMMAND ${CMAKE_COMMAND} -E echo "  cpack: ${THEMIS_CPACK_EXECUTABLE}"
        COMMAND ${CMAKE_COMMAND} -E echo "  7z: ${THEMIS_7Z_EXECUTABLE}"
        COMMAND ${CMAKE_COMMAND} -E echo "  tar: ${THEMIS_TAR_EXECUTABLE}"
        COMMAND ${CMAKE_COMMAND} -E echo "  gzip: ${THEMIS_GZIP_EXECUTABLE}"
        COMMAND ${CMAKE_COMMAND} -E echo "  nsis: ${THEMIS_NSIS_EXECUTABLE}"
        COMMAND ${CMAKE_COMMAND} -E echo "  inno: ${THEMIS_INNO_EXECUTABLE}"
        COMMAND ${CMAKE_COMMAND} -E echo "  wix(candle/light): ${THEMIS_WIX_CANDLE_EXECUTABLE} / ${THEMIS_WIX_LIGHT_EXECUTABLE}"
        COMMAND ${CMAKE_COMMAND} -E echo "  wix(v4): ${THEMIS_WIX_EXECUTABLE}"
        COMMAND ${CMAKE_COMMAND} -E echo "  zip-capable: ${THEMIS_HAS_ZIP_TOOLS}"
        COMMAND ${CMAKE_COMMAND} -E echo "  msi-capable: ${THEMIS_HAS_MSI_TOOLS}"
        COMMAND ${CMAKE_COMMAND} -E echo "  installer-capable: ${THEMIS_HAS_INSTALLER_TOOLS}"
        COMMAND ${CMAKE_COMMAND} -E echo "  tgz-capable: ${THEMIS_HAS_TGZ_TOOLS}"
        VERBATIM
    )
endif()
