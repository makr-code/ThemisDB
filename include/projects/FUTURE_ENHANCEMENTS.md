# Projects Module - Future Enhancements

## Scope

- API-level enhancements to `include/projects/` headers — public C++ interfaces for project lifecycle management
- Project versioning interface: `ProjectVersioning` with immutable snapshot creation, retrieval, and comparison
- Collaboration hook API: `CollaborationHook` callback for real-time change notifications and optimistic locking
- Diff/merge API: `ProjectDiff` and `ProjectMerge` returning structured `DeltaSet`, not raw text
- Project lifecycle management: `ProjectLifecycle` transitions (create → active → archived → deleted)
- Template instantiation interface: `ProjectTemplate` factory with typed enum and `TemplateOptions`

## Design Constraints

- [ ] Project versioning uses immutable snapshots; snapshots are append-only and never mutated after creation
- [ ] Collaboration API is thread-safe; all shared state is protected by `std::shared_mutex` or equivalent
- [ ] Diff API returns structured `DeltaSet` (typed field-level changes), never a raw text diff
- [ ] Project lifecycle transitions are atomic and logged to an append-only audit trail
- [ ] Cross-project query API requires explicit project references; no implicit ambient project context
- [ ] Template instantiation must validate schema before returning a `Result<Project>`

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `ProjectVersioning` | `include/projects/DocumentManager/document_manager.h` | Keep snapshot/version API changes aligned with existing document-management contracts |
| `CollaborationHook` | `CollaborationManager` | Thread-safe; invoked on `shareProject()` and change events |
| `ProjectDiff` | Migration tooling, CI pipelines | Returns `DeltaSet`; field-level granularity |
| `ProjectMerge` | Git-style integration backend | Requires common ancestor `SnapshotId` |
| `ProjectLifecycle` | Admin API, archival tooling | Atomic state transitions with audit log entry |

This document outlines planned enhancements, optimizations, and research areas for the ThemisDB Projects module.

## Planned Features

### 1. Git Integration for Project Versioning (v1.6.0)

**Priority**: High  
**Status**: Planned

Integrate Git for project version control:

```cpp
class GitProjectBackend : public ProjectBackend {
public:
    // Initialize Git repository for project
    Result<void> init(const std::string& projectName);
    
    // Commit project changes
    Result<CommitHash> commit(const std::string& message);
    
    // Create branch for experimental work
    Result<void> createBranch(const std::string& branchName);
    
    // Merge branches
    Result<void> merge(const std::string& sourceBranch);
    
    // Push to remote repository
    Result<void> push(const std::string& remote);
    
    // Pull from remote repository
    Result<void> pull(const std::string& remote);
};
```

**Benefits:**
- Full version history for projects
- Branch-based development workflows
- Collaboration via Git hosting platforms
- Code review integration

**Implementation:**
- Use libgit2 for Git operations
- Store metadata as JSON files
- Track schema changes in DDL files
- Binary diff for data changes (optional)

---

### 2. Project Templates (v1.6.0)

**Priority**: Medium  
**Status**: Planned

Pre-configured project templates for common use cases:

```cpp
enum class ProjectTemplate {
    EMPTY,                    // Blank project
    WEB_APPLICATION,          // Web app backend (users, sessions, etc.)
    MACHINE_LEARNING,         // ML pipeline (datasets, models, features)
    ANALYTICS,                // Analytics warehouse (facts, dimensions)
    TIME_SERIES,              // Time-series data (sensors, metrics)
    GRAPH_ANALYTICS,          // Graph database (nodes, edges)
    DOCUMENT_STORE,           // Document-oriented (collections)
    CUSTOM                    // User-defined template
};

class ProjectManager {
    Result<Project> createFromTemplate(
        const std::string& projectName,
        ProjectTemplate templateType,
        const TemplateOptions& options = {}
    );
};
```

**Built-in Templates:**
- **Web Application**: Users, sessions, posts, comments
- **ML Pipeline**: Datasets, features, models, experiments
- **Analytics**: Star schema with facts and dimensions
- **Time-Series**: Sensor data, metrics, alerts
- **Graph**: Social network, knowledge graph
- **Document Store**: Collections with schemas

---

### 3. Collaborative Features (v1.7.0)

**Priority**: High  
**Status**: Research

Real-time collaboration on projects:

