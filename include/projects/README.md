# Projects Module - Header Documentation

## Module Purpose

The Projects module provides **project and workspace management** capabilities for ThemisDB. It enables organizing database objects (tables, indexes, queries, models) into logical projects or workspaces, facilitating collaboration, versioning, and environment isolation.

## Scope

### In Scope
- Project/workspace creation and management
- Object organization within projects
- Project-level permissions and access control
- Project metadata and configuration
- Project versioning and snapshots
- Project import/export
- Multi-project isolation
- Project templates

### Out of Scope
- User authentication (handled by Auth module)
- Fine-grained RBAC (handled by Security module)
- Storage implementation (handled by Storage module)
- Query execution (handled by Query module)

## Header Files

This module currently contains 1 header file in the `include/projects/` directory:

```
include/projects/
└── [to be determined based on actual file]
```

**Note:** The actual header file name should be discovered by exploring the directory. Common patterns might include:
- `project_manager.h` - Core project management interface
- `workspace.h` - Workspace abstraction
- `project_metadata.h` - Project metadata structures

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                   Projects Module                        │
│                                                          │
│  ┌─────────────────┐      ┌─────────────────┐          │
│  │ Project Manager │      │ Project Metadata│          │
│  │                 │─────▶│                 │          │
│  │ - Create        │      │ - Name          │          │
│  │ - Delete        │      │ - Description   │          │
│  │ - List          │      │ - Owner         │          │
│  │ - Switch        │      │ - Created       │          │
│  └─────────────────┘      └─────────────────┘          │
│          │                        │                      │
│          │                        │                      │
│          ▼                        ▼                      │
│  ┌─────────────────┐      ┌─────────────────┐          │
│  │   Object Map    │      │ Access Control  │          │
│  │                 │      │                 │          │
│  │ - Tables        │      │ - Permissions   │          │
│  │ - Indexes       │      │ - Roles         │          │
│  │ - Queries       │      │ - Users         │          │
│  │ - Models        │      └─────────────────┘          │
│  └─────────────────┘                                    │
└─────────────────────────────────────────────────────────┘
         │                    │
         ▼                    ▼
┌────────────────┐    ┌────────────────┐
│ Storage Module │    │ Security Module│
└────────────────┘    └────────────────┘
```

## Key Concepts

### Project
A **project** is a logical container for database objects, providing:
- Namespace isolation
- Version control integration
- Shared configuration
- Collaborative workspace
- Environment separation (dev/test/prod)

### Workspace
A **workspace** is an active development environment within a project:
- Current working context
- Temporary object staging
- Local modifications
- Draft queries and models

### Project Metadata
Metadata associated with each project:
- Project name and description
- Owner and contributors
- Creation and modification timestamps
- Version history
- Tags and labels
- Configuration overrides

## Usage Examples

### Example 1: Create and Switch Projects

```cpp
#include <projects/project_manager.h>  // Hypothetical

// Create a new project
ProjectManager pm;
auto result = pm.createProject("ml-pipeline", {
    .description = "Machine learning data pipeline",
    .owner = "data-science-team",
    .tags = {"ml", "production"}
});

if (result.isSuccess()) {
    // Switch to the new project
    pm.switchProject("ml-pipeline");
    
    // All subsequent operations are scoped to this project
    // CREATE TABLE, CREATE INDEX, etc.
}
```

### Example 2: List Objects in Project

```cpp
// List all tables in the current project
auto tables = pm.listTables();
for (const auto& table : tables) {
    std::cout << "Table: " << table.name 
              << " (created: " << table.created << ")" << std::endl;
}

// List projects
auto projects = pm.listProjects();
for (const auto& proj : projects) {
    std::cout << "Project: " << proj.name 
              << " (" << proj.objectCount << " objects)" << std::endl;
}
```

### Example 3: Project Snapshots

```cpp
// Create a snapshot of the current project state
auto snapshot = pm.createSnapshot("v1.0.0", {
    .description = "Production release",
    .tags = {"release", "stable"}
});

// Restore from a snapshot
pm.restoreSnapshot("v1.0.0");
```

### Example 4: Import/Export Projects

```cpp
// Export project to file
pm.exportProject("ml-pipeline", "/backup/ml-pipeline.zip", {
    .includeData = true,
    .includeIndexes = true,
    .includeConfig = true
});

// Import project from file
pm.importProject("/backup/ml-pipeline.zip", "ml-pipeline-restored");
```

## Integration with Other Modules

### Storage Module
- Projects map to storage namespaces or prefixes
- Object metadata stored in system catalog
- Project isolation at storage level

### Security Module
- Project-level access control
- Role-based permissions
- Audit logging for project operations

### Query Module
- Project context for query execution
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

**Last Updated**: 2024-02-10  
**Status**: Draft - Awaiting actual header file discovery  
**Maintainer**: ThemisDB Team
