### Context

This issue implements the roadmap item 'Distributed Transactions' for the storage domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: v1.6.x → v1.7.x: Distributed Transactions

### Goal

Deliver the scoped changes for Distributed Transactions in src/storage/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### v1.6.x → v1.7.x: Distributed Transactions
**Breaking Changes:** Transaction API extends

**Old API:**
```cpp
auto tx = db->beginTransaction();
tx->commit();
```

**New API (backward compatible):**
```cpp
auto tx = db->beginTransaction();  // Local transaction
auto dtx = dtx_manager.beginDistributedTransaction();  // Distributed transaction
```

**Migration Steps:**
1. Update to v1.7.0
2. Test existing transactions (no changes needed)
3. Optionally adopt distributed transactions

**Timeline:** 6 months parallel support

---

### Acceptance Criteria

- [ ] Update to v1.7.0
- [ ] Test existing transactions (no changes needed)
- [ ] Optionally adopt distributed transactions

### Relationships

- Roadmap row: #109 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/storage/FUTURE_ENHANCEMENTS.md#distributed-transactions
- Source key: roadmap:109:storage:v1.7.0:distributed-transactions

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:109:storage:v1.7.0:distributed-transactions -->
<!-- roadmap-ref: row=109;module=storage;target=v1.7.0 -->
<!-- roadmap-detail: src/storage/FUTURE_ENHANCEMENTS.md#distributed-transactions -->
