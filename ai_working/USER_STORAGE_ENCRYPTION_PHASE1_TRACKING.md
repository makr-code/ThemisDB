# User Storage Encryption Module - Phase 1 Implementation Tracking

**Date:** 2026-08-08  
**Status:** In Progress  
**Agent:** themisdb-implementer  
**Mode:** Background Async  

---

## Phase 1: Gap Remediation & Code Hardening

### Overview

Phase 1 addresses all Critical (13) and High (36) severity findings from the gap scan to establish a production-stable foundation for Phases 2-6.

**Module Location:**
- Implementation: `/home/runner/work/ThemisDB/ThemisDB/src/user_storage_encrypted/`
- Public API: `/home/runner/work/ThemisDB/ThemisDB/include/user_storage_encrypted/`
- Tests: `/home/runner/work/ThemisDB/ThemisDB/tests/user_storage_encrypted/`
- Benchmarks: `/home/runner/work/ThemisDB/ThemisDB/benchmarks/user_storage_encrypted/`

**Reference:** `/home/runner/work/ThemisDB/ThemisDB/src/user_storage_encrypted/MODULE_GAPS.md`

---

## Gap Remediation Tasks

### Task 1: Resource Management Hardening

**Objective:** Fix all 14 resource_leaked_in_exception findings

**Scope:**
- `gocryptfs_backend.cpp`: FILE* cleanup (popen/pclose)
- `key_derivation_service.cpp`: Process handles, temporary buffers
- `key_rotation_scheduler.cpp`: Timer/scheduler resource cleanup
- `multi_level_storage.cpp`: Mount-state file descriptors, listener registrations

**Implementation Pattern:**
```cpp
// BEFORE (unsafe):
FILE* pipe = popen(cmd, "r");
if (error_condition) return error;  // Leak: pipe not closed

// AFTER (safe via RAII):
auto pipe = makeUnique<PipeGuard>(cmd, "r");
if (error_condition) return error;  // RAII destructor closes pipe automatically
```

**Classes to Create/Update:**
- `PipeGuard` (wraps popen/pclose with unique_ptr semantics)
- `ProcessGuard` (wraps child process lifetime)
- `FileDescriptorGuard` (wraps open/close file descriptors)
- `MountStateGuard` (wraps mount lifecycle resources)

**Validation:**
- Compile with `-fsanitize=address -fsanitize=leak`
- Run focused tests: `ctest -R module_user_storage_encrypted_test_use_contract_hardening_focused`
- Expected: Zero memory leaks, zero use-after-free errors

**Acceptance Criteria:**
- [ ] All 14 resource_leaked_in_exception findings resolved
- [ ] New RAII guard classes added to `include/user_storage_encrypted/` 
- [ ] Address sanitizer passes: `LeakSanitizer: detected memory leaks: 0`
- [ ] No regression in existing tests

---

### Task 2: Exception Safety & Timeout Handling

**Objective:** Fix 8 no_timeout + 5 uncaught_exception findings

**Blocking Operations Needing Timeout Guards (8):**

| Operation | Current | Timeout | Guard Class |
|-----------|---------|---------|-------------|
| `gocryptfs mount` | blocking | 30s | `CommandTimeoutGuard` |
| `gocryptfs unmount` | blocking | 10s | `CommandTimeoutGuard` |
| `fusermount -u` | blocking | 5s | `CommandTimeoutGuard` |
| `Vault secret fetch` | blocking | 10s | `VaultClientGuard` |
| Scheduler rotation wait | blocking | 60s | `SchedulerTimeoutGuard` |
| Key derivation (Argon2id) | blocking | 120s | `KdfTimeoutGuard` |
| Mount-state scan (/proc/mounts) | blocking | 5s | `ProcScanTimeoutGuard` |
| Listener notification broadcast | blocking | 1s | `ListenerBroadcastGuard` |

**Exception Safety Paths (5):**

| Path | Current | Issue | Fix |
|------|---------|-------|-----|
| Mount initialization | partial | Exception in mount leaves system in inconsistent state | Add rollback guard |
| Key rotation transaction | partial | Exception mid-rotation loses audit trail | Add transaction log |
| Listener registration | partial | Exception during broadcast corrupts listener list | Use copy-on-write or transaction |
| Tier transition | partial | Exception leaves tier partially initialized | Atomic state update or rollback |
| Vault token renewal | partial | Exception loses token renewal state | Checkpoint before renewal |

