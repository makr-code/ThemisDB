# AQL DDL Implementation Roadmap

<!-- Status: [x] done | [~] in progress | [ ] open | [I] issue | [P] PR | [?] blocked -->

**Status:** ✅ COMPLETE — Phases 1–4 Delivered (2026-07-22)
**Target Release:** v2.0.0 (Q3 2026)
**Owner:** query module / Team B
**Last Updated:** 2026-07-27

---

## Executive Summary

Full DDL (Data Definition Language) support in AQL for **CREATE COLLECTION, DROP COLLECTION,
CREATE INDEX, DROP INDEX, CREATE VIEW, DROP VIEW** statements. Enables schema management
directly via AQL queries, replacing out-of-band API calls for collection and index lifecycle.

---

## Current State

| Feature | Status | Evidence |
|---------|--------|----------|
| CREATE COLLECTION | ✅ Implemented | `src/query/ddl_executor.cpp` + `include/query/ddl_executor.h` |
| DROP COLLECTION | ✅ Implemented | `src/query/ddl_executor.cpp` |
| CREATE INDEX (HASH, SKIPLIST, GEO, VECTOR, FULLTEXT) | ✅ Implemented | `src/query/ddl_executor.cpp` |
| DROP INDEX | ✅ Implemented | `src/query/ddl_executor.cpp` |
| CREATE VIEW | ✅ Implemented | `src/query/ddl_executor.cpp` |
| DROP VIEW | ✅ Implemented | `src/query/ddl_executor.cpp` |
| Parser token wiring (CREATE, DROP, COLLECTION, INDEX, VIEW, UNIQUE, SPARSE) | ✅ Implemented | `src/query/aql_parser.cpp` |
| DDL validation and conflict detection | ✅ Implemented | `src/query/aql_mutation_validator.cpp` |
| Tests (32 tests) | ✅ Implemented | `tests/aql/` |

---

## Delivered Phases

### Phase 1: Parser Extension — ✅ COMPLETE (2026-07-22)
- [x] Added DDL tokens: CREATE, DROP, COLLECTION, INDEX, VIEW, UNIQUE, SPARSE, GEO, FULLTEXT, VECTOR
- [x] Extend `ASTNodeType` enum: `CreateCollectionNode`, `CreateIndexNode`, `CreateViewNode`, `DropNode`
- [x] Implement `parseCreateCollection()`, `parseCreateIndex()`, `parseCreateView()`, `parseDrop()`
- [x] AST structure for index types (HASH, SKIPLIST, GEO, VECTOR, FULLTEXT)

### Phase 2: Metadata & Catalog — ✅ COMPLETE (2026-07-22)
- [x] Extended `CatalogManager` with DDL execution entry points
- [x] Index registry for tracking created indexes
- [x] Collection metadata persistence via RocksDB

### Phase 3: Execution & Validation — ✅ COMPLETE (2026-07-22)
- [x] `DDLExecutor` class for creation/deletion of collections, indexes, views
- [x] Schema validation: type checking, unique-name enforcement
- [x] Conflict detection: collection already exists, index duplicate, view name clash

### Phase 4: Testing & Documentation — ✅ COMPLETE (2026-07-22)
- [x] 32 tests covering all DDL statement types and error paths
- [x] Header documentation (`include/query/ddl_executor.h`)
- [x] Wired into `release_critical` label in `tests/aql/CMakeLists.txt`

---

## Production Readiness Checklist

- [x] All DDL keywords tokenise correctly
- [x] Parser generates correct AST for all DDL syntaxes
- [x] DDLExecutor applies changes to metadata store
- [x] Conflict detection returns structured errors
- [x] Tests pass: ≥ 32 cases covering creation, deletion, conflict, and error paths
- [x] Backward compatibility: no regression in existing read-only or mutation queries
- [ ] Security audit: ensure DDL cannot bypass collection-level permissions (Target: Q4 2026)
- [ ] Migration guide: documenting DDL vs. API-level collection management (Target: Q4 2026)

---

## Known Limitations

- Phase 2+: Advanced DDL (ALTER COLLECTION, RENAME, schema migration) is out of scope for v2.0.0 — tracked in `src/query/ROADMAP.md` Phase 4 backlog.
- Savepoint support for DDL-within-transaction not yet implemented.
- Distributed DDL (fan-out to all shards) not yet fully validated under concurrent load.

---

## References

- [AQL_V2_0_0_COMPLETE_ROADMAP.md](./AQL_V2_0_0_COMPLETE_ROADMAP.md) — Master v2.0.0 language standard roadmap
- [AQL_MUTATIONS_ROADMAP.md](./AQL_MUTATIONS_ROADMAP.md) — DML roadmap (foundation for DDL)
- `src/query/ddl_executor.cpp` — DDL executor implementation
- `include/query/ddl_executor.h` — Public DDL API
