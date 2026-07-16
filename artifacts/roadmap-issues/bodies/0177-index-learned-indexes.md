### Context

This issue implements the roadmap item 'Learned Indexes' for the index domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: Learned Indexes

### Goal

Deliver the scoped changes for Learned Indexes in src/index/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### Learned Indexes
**Priority:** Medium  
**Target Version:** v1.8.0

Replace B-tree with ML models for improved lookup performance.

**Concept:** Learn CDF (cumulative distribution function) of keys to predict position.

**Sources:**
- Paper: Kraska et al. (2018), "The Case for Learned Index Structures", SIGMOD
- Benefits: 2-3x faster lookups, 10-100x smaller indexes
- Tradeoffs: Requires training, less flexible for updates

**Architecture:**
```
┌──────────────┐
│   ML Model   │  Predicts: position = f(key)
│ (Neural Net) │
└──────┬───────┘
       │
       ▼
┌──────────────────┐
│  Correction      │  Binary search in local region
│  Layer (±ε)      │
└──────┬───────────┘
       │
       ▼
┌──────────────┐
│  Final Value │
└──────────────┘
```

**API:**
```cpp
// Enable learned index
sim.createLearnedIndex("users", "age", {
    .model_type = ModelType::NEURAL_NETWORK,
    .hidden_layers = {128, 64, 32},
    .error_bound = 100,  // Search within ±100 positions
    .retraining_interval = 3600  // Retrain hourly
});
```

**Use Cases:**
- Read-heavy workloads
- Stable key distributions
- Large indexes (> 1M keys)

---

### Acceptance Criteria

- [ ] Paper: Kraska et al. (2018), "The Case for Learned Index Structures", SIGMOD
- [ ] Benefits: 2-3x faster lookups, 10-100x smaller indexes
- [ ] Tradeoffs: Requires training, less flexible for updates
- [ ] Read-heavy workloads
- [ ] Stable key distributions
- [ ] Large indexes (> 1M keys)

### Relationships

- Roadmap row: #177 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/index/FUTURE_ENHANCEMENTS.md#learned-indexes
- Source key: roadmap:177:index:v1.8.0:learned-indexes

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:177:index:v1.8.0:learned-indexes -->
<!-- roadmap-ref: row=177;module=index;target=v1.8.0 -->
<!-- roadmap-detail: src/index/FUTURE_ENHANCEMENTS.md#learned-indexes -->
