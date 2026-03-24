# User Storage Encrypted Plugin – src/user_storage_encrypted
# Public API headers live in include/user_storage_encrypted/.
# Implementation sources live here.

if(NOT THEMIS_PLUGIN_USER_STORAGE_ENCRYPTED)
    return()
endif()

message(STATUS "Configuring User Storage Encrypted Plugin (src/user_storage_encrypted)")

find_package(nlohmann_json REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ARGON2 REQUIRED libargon2)

add_library(themis_user_storage_encrypted SHARED
    multi_level_storage.cpp
    gocryptfs_backend.cpp
    key_rotation_scheduler.cpp
    key_derivation_service.cpp
)

target_include_directories(themis_user_storage_encrypted
    PUBLIC
        $<BUILD_INTERFACE:${THEMIS_ROOT_DIR}/include>
        $<BUILD_INTERFACE:${THEMIS_ROOT_DIR}/include/user_storage_encrypted>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${ARGON2_INCLUDE_DIRS}
)

target_link_libraries(themis_user_storage_encrypted PRIVATE
    nlohmann_json::nlohmann_json
    ${ARGON2_LIBRARIES}
)

target_compile_features(themis_user_storage_encrypted PRIVATE cxx_std_17)

set_target_properties(themis_user_storage_encrypted PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

if(WIN32)
    target_compile_definitions(themis_user_storage_encrypted PRIVATE THEMIS_PLUGIN_EXPORTS)
endif()

install(TARGETS themis_user_storage_encrypted
    LIBRARY DESTINATION lib/themis/plugins
    RUNTIME DESTINATION bin/themis/plugins
    ARCHIVE DESTINATION lib/themis/plugins
)

install(DIRECTORY ${THEMIS_ROOT_DIR}/include/user_storage_encrypted/
    DESTINATION include/user_storage_encrypted
    FILES_MATCHING PATTERN "*.hpp"
)

configure_file(
    ${THEMIS_ROOT_DIR}/plugins/user_storage_encrypted/plugin.json.in
    ${CMAKE_CURRENT_BINARY_DIR}/plugin.json
    @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plugin.json
    DESTINATION lib/themis/plugins/user_storage_encrypted
)

if(EXISTS "${THEMIS_ROOT_DIR}/plugins/user_storage_encrypted/config/storage_config.yaml.example")
    install(FILES ${THEMIS_ROOT_DIR}/plugins/user_storage_encrypted/config/storage_config.yaml.example
        DESTINATION share/themis/config/examples
    )
endif()

if(BUILD_TESTING)
    add_subdirectory(${THEMIS_ROOT_DIR}/plugins/user_storage_encrypted/tests
                     ${CMAKE_CURRENT_BINARY_DIR}/user_storage_encrypted_tests)
endif()

message(STATUS "User Storage Encrypted Plugin configuration complete")
