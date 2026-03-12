# User Storage Encrypted Plugin – migrated to src/plugins/user_storage_encrypted
# Sources live here; public headers live in include/plugins/user_storage_encrypted/

if(NOT THEMIS_PLUGIN_USER_STORAGE_ENCRYPTED)
    return()
endif()

message(STATUS "Configuring User Storage Encrypted Plugin (src/plugins/user_storage_encrypted)")

find_package(nlohmann_json REQUIRED)

add_library(themis_user_storage_encrypted SHARED
    multi_level_storage.cpp
    gocryptfs_backend.cpp
    key_rotation_scheduler.cpp
)

target_include_directories(themis_user_storage_encrypted
    PUBLIC
        $<BUILD_INTERFACE:${THEMIS_ROOT_DIR}/include>
        $<BUILD_INTERFACE:${THEMIS_ROOT_DIR}/include/plugins/user_storage_encrypted>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(themis_user_storage_encrypted PRIVATE
    nlohmann_json::nlohmann_json
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

install(DIRECTORY ${THEMIS_ROOT_DIR}/include/plugins/user_storage_encrypted/
    DESTINATION include/plugins/user_storage_encrypted
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
