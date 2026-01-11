---
title: "Implement Schema Manager for Database Self-Awareness"
labels: enhancement, self-awareness, agentic-ai, mcp, priority-high, v1.5
milestone: v1.5.0
---

## 📋 Summary

Implement a comprehensive `SchemaManager` class that enables ThemisDB to introspect its own schema, providing the foundation for Agentic AI self-awareness features. This is the **critical path** for all self-awareness functionality.

**Type**: Feature Implementation  
**Priority**: HIGH  
**Effort**: 2-4 weeks (~500 LOC)  
**Status**: ❌ Not Implemented (Verified)

## 🔍 Verification

**Checked**:
- ✅ File `include/metadata/schema_manager.h` does not exist
- ✅ File `src/metadata/schema_manager.cpp` does not exist  
- ✅ No `class SchemaManager` found in codebase
- ✅ MCP Server returns empty stubs (see `src/server/mcp_server.cpp:toolGetSchema()`)

**Evidence of Gap**:
```cpp
// Current implementation in src/server/mcp_server.cpp (line ~350)
json McpServer::toolGetSchema(const json& args) {
    return {
        {"nodes", json::array()},  // ❌ Empty!
        {"edges", json::array()},   // ❌ Empty!
        {"message", "Schema discovery requires full query engine integration"}
    };
}
```

## 🎯 Problem Statement

ThemisDB currently has **no ability to introspect its own schema**. When an LLM or user asks:
- "What data do you store?"
- "What tables exist?"  
- "How are the data structured?"
- "What is your purpose?"

The database **cannot answer** because schema discovery is not implemented.

## 📚 Research Background

- **Primary Research**: [`docs/research/AGENTIC_AI_SELF_AWARENESS_RESEARCH.md`](../../docs/research/AGENTIC_AI_SELF_AWARENESS_RESEARCH.md)
- **Implementation Checklist**: [`docs/research/IMPLEMENTATION_CHECKLIST.md`](../../docs/research/IMPLEMENTATION_CHECKLIST.md)
- **Code Examples**: [`docs/research/AGENTIC_AI_IMPLEMENTATION_EXAMPLE.md`](../../docs/research/AGENTIC_AI_IMPLEMENTATION_EXAMPLE.md)
- **MCP Protocol**: [`docs/en/apis/MCP_PROTOCOL_SUPPORT.md`](../../docs/en/apis/MCP_PROTOCOL_SUPPORT.md)

From research summary (Phase 1 in checklist):
> **SchemaManager** ist die Basis für alle Self-Awareness-Features. Die Implementation ist der **kritische Pfad**.

## 🏗️ Proposed Architecture

```
┌─────────────────────────────────────────┐
│         SchemaManager                    │
├─────────────────────────────────────────┤
│ Public API:                              │
│  • getAllTables() → vector<TableSchema>  │
│  • getTable(name) → TableSchema          │
│  • getAllRelationships() → vector<...>   │
│  • getDatabaseMetadata() → Metadata      │
│  • toJSON() → json                       │
├─────────────────────────────────────────┤
│ Cache System:                            │
│  • table_cache_ (std::map)               │
│  • last_refresh_ (timestamp)             │
│  • refresh_interval_seconds = 60         │
│  • mutex for thread-safety               │
└────────────┬────────────────────────────┘
             │
             ├─→ RocksDB (Key Scanning)
             ├─→ SecondaryIndexManager (Index Metadata)
             └─→ BaseEntity (Property Type Detection)
```

## 📝 Implementation Tasks

### Milestone 1: Core SchemaManager (Week 1-2)

- [ ] **Task 1.1**: Create header file
  - [ ] File: `include/metadata/schema_manager.h`
  - [ ] Define structures: `PropertyInfo`, `TableSchema`, `RelationshipSchema`, `DatabaseMetadata`
  - [ ] Define public API methods
  - [ ] Add cache member variables
  
- [ ] **Task 1.2**: Create implementation file
  - [ ] File: `src/metadata/schema_manager.cpp`
  - [ ] Implement RocksDB key scanning for table discovery
  - [ ] Implement property type detection from BaseEntity
  - [ ] Implement index metadata collection
  - [ ] Implement cache refresh logic (60s TTL)
  - [ ] Implement thread-safe cache access

