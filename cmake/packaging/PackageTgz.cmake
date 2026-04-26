# TGZ packaging targets

include_guard(GLOBAL)
include(${CMAKE_CURRENT_LIST_DIR}/PackageCommon.cmake)

if(NOT TARGET package-tgz)
    if(THEMIS_CPACK_EXECUTABLE)
        add_custom_target(package-tgz
            COMMAND ${THEMIS_CPACK_EXECUTABLE} -G TGZ -C ${CMAKE_BUILD_TYPE}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Create TGZ package via CPack"
            VERBATIM
        )
    elseif(THEMIS_TAR_EXECUTABLE AND THEMIS_GZIP_EXECUTABLE)
        add_custom_target(package-tgz
            COMMAND ${CMAKE_COMMAND} -E echo "CPack not found. Please use tar/gzip manually for archive creation."
            COMMENT "TGZ packaging fallback"
            VERBATIM
        )
    else()
        add_custom_target(package-tgz
            COMMAND ${CMAKE_COMMAND} -E echo "TGZ packaging tools missing (need cpack or tar+gzip)."
            COMMENT "TGZ packaging unavailable"
            VERBATIM
        )
    endif()
endif()
