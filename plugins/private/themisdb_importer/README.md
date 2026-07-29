# themisdb_importer

Private plugin repository for ThemisDB data import plugins.

> **Source of Truth:** All implementation is maintained here.  
> The `plugins/private/themisdb_importer` submodule in the main ThemisDB repository
> points to this repo.

## Contained modules

| Module | Description |
|---|---|
| `mysql/` | MySQL / MariaDB importer (mysqldump SQL format) |
| `mongo/` | MongoDB importer (mongoexport JSON-Lines / JSON array) |
| `kafka/` | Apache Kafka stream importer |
| `s3/` | AWS S3 / S3-compatible object store importer |
| `shared/` | Common interfaces, schema validation, audit trail, CRDT, MDM |

## Repository Structure

```
themisdb_importer/
├── CMakeLists.txt
├── README.md
├── ROADMAP.md
├── ARCHITECTURE.md
├── SECURITY.md
├── AUDIT.md
├── FUTURE_ENHANCEMENTS.md
├── shared/
│   ├── CMakeLists.txt
│   ├── include/
│   │   ├── importer_interface.h       ← IImporter base interface
│   │   ├── importer_common.h
│   │   ├── importer_plugin.h
│   │   ├── importer_plugin_api.h
│   │   ├── importer_interfaces.h
│   │   ├── schema_validator.h
│   │   ├── schema_inference.h
│   │   ├── data_quality.h
│   │   ├── audit_trail.h
│   │   ├── conflict_resolver.h
│   │   ├── canonical_resolver.h
│   │   ├── temporal_support.h
│   │   ├── crdt_importer.h
│   │   ├── adaptive_import.h
│   │   ├── polyglot_mapper.h
│   │   └── entity_linker.h
│   └── src/
│       ├── schema_validator.cpp
│       ├── schema_inference.cpp
│       ├── data_quality.cpp
│       ├── audit_trail.cpp
│       ├── conflict_resolver.cpp
│       ├── canonical_resolver.cpp
│       ├── temporal_support.cpp
│       ├── crdt_importer.cpp
│       ├── adaptive_import.cpp
│       ├── polyglot_mapper.cpp
│       └── entity_linker.cpp
├── mysql/
│   ├── CMakeLists.txt
│   ├── plugin.json
│   ├── include/ └── mysql_importer.h
│   └── src/ └── mysql_importer.cpp
├── mongo/
│   ├── CMakeLists.txt
│   ├── plugin.json
│   ├── include/ └── mongo_importer.h
│   └── src/ └── mongo_importer.cpp
├── kafka/
│   ├── CMakeLists.txt
│   ├── plugin.json
│   ├── include/ └── kafka_importer.h
│   └── src/ └── kafka_importer.cpp
├── s3/
│   ├── CMakeLists.txt
│   ├── plugin.json
│   ├── include/ └── s3_importer.h
│   └── src/ └── s3_importer.cpp
├── tests/
│   ├── CMakeLists.txt
│   ├── README.md
│   └── test_*.cpp  (5 test files)
└── docs/
    ├── README.md
    ├── plugin_guide.md
    ├── POSTGRES_IMPORTER_V2.md
    ├── MDM_ARCHITECTURE.md
    ├── MDM_USER_GUIDE.md
    └── MDM_API_REFERENCE.md
```

## Migration from ThemisDB Monorepo

| ThemisDB source | Destination |
|---|---|
| `src/importers/mysql_importer.cpp` | `mysql/src/mysql_importer.cpp` |
| `include/importers/mysql_importer.h` | `mysql/include/mysql_importer.h` |
| `src/importers/mongo_importer.cpp` | `mongo/src/mongo_importer.cpp` |
| `include/importers/mongo_importer.h` | `mongo/include/mongo_importer.h` |
| `src/importers/kafka_importer.cpp` | `kafka/src/kafka_importer.cpp` |
| `include/importers/kafka_importer.h` | `kafka/include/kafka_importer.h` |
| `src/importers/s3_importer.cpp` | `s3/src/s3_importer.cpp` |
| `include/importers/s3_importer.h` | `s3/include/s3_importer.h` |
| `src/importers/{schema_validator,schema_inference,...}.cpp` | `shared/src/` |
| `include/importers/{importer_interface,schema_*,...}.h` | `shared/include/` |
| `tests/importers/test_*.cpp` | `tests/test_*.cpp` |
| `docs/importers/` | `docs/` |

## Build

```bash
cmake -B build \
    -DTHEMISDB_SDK_DIR=/path/to/sdk \
    -DTHEMISDB_IMPORTER_MYSQL=ON \
    -DTHEMISDB_IMPORTER_MONGO=ON \
    -DTHEMISDB_IMPORTER_KAFKA=ON \
    -DTHEMISDB_IMPORTER_S3=ON
cmake --build build
```