- [ ] **Task 1.3**: JSON Export
  - [ ] Implement `toJSON()` - full schema export
  - [ ] Implement `tableToJSON(name)` - single table export
  - [ ] Implement `getDatabaseMetadata()` - statistics

- [ ] **Task 1.4**: CMake Integration
  - [ ] Add to `src/CMakeLists.txt`
  - [ ] Link dependencies: RocksDB, nlohmann_json

### Milestone 2: Testing (Week 2-3)

- [ ] **Unit Tests** (`tests/test_schema_manager.cpp`)
  - [ ] Test schema discovery with mock RocksDB
  - [ ] Test cache refresh mechanism
  - [ ] Test property type detection
  - [ ] Test JSON serialization
  - [ ] Test thread safety

- [ ] **Integration Tests**
  - [ ] Test with real RocksDB instance
  - [ ] Test with multiple tables and relationships
  - [ ] Test performance with large schemas (100+ tables)
  - [ ] Test cache hit/miss scenarios

### Milestone 3: Documentation (Week 3-4)

- [ ] Update API documentation
- [ ] Add usage examples
- [ ] Update `docs/en/apis/HTTP_API_REFERENCE.md`
- [ ] Update research status documents

## 🔗 Dependencies & Related Issues

### Blocks (Depends on this issue)
- Issue #2: REST API Schema Endpoints
- Issue #3: MCP Full Integration  
- Issue #4: Natural Language Self-Awareness

### Depends On
- None (can be implemented independently)

### Related
- [`docs/research/README.md`](../../docs/research/README.md) - Research summary
- MCP Server stub in `src/server/mcp_server.cpp`

## 📊 Success Criteria

### Technical Metrics
- ✅ Discovery time < 100ms for typical schemas
- ✅ Cache hit rate > 90%
- ✅ Memory overhead < 50 MB for 100 tables
- ✅ Thread-safe operations
- ✅ Test coverage > 90%

### Functional Requirements
- ✅ All tables correctly discovered from RocksDB
- ✅ All property types accurately detected
- ✅ Index information collected
- ✅ JSON export is valid and complete
- ✅ Cache mechanism prevents expensive re-scans

### Quality Gates
- ✅ All unit tests passing
- ✅ All integration tests passing
- ✅ Code review approved
- ✅ Documentation complete
- ✅ No memory leaks (valgrind clean)

## 💡 Implementation Guidelines

1. **Thread Safety**: Use `std::shared_mutex` for read-heavy cache access
2. **Performance**: Lazy initialization - only scan on first access
3. **Error Handling**: Graceful degradation if RocksDB access fails
4. **Logging**: Use spdlog with DEBUG level for verbose output
5. **Memory Management**: Clear cache on refresh to prevent leaks
6. **Testing**: Test with empty DB, small DB (10 tables), large DB (100+ tables)

## 🔮 Future Enhancements (Out of Scope)

After v1.5.0, consider:
- Real-time schema change notifications
- Schema diff/comparison utilities
- Schema export to GraphML/Cypher formats
- Support for view schemas
- Stored procedure introspection

## 📅 Timeline Estimate

| Milestone | Duration | Deliverable |
|-----------|----------|-------------|
| Core Implementation | 1-2 weeks | SchemaManager class |
| Testing | 1 week | Unit + Integration tests |
| Documentation | 3-4 days | Updated docs |
| **Total** | **2-4 weeks** | **Feature complete** |

## ✅ Definition of Done

- [ ] `SchemaManager` class fully implemented
- [ ] All tables correctly discovered from RocksDB  
- [ ] Property types accurately detected
- [ ] Index metadata collected
- [ ] Cache mechanism working
- [ ] Unit tests > 90% coverage, all passing
- [ ] Integration tests all passing
- [ ] Thread-safety verified
- [ ] Memory leaks checked (valgrind)
- [ ] Code review approved by 2+ reviewers
- [ ] Documentation updated
- [ ] CHANGELOG.md entry added

---

**Created**: 2026-01-11  
**Research Completed**: 2026-01-11  
**Verified Missing**: 2026-01-11  
**Target Version**: v1.5.0
