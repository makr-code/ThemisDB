> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Base Module - Future Enhancements

## Scope

Plugin lifecycle management (`module_loader.cpp`, `hot_reload_manager.cpp`), secure sandboxing (`module_sandbox.cpp`, `wasm_plugin_sandbox.cpp`, `wasm_runtime_injector.cpp`), remote marketplace client (`remote_registry_client.cpp`), plugin dependency graph (`plugin_dependency_graph.cpp`), and A/B test framework (`ab_test_manager.cpp`). This module is a foundational dependency of every other ThemisDB module.

---

## Design Constraints

- `[x]` `loadedModules_` must support O(1) lookup by name; current `std::vector` + `std::find_if` is O(n) on every `get`/`unload` call. — replaced with `std::unordered_map` + `std::shared_mutex` (v1.8.0)
- `[ ]` Plugin load time (signature verify + dlopen + init hook) must be ≤ 200 ms per plugin on a warm filesystem.
- `[ ]` Hot-reload must achieve zero-downtime: existing in-flight queries using the old plugin version complete before teardown.
- `[ ]` Sandbox memory hard cap per plugin: 256 MB by default; configurable up to 2 GB via cgroup v2 `memory.max`, not just `RLIMIT_AS`.
- `[ ]` Signature verification must use Ed25519 (RFC 8032); RSA-2048 not accepted for new plugins.
- `[ ]` Plugin allowlist path checked on every load; symlink traversal outside the designated plugin directory is rejected.
- `[ ]` Rollback of a failed hot-reload must complete within 500 ms and restore the previous plugin version atomically.
- `[ ]` All lifecycle hooks (init, reload, shutdown) must complete within 5 s or are terminated and logged as failures.
- `[x]` WASM fuel/instruction metering must bound runaway plugin execution; modules exceeding the fuel limit must be terminated, not hung. — `WasmPluginSandbox::Config::max_instructions` + `fuel_check_interval` + `remainingFuel()` implemented (v1.8.0)
- `[x]` `RemoteRegistryClient` retry back-off (`std::this_thread::sleep_for`) must not block the calling thread; async scheduling required.

---

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `PluginLoader::load(path, manifest)` | Core module / plugin registry | Returns `PluginHandle` or structured error |
| `SignatureVerifier::verify(binary_path, sig_path, pubkey)` | `PluginLoader` | Ed25519; rejects on any mismatch |
| `HotReloadManager::reloadModule(name, new_path)` | Admin API / config watcher | Atomic swap; old handle kept until in-flight ops drain |
| `HotReloadManager::rollback(name)` | `HotReloadManager` error path | Must complete ≤ 500 ms |
| `PluginSandbox::createSandbox(plugin_id, limits)` | `PluginLoader` | cgroup v2 + seccomp; per-plugin resource policy |
| `MarketplaceClient::resolve(plugin_id, version)` | Plugin installer CLI / admin API | Returns download URL + signature; TLS required |
| `ABTestManager::recordEvent(test_id, variant, metric, value)` | All modules | Thread-safe; must not hold `tests_` mutex during callbacks |

---

## Planned Features

### O(1) Module Lookup — Replace `loadedModules_` Vector with Unordered Map
**Priority:** High
**Target Version:** v1.2.0
**Status:** ✅ Implemented (v1.8.0)

`loadedModules_` in `module_loader.cpp` is a `std::vector<ModuleInfo>`. Every lookup (`isLoaded`, `getModule`, `unload`, `watchdogLoop`) calls `std::find_if` over the entire list — O(n) per operation. With dozens of loaded plugins this is measurable overhead on every query dispatch.

**Implementation Notes:**
- `[x]` Replace `loadedModules_` (`std::vector`) with `std::unordered_map<std::string, ModuleInfo>` keyed by module name in `module_loader.cpp`.
- `[x]` Introduce a `shared_mutex` so `getModule` / `isLoaded` (read-only) use `shared_lock` and `load` / `unload` use `unique_lock`, reducing read contention.
- `[x]` The watchdog loop at line 1752 notes "loadedModules_ has no dedicated mutex in the existing design" — fix this by making the watchdog hold a `shared_lock` when iterating.
- `[x]` Update `ModuleLoader` unit tests to exercise concurrent `load`/`getModule`/`unload` with TSAN enabled.