**Implementation Pattern:**
```cpp
// BEFORE:
IEncryptedStorageBackend::mount() {
  // No timeout, no exception handling
  executeCommand("gocryptfs ...");  // May hang indefinitely
  initializeEncryption();  // May throw, leaving mount in inconsistent state
}

// AFTER:
IEncryptedStorageBackend::mount() {
  // Strong exception safety with timeout
  auto cmd = makeUnique<CommandTimeoutGuard>("gocryptfs ...", 30s);
  if (!cmd->execute()) return Status::MountTimeoutError();
  
  try {
    initializeEncryption();
  } catch (...) {
    cmd->rollback();  // Unmount on exception
    throw;
  }
}
```

**Doxygen Documentation Pattern:**
```cpp
/**
 * @brief Mount encrypted storage with strong exception safety.
 * 
 * @param path Encrypted storage path
 * @return Status::Ok() on success
 * @return Status::MountTimeoutError() if mount exceeds 30s
 * @return Status::MountInitError(...) if initialization fails
 * 
 * @exception Strong guarantee: either fully mounted or no changes
 * @exception Throws on allocation failure (std::bad_alloc)
 * @exception Does NOT throw on I/O errors (returns Status)
 */
Status mount(const std::string& path);
```

**Acceptance Criteria:**
- [ ] All 8 blocking operations guarded with timeouts
- [ ] All 5 exception paths have strong/basic exception safety (documented)
- [ ] New timeout guard classes in `include/user_storage_encrypted/`
- [ ] Doxygen `@exception` tags updated for all affected functions
- [ ] Tests pass with exception injection (throw_on_call=N)

---

### Task 3: Security Hardening

**Objective:** Fix 7 command_injection + input validation + fail-closed behavior

**Command Injection Risks (7):**

| Risk | Location | Fix |
|------|----------|-----|
| Unsanitized mount path in gocryptfs cmdline | `gocryptfs_backend.cpp` | Use argv array (no shell), validate path |
| Key string passed to Vault API | `key_derivation_service.cpp` | Use HTTP encoding, validate key format |
| Passphrase passed to stdin | `pipe_guard.hpp` | Zero memory after use, validate encoding |
| User input in audit log | `multi_level_storage.cpp` | Escape special chars, validate UTF-8 |
| Config file path from environment | CMakeLists.txt config | Resolve path, check ownership & perms |
| Command output parsing (stale mount detect) | `/proc/mounts` parser | Strict whitespace parsing, bounds check |
| Vault token in error messages | Key provider | Never log tokens, sanitize exception messages |

**Input Validation Boundaries:**

| Boundary | Validation | Action on Invalid |
|----------|------------|-------------------|
| IEncryptedStorageBackend::mount(path) | Path exists, readable, not in /tmp | Return INVALID_PATH_ERROR |
| IKeyProvider::fetchKey(key_id) | Key ID format [A-Za-z0-9_-]{32,64}, not empty | Fail-closed: throw or return error |
| IRotationStore::persistState(state) | State struct non-NULL, timestamps valid | Log warning, skip persist (readonly) |
| Audit logger inputs (user, op, tier) | Non-empty strings, valid enum values | Skip audit entry (non-fatal), log |

**Fail-Closed Behavior:**

```cpp
// BEFORE (fail-open):
if (vault_unavailable) {
  // Silently degrade to unencrypted storage
  return unencrypted_backend->encrypt(...);
}

// AFTER (fail-closed):
if (vault_unavailable) {
  // Explicit error: no automatic fallback
  return Status::VaultUnavailableError();
}
```

**Acceptance Criteria:**
- [ ] All 7 command injection risks fixed (validated with shellcheck or manual review)
- [ ] Input validation at all IEncryptedStorageBackend/IKeyProvider boundaries
- [ ] Fail-closed behavior documented in PRODUCTION_REQUIREMENTS.md
- [ ] Secrets never logged in error messages or debug output
- [ ] CodeQL/Clang-Tidy security alerts on remediated code cleared

---

### Task 4: Performance & Correctness

**Objective:** Fix 6 copy_overhead + 3 unnecessary_copy + 2 range_temporary + 4 posix_only_api

