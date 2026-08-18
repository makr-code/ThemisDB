# Phase 2 - Line-by-Line Finding Reference

Quick lookup guide for all 58 findings organized by file and line number.

---

## plugin_hot_plug_monitor.cpp (17 findings)

### CRITICAL (3)

| Line | Severity | Category | Issue | Fix Type |
|------|----------|----------|-------|----------|
| 357 | 🔴 CRITICAL | no_timeout | `open(watch_directory_.c_str(), O_RDONLY);` blocks indefinitely | Add O_NONBLOCK or timeout |
| 365 | 🔴 CRITICAL | missing_dtor | Class kevent has no destructor for fd cleanup | Create KqueueGuard wrapper |
| 559 | 🔴 CRITICAL | thread_join_no_timeout | `monitor_thread_.join();` blocks indefinitely | Add timeout + error handling |

### HIGH (6)

| Line | Severity | Category | Issue | Fix Type |
|------|----------|----------|-------|----------|
| 131 | 🟠 HIGH | range_temporary | Range-for on temporary container in loop | Assign to named variable |
| 133 | 🟠 HIGH | resource_leaked_in_exception | Resource leak before delete in exception | Use RAII wrapper |
| 151 | 🟠 HIGH | range_temporary | Range-for on temporary container | Assign to named variable |
| 311 | 🟠 HIGH | allocation_loop + resource_leaked | Allocation in loop with leak | Pre-allocate + RAII |
| 383 | 🟠 HIGH | range_temporary | `for (const auto& entry : fs::directory_iterator(...))` | Assign to named variable |

### MEDIUM (8)

| Line | Severity | Category | Issue | Fix Type |
|------|----------|----------|-------|----------|
| 360 | 🟡 MEDIUM | manual_cleanup | `close(kq);` outside exception handler | Use FileDescriptorGuard |
| 373 | 🟡 MEDIUM | manual_cleanup | `close(dir_fd);` outside exception handler | Use FileDescriptorGuard |
| 374 | 🟡 MEDIUM | manual_cleanup | `close(kq);` outside exception handler | Use FileDescriptorGuard |
| 379 | 🟡 MEDIUM | map_vs_unordered_map | `std::map<std::string, fs::file_time_type>` lookups | Change to unordered_map |
| 381 | 🟡 MEDIUM | map_vs_unordered_map | `std::map<std::string, fs::file_time_type>` lookups | Change to unordered_map |
| 452 | 🟡 MEDIUM | manual_cleanup | `close(dir_fd);` outside exception handler | Use FileDescriptorGuard |
| 453 | 🟡 MEDIUM | manual_cleanup | `close(kq);` outside exception handler | Use FileDescriptorGuard |
| 572 | 🟡 MEDIUM | manual_cleanup | `close(inotify_fd_);` outside exception handler | Use FileDescriptorGuard |

---

## oci_registry_client.cpp (11 findings)

### CRITICAL (0)
None

### HIGH (3)

| Line | Severity | Category | Issue | Fix Type |
|------|----------|----------|-------|----------|
| 5 | 🟠 HIGH | uninitialized_access | Container element access before initialization | Add initialization check |
| 95 | 🟠 HIGH | resource_leaked_in_exception | HTTP client resource leak on exception | Use RAII wrapper |
| 521 | 🟠 HIGH | legacy_or_compat_path | "OCI + Docker v2 for compatibility" comment | Document removal target |

### MEDIUM (8)

| Line | Severity | Category | Issue | Fix Type |
|------|----------|----------|-------|----------|
| 106 | 🟡 MEDIUM | manual_cleanup | `EVP_MD_CTX_free(ctx);` outside exception handler | Use EVPContextGuard |
| 114 | 🟡 MEDIUM | manual_cleanup | `EVP_MD_CTX_free(ctx);` outside exception handler | Use EVPContextGuard |
| 117 | 🟡 MEDIUM | manual_cleanup | `EVP_MD_CTX_free(ctx);` outside exception handler | Use EVPContextGuard |
| 257 | 🟡 MEDIUM | unordered_container_iter | `std::unordered_map<...> response_headers` iteration | Use map if order matters |
| 361 | 🟡 MEDIUM | manual_cleanup | `writer.file.close();` outside exception handler | Use RAII file wrapper |
| 536 | 🟡 MEDIUM | copy_overhead | `push_back("Authorization: Bearer " + auth.bearer_token)` | Call `reserve()` before loop |
| 542 | 🟡 MEDIUM | copy_overhead | `push_back("Authorization: Bearer " + *token_res)` | Call `reserve()` before loop |
| 672 | 🟡 MEDIUM | copy_overhead | `push_back("Authorization: Bearer " + auth.bearer_token)` | Call `reserve()` before loop |

---

