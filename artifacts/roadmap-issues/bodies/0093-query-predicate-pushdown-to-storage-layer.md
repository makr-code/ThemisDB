### Context

This issue implements the roadmap item 'Predicate Pushdown to Storage Layer' for the query domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: Predicate Pushdown to Storage Layer

### Goal

Deliver the scoped changes for Predicate Pushdown to Storage Layer in src/query/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### Predicate Pushdown to Storage Layer
**Priority:** High  
**Target Version:** v1.7.0

Push filters directly to RocksDB iterators for early pruning.

**Current:**
```
RocksDB Iterator → Read all rows → Filter in query engine
```

**Optimized:**
```
RocksDB Iterator with filter → Only read matching rows
```

**Implementation:**
```cpp
// Custom RocksDB filter
class PredicateFilter : public rocksdb::FilterPolicy {
public:
    bool KeyMayMatch(const Slice& key, const Slice& filter) const override;
};

// Bloom filter for common predicates
auto bloom = std::make_shared<PredicateFilter>(predicates);
read_options.filter_policy = bloom;

// 10-50x fewer rows read from storage
```

**Benefits:**
- Reduce I/O by 10-100x
- Lower CPU usage (no unnecessary deserialization)
- Better for range queries

---

### Acceptance Criteria

- [ ] Reduce I/O by 10-100x
- [ ] Lower CPU usage (no unnecessary deserialization)
- [ ] Better for range queries

### Relationships

- Roadmap row: #93 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/query/FUTURE_ENHANCEMENTS.md#predicate-pushdown-to-storage-layer
- Source key: roadmap:93:query:v1.7.0:predicate-pushdown-to-storage-layer

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:93:query:v1.7.0:predicate-pushdown-to-storage-layer -->
<!-- roadmap-ref: row=93;module=query;target=v1.7.0 -->
<!-- roadmap-detail: src/query/FUTURE_ENHANCEMENTS.md#predicate-pushdown-to-storage-layer -->
