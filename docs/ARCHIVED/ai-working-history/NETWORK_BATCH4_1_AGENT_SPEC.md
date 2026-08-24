# Network Module Batch 4.1 — Agent Implementation Specification

**Agent:** themisdb-implementer  
**Task:** Implement all 19 CRITICAL network module fixes (R01-R19)  
**Reference:** `ai_working/NETWORK_BATCH4_1_CRITICAL_FIXES_CHECKLIST.md`  
**Timeline:** Aug 16-22, 2026  
**Quality Gate:** Compiles, all tests PASS, ASan/UBSan/TSan = 0 alerts

---

## Overview

Implement 19 CRITICAL fixes across 11 files in the network module. All fixes have been analyzed with specific line numbers, fix strategies, and acceptance criteria documented.

**Repository:** /home/runner/work/ThemisDB/ThemisDB  
**Module:** src/network/  
**Build Presets:** windows-release (local) + release_critical CI (GitHub Actions)  

---

## Fix Categories & Implementation Order

### Phase A: Brace Imbalance (5 fixes, R01-R05)

**Files:**
1. `src/network/kernel_bypass.cpp:1`
2. `src/network/quic_server.cpp:1`
3. `src/network/raft_load_balancer.cpp:1`
4. `src/network/wire_protocol_performance.cpp:1`
5. `src/network/wire_protocol_v2.cpp:1`

**Fix Strategy:**
- Inspect first 50 lines of each file
- Check for missing opening `{` or closing `}`
- Use `clang-format` to validate brace balancing
- Ensure control structures (if/for/while) have properly paired braces
- Compile after each fix: `cmake --build --preset windows-release`

**Acceptance:**
- File compiles without errors
- Existing tests still pass
- Consistent with codebase style

---

### Phase B: Missing Destructors (3 fixes, R06-R08)

**Files:**
1. `src/network/socket_timeout_manager.cpp:71`
2. `src/network/service_mesh.cpp:175`
3. `src/network/service_mesh.cpp:194`

**Fix Strategy:**
- Locate class definition at/near specified line
- Check if class has virtual methods or is used polymorphically
- Add virtual destructor: `virtual ~ClassName() = default;`
- If cleanup needed, ensure it's noexcept
- Test with AddressSanitizer: `ASan` should report 0 leaks

**Acceptance:**
- Virtual destructor added
- Destructor is noexcept
- ASan reports 0 new issues
- Existing tests pass

---

### Phase C: Timeout Enforcement (4 fixes, R09-R11, R16)

**Files:**
1. `src/network/wire_protocol_zero_copy.cpp:112` (R09)
2. `src/network/wire_protocol_zero_copy.cpp:160` (R11)
3. `src/network/service_mesh.cpp:243` (R10)
4. `src/network/wire_protocol_performance.cpp:232` (R16)

**Fix Strategy:**
- Inspect line + context (~10 lines)
- Identify blocking operation (socket read/write, mutex wait)
- Add timeout parameter:
  - For socket ops: `setsockopt(SO_RCVTIMEO)` or `select/poll with timeout`
  - For mutex: `std::unique_lock` with `std::chrono::duration`
- Handle timeout: log and retry or fail fast
- Validate with ThreadSanitizer: `TSan` should report 0 deadlocks

**Acceptance:**
- Blocking op has explicit timeout
- Timeout is logged when exceeded
- Tests include timeout scenario
- TSan reports 0 new deadlock issues

---

### Phase D: Buffer Overflow Prevention (2 fixes, R12-R13)

**Files:**
1. `src/network/connection_compression.cpp:114` (R12)
2. `src/network/io_uring_batcher.cpp:251` (R13)

**Fix Strategy:**
- Inspect line + surrounding decompression/copy context
- Identify source, destination, copy size
- Add validation before memcpy:
  ```cpp
  if (source_size > MAX_DECOMPRESSED_SIZE || dest_available < source_size) {
      throw std::runtime_error("Buffer overflow check failed");
  }
  ```
- Alternatively use safe copy: `std::copy(src.begin(), src.end(), dst.begin())`
- Test with AddressSanitizer: `ASan` should catch any buffer overflow

**Acceptance:**
- Buffer sizes validated before memcpy
- Out-of-bounds raises exception
- ASan reports 0 buffer overflows
- Test case includes oversized input

---

### Phase E: Smart Pointer & Exception Safety (2 fixes, R14-R15)

**Files:**
1. `src/network/socket_timeout_manager.cpp:202` (R14)
2. `src/network/wire_protocol_zero_copy.cpp:220` (R15)

**Fix Strategy for R14 (Smart Pointer Misuse):**
- Inspect line + context
- Identify issue: raw pointer cast? dangling reference? double-delete?
- Replace with correct smart pointer:
  - For shared ownership: `std::shared_ptr<T>`
  - For exclusive ownership: `std::unique_ptr<T>`
  - For observation: `.get()` or `std::weak_ptr<T>`
- Document ownership semantics in header comment

**Fix Strategy for R15 (Exception in Destructor):**
- Inspect destructor at line 220
- Identify all exception-throwing ops
- Move to separate method or wrap in try-catch
- Mark destructor `noexcept`
- Document cleanup contract

**Acceptance:**
- Smart pointers follow RAII correctly
- No raw pointer access without ownership doc
- Destructor noexcept with no throw paths
- ASan/memory sanitizer = 0 issues

---

### Phase F: Connection Leak Fixes (3 fixes, R17-R19)