## wasm_plugin_loader.cpp (11 findings)

### CRITICAL (1)

| Line | Severity | Category | Issue | Fix Type |
|------|----------|----------|-------|----------|
| 306 | 🔴 CRITICAL | missing_dtor | Class WasmtimeBundle: no destructor for engine/store/module | Implement destructor |

### HIGH (7)

| Line | Severity | Category | Issue | Fix Type |
|------|----------|----------|-------|----------|
| 256 | 🟠 HIGH | resource_leaked_in_exception | Wasmtime resource leak on exception | Use WasmtimeGuard |
| 257 | 🟠 HIGH | resource_leaked_in_exception | Wasmtime resource leak on exception | Use WasmtimeGuard |
| 277 | 🟠 HIGH | resource_leaked_in_exception | Wasmtime resource leak on exception | Use WasmtimeGuard |
| 370 | 🟠 HIGH | manual_cleanup_in_destructor | Manual cleanup in `WasmHostAPI::~WasmHostAPI()` | Extract to RAII class |
| 388 | 🟠 HIGH | delete_no_nullptr + delete_without_nullptr + explicit_delete | Cluster: `delete b;` (multiple issues) | Replace with unique_ptr |

### MEDIUM (3)

| Line | Severity | Category | Issue | Fix Type |
|------|----------|----------|-------|----------|
| 234 | 🟡 MEDIUM | stale_doc_section_reference | Comment references 'Security)' in FUTURE_ENHANCEMENTS.md | Update comment |
| 388 | 🟡 MEDIUM | manual_cleanup | Manual resource cleanup not in exception handler | Use RAII (see HIGH) |
| 493 | 🟡 MEDIUM | stale_doc_section_reference | Comment references 'WASMRuntime.' in FUTURE_ENHANCEMENTS.md | Update comment |

---

## signed_plugin_repository.cpp (10 findings)

### CRITICAL (0)
None

### HIGH (3)

| Line | Severity | Category | Issue | Fix Type |
|------|----------|----------|-------|----------|
| 34 | 🟠 HIGH | resource_leaked_in_exception | Cryptographic context leak on exception | Use EVPContextGuard |
| 35 | 🟠 HIGH | resource_leaked_in_exception | Cryptographic context leak on exception | Use EVPContextGuard |
| 298 | 🟠 HIGH | resource_leaked_in_exception | Cryptographic context leak on exception | Use RAII wrapper |

### MEDIUM (7)

| Line | Severity | Category | Issue | Fix Type |
|------|----------|----------|-------|----------|
| 231 | 🟡 MEDIUM | string_concat_loop | `case '"': out += "\\\""; break;` (O(n²)) | Use stringstream |
| 232 | 🟡 MEDIUM | string_concat_loop | `case '\\': out += "\\\\"; break;` (O(n²)) | Use stringstream |
| 233 | 🟡 MEDIUM | string_concat_loop | `case '\n': out += "\\n"; break;` (O(n²)) | Use stringstream |
| 234 | 🟡 MEDIUM | string_concat_loop | `case '\r': out += "\\r"; break;` (O(n²)) | Use stringstream |
| 235 | 🟡 MEDIUM | string_concat_loop | `case '\t': out += "\\t"; break;` (O(n²)) | Use stringstream |
| 316 | 🟡 MEDIUM | manual_cleanup | `EVP_MD_CTX_free(ctx);` outside exception handler | Use EVPContextGuard |
| 317 | 🟡 MEDIUM | manual_cleanup | `EVP_PKEY_free(pkey);` outside exception handler | Use EVPKeyGuard |

---

## plugin_health_monitor.cpp (9 findings)

### CRITICAL (1)

| Line | Severity | Category | Issue | Fix Type |
|------|----------|----------|-------|----------|
| 59 | 🔴 CRITICAL | thread_join_no_timeout | `monitor_thread_.join();` blocks indefinitely | Add timeout + error handling |

### HIGH (5)

| Line | Severity | Category | Issue | Fix Type |
|------|----------|----------|-------|----------|
| 5 | 🟠 HIGH | uninitialized_access | Container element access before initialization | Add initialization check |
| 277 | 🟠 HIGH | range_temporary | Range-for on temporary container in sleep loop | Assign to named variable |
| 292 | 🟠 HIGH | lock_in_loop | `for (const auto& name : names) { lock_guard... }` | Move lock outside loop |
| 474 | 🟠 HIGH | repeated_search | `std::find(strategies.begin(), strategies.end(), action)` in loop | Use set or build lookup |
| 579 | 🟠 HIGH | uninitialized_access | Container element access before initialization | Add initialization check |

### MEDIUM (3)

