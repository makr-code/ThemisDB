# ThemisDB Modularization Guide (v1.4.0)

## Overview

Starting with v1.4.0, ThemisDB supports a modular build architecture that splits the monolithic `themis_core` library into focused, independently-built modules. This resolves critical issues:

- **Windows Build Problems**: Eliminates COFF symbol limit errors (>65,000 symbols)
- **Faster Build Times**: Parallel compilation and incremental rebuilds of individual modules
- **Feature Selectivity**: Optional modules can be excluded from builds
- **Improved Maintainability**: Clear separation of concerns and dependencies

## Build Modes

### Monolithic Build (Default)

The traditional single `themis_core` library containing all functionality:

```bash
cmake -B build -DTHEMIS_BUILD_MODULAR=OFF
cmake --build build
```

This mode is the default and maintains backward compatibility with v1.3.x.

### Modular Build (New in v1.4.0)

Separate libraries for each component:

```bash
cmake -B build -DTHEMIS_BUILD_MODULAR=ON
cmake --build build
```

## Module Architecture

### Core Modules (Always Built)

1. **themis_base** - Foundation layer
   - Core utilities (logging, serialization, tracing)
   - Cross-cutting concerns abstraction
   - Plugin system infrastructure
   - Hardware acceleration registry

2. **themis_storage** - Storage engine
   - RocksDB wrapper
   - Indexes (secondary, vector, spatial, adaptive)
   - Backup and PITR management
   - Schema management

3. **themis_query** - Query processing
   - Query engine and optimizer
   - AQL parser and translator
   - Analytics (OLAP, process mining, NLP)
   - Import/export functionality

4. **themis_security** - Security layer
   - Encryption and key management (Vault, PKI, HSM)
   - Authentication (JWT, Kerberos/GSSAPI)
   - RBAC and access control
   - PII detection and governance

5. **themis_transaction** - Transaction management
   - ACID transaction manager
   - Saga pattern support
   - Snapshot management
   - Change data capture (CDC)

6. **themis_network** - Network services
   - HTTP/gRPC servers
   - API handlers (REST endpoints)
   - Protocol support (WebSocket, MQTT, PostgreSQL wire)
   - Rate limiting and load shedding

### Optional Modules (Configurable)

7. **themis_sharding** - Distributed system
   - Horizontal scaling and sharding
   - Raft consensus
   - WAL replication
   - Cross-shard transactions
   
   Enable with: `-DTHEMIS_MODULE_SHARDING=ON`

8. **themis_llm** - LLM integration
   - LLM interaction store
   - Prompt management
   - Paged KV cache
   - RAG knowledge gap detector
   
   Enable with: `-DTHEMIS_MODULE_LLM=ON`

9. **themis_content** - Content processors
   - Content management
   - Text/image/office processors
   - MIME detection
   - Archive processing
   
   Enable with: `-DTHEMIS_MODULE_CONTENT=ON`

10. **themis_timeseries** - Time-series support
    - Time-series storage
    - Gorilla compression
    - Continuous aggregates
    - Retention policies
    
    Enable with: `-DTHEMIS_MODULE_TIMESERIES=ON`

11. **themis_graph** - Graph analytics
    - Graph indexes (temporal, property, process)
    - GNN embeddings
    - Graph query optimizer
    - Graph analytics
    
    Enable with: `-DTHEMIS_MODULE_GRAPH=ON`

12. **themis_geo** - Geospatial features
    - Geospatial indexing
    - CPU/GPU backends
    - EWKB support
    - Boost.Geometry integration
    
    Enable with: `-DTHEMIS_MODULE_GEO=ON`

## Module Dependencies

```
themis_base (foundation)
    ├── themis_storage (depends on: base, RocksDB)
    │   ├── themis_query (depends on: base, storage)
    │   ├── themis_transaction (depends on: base, storage)
    │   ├── themis_security (depends on: base, OpenSSL)
    │   ├── themis_timeseries (depends on: base, storage)
    │   ├── themis_graph (depends on: base, storage)
    │   └── themis_geo (depends on: base, storage, Boost.Geometry)
    │
    ├── themis_network (depends on: base, storage, query, transaction)
    │
    ├── themis_sharding (depends on: base, storage, security, transaction)
    │
    └── themis_llm (depends on: base, storage)
```

## Configuration Examples

### Minimal Build (No Optional Modules)

