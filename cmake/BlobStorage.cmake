# Blob Storage Backend Sources
# Multiple blob storage implementations: S3, Azure, FileSystem, WebDAV
# Requires: THEMIS_ENABLE_S3, THEMIS_ENABLE_AZURE, THEMIS_ENABLE_WEBDAV (optional)

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

# WebDAV blob storage
if(THEMIS_ENABLE_WEBDAV)
    list(APPEND THEMIS_CORE_SOURCES
        ../src/storage/blob_backend_webdav.cpp
    )
endif()