```cpp
class CollaborationManager {
public:
    // Share project with users
    Result<void> shareProject(
        const std::string& projectName,
        const std::vector<User>& users,
        Permission permission
    );
    
    // Real-time notifications
    void subscribe(ProjectEventCallback callback);
    
    // Concurrent editing with conflict resolution
    Result<void> lockObject(const std::string& objectName);
    Result<void> unlockObject(const std::string& objectName);
    
    // Change tracking
    std::vector<Change> getChanges(
        const std::string& projectName,
        Timestamp since
    );
};
```

**Features:**
- Project sharing with granular permissions
- Real-time notifications (WebSocket)
- Optimistic locking for objects
- Change feed for auditing
- Collaborative query editor

---

### 4. Project Environments (v1.6.0)

**Priority**: Medium  
**Status**: Planned

Separate environments within projects:

```cpp
enum class Environment {
    DEVELOPMENT,
    TESTING,
    STAGING,
    PRODUCTION
};

class EnvironmentManager {
    // Create environment
    Result<void> createEnvironment(
        const std::string& projectName,
        Environment env,
        const EnvironmentConfig& config
    );
    
    // Promote between environments
    Result<void> promote(
        const std::string& projectName,
        Environment from,
        Environment to
    );
    
    // Environment-specific configuration
    Result<void> setConfig(
        const std::string& projectName,
        Environment env,
        const ConfigMap& config
    );
};
```

**Benefits:**
- Isolate development and production data
- Test schema changes safely
- Gradual rollout of changes
- Environment-specific configurations

---

### 5. Cross-Project Queries (v1.7.0)

**Priority**: High  
**Status**: In Development

Query across multiple projects:

```aql
-- Explicit project reference
FOR user IN project1.users
  FOR order IN project2.orders
    FILTER order.user_id == user.id
    RETURN {user: user, order: order}

-- Project alias
USE project1 AS p1, project2 AS p2
FOR user IN p1.users
  FOR order IN p2.orders
    FILTER order.user_id == user.id
    RETURN {user: user, order: order}
```

**Challenges:**
- Permission checking across projects
- Performance optimization
- Transaction semantics
- Query planning

---

### 6. Project Quotas and Limits (v1.6.0)

**Priority**: Medium  
**Status**: Planned

Resource limits per project:

```cpp
struct ProjectQuota {
    size_t maxTables = 1000;
    size_t maxIndexes = 10000;
    size_t maxStorageBytes = 100 * 1024 * 1024 * 1024ULL;  // 100 GB
    size_t maxQueriesPerSecond = 1000;
    size_t maxConcurrentConnections = 100;
};

class QuotaManager {
    Result<void> setQuota(
        const std::string& projectName,
        const ProjectQuota& quota
    );
    
    Result<ProjectUsage> getUsage(const std::string& projectName);
    
    bool isQuotaExceeded(
        const std::string& projectName,
        ResourceType resource
    );
};
```

**Benefits:**
- Prevent resource abuse
- Fair resource allocation
- Cost tracking and billing
- Capacity planning

---

### 7. Project Analytics Dashboard (v1.7.0)

**Priority**: Low  
**Status**: Planned

Dashboard for project insights:

```cpp
struct ProjectAnalytics {
    size_t totalObjects;
    size_t totalStorageBytes;
    size_t queriesExecuted;
    double avgQueryLatency;
    std::vector<TopQuery> slowestQueries;
    std::vector<TopTable> largestTables;
    std::map<User, size_t> userActivity;
    std::map<Timestamp, size_t> activityTimeline;
};

class AnalyticsDashboard {
    Result<ProjectAnalytics> getAnalytics(
        const std::string& projectName,
        TimeRange range
    );
};
```

**Metrics:**
- Storage usage trends
- Query performance
- User activity
- Object growth
- Resource utilization

---

### 8. Project Archival (v1.8.0)

**Priority**: Low  
**Status**: Research

Archive inactive projects:

```cpp
class ArchivalManager {
    // Archive project to cold storage
    Result<void> archiveProject(
        const std::string& projectName,
        const ArchivalPolicy& policy
    );
    
    // Restore archived project
    Result<void> restoreProject(
        const std::string& projectName,
        const RestoreOptions& options
    );
    
    // List archived projects
    std::vector<ArchivedProject> listArchivedProjects();
};
```

**Features:**
- Compress project data
- Move to cold storage (S3 Glacier, Azure Archive)
- Quick restore capability
- Partial restore (metadata only)

---

### 9. Project Migrations (v1.7.0)

