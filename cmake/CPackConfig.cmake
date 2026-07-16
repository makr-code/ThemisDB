## CPackConfig.cmake
# Generated scaffold based on docs/CPACK_ANALYSIS.md
# Adds basic multi-edition, multi-generator defaults for ThemisDB.

set(CPACK_PACKAGE_NAME "ThemisDB")
set(CPACK_PACKAGE_VERSION "${THEMIS_VERSION_STRING}")
set(CPACK_PACKAGE_VENDOR "ThemisDB Contributors")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://themisdb.io")
set(CPACK_PACKAGE_CONTACT "ThemisDB <support@themisdb.io>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Enterprise distributed database with GPU acceleration")

# Component model
set(CPACK_COMPONENTS_ALL runtime development documentation)
set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "Runtime")
set(CPACK_COMPONENT_DEVELOPMENT_DISPLAY_NAME "Development Headers & Libraries")
set(CPACK_COMPONENT_DOCUMENTATION_DISPLAY_NAME "Documentation")

# Default generators: adjust in RELEASE_STRATEGY.md / CI if needed
set(CPACK_GENERATOR "ZIP;TGZ")

# Edition-aware packaging (override via -DTHEMIS_EDITION_STRING=...)
if(DEFINED THEMIS_EDITION_STRING)
  set(THEMIS_EDITION "${THEMIS_EDITION_STRING}")
  if(THEMIS_EDITION STREQUAL "minimal")
    set(CPACK_PACKAGE_FILE_NAME "themisdb-${CPACK_PACKAGE_VERSION}-minimal-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
    set(CPACK_COMPONENTS_ALL runtime)
  elseif(THEMIS_EDITION STREQUAL "community")
    set(CPACK_PACKAGE_FILE_NAME "themisdb-${CPACK_PACKAGE_VERSION}-community-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
    set(CPACK_COMPONENTS_ALL runtime documentation tools)
  else()
    set(CPACK_PACKAGE_FILE_NAME "themisdb-${CPACK_PACKAGE_VERSION}-${THEMIS_EDITION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
  endif()
endif()

# Windows: optional WIX/MSI support
if(WIN32)
  set(CPACK_WIX_UPGRADE_GUID "01234567-89AB-CDEF-0123-456789ABCDEF")
  set(CPACK_WIX_PRODUCT_ICON "${THEMIS_ROOT_DIR}/assets/themis-icon-64x64.ico")
  list(APPEND CPACK_GENERATOR "WIX")
endif()

# Debian (.deb) settings
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libssl3, libcurl4")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "ThemisDB <support@themisdb.io>")
set(CPACK_DEBIAN_PACKAGE_SECTION "database")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "standard")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${THEMIS_ROOT_DIR}/debian/preinst;${THEMIS_ROOT_DIR}/debian/postinst")

# RPM settings
set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
set(CPACK_RPM_PACKAGE_REQUIRES "openssl-libs, libcurl")
set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0-or-later")
set(CPACK_RPM_PACKAGE_URL "https://themisdb.io")

