# Blob Storage Backend Sources
# Multiple blob storage implementations: S3, Azure, FileSystem, WebDAV, GCS
# Requires: THEMIS_ENABLE_S3, THEMIS_ENABLE_AZURE, THEMIS_ENABLE_WEBDAV, THEMIS_ENABLE_GCS (optional)

list(APPEND THEMIS_CORE_SOURCES
    # Unified blob storage interface
    ../src/server/rpc/blob_transfer_handler.cpp
)

# S3-compatible blob storage
if(THEMIS_ENABLE_S3)
    list(APPEND THEMIS_CORE_SOURCES
        ../src/storage/blob_backend_s3.cpp
    )
endif()

# Azure Blob Storage
if(THEMIS_ENABLE_AZURE)
    list(APPEND THEMIS_CORE_SOURCES
        ../src/storage/blob_backend_azure.cpp
    )
endif()

# Local filesystem blob storage
list(APPEND THEMIS_CORE_SOURCES
    ../src/storage/blob_backend_filesystem.cpp
)

# Zero-Copy Blob Transfer (sendfile, mmap, S3 multipart streaming – Issue #231, v1.7.0)
list(APPEND THEMIS_CORE_SOURCES
    ../src/storage/zero_copy_blob_transfer.cpp
)

# WebDAV blob storage
if(THEMIS_ENABLE_WEBDAV)
    list(APPEND THEMIS_CORE_SOURCES
        ../src/storage/blob_backend_webdav.cpp
    )
endif()

# Google Cloud Storage blob backend
if(THEMIS_ENABLE_GCS)
    list(APPEND THEMIS_CORE_SOURCES
        ../src/storage/blob_backend_gcs.cpp
    )
    add_compile_definitions(THEMIS_ENABLE_GCS)
endif()
