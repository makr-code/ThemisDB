# Installer packaging targets (EXE/installer)

include_guard(GLOBAL)
include(${CMAKE_CURRENT_LIST_DIR}/PackageCommon.cmake)

if(WIN32 AND NOT TARGET package-installer)
    if(THEMIS_CPACK_EXECUTABLE AND THEMIS_NSIS_EXECUTABLE)
        add_custom_target(package-installer
            COMMAND ${THEMIS_CPACK_EXECUTABLE} -G NSIS -C ${CMAKE_BUILD_TYPE}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Create installer via CPack/NSIS"
            VERBATIM
        )
    elseif(THEMIS_HAS_MSI_TOOLS)
        add_custom_target(package-installer
            COMMAND ${CMAKE_COMMAND} -E echo "NSIS not found; using MSI installer target instead."
            COMMAND ${THEMIS_CPACK_EXECUTABLE} -G WIX -C ${CMAKE_BUILD_TYPE}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Create installer via CPack/WiX"
            VERBATIM
        )
    else()
        add_custom_target(package-installer
            COMMAND ${CMAKE_COMMAND} -E echo "Installer tools missing (need NSIS or WiX)."
            COMMENT "Installer packaging unavailable"
            VERBATIM
        )
    endif()
endif()
