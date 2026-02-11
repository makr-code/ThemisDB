# Multi-Level Encrypted User Storage Plugin
cmake_minimum_required(VERSION 3.20)

project(user_storage_encrypted_plugin VERSION 1.0.0)

# Plugin source files
set(PLUGIN_SOURCES
    src/multi_level_storage.cpp
    src/gocryptfs_backend.cpp
    src/key_rotation_scheduler.cpp
)

# Plugin headers
set(PLUGIN_HEADERS
    include/multi_level_storage.hpp
    include/gocryptfs_backend.hpp
    include/encryption_backend_interface.hpp
    include/key_rotation_scheduler.hpp
    include/security_level.hpp
    include/user_models.hpp
)

# Create shared library
add_library(themis_user_storage_encrypted SHARED
    ${PLUGIN_SOURCES}
    ${PLUGIN_HEADERS}
)

# Include directories
target_include_directories(themis_user_storage_encrypted
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
)

# Link dependencies
target_link_libraries(themis_user_storage_encrypted
    PRIVATE
        nlohmann_json::nlohmann_json
)

# C++17 required
target_compile_features(themis_user_storage_encrypted PRIVATE cxx_std_17)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(themis_user_storage_encrypted PRIVATE
        THEMIS_PLUGIN_EXPORTS
    )
endif()

# Install plugin library
install(TARGETS themis_user_storage_encrypted
    LIBRARY DESTINATION lib/themis/plugins
    RUNTIME DESTINATION bin/themis/plugins
    ARCHIVE DESTINATION lib/themis/plugins
)

# Install headers
install(DIRECTORY include/
    DESTINATION include/themis/plugins/user_storage_encrypted
    FILES_MATCHING PATTERN "*.hpp"
)

# Generate plugin manifest
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin.json.in
    ${CMAKE_CURRENT_BINARY_DIR}/plugin.json
    @ONLY
)

# Install manifest
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plugin.json
    DESTINATION lib/themis/plugins/user_storage_encrypted
)

# Install configuration example
install(FILES config/storage_config.yaml.example
    DESTINATION share/themis/config/examples
)

# Tests (if enabled)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
