# Phase 2B Implementation: Exception Safety & Resource Leak Fixes (13 gaps)

**Phase:** 2B (Weeks 2-3, Aug 22 – Sep 5, parallel to Phase 2A)  
**Agent Type:** `themisdb-implementer` (coding + build/test)  
**Scope:** 13 HIGH resource_leak_exception_safety gaps  
**Blocker:** Depends on Phase 2A CRITICAL completion (gate: P2A tests 100% PASS before P2B launch)  
**Target Artifact:** `IMPORTERS_PHASE2B_EXCEPTION_SAFETY_FIXES_COMPLETE.md`

---

## Resource Leak Exception Safety Cluster

**Problem:** Memory allocations in exception paths are not properly released; missing unique_ptr adoption or try-catch guards.

**Files & Gaps:**

### 1. kafka_importer.cpp (4 HIGH resource_leak_exception_safety gaps)
**Shared State:** Connection pool, consumer handle, offset state

**Gap Pattern:** New allocations in try blocks without catch handlers

**Gap Lines & Context:**
- Line 1234: `KafkaConsumer* consumer = new KafkaConsumer(config)` → missing try-catch, leak on exception
- Line 1567: `ConsumerGroup* group = new ConsumerGroup(broker_list)` → leak on OffsetCommitFail
- Line 2101: `Message* buffer = new Message[batch_size]` → leak if ProcessBatch throws
- Line 2345: `ConnectionPool* pool = new ConnectionPool(...)` → no cleanup on init failure

**Fix Pattern:**
```cpp
// Before:
std::unique_ptr<KafkaConsumer> CreateConsumer(const KafkaConfig& cfg) {
    KafkaConsumer* consumer = new KafkaConsumer(cfg);  // RESOURCE LEAK if next line throws
    consumer->Initialize();  // Can throw
    return std::unique_ptr<KafkaConsumer>(consumer);
}

// After:
std::unique_ptr<KafkaConsumer> CreateConsumer(const KafkaConfig& cfg) {
    auto consumer = std::make_unique<KafkaConsumer>(cfg);  // Exception-safe allocation
    consumer->Initialize();  // Can throw, but unique_ptr auto-destructs
    return consumer;
}
```

**Additional Exception Handling:**
```cpp
// Connection pool with nested cleanup:
Status InitializeConnectionPool() {
    try {
        pool_ = std::make_unique<ConnectionPool>(broker_list_);
        pool_->Connect();  // Can throw
        pool_->SetConsumerGroup(group_);  // Can throw
        return Status::OK();
    } catch (const std::exception& e) {
        pool_ = nullptr;  // Explicit cleanup on error
        return Status::FromException(e);
    }
}
```

**Tests to Add:** IMPI-2B-KA-01..04 (exception during init, cleanup, offset commit)

---

### 2. canonical_resolver.cpp (3 HIGH resource_leak_exception_safety gaps)
**Problem:** Entity resolver allocations not guarded

**Gap Lines:**
- Line 456: `EntityResolver* resolver = new EntityResolver(schema)` → throw on schema validation
- Line 789: `TypeResolver* type_resolver = new TypeResolver(type_rules)` → leak on rule load failure
- Line 1023: `NamespaceResolver* ns_resolver = new NamespaceResolver(ns_hints)` → no cleanup

**Fix Strategy:** Replace all raw `new` with `std::make_unique<T>(...)`

**Tests to Add:** IMPI-2B-CR-01..03

---

### 3. mdm_engine.cpp (1 HIGH resource_leak_exception_safety gap)
**Problem:** Entity update with partial allocation

**Gap Line:** 1567: Allocate entity snapshot, throw during merge → leak

**Fix Pattern:** RAII wrapper for snapshot state

**Tests to Add:** IMPI-2B-MD-01

---

### 4. audit_trail.cpp (1 HIGH resource_leak_exception_safety gap)
**Gap Line:** 2101: Allocate audit record, throw on signing → leak

**Fix Pattern:** unique_ptr<AuditRecord> for all allocations

**Tests to Add:** IMPI-2B-AT-01

---

### 5. postgres_importer_mdm.cpp (2 HIGH resource_leak_exception_safety gaps)
**Problem:** MDM metadata resolver allocations

**Gap Lines:**
- Line 834: MetadataResolver allocation, throw on MDM connect
- Line 1205: Lineage tracker allocation, throw on schema load

**Fix Pattern:** unique_ptr adoption

**Tests to Add:** IMPI-2B-PM-01..02

---

### 6. s3_importer.cpp (1 HIGH resource_leak_exception_safety gap)
**Problem:** S3 object stream allocation

**Gap Line:** 1890: `S3ObjectStream* stream = new S3ObjectStream(bucket, key)` → throw on open

**Fix Pattern:** unique_ptr<S3ObjectStream>

**Tests to Add:** IMPI-2B-S3-01

---

## Implementation Strategy

### Week 2 (Day 1-7): Sequential Exception Safety Fixes

**Priority Order (Largest Impact First):**

1. **kafka_importer.cpp (Day 1-2, 4 gaps)**
   - Audit all `new` allocations (KafkaConsumer, ConsumerGroup, Message buffer, ConnectionPool)
   - Replace with `std::make_unique<T>(...)`
   - Add try-catch blocks for exception handling with explicit cleanup
   - Verify no double-delete or use-after-free in error paths
   - Tests: IMPI-2B-KA-01..04

