<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Future Enhancements — Maintenance Module (Public Headers)

> See also: `../../src/maintenance/FUTURE_ENHANCEMENTS.md`

## Planned Interface Enhancements

### DAG-Based Task Dependencies (Target: v1.2.0)

**Scope:** Extend `MaintenanceTaskDependency` to support a full directed acyclic graph of task prerequisites.

**Required Interface Change:**
```cpp
struct MaintenanceTaskDependency {
    std::string task_id;
    std::vector<std::string> depends_on;  // DAG edges
    bool allow_parallel;
};
```

**Design Constraints:** Cycle detection required at schedule registration time; topological sort for execution ordering.

### RocksDB Schedule Persistence (Target: v1.1.0)

**Scope:** `MaintenanceScheduleStore` gains a `persist()` / `load()` interface backed by RocksDB, surviving process restarts.

**Required Interface Change:**
```cpp
class MaintenanceScheduleStore {
    virtual bool persist() = 0;
    virtual bool load() = 0;
};
```

### Force-Run Override (Target: v1.1.0)

**Scope:** `OrchestratorJob` gains a `force` flag to bypass time-window restrictions for immediate execution.

### Compaction Manager Integration (Target: v1.2.0)

**Scope:** `StorageCompactionHandler` wired to `CompactionManager::triggerCompaction()`; currently a stub.
