option(THEMIS_HAS_AWS_SDK "Enable AWS SDK v3 for S3" OFF)

add_library(s3_blob_storage SHARED
    src/blob_backend_s3.cpp
)

target_include_directories(s3_blob_storage
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../shared/include
)

target_link_libraries(s3_blob_storage PRIVATE themisdb_storage_shared)

if(THEMIS_HAS_AWS_SDK)
    find_package(AWSSDK REQUIRED COMPONENTS s3)
    target_link_libraries(s3_blob_storage PRIVATE ${AWSSDK_LINK_LIBRARIES})
    find_package(OpenSSL REQUIRED)
    target_link_libraries(s3_blob_storage PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(s3_blob_storage PRIVATE THEMIS_HAS_AWS_SDK=1)
else()
    target_compile_definitions(s3_blob_storage PRIVATE THEMIS_HAS_AWS_SDK=0)
endif()

set_target_properties(s3_blob_storage PROPERTIES
    OUTPUT_NAME "s3_blob_storage"
    PREFIX      ""
    VERSION     ${PROJECT_VERSION}
    SOVERSION   1
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

include(GNUInstallDirs)
install(TARGETS s3_blob_storage
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}/themisdb/plugins
    RUNTIME  DESTINATION ${CMAKE_INSTALL_LIBDIR}/themisdb/plugins
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}/themisdb/plugins
)
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/s3_blob_storage
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)
install(FILES plugin.json
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/themisdb/plugins
)
