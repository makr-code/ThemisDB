### Context

This issue implements the roadmap item '`AQLQueryBuilder` — Graph Traversal and DML Support' for the aql domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: 11 · `AQLQueryBuilder` — Graph Traversal and DML Support

### Goal

Deliver the scoped changes for `AQLQueryBuilder` — Graph Traversal and DML Support in src/aql/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### 11 · `AQLQueryBuilder` — Graph Traversal and DML Support
**Priority:** High
**Target Version:** v1.7.0

**Problem (from code):** `aql_query_builder.cpp:Impl::render()` and the public API (`include/aql/aql_query_builder.h`) support only `FOR`, `LET`, `FILTER`, `COLLECT`, `SORT`, `LIMIT`, `RETURN`. Graph traversal (`FOR v, e, p IN 1..N OUTBOUND start GRAPH g`), DML (`INSERT`, `UPDATE`, `REMOVE`, `UPSERT`, `REPLACE`), subquery expressions (`( FOR x IN ... RETURN x )`), and `WINDOW` analytics clauses are completely absent from the builder API. Any caller that needs these constructs must fall back to raw string concatenation, losing all validation and type-safety.

**Implementation Notes:**
- `[ ]` Add `AQLQueryBuilder& forTraverse(const std::string& vertex_var, const std::string& edge_var, const std::string& path_var, const std::string& start, const std::string& graph, const std::string& direction = "OUTBOUND", int min_depth = 1, int max_depth = 1)` to the builder
- `[ ]` Add `AQLQueryBuilder& insertInto(const std::string& collection, const std::string& doc_expr)`, `updateIn()`, `removeIn()`, `upsertIn()`, `replaceIn()` DML methods
- `[ ]` Add `AQLQueryBuilder& window(const std::string& partition_expr, const std::string& window_spec)` for timeseries queries
- `[ ]` Add `AQLQueryBuilder& subquery(const std::string& variable, const AQLQueryBuilder& inner)` that renders `LET variable = ( <inner> )`
- `[ ]` Update `Impl::render()` to emit these new clauses in correct AQL clause-ordering position
- `[ ]` Update `AQLQueryValidator` to check new clauses for common mistakes (e.g. `min_depth > max_depth`)
- `[ ]` Add grammar-coverage tests: at least one test per new clause type

---

### Acceptance Criteria

- [ ] Add `AQLQueryBuilder& forTraverse(const std::string& vertex_var, const std::string& edge_var, const std::string& path_var, const std::string& start, const std::string& graph, const std::string& direction = "OUTBOUND", int min_depth = 1, int max_depth = 1)` to the builder
- [ ] Add `AQLQueryBuilder& insertInto(const std::string& collection, const std::string& doc_expr)`, `updateIn()`, `removeIn()`, `upsertIn()`, `replaceIn()` DML methods
- [ ] Add `AQLQueryBuilder& window(const std::string& partition_expr, const std::string& window_spec)` for timeseries queries
- [ ] Add `AQLQueryBuilder& subquery(const std::string& variable, const AQLQueryBuilder& inner)` that renders `LET variable = ( <inner> )`
- [ ] Update `Impl::render()` to emit these new clauses in correct AQL clause-ordering position
- [ ] Update `AQLQueryValidator` to check new clauses for common mistakes (e.g. `min_depth > max_depth`)
- [ ] Add grammar-coverage tests: at least one test per new clause type

### Relationships

- Roadmap row: #148 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/aql/FUTURE_ENHANCEMENTS.md#11--aqlquerybuilder--graph-traversal-and-dml-support
- Source key: roadmap:148:aql:v1.7.0:11-aqlquerybuilder-graph-traversal-and-dml-support

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:148:aql:v1.7.0:11-aqlquerybuilder-graph-traversal-and-dml-support -->
<!-- roadmap-ref: row=148;module=aql;target=v1.7.0 -->
<!-- roadmap-detail: src/aql/FUTURE_ENHANCEMENTS.md#11--aqlquerybuilder--graph-traversal-and-dml-support -->
