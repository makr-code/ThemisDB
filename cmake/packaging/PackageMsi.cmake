# MSI packaging targets (Windows)

include_guard(GLOBAL)
include(${CMAKE_CURRENT_LIST_DIR}/PackageCommon.cmake)

if(WIN32 AND NOT TARGET package-msi)
    if(THEMIS_HAS_MSI_TOOLS)
        set(_themis_msi_components runtime tools models)
        if(THEMIS_PACKAGE_INCLUDE_TESTS)
            list(APPEND _themis_msi_components tests)
        endif()
        if(THEMIS_PACKAGE_INCLUDE_BENCHMARKS)
            list(APPEND _themis_msi_components benchmarks)
        endif()
        string(JOIN ";" _themis_msi_components_arg ${_themis_msi_components})

        add_custom_target(package-msi
            COMMAND ${THEMIS_CPACK_EXECUTABLE}
                -G WIX
                -C ${CMAKE_BUILD_TYPE}
                -D "CPACK_COMPONENTS_ALL=${_themis_msi_components_arg}"
                -D CPACK_WIX_CANDLE_EXECUTABLE=${THEMIS_WIX_CANDLE_EXECUTABLE}
                -D CPACK_WIX_LIGHT_EXECUTABLE=${THEMIS_WIX_LIGHT_EXECUTABLE}
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
