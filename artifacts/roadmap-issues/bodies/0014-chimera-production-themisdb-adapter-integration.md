### Context

This issue implements the roadmap item 'Production ThemisDB Adapter Integration' for the chimera domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Immediate (≤ v1.4.0) and targets milestone v1.1.0.

Primary detail section: Production ThemisDB Adapter Integration

### Goal

Deliver the scoped changes for Production ThemisDB Adapter Integration in src/chimera/ and complete the linked detail section in a release-ready state for v1.1.0.

### Detailed Scope

### Production ThemisDB Adapter Integration
**Priority:** High
**Target Version:** v1.1.0

`src/chimera/themisdb_adapter.cpp` has **7 confirmed stubs** (lines 105, 121, 136, 161, 192, 247, and the class-level comment "This is a stub implementation demonstrating the adapter pattern"). All data operations (`query`, `insert`, `update`, `delete`, `vector_search`, `graph_traverse`) return empty results or placeholder IDs without calling the actual ThemisDB API.

Replace stub implementation with full ThemisDB client integration.

**Implementation Notes:**
- `[ ]` Inject a `ThemisDBClient` (or `StorageEngine*`) into `ThemisDBAdapter` constructor; remove the in-process simulation maps.
- `[ ]` Replace stub `query()` (line 105 "Would execute actual query via ThemisDB API") with real AQL execution via `AQLRunner::execute()`.
- `[ ]` Replace stub `vector_search()` (line 192 "Return empty results") with `VectorIndex::search()` dispatch.
- `[ ]` Replace stub `graph_traverse()` (line 247 "Return empty path") with `GraphEngine::traverse()`.
- `[ ]` Replace stub `generateId()` (line 161) with UUID generation via `utils/uuid.h`.
- `[ ]` Add integration tests for the wired adapter against a live (in-process) ThemisDB instance.

**Implementation:**
```cpp
class ThemisDBAdapter : public IDatabaseAdapter {
private:
    std::unique_ptr<ThemisDBClient> client_;
    std::unique_ptr<ConnectionPool> pool_;
    
public:
    Result<bool> connect(
        const std::string& connection_string,
        const std::map<std::string, std::string>& options
    ) override {
        client_ = std::make_unique<ThemisDBClient>(connection_string);
        
        // Parse options
        size_t pool_size = parse_option(options, "pool_size", 10);
        pool_ = std::make_unique<ConnectionPool>(client_.get(), pool_size);
        
        // Establish connection
        return client_->connect() ? 
            Result<bool>::ok(true) : 
            Result<bool>::err(ErrorCode::CONNECTION_ERROR, "Failed to connect");
    }
    
    Result<RelationalTable> execute_query(
        const std::string& query,
        const std::vector<Scalar>& params
    ) override {
        auto conn = pool_->acquire();
        auto result = conn->executeAQL(query, params);
        return convert_result(result);
    }
};
```

**Expected Performance:**
- Query execution: Match native ThemisDB performance
- Connection pooling: Support 100+ concurrent connections
- Batch operations: 10K-100K inserts/second
- Vector search: 1-10ms for k=10 on 1M vectors

---

### Acceptance Criteria

- [ ] Inject a `ThemisDBClient` (or `StorageEngine*`) into `ThemisDBAdapter` constructor; remove the in-process simulation maps.
- [ ] Replace stub `query()` (line 105 "Would execute actual query via ThemisDB API") with real AQL execution via `AQLRunner::execute()`.
- [ ] Replace stub `vector_search()` (line 192 "Return empty results") with `VectorIndex::search()` dispatch.
- [ ] Replace stub `graph_traverse()` (line 247 "Return empty path") with `GraphEngine::traverse()`.
- [ ] Replace stub `generateId()` (line 161) with UUID generation via `utils/uuid.h`.
- [ ] Add integration tests for the wired adapter against a live (in-process) ThemisDB instance.
- [ ] Query execution: Match native ThemisDB performance
- [ ] Connection pooling: Support 100+ concurrent connections
- [ ] Batch operations: 10K-100K inserts/second
- [ ] Vector search: 1-10ms for k=10 on 1M vectors

### Relationships

- Roadmap row: #14 (🟠 High Priority — Immediate (≤ v1.4.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/chimera/FUTURE_ENHANCEMENTS.md#production-themisdb-adapter-integration
- Source key: roadmap:14:chimera:v1.1.0:production-themisdb-adapter-integration

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:14:chimera:v1.1.0:production-themisdb-adapter-integration -->
<!-- roadmap-ref: row=14;module=chimera;target=v1.1.0 -->
<!-- roadmap-detail: src/chimera/FUTURE_ENHANCEMENTS.md#production-themisdb-adapter-integration -->
