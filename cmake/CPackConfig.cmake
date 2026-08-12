# cmake/CPackConfig.cmake — CPack extension hook
#
# This file is included by CMakeLists.txt BEFORE the main CPACK_* variable
# block (see CMakeLists.txt line ~474). It serves as an optional override
# point for edition-specific or environment-specific CPack settings.
#
# The canonical CPack activation (include(CPack)) happens unconditionally at
# the bottom of CMakeLists.txt after all CPACK_* variables are set.
# Nothing needs to be added here for a standard build.
#
# Extension examples (uncomment and adjust as needed):
#
#   # Override generator list for a custom build type:
#   # if(THEMIS_EDITION STREQUAL "enterprise")
#   #     set(CPACK_GENERATOR "ZIP;WIX")
#   # endif()
#
#   # Include custom WiX template:
#   # set(CPACK_WIX_TEMPLATE "${CMAKE_SOURCE_DIR}/packaging/wix/template.wxs")

message(STATUS "CPack extension hook loaded (cmake/CPackConfig.cmake)")