**Priority**: Medium  
**Status**: Planned

Schema migration framework:

```cpp
class MigrationManager {
    // Define migration
    void addMigration(
        const std::string& name,
        MigrationFunction up,
        MigrationFunction down
    );
    
    // Apply migrations
    Result<void> migrate(const std::string& projectName);
    
    // Rollback migration
    Result<void> rollback(
        const std::string& projectName,
        size_t steps = 1
    );
    
    // Migration status
    std::vector<MigrationStatus> getMigrationStatus(
        const std::string& projectName
    );
};
```

**Example Migration:**
```cpp
migration.add("add_email_verification", 
    // Up
    [](Project& proj) {
        proj.execute("ALTER TABLE users ADD COLUMN email_verified BOOLEAN DEFAULT FALSE");
        proj.execute("CREATE INDEX idx_email_verified ON users(email_verified)");
    },
    // Down
    [](Project& proj) {
        proj.execute("DROP INDEX idx_email_verified");
        proj.execute("ALTER TABLE users DROP COLUMN email_verified");
    }
);
```

---

### 10. Project Cloning and Forking (v1.6.0)

**Priority**: Medium  
**Status**: Planned

Clone projects for experimentation:

```cpp
class ProjectCloner {
    // Clone entire project
    Result<Project> cloneProject(
        const std::string& sourceProject,
        const std::string& targetProject,
        const CloneOptions& options
    );
    
    // Fork project (shallow copy)
    Result<Project> forkProject(
        const std::string& sourceProject,
        const std::string& targetProject
    );
};

struct CloneOptions {
    bool includeData = true;
    bool includeIndexes = true;
    bool includeUsers = false;
    std::vector<std::string> excludeTables;
};
```

**Use Cases:**
- Testing schema changes
- Creating sandboxes
- Training environments
- A/B testing

---

## Performance Optimizations

### 1. Metadata Caching Strategy

**Current**: Simple in-memory cache  
**Target**: Multi-tier cache with invalidation

```cpp
class MetadataCache {
    // L1: Thread-local cache
    thread_local static std::unordered_map<std::string, ProjectMetadata> l1Cache;
    
    // L2: Process-wide shared cache
    static std::shared_ptr<LRUCache<std::string, ProjectMetadata>> l2Cache;
    
    // L3: Distributed cache (Redis)
    static std::shared_ptr<RedisClient> l3Cache;
    
    ProjectMetadata get(const std::string& projectName);
    void invalidate(const std::string& projectName);
};
```

**Benefits:**
- Sub-microsecond L1 cache access
- Reduced database queries
- Scalable across instances

---

### 2. Lazy Object Loading

**Current**: Eager loading of all objects  
**Target**: Load objects on first access

```cpp
class LazyProjectObjects {
    std::unordered_map<std::string, std::future<Table>> tables;
    
    Table& getTable(const std::string& name) {
        if (!tables.contains(name)) {
            tables[name] = std::async([this, name]() {
                return loadTableFromStorage(name);
            });
        }
        return tables[name].get();
    }
};
```

---

### 3. Parallel Project Operations

**Current**: Sequential operations  
**Target**: Parallel execution where safe

```cpp
// Parallel project export
Result<void> exportProjectParallel(
    const std::string& projectName,
    const std::string& outputPath
) {
    std::vector<std::future<void>> tasks;
    
    // Export tables in parallel
    for (const auto& table : listTables()) {
        tasks.push_back(std::async([table, outputPath]() {
            exportTable(table, outputPath);
        }));
    }
    
    // Wait for all tasks
    for (auto& task : tasks) {
        task.wait();
    }
}
```

---

## Refactoring Opportunities

### 1. Extract Project Storage Interface

Separate storage concerns from project logic:

```cpp
class ProjectStorage {
public:
    virtual Result<Project> load(const std::string& name) = 0;
    virtual Result<void> save(const Project& project) = 0;
    virtual Result<void> delete(const std::string& name) = 0;
    virtual Result<std::vector<Project>> list() = 0;
};

class RocksDBProjectStorage : public ProjectStorage { /*...*/ };
class PostgreSQLProjectStorage : public ProjectStorage { /*...*/ };
class FileSystemProjectStorage : public ProjectStorage { /*...*/ };
```

---

### 2. Improve Error Handling

Use Result<T> consistently:

```cpp
// Before
Project* getProject(const std::string& name);  // Returns nullptr on error

// After
Result<Project> getProject(const std::string& name);
```