**Copy Overhead Issues (6+3):**

| Issue | Location | Fix |
|-------|----------|-----|
| `std::string` copy in loop | `key_rotation_scheduler.cpp` loop | Use `std::string_view` or reference |
| Vector allocation in hot path | `multi_level_storage.cpp` rotate | Pre-allocate, reuse via `swap()` |
| Listener copy on notification | Broadcaster callback | Pass const ref, avoid copy |
| Argon2id result struct copy | `key_derivation_service.cpp` | Return via out-param or unique_ptr |
| Error context serialization | Audit logger | Build string in-place, avoid temp |
| Config map copy on lookup | Initialization | Use `at()` with const ref |

**Example Fix:**
```cpp
// BEFORE:
for (const auto& tenant : tenants) {
  std::string key_id = buildKeyId(tenant.id);  // Copy per iteration
  auto key = fetchKey(key_id);
}

// AFTER:
for (const auto& tenant : tenants) {
  auto key_id_view = buildKeyId(tenant.id);  // String built in-place
  auto key = fetchKey(key_id_view);  // Pass by const ref
}
```

**Range Temporary Issues (2):**

| Issue | Location | Fix |
|-------|----------|-----|
| Temporary vector from algorithm | `mount_reconciler.cpp` | Use `std::vector<T>&` param |
| Temporary pair return | Key rotation state | Return by reference or value, not temp |

**POSIX-Only API Issues (4):**

| API | Issue | Windows Alternative |
|-----|-------|-------------------|
| `/proc/mounts` | Linux-only | Use WMI or Registry on Windows |
| `fusermount -u` | Linux/macOS | Use umount on POSIX, `Dismount` on Windows |
| `popen()` | Not on Windows MSVC | Use CreateProcess on Windows |
| Signal handling | SIGTERM/SIGKILL | Use Windows events/APCs |

**Abstract Platform Layer:**
```cpp
// NEW: include/user_storage_encrypted/platform_abstraction.hpp
namespace themisdb::user_storage_encrypted {
  class PlatformAbstraction {
  public:
    // Mount detection (Linux: /proc/mounts, Windows: WMI)
    virtual std::vector<MountPoint> detectMounts(const std::string& basePath) = 0;
    
    // Mount/unmount (Linux: fusermount, Windows: equivalent)
    virtual Status unmountPath(const std::string& path) = 0;
    
    // Command execution (Windows: CreateProcess, POSIX: fork/exec)
    virtual Status executeCommand(const std::string& cmd, const std::vector<std::string>& argv) = 0;
    
    // Filesystem operations (Windows: native, POSIX: POSIX APIs)
    virtual std::optional<FileInfo> getFileInfo(const std::string& path) = 0;
  };
  
  // Platform-specific factory
  std::unique_ptr<PlatformAbstraction> createPlatformAbstraction();
}
```

**Acceptance Criteria:**
- [ ] All 6 copy_overhead findings fixed (verified via perf profiling)
- [ ] All 3 unnecessary_copy findings eliminated
- [ ] All 2 range_temporary issues resolved
- [ ] PlatformAbstraction base class added (Linux impl only for Phase 1)
- [ ] No performance regression (benchmark baseline comparison)
- [ ] Move semantics used where appropriate (unique_ptr returns, rvalue refs)

---

## Testing & Validation

### Unit Test Expansion

**New Test File:** `tests/user_storage_encrypted/test_user_storage_encrypted_phase1_hardening.cpp`

**Test Matrix (20+ test cases):**

| Category | Test Cases | Coverage |
|----------|-----------|----------|
| Resource Management (5) | RAII cleanup, exception cleanup, nested guards, guard reuse, stateful cleanup | All 14 resource findings |
| Timeout Handling (5) | Mount timeout, unmount timeout, concurrent timeouts, timeout overflow, timeout 0 | All 8 blocking ops |
| Exception Safety (5) | Mount exception recovery, rotation exception rollback, listener broadcast exception, state corruption avoidance, exception re-throw | All 5 exception paths |
| Input Validation (3) | Invalid path, invalid key ID, empty/null inputs | 3 boundary cases |
| Command Injection (2) | Malicious path in gocryptfs, malicious key in Vault API | 2 injection scenarios |

