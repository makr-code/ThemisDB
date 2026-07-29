add_library(user_storage_encrypted SHARED
    src/multi_level_storage.cpp
    src/gocryptfs_backend.cpp
    src/key_rotation_scheduler.cpp
    src/key_derivation_service.cpp
)

target_include_directories(user_storage_encrypted
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(user_storage_encrypted PRIVATE themisdb_storage_shared)

if(themisdb_sdk_FOUND)
    target_link_libraries(user_storage_encrypted PUBLIC themisdb_sdk::themisdb_sdk)
endif()

set_target_properties(user_storage_encrypted PROPERTIES
    OUTPUT_NAME "user_storage_encrypted"
    PREFIX      ""
    VERSION     ${PROJECT_VERSION}
    SOVERSION   1
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

configure_file(plugin.json.in plugin.json @ONLY)

include(GNUInstallDirs)
install(TARGETS user_storage_encrypted
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}/themisdb/plugins
    RUNTIME  DESTINATION ${CMAKE_INSTALL_LIBDIR}/themisdb/plugins
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}/themisdb/plugins
)
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/user_storage_encrypted
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plugin.json
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/themisdb/plugins
)
install(DIRECTORY config/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/themisdb/plugins/user_storage_encrypted/config
)