---

### 3. Modularize Project Components

Break into smaller, focused classes:

```cpp
// project_manager.h -> Multiple focused headers
#include <projects/project_creator.h>
#include <projects/project_switcher.h>
#include <projects/project_exporter.h>
#include <projects/project_analyzer.h>
```

---

## Known Issues

### 1. Project Name Conflicts
**Issue**: No validation for duplicate project names  
**Workaround**: Manual checking before creation  
**Fix**: Add unique constraint in v1.6.0

### 2. Large Project Export
**Issue**: Export fails for projects >10GB  
**Workaround**: Export tables individually  
**Fix**: Streaming export in v1.6.0

### 3. Cross-Project Query Performance
**Issue**: Slow for large result sets  
**Workaround**: Use smaller batch sizes  
**Fix**: Query optimization in v1.7.0

---

## Research Areas

### 1. Distributed Projects

Multi-region project coordination:
- Raft-based consensus for metadata
- CRDT for conflict-free replication
- Geo-distributed project sharding

### 2. Project-as-Code

Declarative project definitions:
```yaml
# project.yaml
name: ml-pipeline
description: Machine learning data pipeline
tables:
  - name: datasets
    schema:
      - id: UUID PRIMARY KEY
      - name: STRING
      - created_at: TIMESTAMP
  - name: models
    schema:
      - id: UUID PRIMARY KEY
      - dataset_id: UUID REFERENCES datasets(id)
      - accuracy: FLOAT
```

### 3. AI-Powered Project Insights

Use ML to provide insights:
- Unused table detection
- Query optimization suggestions
- Schema evolution recommendations
- Anomaly detection

---

## Migration Paths

### v1.5.x → v1.6.0

**Breaking Changes:**
- `ProjectManager::listObjects()` now returns `Result<std::vector<Object>>`
- Project names must be lowercase (auto-converted)

**Migration Steps:**
1. Update error handling to use Result<T>
2. Convert project names to lowercase
3. Regenerate project metadata cache

### v1.6.x → v1.7.0

**Breaking Changes:**
- Cross-project queries require explicit syntax
- Project metadata format changed (v2)

**Migration Steps:**
1. Update cross-project queries to use `project.table` syntax
2. Run metadata migration tool: `themisdb migrate-projects`
3. Test in staging environment

---

## Community Requests

Track community feature requests:

| Feature | Votes | Status | Target Version |
|---------|-------|--------|----------------|
| Git integration | 156 | Planned | v1.6.0 |
| Project templates | 89 | Planned | v1.6.0 |
| Collaborative editing | 67 | Research | v1.7.0 |
| Project analytics | 54 | Planned | v1.7.0 |
| Multi-region projects | 43 | Research | v2.0.0 |

---

**Last Updated**: 2026-04-06  
**Contributors**: ThemisDB Team, Community  
**Status**: Living Document

---

## Test Strategy

- Unit tests for `ProjectVersioning`: create snapshots, verify immutability, compare two snapshots via `ProjectDiff`
- Unit tests for `CollaborationHook`: register callback, mutate project from 2 threads, assert callback invoked once per change
- Unit tests for `ProjectDiff`: diff two snapshots with known field changes; assert `DeltaSet` contains exactly expected deltas
- Integration tests: create project from template, apply migrations, archive; verify lifecycle audit log entries
- Permission tests: attempt `shareProject()` without user consent; assert `std::errc::permission_denied` is returned
- Performance tests: snapshot a 10,000-document project and assert completion ≤ 100 ms

## Performance Targets

- Project snapshot creation ≤ 100 ms for projects with up to 10,000 documents
- Diff computation between two snapshots ≤ 50 ms for projects with up to 1,000 changed fields
- Collaboration hook invocation latency ≤ 5 ms from write commit to callback delivery
- Project template instantiation ≤ 200 ms including schema validation
- Metadata cache L1 hit latency ≤ 1 µs; L2 shared cache ≤ 10 µs
- Cross-project query planning overhead ≤ 20 ms for up to 5 project references

## Security / Reliability

- Project access is gated by RBAC; all `ProjectManager` methods validate caller role before proceeding
- Project sharing API requires explicit user consent token; sharing without consent returns an error
- Snapshot data is checksummed (SHA-256) at creation and verified at read time
- Audit log entries are append-only and tamper-evident; deletion requires admin privilege
- Cross-project queries validate read permission on each referenced project independently