**Compile & Run:**
```bash
cd /home/runner/work/ThemisDB/ThemisDB
cmake --preset linux-release
cmake --build build-linux-release --target module_user_storage_encrypted_test_use_contract_hardening_focused -j 16
ctest --preset linux-release -R "module_user_storage_encrypted" --output-on-failure
```

### Address Sanitizer Validation

```bash
cmake --preset linux-release -DCMAKE_CXX_FLAGS="-fsanitize=address -fsanitize=leak"
cmake --build build-linux-release --target module_user_storage_encrypted_test_use_contract_hardening_focused -j 16
LSAN_OPTIONS=verbosity=1:log_threads=1 ctest --preset linux-release -R "user_storage_encrypted" --output-on-failure
```

**Expected Output:**
```
LeakSanitizer: detected memory leaks: 0
Errors found: 0
```

### Static Analysis (CodeQL/Clang-Tidy)

```bash
cmake --preset linux-release -DCMAKE_CXX_CLANG_TIDY="clang-tidy;-checks=*;-header-filter=.*"
cmake --build build-linux-release --target module_user_storage_encrypted_test_use_contract_hardening_focused 2>&1 | grep -E "(error|warning)" | wc -l
```

**Expected:** Zero security category warnings/errors

---

## Deliverables Checklist

### Code Deliverables

- [ ] `src/user_storage_encrypted/gocryptfs_backend.cpp` - RAII guards, timeout handling, input validation
- [ ] `src/user_storage_encrypted/key_derivation_service.cpp` - Exception safety, timeout, copy elimination
- [ ] `src/user_storage_encrypted/key_rotation_scheduler.cpp` - Resource cleanup, timeout, audit trail
- [ ] `src/user_storage_encrypted/multi_level_storage.cpp` - Fail-closed behavior, listener safety, POSIX abstraction
- [ ] `include/user_storage_encrypted/pipe_guard.hpp` - NEW: RAII wrapper for popen/pclose
- [ ] `include/user_storage_encrypted/process_guard.hpp` - NEW: RAII wrapper for child processes
- [ ] `include/user_storage_encrypted/timeout_guard.hpp` - NEW: Template for timeout management
- [ ] `include/user_storage_encrypted/platform_abstraction.hpp` - NEW: Platform-agnostic APIs (Linux impl)

### Test Deliverables

- [ ] `tests/user_storage_encrypted/test_user_storage_encrypted_phase1_hardening.cpp` - 20+ new test cases
- [ ] Updated `tests/user_storage_encrypted/test_user_storage_encrypted_contract_hardening_focused.cpp` - Regression suite
- [ ] All tests pass with `-fsanitize=address -fsanitize=leak`

### Documentation Deliverables

- [ ] `src/user_storage_encrypted/PHASE_1_ACCEPTANCE_CHECKLIST.md` - Gap remediation status
- [ ] Updated `src/user_storage_encrypted/PRODUCTION_REQUIREMENTS.md` - Validation evidence
- [ ] Updated `src/user_storage_encrypted/ARCHITECTURE.md` - Exception safety guarantees
- [ ] Updated `plugins/user_storage_encrypted/ROADMAP.md` - Phase 1 closure date & links

### Validation Report

- [ ] Build log (linux-release, no warnings)
- [ ] Test run output (all pass, TIMEOUT < 120s)
- [ ] Address sanitizer report (zero leaks)
- [ ] CodeQL scan results (security category clear)
- [ ] Benchmark comparison (no performance regression)

---

## Success Criteria

**Phase 1 is complete when:**

1. ✅ All 13 Critical gap findings resolved
2. ✅ All 36 High gap findings resolved
3. ✅ 90%+ line coverage in user_storage_encrypted module
4. ✅ Address sanitizer: zero leaks, zero errors
5. ✅ CodeQL/Clang-Tidy: no security alerts
6. ✅ All new tests pass (flakiness < 1%)
7. ✅ Performance benchmarks show no regression
8. ✅ PHASE_1_ACCEPTANCE_CHECKLIST.md fully signed off

---

## Timeline

**Start:** 2026-08-08  
**Target Completion:** 2026-08-22 (2 weeks)  
**Status:** In progress (themisdb-implementer agent)

---

**Next Phase:** Phase 2 - Integration Testing & Vault Hardening (TBD)