```bash
cmake -B build \
  -DTHEMIS_BUILD_MODULAR=ON \
  -DTHEMIS_MODULE_SHARDING=OFF \
  -DTHEMIS_MODULE_LLM=OFF \
  -DTHEMIS_MODULE_CONTENT=OFF \
  -DTHEMIS_MODULE_TIMESERIES=OFF \
  -DTHEMIS_MODULE_GRAPH=OFF \
  -DTHEMIS_MODULE_GEO=OFF
```

### Full Build (All Modules)

```bash
cmake -B build \
  -DTHEMIS_BUILD_MODULAR=ON \
  -DTHEMIS_MODULE_SHARDING=ON \
  -DTHEMIS_MODULE_LLM=ON \
  -DTHEMIS_MODULE_CONTENT=ON \
  -DTHEMIS_MODULE_TIMESERIES=ON \
  -DTHEMIS_MODULE_GRAPH=ON \
  -DTHEMIS_MODULE_GEO=ON
```

### Enterprise Build (Core + Sharding)

```bash
cmake -B build \
  -DTHEMIS_BUILD_MODULAR=ON \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DTHEMIS_MODULE_SHARDING=ON \
  -DTHEMIS_MODULE_LLM=OFF \
  -DTHEMIS_MODULE_CONTENT=OFF
```

## Export Macros

Each module uses platform-specific export macros defined in `include/themis/export.h`:

```cpp
#include <themis/export.h>

class THEMIS_STORAGE_API RocksDBWrapper {
    // ...
};
```

Available macros:
- `THEMIS_BASE_API`
- `THEMIS_STORAGE_API`
- `THEMIS_QUERY_API`
- `THEMIS_SECURITY_API`
- `THEMIS_TRANSACTION_API`
- `THEMIS_NETWORK_API`
- `THEMIS_SHARDING_API`
- `THEMIS_LLM_API`
- `THEMIS_CONTENT_API`
- `THEMIS_TIMESERIES_API`
- `THEMIS_GRAPH_API`
- `THEMIS_GEO_API`

## Migration from Monolithic to Modular

Existing code that links to `themis_core` will continue to work in modular mode. The build system automatically creates an interface library `themis_core` that links all enabled modules.

### For Library Users

No code changes required. Link to `themis_core` as before:

```cmake
target_link_libraries(my_app PRIVATE themis_core)
```

### For ThemisDB Developers

When adding new source files, add them to the appropriate module in `cmake/ModularBuild.cmake`:

```cmake
set(THEMIS_STORAGE_SOURCES
    ../src/storage/rocksdb_wrapper.cpp
    ../src/storage/your_new_file.cpp  # Add here
    # ...
)
```

## Performance Considerations

### Build Time

- **Monolithic**: Single large compilation, ~15-30 minutes full rebuild
- **Modular**: Parallel module compilation, ~10-20 minutes full rebuild
- **Incremental**: Only recompile changed modules (significant speedup)

### Runtime

- **Monolithic**: Single binary, slightly faster startup
- **Modular**: Multiple shared libraries, minimal overhead (<1% typically)

### Binary Size

- **Monolithic**: Single large binary (~50-150 MB)
- **Modular**: Total size similar, but distributed across multiple .so/.dll files

## Troubleshooting

### Symbol Visibility Issues

If you encounter undefined symbol errors with modular builds:

1. Ensure export macros are correctly applied to public APIs
2. Check that dependencies are properly declared in `themis_add_module()`
3. Verify that `THEMIS_BUILD_MODULAR=ON` is set consistently

### Missing Module Dependencies

If a module fails to link:

1. Check dependency order in `cmake/ModularBuild.cmake`
2. Ensure required system libraries are available
3. Verify vcpkg packages are installed

### Windows-Specific Issues

On Windows with MSVC:

- Ensure `WINDOWS_EXPORT_ALL_SYMBOLS` is enabled for each module
- Use `/bigobj` flag for large translation units (automatically applied)
- Disable `/GL` (IPO) for shared builds to avoid COFF issues

## Roadmap

### v1.4.0 (Current)
- ✅ Modular build infrastructure
- ✅ Export macro system
- ✅ Module dependency management
- ✅ Backward compatibility with monolithic builds

### v1.5.0 (Planned)
- Module-level testing isolation
- Plugin system for dynamic module loading
- Finer-grained module splitting (e.g., separate HTTP/gRPC servers)

### v2.0.0 (Future)
- Pure modular architecture (monolithic deprecated)
- Language bindings per module
- Microservice deployment support

## References

- [ModularBuild.cmake](../cmake/ModularBuild.cmake) - Module definitions
- [export.h](../include/themis/export.h) - Export macros
- [CMakeLists.txt](../cmake/CMakeLists.txt) - Build system integration
