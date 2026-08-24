# ZIP packaging targets

include_guard(GLOBAL)
include(${CMAKE_CURRENT_LIST_DIR}/PackageCommon.cmake)

if(NOT TARGET package-zip)
    set(THEMIS_RELEASE_PACKAGE_DIR "${CMAKE_SOURCE_DIR}/releases")

    set(_themis_include_development OFF)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(_themis_include_development ON)
    endif()

    if(DEFINED THEMIS_ZIP_INCLUDE_DEVELOPMENT)
        set(_themis_include_development "${THEMIS_ZIP_INCLUDE_DEVELOPMENT}")
    endif()

    if(THEMIS_CPACK_EXECUTABLE OR CMAKE_VERSION VERSION_GREATER_EQUAL "3.20")
        add_custom_target(package-zip
            COMMAND ${CMAKE_COMMAND}
                -DTHEMIS_BINARY_DIR=${CMAKE_BINARY_DIR}
                -DTHEMIS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
                -DTHEMIS_VCPKG_ROOT=${THEMIS_VCPKG_ROOT}
                -DVCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET}
                -DTHEMIS_CONFIG=${CMAKE_BUILD_TYPE}
                -DTHEMIS_RELEASE_DIR=${THEMIS_RELEASE_PACKAGE_DIR}
                -DTHEMIS_PACKAGE_NAME=ThemisDB-${THEMIS_EDITION}-${THEMIS_VERSION_STRING}-windows-x64
                -DTHEMIS_INCLUDE_DEVELOPMENT=${_themis_include_development}
                -DTHEMIS_PACKAGE_INCLUDE_TESTS=${THEMIS_PACKAGE_INCLUDE_TESTS}
                -DTHEMIS_PACKAGE_INCLUDE_BENCHMARKS=${THEMIS_PACKAGE_INCLUDE_BENCHMARKS}
                -P ${CMAKE_CURRENT_LIST_DIR}/CreateZipPackage.cmake
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Create deployable Windows ZIP package"
            VERBATIM
        )
    elseif(THEMIS_7Z_EXECUTABLE)
        add_custom_target(package-zip
            COMMAND ${CMAKE_COMMAND} -E echo "CPack not found. Please use 7z manually for archive creation."
            COMMENT "ZIP packaging fallback"
            VERBATIM
        )
    else()
        add_custom_target(package-zip
            COMMAND ${CMAKE_COMMAND} -E echo "ZIP packaging tools missing (need cpack or 7z)."
            COMMENT "ZIP packaging unavailable"
            VERBATIM
        )
    endif()
endif()
