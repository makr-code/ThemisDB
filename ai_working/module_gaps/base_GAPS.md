# base Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: base
- Generated: 2026-06-02 11:55:47
- Status: Critical Findings Present
- Total Findings: 156
- Actionable Findings (Critical + High): 74
- Affected Files: 8

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 15 |
| High | 59 |
| Medium | 82 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 46 |
| container | 30 |
| exception_safety | 24 |
| performance | 12 |
| raii | 11 |
| platform | 8 |
| reliability | 8 |
| legacy_duplication | 7 |
| concurrency | 4 |
| input_validation | 2 |
| security | 2 |
| uninitialized | 2 |
| observability | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/base/module_sandbox.cpp | 36 | 1 | 13 | 22 | 0 |
| src/base/plugin_dependency_graph.cpp | 35 | 0 | 4 | 31 | 0 |
| src/base/module_loader.cpp | 31 | 1 | 18 | 12 | 0 |
| src/base/hot_reload_manager.cpp | 19 | 1 | 17 | 1 | 0 |
| src/base/remote_registry_client.cpp | 19 | 12 | 2 | 5 | 0 |
| src/base/wasm_plugin_sandbox.cpp | 12 | 0 | 5 | 7 | 0 |
| src/base/ab_test_manager.cpp | 2 | 0 | 0 | 2 | 0 |
| src/base/wasm_runtime_injector.cpp | 2 | 0 | 0 | 2 | 0 |

## Full Scanner Findings

### src/base/module_sandbox.cpp
Total findings: 36

- Line 548: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: platform_->cgroup_path = cg_path;
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 12: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // AbiChecker: deep ABI compatibility validation for hot-reload
  Confidence: band=high; score=0.8
- Line 140: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // No deprecated symbols in v1.x
  Confidence: band=high; score=0.8
- Line 232: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Deprecated symbols are warnings only
  Confidence: band=high; score=0.8