2. **canonical_resolver.cpp (Day 3, 3 gaps)**
   - Replace 3 raw `new` with `std::make_unique<T>`
   - Tests: IMPI-2B-CR-01..03

3. **Remaining (Day 4-7, 5 gaps)**
   - mdm_engine.cpp (1 gap) + audit_trail.cpp (1 gap) + postgres_importer_mdm.cpp (2 gaps) + s3_importer.cpp (1 gap)
   - Tests: IMPI-2B-MD-01, AT-01, PM-01..02, S3-01

### Build & Test Cycle

**Per File:**
1. Identify all raw `new`/`delete` in file
2. Replace with `std::make_unique<T>(...)`/`std::make_shared<T>(...)`
3. Add try-catch blocks where allocation follows initialize()
4. Build focused target
5. Run focused tests (exception throws + cleanup verification)

**Verification Commands:**
```bash
cmake --build --preset community-release-allow-missing-rocksdb --target module_importers_kafka_importer_focused
ctest --preset community-release-allow-missing-rocksdb -R "importers_kafka_importer_focused" --output-on-failure

# Leak detection
cmake --build --preset community-release-allow-missing-rocksdb --target module_importers_kafka_importer_focused
LSAN_OPTIONS=verbosity=2:log_pointers=1:log_threads=1 ctest -R "importers_kafka_importer_focused"
```

---

## Test Coverage (13 focused tests: IMPI-2B-*)

| File | Gap Count | Test Cases | Exception Scenario |
|------|-----------|-----------|-------------------|
| kafka_importer | 4 | IMPI-2B-KA-01..04 | Init fail, offset commit fail, batch process fail, pool fail |
| canonical_resolver | 3 | IMPI-2B-CR-01..03 | Schema validation fail, rule load fail, namespace hint fail |
| mdm_engine | 1 | IMPI-2B-MD-01 | Entity snapshot merge fail |
| audit_trail | 1 | IMPI-2B-AT-01 | Signing failure |
| postgres_importer_mdm | 2 | IMPI-2B-PM-01..02 | MDM connect fail, schema load fail |
| s3_importer | 1 | IMPI-2B-S3-01 | S3 stream open fail |
| **TOTAL** | **13** | **13 tests** | Exception paths verified |

**Test Requirements:**
- Each test: throw exception during allocation/init, verify cleanup (no leak)
- Verification: LSAN (Address Sanitizer leak detector) shows 0 bytes leaked
- Timeout: 30s per test (exception path is fast)

---

## Acceptance Criteria (Phase 2B Exit Gate)

✅ **All 13 resource_leak_exception_safety gaps fixed:**
- [ ] kafka_importer.cpp: 4/4 gaps (make_unique + try-catch)
- [ ] canonical_resolver.cpp: 3/3 gaps (make_unique adoption)
- [ ] mdm_engine.cpp: 1/1 gap (RAII wrapper)
- [ ] audit_trail.cpp: 1/1 gap (make_unique)
- [ ] postgres_importer_mdm.cpp: 2/2 gaps (make_unique)
- [ ] s3_importer.cpp: 1/1 gap (make_unique)

✅ **Testing & Verification:**
- [ ] All 13 focused tests (IMPI-2B-*) PASS
- [ ] LSAN detects 0 bytes leaked in exception paths
- [ ] AddressSanitizer completes without new issues
- [ ] Code review confirms RAII patterns + exception safety

✅ **Code Quality:**
- [ ] No raw `new`/`delete` in modified sections (all replaced with std::make_unique)
- [ ] Exception-safe: unique_ptr handles cleanup automatically
- [ ] Try-catch blocks added where needed for nested init
- [ ] Comments explain exception safety guarantee

✅ **Git Commit:**
- Message: `IMPORTERS-P2B-EXCEPTION-SAFETY: Fix 13 HIGH resource_leak_exception_safety gaps`
- All 6 files modified in single commit
- 13 focused tests added

---

## Blockers & Risks

| Risk | Mitigation |
|------|-----------|
| Phase 2A not complete | WAIT: Phase 2A exit gate must pass (all 21 tests) before launching Phase 2B |
| Nested unique_ptr with custom deleters | Verify deleters needed; prefer default deleters where possible |
| Exception during unique_ptr.get() in legacy code | Replace legacy usage of raw pointers with accessor methods |
| double-delete from exception cleanup + RAII | Carefully review exception handlers; avoid manual delete |
| Backward compat with raw pointer APIs | Update caller sites to accept unique_ptr/shared_ptr params |

---

## Related Documentation

- Phase 1 Triage: `ai_working/IMPORTERS_PHASE1_GAP_TRIAGE.md` (Resource Leak Exception Safety section)
- Master Coordination: `ai_working/IMPORTERS_GAP_CLOSURE_COORDINATION.md`
- C++ Best Practices: `.github/instructions/cpp-best-practices.instructions.md` (RAII, resource management)
- Phase 2A Spec: `ai_working/IMPORTERS_PHASE2A_DATA_RACE_AGENT_SPEC.md`

---

## Success Indicators

✅ **Phase 2B is successful when:**
- All 13 exception_safety gaps closed with std::make_unique adoption + try-catch
- 100% of tests (IMPI-2B-01..13) PASS with LSAN clean
- No double-delete or use-after-free
- Code review approved (RAII, exception guarantees validated)
- Ready for Phase 2C dispatch (Iterator Invalidation, 3 gaps)
