add_library(s3_importer SHARED
    src/s3_importer.cpp
)

target_include_directories(s3_importer
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../shared/include
)

target_link_libraries(s3_importer PRIVATE themisdb_importer_shared)

if(themisdb_sdk_FOUND)
    target_link_libraries(s3_importer PUBLIC themisdb_sdk::themisdb_sdk)
endif()

set_target_properties(s3_importer PROPERTIES
    OUTPUT_NAME "s3_importer"
    PREFIX      ""
    VERSION     ${PROJECT_VERSION}
    SOVERSION   1
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

include(GNUInstallDirs)
install(TARGETS s3_importer
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}/themisdb/plugins
    RUNTIME  DESTINATION ${CMAKE_INSTALL_LIBDIR}/themisdb/plugins
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}/themisdb/plugins
)
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/s3_importer
    FILES_MATCHING PATTERN "*.h"
)
install(FILES plugin.json
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/themisdb/plugins
)
