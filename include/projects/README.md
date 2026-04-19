> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Projects Module - Header Documentation

## Module Purpose

The include-side Projects module currently exposes document-management contracts
for project-scoped document ingestion and retrieval.

## Current Header Surface

The active public API is:

```text
include/projects/DocumentManager/document_manager.h
```

Primary exported types:
- `DocumentManager`
- `DocumentMeta`
- `ChunkMeta`
- `ChunkingConfig`
- `UploadResult`
- `Status`

## Functional Scope (Current)

- Upload and process document blobs/text
- Chunk generation and metadata persistence
- Optional vector/graph index integration hooks
- Document/chunk retrieval and cascade delete

## Usage Example

```cpp
#include <projects/DocumentManager/document_manager.h>

using themis::projects::DocumentManager;
using themis::projects::ChunkingConfig;
```

## Notes

- Earlier references to `project_manager.h` were placeholders and are removed.
- Future project/workspace orchestration APIs should be documented when concrete
  headers are added under `include/projects/`.
- Name resolution within project scope
- Cross-project queries with explicit references

### Observability Module
- Project-level metrics and monitoring
- Per-project query performance tracking
- Resource usage attribution

## Design Patterns

### Repository Pattern
Projects act as repositories for database objects:
```cpp
class ProjectRepository {
    virtual Result<Table> getTable(const std::string& name) = 0;
    virtual Result<void> saveTable(const Table& table) = 0;
    virtual Result<std::vector<Table>> listTables() = 0;
};
```

### Unit of Work Pattern
Project transactions group multiple operations:
```cpp
ProjectTransaction tx = pm.beginTransaction();
tx.createTable("users", schema);
tx.createIndex("users_email_idx", indexDef);
tx.commit();  // Atomic
```

### Strategy Pattern
Different project storage strategies:
- LocalFileSystemStrategy
- RemoteGitStrategy
- DatabaseBackedStrategy
- HybridStrategy

## Performance Considerations

### Metadata Caching
- Cache project metadata in memory
- Invalidate on project modifications
- LRU eviction for inactive projects

### Lazy Loading
- Load project objects on demand
- Prefetch frequently accessed objects
- Background refresh of metadata

### Scalability
- Support thousands of projects per instance
- Efficient project switching (O(1))
- Concurrent project operations

## Thread Safety

Project operations are **thread-safe** with the following guarantees:
- **Read operations**: Concurrent reads allowed
- **Write operations**: Serialized per project
- **Project switching**: Thread-local context

## Known Limitations

1. **Project Name Length**: Limited to 255 characters
2. **Object Count**: Soft limit of 100,000 objects per project
3. **Snapshot Size**: Snapshots can be large for data-heavy projects
4. **Cross-Project Queries**: Limited support, requires explicit syntax
5. **Concurrent Modifications**: Last-write-wins for metadata conflicts

## Status

- **Version**: 1.5.x
- **Stability**: Beta
- **API Status**: Evolving (may change in future versions)

**Production Readiness:**
- ✅ Core project management: Stable
- ✅ Project switching: Stable
- ⚠️ Snapshots: Beta
- ⚠️ Import/Export: Beta
- 🚧 Cross-project queries: In development

## Dependencies

### Required
- Storage module (metadata storage)
- Security module (access control)

### Optional
- Query module (cross-project queries)
- Observability module (project metrics)

## Related Documentation

- [Storage Module](../storage/README.md) - Object persistence
- [Security Module](../security/README.md) - Access control
- [Query Module](../query/README.md) - Query execution context
- [Metadata Module](../metadata/README.md) - Schema information

## Version History

### v1.5.0 (Current)
- Basic project creation and management
- Project switching
- Object listing

### Planned v1.6.0
- Project snapshots
- Import/export functionality
- Project templates

### Planned v1.7.0
- Cross-project queries
- Project versioning with Git integration
- Collaborative features

## References

- **Repository Pattern**: Fowler, M. "Patterns of Enterprise Application Architecture"
- **Workspace Management**: Git workflow patterns
- **Multi-tenancy**: "Multi-Tenant Data Architecture" (Microsoft Azure Docs)

---

**Last Updated**: 2026-04-06
**Status**: Draft - Awaiting actual header file discovery
**Maintainer**: ThemisDB Team

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