**Performance Targets:**
- `getModule(name)` lookup: O(1) average, ≤ 1 µs under contention from 8 concurrent reader threads.

---

### cgroup v2 Resource Enforcement for Module Sandbox
**Priority:** High
**Target Version:** v1.2.0
**Status:** ✅ Implemented (v1.8.0)

`module_sandbox.cpp` uses `setrlimit(RLIMIT_AS)` and `setrlimit(RLIMIT_CPU)` as a "coarse fallback" (lines 372, 416–417). The source comments explicitly note that real production deployments need cgroup v2. The cgroup path is allocated in `platform_->cgroup_path` (line 238) but cleanup is commented out with "On a real production system, we'd also remove the cgroup" (line 330).

**Implementation Notes:**
- `[x]` Implement `setupCgroupV2()` in `module_sandbox.cpp`: write `memory.max` and `cpu.max` to `/sys/fs/cgroup/themis/<sandbox_id>/` using the pre-allocated `cgroup_path`.
- `[x]` Implement `teardownCgroupV2()` to remove the cgroup directory on `stop()` — replace the "would also remove the cgroup" placeholder comment.
- `[x]` Detect cgroup v2 availability at startup; fall back to `RLIMIT_*` with a `spdlog::warn` when unavailable (container environments without cgroup delegation).
- `[ ]` Add integration test that launches a sandbox plugin allocating > limit bytes and verifies it is killed within 500 ms. (Issue: #1574)

**Performance Targets:**
- Sandbox creation (cgroup v2 setup): ≤ 50 ms per plugin.

---

### WASM Instruction Fuel Metering
**Priority:** High
**Target Version:** v1.2.0
**Status:** ✅ Implemented (v1.8.0)

`wasm_plugin_sandbox.cpp` allocates linear memory and validates imports/exports but has no instruction-counting / fuel mechanism. A malicious or buggy WASM plugin can spin indefinitely without triggering any timeout.

**Implementation Notes:**
- `[x]` Add `WasmSandboxConfig::max_instructions` (default: 0 = unlimited) and `WasmSandboxConfig::fuel_check_interval` (default: 1) fields in `wasm_plugin_sandbox.h`.
- `[x]` Implement a fuel counter (`fuel_remaining_`) decremented by `fuel_check_interval` units on each `callExport()` call; when fuel reaches zero, set `last_error_` and return a structured "fuel exhausted" error without invoking the runtime.
- `[x]` Expose remaining fuel via `WasmPluginSandbox::remainingFuel()` for observability (returns `UINT64_MAX` when `max_instructions == 0`).
- `[x]` Add unit tests: fuel initialised from config, fuel deducted per call, exhausted fuel returns structured error, reload resets fuel, "infinite loop" bounded by budget (8 tests in `tests/test_wasm_plugin_sandbox.cpp`).

**Performance Targets:**
- Fuel check overhead: ≤ 3 % CPU overhead vs. unchecked execution on a tight compute loop.

---

### WASM Non-Function Import Parsing Completeness
**Priority:** Medium
**Target Version:** v1.2.0
**Status:** ✅ Implemented (v1.8.0)

In `wasm_plugin_sandbox.cpp` (lines 192–203), parsing of the imports section stops accumulating entries when a non-function import (table, memory, global) is encountered before all function imports have been listed. The comment acknowledges this limitation: "only the imports before the first non-function entry will appear in `info.imports`." This means capability-model enforcement is incomplete for WASM modules that declare memory/table imports before their function imports.

**Implementation Notes:**
- `[x]` Fix the import-section parser in `wasm_plugin_sandbox.cpp` to correctly skip non-function import descriptors (table: `0x01`, memory: `0x02`, global: `0x03`) and continue accumulating function imports regardless of ordering.
- `[x]` Add unit tests with WASM binaries that interleave memory and function imports; verify all function imports appear in `info.imports`.

---

### A/B Test Persistence and Observability Export
**Priority:** Medium
**Target Version:** v1.3.0
**Status:** ✅ Implemented

`ab_test_manager.cpp` stores `ABVariantMetrics` exclusively in memory (in `tests_` map). All metrics are lost on server restart. There is also no export to the observability stack (MetricsCollector / OpenTelemetry).

**Implementation Notes:**
- `[x]` Persist `ABTestConfig` and `ABVariantMetrics` snapshots to RocksDB using key prefix `ab_test::` via the `StorageEngine` interface; reload on `ABTestManager::start()`.
- `[x]` Emit per-variant counters (`ab_test.<test_id>.<variant>.requests`, `.conversions`, `.latency_p99`) to `MetricsCollector` on every `recordOutcome()` call without holding the `tests_` mutex.
- `[x]` Add `ABTestManager::exportMetricsSnapshot()` returning a `std::vector<ABTestMetricRow>` for admin API consumption.
- `[x]` Add a Bayesian Thompson Sampling auto-stop: when posterior probability that treatment beats control exceeds a configurable threshold (default 0.95), mark the test as concluded and route all traffic to the winner.

**Performance Targets:**
- `recordOutcome()` (hot path): ≤ 2 µs with metrics emission; no mutex held during MetricsCollector call. ✅

---

### Async Retry Back-Off in `RemoteRegistryClient`
**Priority:** Medium
**Target Version:** v1.3.0

`remote_registry_client.cpp` uses `std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms))` in both `httpGet` (line 309) and `httpGetBinary` (line 394) retry loops. This blocks the calling thread — potentially a server I/O thread — for up to 16 s.

**Implementation Notes:**
- [x] Replace blocking sleep with a `std::async`/future or a scheduler callback so the calling thread is released during back-off; use the existing `TaskScheduler` for delayed retry dispatch.
- [x] Add a `RemoteRegistryConfig::max_total_retry_time_ms` cap (default: 30 000 ms) to prevent retries from exceeding a caller's timeout budget.
- [x] Expose retry attempt count and last error in a `RemoteRegistryClient::lastRequestStats()` struct for observability.

---

### Hot-Reload Reader/Writer Lock Upgrade
**Priority:** Medium
**Target Version:** v1.3.0
**Status:** ✅ Implemented (v1.8.0)

`hot_reload_manager.cpp` uses a single `std::mutex` for all operations (lines 55–495). All `getVersion()`, `isLoaded()`, and status queries (read-only operations) contend with `reloadModule()` (write operation), limiting read throughput under concurrent query load.

**Implementation Notes:**
- `[x]` Replace `std::mutex mutex_` with `std::shared_mutex` in `HotReloadManager`; upgrade `getVersion`, `getCurrentVersion`, `isLoaded`, `getModuleNames` to `std::shared_lock`.
- `[x]` Keep `reloadModule` and `rollback` on `std::unique_lock`.
- `[ ]` Add TSAN-enabled test with 16 reader threads + 1 reload thread running concurrently. (Issue: #1574)

---

### Cross-Platform Module Format
**Priority:** Low
**Target Version:** v1.4.0

Universal module packaging format across Linux/macOS/Windows, including platform-independent manifest, auto-detected native library bundling, and resource embedding.

**Implementation Notes:**
- `[x]` Define a `PluginBundle` format (zip archive with `manifest.json`, native `.so`/`.dll`/`.dylib`, optional WASM fallback, and Ed25519 signature file).
- `[x]` Implement `PluginBundleLoader` in `module_loader.cpp` that unpacks to a temp dir, verifies signature, selects the correct native binary for the current platform, and delegates to the existing `PluginLoader`.
- `[x]` Support WASM-only bundles as a portable fallback when no native library for the current platform is present.

---

## Test Strategy

- **Unit tests** (≥ 90 % line coverage): `PluginLoader` path-validation logic; `SignatureVerifier` with valid, tampered, and missing signatures; `HotReloadManager` state machine transitions with TSAN.
- **Integration tests**: load 10 real plugin binaries (including one with an invalid signature); verify hot-reload cycles complete without dropping in-flight queries; verify rollback restores functionality after a broken plugin.
- **Sandbox tests**: attempt to exceed memory cap (256 MB) from within sandboxed plugin code; verify SIGKILL + structured error returned within 500 ms.
- **WASM tests**: parse WASM modules with interleaved non-function imports; verify fuel metering terminates infinite loops.
- **Fuzz tests** (libFuzzer): fuzz `PluginLoader` with malformed manifest JSON and adversarial binary paths (symlinks, null bytes, path traversal).
- **Marketplace mock tests**: dependency resolution with circular dependencies must return a clear error, never infinite loop.
- **CI coverage gate**: line coverage ≥ 85 % enforced; sandbox tests run in an isolated container.

## Performance Targets

- Plugin load (signature verify + dlopen + init): ≤ 200 ms per plugin on warm filesystem.
- Hot-reload swap (old → new, no in-flight queries): ≤ 150 ms end-to-end.
- Hot-reload rollback on failure: ≤ 500 ms to restore previous functional state.
- Signature verification (Ed25519, 1 MB binary): ≤ 5 ms.
- Sandbox creation (cgroup v2 setup): ≤ 50 ms per plugin.
- `getModule(name)` lookup: O(1) average, ≤ 1 µs under 8 concurrent readers.
- Plugin discovery scan of a 500-plugin directory: ≤ 1 s.

## Security / Reliability

- Ed25519 signature mandatory for all plugins; unsigned binaries rejected before dlopen; public key pinned in server config.
- Plugin paths canonicalised and restricted to the configured plugin root; symlink traversal outside root returns `EPERM`.
- Sandboxed plugins run under seccomp-bpf allowlist and cgroup v2 memory/CPU limits.
- Plugin init/shutdown hooks killed via `SIGKILL` if they exceed 5 s timeout; crash reported as structured error.
- Marketplace downloads verified by TLS + Ed25519 signature before installation; SHA-256 checksum logged for audit trail.
- All plugin load/unload/reload events written to immutable audit log with timestamp, plugin name, version, and outcome.

---

## See Also

- [README.md](README.md)
- [ROADMAP.md](ROADMAP.md)

*Last Updated: 2026-03-22*
*Module Version: v1.8.0*

---

## Security Hardening Backlog (Q3 2026)

> GAP-014 – identified via static analysis (2026-04-21).
> Reference: `docs/governance/SOURCECODE_COMPLIANCE_GOVERNANCE.md`.

### GAP-014 – Replace `popen(gpg …)` with `execvp()` in Module Loader

**Scope:** `src/base/module_loader.cpp:1449`

### Design Constraints
- GPG signature verification semantics must be preserved (exit code 0 + "Good signature" in output)
- The replacement must work on Linux (the primary deployment target); macOS support is secondary
- kForbidden character check can be retained as defence-in-depth but must not be relied upon
  as the sole injection mitigation

### Required Interfaces
```cpp
// New helper: run gpg without shell, capture stdout+stderr via pipe pair
struct GpgResult { int exit_code; std::string output; };
static GpgResult runGpgVerify(const std::string& sig_path, const std::string& module_path);
```
- Uses `pipe()` + `fork()` + `execvp("gpg", ...)` + `waitpid()` pattern
- No `/bin/sh` involved; arguments are passed directly as `char* const[]`

### Implementation Notes
```cpp
// Sketch:
const char* args[] = {"gpg", "--verify", sig_path.c_str(), module_path.c_str(), nullptr};
// pipe stdout+stderr to parent, execvp in child, waitpid in parent
```
- `execvp` resolves `gpg` from `PATH`; alternatively use full path `/usr/bin/gpg` from config
- The child's stdout+stderr is redirected to a pipe; the parent reads it after `waitpid`

### Test Strategy
- Unit test with a mock `gpg` binary (shell script) that exits 0 and prints "Good signature"
- Unit test with a mock `gpg` that exits 1 → function returns `false`
- Unit test with a path containing `'` → verify that no shell interprets the quote

### Performance Targets
- Execution time: ≤ 500 ms (dominated by gpg's asymmetric crypto, not the syscall overhead)

### Security / Reliability
- No `/bin/sh` invoked; all arguments are null-terminated strings passed directly to `execvp`
- On `fork()` failure: return `false` (fail-closed)
- Child process timeout: `alarm(30)` in child to avoid hanging indefinitely
