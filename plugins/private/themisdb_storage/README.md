# themisdb_storage

Private plugin repository for ThemisDB storage backends.

> **Source of Truth:** All implementation is maintained here.  
> The `plugins/private/themisdb_storage` submodule in the main ThemisDB repository
> points to this repo.

## Contained modules

| Module | Description |
|---|---|
| `user_storage_encrypted/` | Multi-level encrypted user/group storage (4-tier: offen/vs-nfd/geheim/streng-geheim) |
| `azure_blob_storage/` | Azure Blob Storage backend |
| `s3_blob_storage/` | AWS S3 (and S3-compatible) Blob Storage backend |
| `shared/` | Common blob backend interfaces, manager, redundancy manager |

## Repository Structure

```
themisdb_storage/
├── CMakeLists.txt
├── README.md
├── shared/
│   ├── include/
│   │   ├── blob_storage_backend.h        ← IBlobStorageBackend interface
│   │   ├── blob_storage_manager.h
│   │   ├── encrypted_blob_backend.h
│   │   ├── blob_redundancy_manager.h
│   │   └── zero_copy_blob_transfer.h
│   └── src/
│       ├── encrypted_blob_backend.cpp
│       ├── blob_redundancy_manager.cpp
│       └── zero_copy_blob_transfer.cpp
├── user_storage_encrypted/
│   ├── CMakeLists.txt
│   ├── plugin.json.in
│   ├── README.md
│   ├── ROADMAP.md
│   ├── ARCHITECTURE.md
│   ├── SECURITY.md
│   ├── AUDIT.md
│   ├── FUTURE_ENHANCEMENTS.md
│   ├── config/
│   │   └── storage_config.yaml.example
│   ├── include/
│   │   ├── multi_level_storage.hpp
│   │   ├── encryption_backend_interface.hpp
│   │   ├── gocryptfs_backend.hpp
│   │   ├── key_rotation_scheduler.hpp
│   │   ├── key_derivation_service.hpp
│   │   ├── irotation_store.hpp
│   │   ├── user_models.hpp
│   │   └── security_level.hpp
│   ├── src/
│   │   ├── multi_level_storage.cpp
│   │   ├── gocryptfs_backend.cpp
│   │   ├── key_rotation_scheduler.cpp
│   │   └── key_derivation_service.cpp
│   └── tests/
│       ├── CMakeLists.txt
│       └── README.md
├── azure_blob_storage/
│   ├── CMakeLists.txt
│   ├── plugin.json
│   ├── plugin.json.sig
│   ├── README.md
│   ├── ROADMAP.md
│   ├── FUTURE_ENHANCEMENTS.md
│   ├── include/
│   │   └── blob_backend_azure.h
│   └── src/
│       └── blob_backend_azure.cpp
└── s3_blob_storage/
    ├── CMakeLists.txt
    ├── plugin.json
    ├── plugin.json.sig
    ├── include/
    │   └── blob_backend_s3.h
    └── src/
        └── blob_backend_s3.cpp
```

## Migration from ThemisDB Monorepo

| ThemisDB source | Destination |
|---|---|
| `src/user_storage_encrypted/*.cpp` | `user_storage_encrypted/src/` |
| `include/user_storage_encrypted/*.hpp` | `user_storage_encrypted/include/` |
| `plugins/user_storage_encrypted/plugin.json.in` | `user_storage_encrypted/plugin.json.in` |
| `plugins/user_storage_encrypted/config/` | `user_storage_encrypted/config/` |
| `src/storage/blob_backend_azure.cpp` | `azure_blob_storage/src/` |
| `include/storage/blob_backend_azure.h` | `azure_blob_storage/include/` |
| `plugins/blob_storage/azure/plugin.json` | `azure_blob_storage/plugin.json` |
| `src/storage/blob_backend_s3.cpp` | `s3_blob_storage/src/` |
| `include/storage/blob_backend_s3.h` | `s3_blob_storage/include/` |
| `plugins/blob_storage/s3/plugin.json` | `s3_blob_storage/plugin.json` |
| `src/storage/encrypted_blob_backend.cpp` | `shared/src/` |
| `src/storage/blob_redundancy_manager.cpp` | `shared/src/` |
| `src/storage/zero_copy_blob_transfer.cpp` | `shared/src/` |
| `include/storage/blob_storage_backend.h` | `shared/include/` |
| `include/storage/blob_storage_manager.h` | `shared/include/` |
| `include/storage/encrypted_blob_backend.h` | `shared/include/` |
| `include/storage/blob_redundancy_manager.h` | `shared/include/` |
| `include/storage/zero_copy_blob_transfer.h` | `shared/include/` |
| `tests/user_storage_encrypted/` | `user_storage_encrypted/tests/` |

## Build

```bash
# With Azure support
cmake -B build -DTHEMISDB_SDK_DIR=/path/to/sdk -DTHEMISDB_STORAGE_AZURE=ON
# With S3 support
cmake -B build -DTHEMISDB_SDK_DIR=/path/to/sdk -DTHEMISDB_STORAGE_S3=ON
cmake --build build
```