| Line | Severity | Category | Issue | Fix Type |
|------|----------|----------|-------|----------|
| 185 | 🟡 MEDIUM | unordered_container_iter | `std::unordered_map<...> getAllPluginStats()` iteration | Use map if order matters |
| 649 | 🟡 MEDIUM | generic_catch + uncaught_exception | `catch (...)` swallows exceptions | Catch specific types |

---

## Summary Statistics

### By Severity
| Severity | Count | Status |
|----------|-------|--------|
| 🔴 CRITICAL | 5 | BLOCKING |
| 🟠 HIGH | 28 | REQUIRED |
| 🟡 MEDIUM | 25 | QUALITY |
| Total | 58 | - |

### By Category (Top 10)
| Category | Count | Primary Concern |
|----------|-------|-----------------|
| resource_leaked_in_exception | 11 | Memory leaks on initialization failure |
| manual_cleanup | 11 | Manual cleanup unsafe in exceptions |
| string_concat_loop | 5 | O(n²) performance in escape function |
| range_temporary | 4 | Invalid iterator references |
| copy_overhead | 3 | Unnecessary allocations in loops |
| uninitialized_access | 3 | Access before init crashes |
| lock_in_loop | 1 | Lock contention per iteration |
| allocation_loop | 1 | Memory pressure in hot path |
| explicit_delete | 3 | Use-after-free risk (prefer smart ptr) |
| legacy_or_compat_path | 1 | Legacy Docker v2 support |

### By File (Ranked by Criticality)
| File | Total | Criticality | Status |
|------|-------|-------------|--------|
| plugin_hot_plug_monitor.cpp | 17 | **HIGH** (3 CRITICAL) | NEEDS URGENT WORK |
| wasm_plugin_loader.cpp | 11 | **HIGH** (1 CRITICAL) | NEEDS URGENT WORK |
| plugin_health_monitor.cpp | 9 | **HIGH** (1 CRITICAL) | NEEDS URGENT WORK |
| oci_registry_client.cpp | 11 | MEDIUM (0 CRITICAL) | Secondary priority |
| signed_plugin_repository.cpp | 10 | MEDIUM (0 CRITICAL) | Secondary priority |

---

## RAII Wrapper Summary

These 5 new classes will fix 28+ findings:

1. **FileDescriptorGuard** (6 findings)
   - plugin_hot_plug_monitor.cpp: lines 360, 373, 374, 452, 453, 572

2. **KqueueGuard** (1 finding)
   - plugin_hot_plug_monitor.cpp: line 365

3. **EVPContextGuard** (6 findings)
   - oci_registry_client.cpp: lines 106, 114, 117
   - signed_plugin_repository.cpp: lines 316

4. **EVPKeyGuard** (1 finding)
   - signed_plugin_repository.cpp: line 317

5. **WasmtimeGuard** (6 findings)
   - wasm_plugin_loader.cpp: lines 256, 257, 277, 306, 370, 388

---

## Quick Action Checklist

### Immediate (Today)
- [ ] Review this reference guide
- [ ] Plan RAII wrapper implementation

### This Sprint (Phase 2a - CRITICAL)
- [ ] Line 357: Add non-blocking file open
- [ ] Line 365: Create KqueueGuard class
- [ ] Line 306: Implement WasmtimeBundle destructor
- [ ] Line 559: Add thread join timeout
- [ ] Line 59: Add thread join timeout
- [ ] Write unit tests for each

### Next Sprint (Phase 2b - HIGH)
- [ ] Create EVPContextGuard class
- [ ] Create EVPKeyGuard class
- [ ] Fix resource leak paths (11 findings)
- [ ] Fix range temporaries (4 findings)
- [ ] Fix container access (3 findings)
- [ ] Fix performance patterns (2 findings)

### Following Sprint (Phase 2c - MEDIUM)
- [ ] Apply remaining RAII wrappers
- [ ] Fix string perf (5 findings)
- [ ] Fix map→unordered_map (2 findings)
- [ ] Update stale docs (2 findings)
- [ ] Polish exception handling (2 findings)

---

## Risk Ranking

### P0 (Fix Immediately - Blocking Production)
1. Line 306 - WasmtimeBundle dtor → Memory leak
2. Line 357 - File I/O no timeout → Hangs
3. Line 559/59 - Thread join → Shutdown hangs

### P1 (Fix This Sprint - Required)
1. Lines 34, 35, 95, 298 - Resource leaks → Memory leak
2. Line 388 - Explicit delete → Use-after-free
3. Lines 256, 257, 277 - Wasmtime leak → Memory leak

### P2 (Fix Next Sprint - Important)
1. Lines 131, 151, 383, 277 - Range temporaries → Crash risk
2. Lines 292, 474 - Performance → Scalability
3. Lines 5, 579 - Uninitialized → Crash

### P3 (Refactor - Quality)
1. All MEDIUM findings → Code maintainability