- Line 506: severity=HIGH; category=posix_only_api
  Description: POSIX-only API getpid( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: const std::string safe_name = sanitizeCgroupName(module_name_) + "_" + std::to_string(::getpid());
- Line 583: severity=HIGH; category=posix_only_api
  Description: POSIX-only API getpid( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: procs << ::getpid() << "\n";
- Line 614: severity=HIGH; category=posix_only_api
  Description: POSIX-only API getpid( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: root_procs << ::getpid() << "\n";
- Line 111: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += (std::isalnum(c) || c == '_' || c == '-') ? static_cast<char>(c) : '_';
  Confidence: band=high; score=0.74
- Line 127: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: required_symbols_.push_back(sym);
  Confidence: band=high; score=0.74
- Line 182: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.issues.push_back("Required symbol missing: " + sym);
  Confidence: band=high; score=0.74
- Line 183: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.issues.push_back("Required symbol missing: " + sym);
- Line 197: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.issues.push_back("Deprecated symbol still present: " + sym
  Confidence: band=high; score=0.74
- Line 198: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.issues.push_back("Deprecated symbol still present: " + sym
- Line 217: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: combined.issues.push_back(i);
  Confidence: band=high; score=0.74
- Line 226: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: combined.issues.push_back(i);
  Confidence: band=high; score=0.74
- Line 233: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: combined.issues.push_back("[WARN] " + i);
  Confidence: band=high; score=0.74
- Line 234: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: combined.issues.push_back("[WARN] " + i);
- Line 304: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: launch_warnings_.push_back("WASM isolation requested but no WasmRuntime backend is registered; "
- Line 310: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: launch_warnings_.push_back("WASM isolation: WasmRuntimeInjector::create() returned nullptr; "
- Line 421: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: launch_warnings_.push_back("RLIMIT_AS not supported on this kernel – memory limit not enforced");
- Line 427: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: launch_warnings_.push_back("Memory limit: platform not supported");
- Line 485: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: launch_warnings_.push_back("RLIMIT_CPU not applied on this kernel – CPU time limit not enforced");
- Line 491: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: launch_warnings_.push_back("CPU limit: platform not supported");
- Line 655: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: launch_warnings_.push_back("Network isolation requires CAP_SYS_ADMIN (network namespace) – skipped")
- Line 657: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: launch_warnings_.push_back("Network isolation not supported on this platform");
- Line 670: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: launch_warnings_.push_back("Filesystem restrictions (chroot/bind-mount) require CAP_SYS_CHROOT – ski
- Line 673: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: launch_warnings_.push_back("Filesystem restrictions not supported on this platform");
- Line 682: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: launch_warnings_.push_back("Syscall filter (seccomp-bpf) requires additional privileges – skipped. "
- Line 686: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: launch_warnings_.push_back("Syscall filtering not supported on this platform");

### src/base/plugin_dependency_graph.cpp
Total findings: 35

- Line 70: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& mod : ModuleRegistry::instance().getAllModules()) {
- Line 152: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: auto start = std::find(path.begin(), path.end(), neighbour);
- Line 214: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = adj.find(cur);
- Line 214: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = adj.find(cur);
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 109: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(n));
  Confidence: band=high; score=0.74
- Line 121: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::set<std::string>>
  Confidence: band=high; score=0.74
- Line 124: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::set<std::string>> adj;
  Confidence: band=high; score=0.74
- Line 136: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::set<std::string>>& adj,
  Confidence: band=high; score=0.74
- Line 137: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, int>& color,
  Confidence: band=high; score=0.74
- Line 142: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: path.push_back(node);
  Confidence: band=high; score=0.74
- Line 142: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: path.push_back(node);
  Confidence: band=high; score=0.74
- Line 155: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cycles.push_back(cycle);
  Confidence: band=high; score=0.74
- Line 203: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ready.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 203: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ready.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 203: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ready.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 204: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ready.push_back(kv.first);
- Line 273: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += '\\';
  Confidence: band=high; score=0.74
- Line 274: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += '\\';
- Line 308: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: attrs += "style=dashed";
  Confidence: band=high; score=0.74
- Line 308: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: attrs += "style=dashed";
  Confidence: band=high; score=0.74
- Line 348: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"':  out += "\\\""; break;
  Confidence: band=high; score=0.74
- Line 349: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  out += "\\\""; break;
- Line 350: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': out += "\\\\"; break;
- Line 351: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\n': out += "\\n";  break;
- Line 352: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\r': out += "\\r";  break;
- Line 353: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\t': out += "\\t";  break;
- Line 416: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: order.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 417: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: order.push_back(kv.first);
- Line 433: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: di.versionConstraint += ">=" + e.minVersion;
  Confidence: band=high; score=0.74
- Line 433: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: di.versionConstraint += ">=" + e.minVersion;
  Confidence: band=high; score=0.74
- Line 434: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: di.versionConstraint += ">=" + e.minVersion;
- Line 438: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: di.versionConstraint += " ";
- Line 440: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: di.versionConstraint += "<=" + e.maxVersion;
- Line 442: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: deps[e.from].push_back(std::move(di));
  Confidence: band=high; score=0.74

### src/base/module_loader.cpp
Total findings: 31

- Line 1472: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: while ((n = ::read(pipe_fds[0], buf, sizeof(buf) - 1)) > 0) {
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    }', '', '    char buffer[kZoneIdBufferSize] = {};', '    DWORD bytesRead = 0;', '    ReadFile(hFile, buffer, kZoneIdBufferSize - 1, &bytesRead, nullptr);']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['                    signer->pChainContext->rgpChain[0]->rgpElement[0]->pCertContext;', '                if (cert) {', '                    char nameBuffer[kCertNameBufferSize] = {};', '                    CertGetNameStringA(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0,', '                                       nullptr, nameBuffer, kCertNameBufferSize);']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 318: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // STAGED: Validation stage - Check ABI compatibility
  Confidence: band=high; score=0.8
- Line 532: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : std::filesystem::directory_iterator(moduleDirectory)) {
- Line 1044: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // ABI Compatibility Implementation
  Confidence: band=high; score=0.8
- Line 1054: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // ABI compatibility rules:
  Confidence: band=high; score=0.8
- Line 1430: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pipe( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: if (::pipe(pipe_fds) != 0) {
- Line 1431: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pipe( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: spdlog::error("verifyGPGSignature: pipe() failed ({}): {}", errno, std::strerror(errno));
- Line 1435: severity=HIGH; category=posix_only_api
  Description: POSIX-only API fork( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: const pid_t child = ::fork();
- Line 1439: severity=HIGH; category=posix_only_api
  Description: POSIX-only API fork( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: spdlog::error("verifyGPGSignature: fork() failed ({}): {}", errno, std::strerror(errno));
- Line 1556: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: while (remaining >= sizeof(Elf64_Nhdr)) {
- Line 1772: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lk(watchdogMutex_);
- Line 1818: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = loadedModules_.find(name);
- Line 637: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(module);
  Confidence: band=high; score=0.74
- Line 998: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: quarantined.push_back(path);
  Confidence: band=high; score=0.74
- Line 1186: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: module.healthChecks.push_back(healthResult);
  Confidence: band=high; score=0.74
- Line 1215: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: ModuleMetadata ModuleLoader::extractMetadataFromHandle(void* handle) {
  Confidence: band=high; score=0.74
- Line 1467: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipe_fds[1]);
- Line 1476: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipe_fds[0]);
- Line 1591: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!metadata.empty()) metadata += "; ";
  Confidence: band=high; score=0.74
- Line 1646: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!metadata.empty()) metadata += "; ";
  Confidence: band=high; score=0.74
- Line 1646: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!metadata.empty()) metadata += "; ";
  Confidence: band=high; score=0.74
- Line 1693: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!metadata.empty()) metadata += "; ";
  Confidence: band=high; score=0.74
- Line 1693: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!metadata.empty()) metadata += "; ";
  Confidence: band=high; score=0.74
- Line 1804: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: snapshot.emplace_back(mod.name, mod.path);
  Confidence: band=high; score=0.74

### src/base/hot_reload_manager.cpp
Total findings: 19

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4135 feat(base): Upgrade HotRelo... (2026-03-12) | #3272 feat(plugins): runt
- Line 189: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: spdlog::debug("HotReloadManager: sandbox [{}]: {}", module_name, w);
- Line 267: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: loader_ptr->unloadModule(module_name);
- Line 269: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto load_result = loader_ptr->loadModule(backup_path, module_name);
- Line 286: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: spdlog::debug("HotReloadManager: sandbox rollback [{}]: {}", module_name, w);
- Line 354: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(name);
  Confidence: band=high; score=0.74

### src/base/remote_registry_client.cpp
Total findings: 19

- Line 62: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: future.wait();
- Line 94: severity=CRITICAL; category=missing_dtor
  Description: Class Task allocates resources but has no destructor
  Remediation: Add explicit destructor: ~Task() { /* cleanup */ }
  Context: class/struct Task
- Line 99: severity=CRITICAL; category=missing_dtor
  Description: Class TaskCompare allocates resources but has no destructor
  Remediation: Add explicit destructor: ~TaskCompare() { /* cleanup */ }
  Context: class/struct TaskCompare
- Line 116: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: cv_.wait(lock, [&] { return stop_token.stop_requested() || !tasks_.empty(); });
- Line 141: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 149: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 177: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: static_cast<std::ofstream *>(userp)->write(static_cast<char *>(contents), static_cast<std::streamsiz
- Line 384: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: return std::async(std::launch::async, [self = shared_from_this()]() { return self->listPlugins(); })
- Line 388: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: return std::async(std::launch::async, [self = shared_from_this(), name]() { return self->fetchPlugin
- Line 392: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: return std::async(std::launch::async, [self = shared_from_this(), entry]() { return self->downloadPl
- Line 731: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: auto self = weak_self.lock();
- Line 751: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: auto self = weak_self.lock();
- Line 412: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: auto f = std::async(std::launch::async, [ms] { std::this_thread::sleep_for(std::chrono::milliseconds
- Line 455: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(ms));
- Line 201: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 209: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 212: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 258: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back(std::move(entry));
  Confidence: band=high; score=0.74
- Line 684: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: out.close();

### src/base/wasm_plugin_sandbox.cpp
Total findings: 12

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4121 fix(base): WASM non-functio... (2026-03-12) | #4106 [WIP] Implement WAS
- Line 446: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!allocateLinearMemory()) {
- Line 638: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: bool found = std::find(allowed.begin(), allowed.end(), imp) != allowed.end();
- Line 660: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: bool WasmPluginSandbox::allocateLinearMemory() {
- Line 672: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: last_error_ = "Failed to allocate WASM linear memory (" + std::to_string(linear_memory_size_ / 1024
- Line 275: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: info.exports.push_back(exp_name);
  Confidence: band=high; score=0.74
- Line 632: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: allowed.push_back(hf.module_name + "." + hf.function_name);
  Confidence: band=high; score=0.74
- Line 639: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: unknown.push_back(imp);
  Confidence: band=high; score=0.74
- Line 661: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: load_warnings_.push_back("linear_memory_pages=0: WASM module gets no linear memory");
  Confidence: band=high; score=0.74
- Line 662: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: load_warnings_.push_back("linear_memory_pages=0: WASM module gets no linear memory");
- Line 698: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: load_warnings_.push_back("[OS sandbox] " + w);
  Confidence: band=high; score=0.74
- Line 699: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: load_warnings_.push_back("[OS sandbox] " + w);

### src/base/ab_test_manager.cpp
Total findings: 2

- Line 547: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active.push_back(id);
  Confidence: band=high; score=0.74
- Line 584: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rows.push_back(makeRow("control", entry.control.metrics));
  Confidence: band=high; score=0.74

### src/base/wasm_runtime_injector.cpp
Total findings: 2

- Line 55: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: reg.entries.push_back(std::move(desc));
  Confidence: band=high; score=0.74
- Line 115: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(p->name);
  Confidence: band=high; score=0.74

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
