option(THEMIS_HAS_AZURE_STORAGE "Enable Azure Blob Storage SDK" OFF)

add_library(azure_blob_storage SHARED
    src/blob_backend_azure.cpp
)

target_include_directories(azure_blob_storage
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../shared/include
)

target_link_libraries(azure_blob_storage PRIVATE themisdb_storage_shared)

if(THEMIS_HAS_AZURE_STORAGE)
    find_package(azure-storage-blobs-cpp QUIET)
    if(azure-storage-blobs-cpp_FOUND)
        target_link_libraries(azure_blob_storage PRIVATE
            Azure::azure-storage-blobs)
    else()
        find_package(OpenSSL REQUIRED)
        target_link_libraries(azure_blob_storage PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    endif()
    target_compile_definitions(azure_blob_storage PRIVATE
        THEMIS_HAS_AZURE_STORAGE=1)
else()
    target_compile_definitions(azure_blob_storage PRIVATE
        THEMIS_HAS_AZURE_STORAGE=0)
endif()

set_target_properties(azure_blob_storage PROPERTIES
    OUTPUT_NAME "azure_blob_storage"
    PREFIX      ""
    VERSION     ${PROJECT_VERSION}
    SOVERSION   1
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

include(GNUInstallDirs)
install(TARGETS azure_blob_storage
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}/themisdb/plugins
    RUNTIME  DESTINATION ${CMAKE_INSTALL_LIBDIR}/themisdb/plugins
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}/themisdb/plugins
)
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/azure_blob_storage
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)
install(FILES plugin.json
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/themisdb/plugins
)
