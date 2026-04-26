# MSI packaging targets (Windows)

include_guard(GLOBAL)
include(${CMAKE_CURRENT_LIST_DIR}/PackageCommon.cmake)

if(WIN32 AND NOT TARGET package-msi)
    if(THEMIS_HAS_MSI_TOOLS)
        add_custom_target(package-msi
            COMMAND ${THEMIS_CPACK_EXECUTABLE} -G WIX -C ${CMAKE_BUILD_TYPE}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Create MSI package via CPack/WiX"
            VERBATIM
        )
    else()
        add_custom_target(package-msi
            COMMAND ${CMAKE_COMMAND} -E echo "MSI tools missing (need cpack + WiX)."
            COMMENT "MSI packaging unavailable"
            VERBATIM
        )
    endif()
endif()