**Files:**
1. `src/network/raft_load_balancer.cpp:288` (R17)
2. `src/network/raft_load_balancer.cpp:289` (R18)
3. `src/network/raft_load_balancer.cpp:290` (R19)

**Fix Strategy:**
- Inspect lines 288-290 context
- Identify connection lifecycle (acquire/release)
- Implement RAII wrapper:
  ```cpp
  class ConnectionGuard {
      DatabaseConnection* conn;
  public:
      ConnectionGuard(Pool& p) : conn(p.acquire()) {}
      ~ConnectionGuard() { if(conn) conn->close(); }
  };
  ```
- Or use scope guard: `ON_SCOPE_EXIT { connection->close(); }`
- Verify connection pool size stable (no increase)

**Acceptance:**
- Connection closed in all code paths
- Exception-safe (RAII or try-finally)
- Connection pool size stable
- ASan reports 0 resource leaks

---

## Quality Assurance Checkpoints

### Pre-Implementation Checkpoint (Before Starting)

- [ ] All files exist and are accessible
- [ ] Line numbers verified against current develop branch
- [ ] Baseline: All tests currently PASS on develop
- [ ] Build successful: `cmake --preset windows-release && cmake --build --preset windows-release`

### Per-Fix Validation Checkpoint

For each fix (after implementation):

- [ ] Code compiles: `cmake --build --preset windows-release`
- [ ] Unit tests pass: `ctest --preset windows-release -k network --output-on-failure`
- [ ] If applicable: `AddressSanitizer` reports 0 new issues
- [ ] If applicable: `ThreadSanitizer` reports 0 new issues
- [ ] No regression: Existing tests still pass

### Phase Completion Checkpoint (After Each Phase A-F)

- [ ] All fixes in phase committed with clear commit message
- [ ] Phase tests pass: `ctest --preset windows-release -k network --output-on-failure`
- [ ] Sanitizer report clean: ASan/UBSan/TSan = 0 alerts (if applicable)

### Batch 4.1 Final Validation (After All 6 Phases)

- [ ] **Full Build:** Clean: `rm -rf build && cmake --preset windows-release && cmake --build --preset windows-release`
- [ ] **Full Test Suite:** `ctest --preset windows-release -j 8 --output-on-failure`
- [ ] **Network Tests:** `ctest --preset windows-release -R network -j 4 --output-on-failure`
- [ ] **Sanitizer Run:** ASan + UBSan + TSan all report 0 alerts
- [ ] **Benchmark Stability:** Verify no performance regression
- [ ] **CI Gate:** release_critical workflow GREEN

---

## Implementation Timeline

| Phase | Target | Scope | Estimated Time |
|-------|--------|-------|-----------------|
| **Pre-Impl** | Aug 16 (Fri) AM | Verification & setup | 1-2 hours |
| **Phase A** | Aug 16 (Fri) | Braces (5 fixes) | 2-3 hours |
| **Phase B** | Aug 17 (Sat) | Dtors (3 fixes) | 1.5-2 hours |
| **Phase C** | Aug 18 (Sun) | Timeouts (4 fixes) | 2-3 hours |
| **Phase D** | Aug 19 (Mon) | Memcpy (2 fixes) | 1-1.5 hours |
| **Phase E** | Aug 19 (Mon) | Smart ptr/except (2 fixes) | 1.5-2 hours |
| **Phase F** | Aug 20 (Tue) | Conn leaks (3 fixes) | 1.5-2 hours |
| **Validation** | Aug 21-22 | Full test suite, sanitizers | 4-6 hours |

**Total Estimated:** 14-20 hours (with contingency)

---

## Escalation & Blockers

If you encounter issues:

1. **Compilation Error:** Check if fix introduced syntax error; revert and re-examine
2. **Test Failure:** Determine if regression or pre-existing; isolate with git bisect if needed
3. **Design Uncertainty:** Reference `src/network/ARCHITECTURE.md` for module contracts
4. **Architectural Issue:** Document in `ai_working/BATCH4_1_BLOCKERS.md` and escalate to human maintainer

---

## Success Definition

✅ **Batch 4.1 is COMPLETE when:**

1. All 19 fixes implemented in 6 phases (A-F)
2. Code compiles without errors or warnings
3. All tests PASS (unit + integration + network-specific)
4. ASan/UBSan/TSan all report 0 alerts
5. No performance regression (benchmarks stable)
6. Implementation documented with evidence
7. Ready for Batch 4.2 (HIGH priority fixes)

---

## Documentation & Sign-Off

After completing all fixes, create:

**`ai_working/NETWORK_BATCH4_1_IMPLEMENTATION_SUMMARY.md`**

Contents:
- All 19 fixes implemented ✓
- Files modified with summary of changes
- Test results (unit, integration, sanitizer)
- Performance impact (if any)
- Known issues or deferred items (if any)
- Recommendation for next phase (Batch 4.2)

---

## References

- Full checklist: `ai_working/NETWORK_BATCH4_1_CRITICAL_FIXES_CHECKLIST.md`
- Gap analysis: `ai_working/NETWORK_MODULE_GAPS_BATCH4_ANALYSIS.md`
- Module architecture: `src/network/ARCHITECTURE.md`
- Module roadmap: `src/network/ROADMAP.md`

---

**Specification Status:** ✅ READY FOR IMPLEMENTATION  
**Assigned To:** themisdb-implementer AI agent  
**Start Date:** Aug 16, 2026  
**Target Completion:** Aug 22, 2026
