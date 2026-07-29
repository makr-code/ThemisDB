add_library(themisdb_storage_shared STATIC
    src/encrypted_blob_backend.cpp
    src/blob_redundancy_manager.cpp
    src/zero_copy_blob_transfer.cpp
)

target_include_directories(themisdb_storage_shared
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(themisdb_sdk_FOUND)
    target_link_libraries(themisdb_storage_shared PUBLIC themisdb_sdk::themisdb_sdk)
endif()

set_target_properties(themisdb_storage_shared PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)
