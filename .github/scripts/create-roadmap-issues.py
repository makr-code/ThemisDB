#!/usr/bin/env python3
"""
create-roadmap-issues.py
------------------------
Batch-creates GitHub Issues for the ThemisDB source-code roadmap backlog
based on src/ROADMAP.md (generated 2026-03-12) and module-level
FUTURE_ENHANCEMENTS.md files.

Covers:
  - Batch A : 🔴 Critical (4 items, #3825–#3828)
  - Batch B : 🟠 High Priority — Immediate ≤ v1.4.0 (30 items, #3833–#3862)

Usage:
    GITHUB_TOKEN=<token> GITHUB_REPOSITORY=makr-code/ThemisDB \\
        python3 create-roadmap-issues.py

    # Preview only:
    DRY_RUN=1 python3 create-roadmap-issues.py

    # Single batch:
    BATCH=A python3 create-roadmap-issues.py
    BATCH=B python3 create-roadmap-issues.py

Environment variables:
    GITHUB_TOKEN       – Personal access token with `repo` scope
    GITHUB_REPOSITORY  – Owner/repo (default: makr-code/ThemisDB)
    DRY_RUN            – "1" to print without creating (default: "0")
    BATCH              – "A" = Critical only, "B" = High-Immediate only,
                         "" = all (default)

Source: src/ROADMAP.md · src/*/FUTURE_ENHANCEMENTS.md
"""

import json
import os
import sys
import time
from urllib.request import Request, urlopen
from urllib.error import HTTPError

GITHUB_API = "https://api.github.com"
GITHUB_TOKEN = os.environ.get("GITHUB_TOKEN", "")
GITHUB_REPOSITORY = os.environ.get("GITHUB_REPOSITORY", "makr-code/ThemisDB")
DRY_RUN = os.environ.get("DRY_RUN", "0") == "1"
BATCH = os.environ.get("BATCH", "").upper()

# ---------------------------------------------------------------------------
# Batch A — 🔴 Critical Priority  (GA-Blocker)
# Source: src/ROADMAP.md §Critical Priority
# ---------------------------------------------------------------------------

BATCH_A_CRITICAL = [
    {
        "roadmap_id": "#3825",
        "title": "[security][auth] Thread-Safety: Add std::mutex to JWTValidator JWKS Cache",
        "labels": ["security", "thread-safety", "module:auth", "correctness", "priority:critical", "status:open"],
        "body": """\
## Summary

`JWTValidator` in `src/auth/jwt_validator.cpp` exposes a data race on the JWKS
cache fields `jwks_cache_` and `jwks_cache_time_`. When multiple threads call
`JWTValidator::validate()` concurrently and the cache expires, they all race
into `fetchJWKS()` simultaneously — writing from multiple threads is undefined
behaviour under C++11 and later.

## Module

`src/auth/` · `include/auth/jwt_validator.h`

## Priority / Target Version

🔴 Critical · v1.1.0

## Detailed Implementation Description

See: [auth/FUTURE_ENHANCEMENTS.md §1](../src/auth/FUTURE_ENHANCEMENTS.md#1-thread-safety-add-mutex-to-jwtvalidator-jwks-cache)

## Acceptance Criteria

- [ ] Add `mutable std::shared_mutex jwks_cache_mutex_` to `jwt_validator.h` alongside `jwks_cache_`
- [ ] Wrap all reads of `jwks_cache_` in `fetchJWKS()` with `std::shared_lock`; all writes with `std::unique_lock`
- [ ] Implement double-checked locking: acquire shared lock first, check staleness, upgrade to unique lock only if refresh is needed
- [ ] Add unit test: spawn 32 threads each calling `validate()` concurrently with cache TTL=0, no crash under TSAN
- [ ] Warm-cache path overhead: zero (shared_lock allows concurrent readers)
- [ ] At most one actual HTTP fetch per cache expiry under concurrent load

## Labels

`security`, `thread-safety`, `module:auth`, `correctness`, `priority:critical`
""",
    },
    {
        "roadmap_id": "#3826",
        "title": "[security][auth] LDAP DN and Filter Injection Prevention",
        "labels": ["security", "injection", "module:auth", "priority:critical", "status:open"],
        "body": """\
## Summary

`ldap_authenticator.cpp:buildUserDN()` substitutes the raw `username` into a
DN template with no escaping. An attacker can supply DN special characters
(`,`, `=`, `+`, `<`, `>`, `#`, `;`, `\\`, `"`) to manipulate the constructed
DN and bind as a different directory entry. This is a textbook LDAP injection
vulnerability.

## Module

`src/auth/` · `src/auth/ldap_authenticator.cpp`

## Priority / Target Version

🔴 Critical (Security) · v1.1.0

## Detailed Implementation Description

See: [auth/FUTURE_ENHANCEMENTS.md §3](../src/auth/FUTURE_ENHANCEMENTS.md#3-ldap-dn-and-filter-injection-prevention)

## Acceptance Criteria

- [ ] Implement `escapeLDAPDNComponent(const std::string& value)` following RFC 4514 §2.4
- [ ] Implement `escapeLDAPFilterValue(const std::string& value)` following RFC 4515 §3 (escape `*`, `(`, `)`, `\\`, NUL)
- [ ] Call `escapeLDAPDNComponent()` on `username` inside `buildUserDN()` before substitution (line 96)
- [ ] Call `escapeLDAPFilterValue()` on all user-controlled values in LDAP search filter strings (lines 257, 379)
- [ ] Add `LDAP_OPT_REFERRALS = LDAP_OPT_OFF` on both Windows and POSIX code paths
- [ ] Add libFuzzer fuzz test targeting `buildUserDN()` with adversarial username inputs
- [ ] Escaping adds < 5 µs overhead per authentication call

## Labels

`security`, `injection`, `module:auth`, `priority:critical`
""",
    },
    {
        "roadmap_id": "#3827",
        "title": "[build][themis] Modular Build System",
        "labels": ["build", "module:themis", "infrastructure", "priority:critical", "status:open"],
        "body": """\
## Summary

ThemisDB currently uses a monolithic CMake build system. All modules compile
into a single binary. This blocks independent module versioning, optional
feature gating (Community/Enterprise/Cloud editions), and plugin hot-loading.
A modular build system is required before GA.

## Module

`src/themis/` · `cmake/CMakeLists.txt`

## Priority / Target Version

🔴 Critical · v1.7.0

## Detailed Implementation Description

See: [themis/FUTURE_ENHANCEMENTS.md §Modular Build System](../src/themis/FUTURE_ENHANCEMENTS.md#modular-build-system)

## Acceptance Criteria

- [ ] Extract core implementations to `src/themis/`
- [ ] Create `libthemis-base.so` / `themis-base.dll` as first shared library target
- [ ] Update `cmake/CMakeLists.txt` to use `add_library(themis-base SHARED …)`
- [ ] Add export macros (`THEMIS_API`) to all public headers
- [ ] Other module libraries (`themis-storage`, `themis-query`) link against `themis-base`
- [ ] Build verified on both Windows and Linux
- [ ] Add compile-time test that fails if LZ4 compress/decompress stubs remain in a release build

## Labels

`build`, `module:themis`, `infrastructure`, `priority:critical`
""",
    },
    {
        "roadmap_id": "#3828",
        "title": "[build][themis] Module Loader Implementation",
        "labels": ["build", "module:themis", "infrastructure", "priority:critical", "status:open"],
        "body": """\
## Summary

The `ModuleLoader` class exists as a header/stub but does not implement secure
`dlopen`/`LoadLibrary` with SHA-256 hash verification, signature verification,
or audit logging. Without a working module loader, runtime plugin loading and
the planned edition feature-gating system cannot function.

## Module

`src/themis/` · `include/themis/module_loader.h`

## Priority / Target Version

🔴 Critical · v1.7.0

## Detailed Implementation Description

See: [themis/FUTURE_ENHANCEMENTS.md §Module Loader Implementation](../src/themis/FUTURE_ENHANCEMENTS.md#module-loader-implementation)

## Acceptance Criteria

- [ ] Implement `ModuleLoader` class in `src/themis/module_loader.cpp`
- [ ] Implement platform-specific loaders: `module_loader_linux.cpp` (`dlopen`) and `module_loader_win32.cpp` (`LoadLibrary`)
- [ ] Implement `ModuleSecurityVerifier` in `src/themis/module_security.cpp`
- [ ] SHA-256 hash verification against signed manifest before `dlopen`/`LoadLibrary`; mismatch aborts load
- [ ] Signature verification (X.509/GPG) with allowlist/blacklist support
- [ ] Zone.Identifier detection on Windows (quarantine flag)
- [ ] Authenticode verification on Windows
- [ ] Audit logging on every load/unload attempt
- [ ] Module load time (signature verify + dlopen + init hook): ≤ 200 ms per plugin

## Labels

`build`, `module:themis`, `infrastructure`, `priority:critical`
""",
    },
]

# ---------------------------------------------------------------------------
# Batch B — 🟠 High Priority — Immediate (≤ v1.4.0)
# Source: src/ROADMAP.md §High Priority — Immediate
# ---------------------------------------------------------------------------

BATCH_B_HIGH_IMMEDIATE = [
    {
        "roadmap_id": "#3833",
        "title": "[security][auth] Constant-Time Comparison for Recovery Codes and Session IDs",
        "labels": ["security", "timing-attack", "module:auth", "priority:high", "status:open"],
        "body": """\
## Summary

`mfa_authenticator.cpp:173` uses `std::find` for recovery code lookup — subject
to timing oracle via early-exit short-circuit. `session_manager.cpp:205` uses
`unordered_map::find` with `std::string::operator==` for session ID lookup,
also timing-vulnerable. An attacker measuring sub-microsecond timing differences
can brute-force recovery codes or session tokens.

## Module

`src/auth/` · `src/auth/mfa_authenticator.cpp` · `src/auth/session_manager.cpp`

## Priority / Target Version

🟠 High (Security) · v1.1.0

## Detailed Implementation Description

See: [auth/FUTURE_ENHANCEMENTS.md §4](../src/auth/FUTURE_ENHANCEMENTS.md#4-constant-time-comparison-for-recovery-codes-and-session-ids)

## Acceptance Criteria

- [ ] In `mfa_authenticator.cpp`, replace `std::find` over recovery codes with a loop using `CRYPTO_memcmp()` that always iterates all entries regardless of match (prevents early-exit timing leak)
- [ ] In `session_manager.cpp`, store session IDs as SHA-256 hash in the lookup map; compare incoming tokens by hashing first (normalises comparison time)
- [ ] Add microbenchmark: TOTP/recovery-code verification latency variance < 100 ns regardless of match position (list of 10 codes)
- [ ] CI gate: latency variance < 100 µs under sanitizer/scheduler overhead
- [ ] All existing auth tests continue to pass

## Labels

`security`, `timing-attack`, `module:auth`, `priority:high`
""",
    },
    {
        "roadmap_id": "#3834",
        "title": "[security][auth] Mandatory JWT Issuer and Audience Validation",
        "labels": ["security", "jwt", "module:auth", "priority:high", "status:open"],
        "body": """\
## Summary

`jwt_validator.cpp` silently skips issuer and audience validation when
`expected_issuer` / `expected_audience` are empty strings — a misconfiguration
footgun that allows tokens from any issuer to pass validation in production if
the config is incomplete.

## Module

`src/auth/` · `src/auth/jwt_validator.cpp` · `include/auth/jwt_validator.h`

## Priority / Target Version

🟠 High (Security) · v1.1.0

## Detailed Implementation Description

See: [auth/FUTURE_ENHANCEMENTS.md §5](../src/auth/FUTURE_ENHANCEMENTS.md#5-mandatory-jwt-issuer-and-audience-validation)

## Acceptance Criteria

- [ ] Replace `expected_issuer`/`expected_audience` plain strings with `std::optional<std::string>` in `JWTValidator::Config`
- [ ] Add `require_issuer_validation = true` / `require_audience_validation = true` flags
- [ ] Throw `std::runtime_error` in constructor if require flag is true but field is unset
- [ ] Emit `spdlog::warn` when field is unset and require flag is false
- [ ] Unit tests: validate token with correct/wrong issuer, correct/wrong audience, missing issuer, missing audience

## Labels

`security`, `jwt`, `module:auth`, `priority:high`
""",
    },
    {
        "roadmap_id": "#3835",
        "title": "[security][auth] Secure Memory for Key Material in jwks_security.cpp",
        "labels": ["security", "memory", "module:auth", "priority:high", "status:open"],
        "body": """\
## Summary

`jwks_security.cpp:274–276` passes `client_key_password` directly as a plain
`std::string` via `CURLOPT_KEYPASSWD`. Key material may linger in process
memory long after use because `std::string` does not zero memory on destruction.
Any heap dump or core file exposes the private key passphrase.

## Module

`src/auth/` · `src/auth/jwks_security.cpp`

## Priority / Target Version

🟠 High · v1.2.0

## Detailed Implementation Description

See: [auth/FUTURE_ENHANCEMENTS.md §10](../src/auth/FUTURE_ENHANCEMENTS.md#10-secure-memory-for-key-material-in-jwks_securitycpp)

## Acceptance Criteria

- [ ] Introduce a `SecureString` RAII wrapper that calls `OPENSSL_cleanse()` / `explicit_bzero()` in its destructor
- [ ] Replace all plain `std::string` holding key material in `jwks_security.cpp` with `SecureString`
- [ ] Ensure `CURLOPT_KEYPASSWD` callback reads from `SecureString` without copying to a plain string
- [ ] Add unit test: after `SecureString` destruction, verify memory region is zeroed (using ASAN/MSAN or manual check)
- [ ] No plain `std::string` containing key passwords remains in the auth module

## Labels

`security`, `memory`, `module:auth`, `priority:high`
""",
    },
    {
        "roadmap_id": "#3837",
        "title": "[auth] Token Blacklist Persistence and Distributed Support",
        "labels": ["distributed", "auth", "module:auth", "priority:high", "status:open"],
        "body": """\
## Summary

`token_blacklist.cpp` is currently in-memory only. All revoked tokens are lost
on process restart, making it impossible to enforce token revocation across
rolling deployments or after crashes.

## Module

`src/auth/` · `src/auth/token_blacklist.cpp` · `include/auth/token_blacklist.h`

## Priority / Target Version

🟠 High · v1.3.0

## Detailed Implementation Description

See: [auth/FUTURE_ENHANCEMENTS.md §7](../src/auth/FUTURE_ENHANCEMENTS.md#7-token-blacklist-persistence-and-distributed-support)

## Acceptance Criteria

- [ ] Define abstract `ITokenBlacklist` interface with `add(jti, expiry)`, `isRevoked(jti)`, `purgeExpired()`
- [ ] Implement `RedisTokenBlacklist : ITokenBlacklist` using Redis `SET jti EX ttl NX`
- [ ] Implement `RocksDBTokenBlacklist : ITokenBlacklist` with dedicated CF and background expiry thread
- [ ] Add hand-rolled Bloom filter (double-hash, ~1% false-positive rate) for non-revoked fast path
- [ ] Bound in-memory list to `max_entries = 1,000,000` with earliest-expiry eviction
- [ ] `isRevoked()` hot path (warm Bloom filter, non-revoked): ≤ 1 µs
- [ ] Redis-backed `isRevoked()`: ≤ 2 ms P99 on local network

## Labels

`distributed`, `auth`, `module:auth`, `priority:high`
""",
    },
    {
        "roadmap_id": "#3838",
        "title": "[performance][auth] LDAP Connection Pooling",
        "labels": ["performance", "module:auth", "priority:high", "status:open"],
        "body": """\
## Summary

`ldap_authenticator.cpp` opens a new LDAP connection for every authentication
request and closes it immediately after. Under load this causes connection-setup
overhead (TCP + TLS handshake + LDAP bind) on every call, dramatically
increasing authentication latency.

## Module

`src/auth/` · `src/auth/ldap_authenticator.cpp`

## Priority / Target Version

🟠 High · v1.2.0

## Detailed Implementation Description

See: [auth/FUTURE_ENHANCEMENTS.md §8](../src/auth/FUTURE_ENHANCEMENTS.md#8-ldap-connection-pooling)

## Acceptance Criteria

- [ ] Implement `LDAPConnectionPool` with configurable `min_connections` (default 2) and `max_connections` (default 16)
- [ ] Pool handles connection health checks and automatic reconnect on broken connections
- [ ] `authenticate()` acquires a connection from the pool with a configurable timeout
- [ ] Pool pre-warms `min_connections` on startup
- [ ] Authentication latency P99 ≤ 20 ms for warm pool (vs. > 100 ms per new connection)
- [ ] Pool metrics exposed: active, idle, pending-acquire counts

## Labels

`performance`, `module:auth`, `priority:high`
""",
    },
    {
        "roadmap_id": "#3839",
        "title": "[performance][base] O(1) Module Lookup — Replace loadedModules_ Vector with Unordered Map",
        "labels": ["performance", "module:base", "priority:high", "status:open"],
        "body": """\
## Summary

`loadedModules_` in `module_loader.cpp` is a `std::vector<ModuleInfo>`. Every
lookup (`isLoaded`, `getModule`, `unload`, `watchdogLoop`) calls `std::find_if`
over the entire list — O(n) per operation. With dozens of plugins this is
measurable overhead on every query dispatch.

## Module

`src/base/` · `src/base/module_loader.cpp`

## Priority / Target Version

🟠 High · v1.2.0

## Detailed Implementation Description

See: [base/FUTURE_ENHANCEMENTS.md §O(1) Module Lookup](../src/base/FUTURE_ENHANCEMENTS.md#o1-module-lookup--replace-loadedmodules_-vector-with-unordered-map)

## Acceptance Criteria

- [ ] Replace `loadedModules_` (`std::vector`) with `std::unordered_map<std::string, ModuleInfo>` keyed by module name
- [ ] Introduce `std::shared_mutex`: `getModule`/`isLoaded` use `shared_lock`, `load`/`unload` use `unique_lock`
- [ ] Watchdog loop (line 1752) holds `shared_lock` when iterating
- [ ] Update `ModuleLoader` unit tests for concurrent `load`/`getModule`/`unload` with TSAN enabled
- [ ] `getModule(name)` lookup: O(1) average, ≤ 1 µs under contention from 8 concurrent reader threads

## Labels

`performance`, `module:base`, `priority:high`
""",
    },
    {
        "roadmap_id": "#3840",
        "title": "[security][base] cgroup v2 Resource Enforcement for Module Sandbox",
        "labels": ["security", "sandbox", "module:base", "priority:high", "status:open"],
        "body": """\
## Summary

`module_sandbox.cpp` uses `setrlimit(RLIMIT_AS)` and `setrlimit(RLIMIT_CPU)` as
a "coarse fallback". Production deployments require cgroup v2 for hard memory
caps. The cgroup path is allocated but `teardownCgroupV2()` is commented out
with "On a real production system, we'd also remove the cgroup".

## Module

`src/base/` · `src/base/module_sandbox.cpp`

## Priority / Target Version

🟠 High · v1.2.0

## Detailed Implementation Description

See: [base/FUTURE_ENHANCEMENTS.md §cgroup v2 Resource Enforcement](../src/base/FUTURE_ENHANCEMENTS.md#cgroup-v2-resource-enforcement-for-module-sandbox)

## Acceptance Criteria

- [ ] Implement `setupCgroupV2()`: write `memory.max` and `cpu.max` to `/sys/fs/cgroup/themis/<sandbox_id>/`
- [ ] Implement `teardownCgroupV2()`: remove the cgroup directory on `stop()`
- [ ] Detect cgroup v2 availability at startup; fall back to `RLIMIT_*` with `spdlog::warn` when unavailable
- [ ] Add integration test: sandbox plugin allocating > limit bytes is killed within 500 ms
- [ ] Sandbox creation (cgroup v2 setup): ≤ 50 ms per plugin
- [ ] Sandbox memory hard cap: 256 MB default, configurable up to 2 GB via `memory.max`

## Labels

`security`, `sandbox`, `module:base`, `priority:high`
""",
    },
    {
        "roadmap_id": "#3841",
        "title": "[security][base] WASM Instruction Fuel Metering",
        "labels": ["security", "wasm", "module:base", "priority:high", "status:open"],
        "body": """\
## Summary

`wasm_plugin_sandbox.cpp` does not limit the number of WASM instructions a
plugin may execute. A malicious or buggy plugin can run an infinite loop,
starving the host process indefinitely. Instruction fuel metering must bound
runaway plugin execution.

## Module

`src/base/` · `src/base/wasm_plugin_sandbox.cpp` · `include/base/wasm_plugin_sandbox.h`

## Priority / Target Version

🟠 High · v1.2.0

## Detailed Implementation Description

See: [base/FUTURE_ENHANCEMENTS.md §WASM Instruction Fuel Metering](../src/base/FUTURE_ENHANCEMENTS.md#wasm-instruction-fuel-metering)

## Acceptance Criteria

- [ ] Add `WasmPluginSandbox::Config::max_instructions` (default 1,000,000) and `fuel_check_interval`
- [ ] Implement `remainingFuel()` accessor
- [ ] Modules exceeding the fuel limit are terminated (not hung), with a structured error returned to caller
- [ ] Add `Config::fuel_check_interval` to avoid checking every instruction (performance trade-off)
- [ ] Unit test: plugin with infinite loop is terminated within `fuel_check_interval` cycles
- [ ] Fuel overhead: < 5% throughput reduction on typical short-lived WASM modules

## Labels

`security`, `wasm`, `module:base`, `priority:high`
""",
    },
    {
        "roadmap_id": "#3842",
        "title": "[stub-replacement][chimera] Production ThemisDB Adapter Integration",
        "labels": ["stub-replacement", "module:chimera", "priority:high", "status:open"],
        "body": """\
## Summary

The Chimera multi-database bridge uses an in-process stub/simulation for the
ThemisDB adapter instead of connecting to a real ThemisDB instance via the
native wire protocol. This means any application using Chimera in production
cannot actually write to or read from ThemisDB.

## Module

`src/chimera/` · `include/chimera/`

## Priority / Target Version

🟠 High · v1.1.0

## Detailed Implementation Description

See: [chimera/FUTURE_ENHANCEMENTS.md §Production ThemisDB Adapter Integration](../src/chimera/FUTURE_ENHANCEMENTS.md#production-themisdb-adapter-integration)

## Acceptance Criteria

- [ ] Implement `ThemisDBAdapter` using the ThemisDB native wire protocol (not in-process simulation)
- [ ] Support all CRUD operations: `insert`, `update`, `delete`, `query`, `upsert`
- [ ] TLS connection with configurable CA certificate verification
- [ ] Connection pooling with health checks and automatic reconnect
- [ ] Expose all adapter errors as structured `ChimeraError` types (no swallowed exceptions)
- [ ] Integration test against a live ThemisDB instance (use Docker Compose in CI)

## Labels

`stub-replacement`, `module:chimera`, `priority:high`
""",
    },
    {
        "roadmap_id": "#3843",
        "title": "[stub-replacement][chimera] MongoDB / Qdrant / Neo4j: Replace In-Process Simulation with Real Drivers",
        "labels": ["stub-replacement", "module:chimera", "priority:high", "status:open"],
        "body": """\
## Summary

Chimera's MongoDB, Qdrant, and Neo4j adapters use in-process simulation
(fake data stores) instead of connecting to real external databases via their
native drivers. This makes Chimera unsuitable for any production use case
involving these databases.

## Module

`src/chimera/` · `include/chimera/`

## Priority / Target Version

🟠 High · v1.2.0

## Detailed Implementation Description

See: [chimera/FUTURE_ENHANCEMENTS.md §MongoDB / Qdrant / Neo4j](../src/chimera/FUTURE_ENHANCEMENTS.md#mongodb--qdrant--neo4j-replace-in-process-simulation-with-real-drivers)

## Acceptance Criteria

- [ ] `MongoDBAdapter`: replace simulation with `mongocxx` driver; support BSON documents, collections, indexes
- [ ] `QdrantAdapter`: replace simulation with Qdrant REST/gRPC client; support vector upsert, search, filter
- [ ] `Neo4jAdapter`: replace simulation with Bolt protocol driver; support Cypher queries, transactions
- [ ] Each adapter has an integration test against its real database (Docker Compose in CI)
- [ ] Adapter configuration via `ChimeraConfig` YAML (host, port, auth, TLS)
- [ ] Structured error mapping: database-specific errors → `ChimeraError` codes

## Labels

`stub-replacement`, `module:chimera`, `priority:high`
""",
    },
    {
        "roadmap_id": "#3844",
        "title": "[correctness][chimera] Transaction Management Enhancements",
        "labels": ["correctness", "module:chimera", "priority:high", "status:open"],
        "body": """\
## Summary

Chimera's transaction management does not implement proper ACID semantics across
adapters. Cross-adapter transactions (e.g. writing to ThemisDB and MongoDB
atomically) rely on best-effort compensation with no formal 2PC or SAGA
coordinator, leading to possible partial failures with no rollback.

## Module

`src/chimera/` · `include/chimera/`

## Priority / Target Version

🟠 High · v1.1.0

## Detailed Implementation Description

See: [chimera/FUTURE_ENHANCEMENTS.md §Transaction Management Enhancements](../src/chimera/FUTURE_ENHANCEMENTS.md#transaction-management-enhancements)

## Acceptance Criteria

- [ ] Define `IChimeraTransaction` interface with `begin()`, `commit()`, `rollback()`, `savepoint()`
- [ ] Implement per-adapter transaction wrappers for ThemisDB, MongoDB, and Neo4j
- [ ] Implement SAGA coordinator for cross-adapter transactions with compensating actions
- [ ] On partial failure: all completed steps are compensated in reverse order
- [ ] Transaction timeout with automatic rollback (configurable, default 30 s)
- [ ] Unit + integration tests covering commit, rollback, timeout, and partial-failure compensation

## Labels

`correctness`, `module:chimera`, `priority:high`
""",
    },
    {
        "roadmap_id": "#3845",
        "title": "[reliability][chimera] Error Recovery and Retry Logic",
        "labels": ["reliability", "module:chimera", "priority:high", "status:open"],
        "body": """\
## Summary

Chimera adapters do not implement retry logic for transient failures (network
timeouts, temporary unavailability). A single failed request causes the entire
operation to fail immediately, even when retrying would succeed.

## Module

`src/chimera/` · `include/chimera/`

## Priority / Target Version

🟠 High · v1.1.0

## Detailed Implementation Description

See: [chimera/FUTURE_ENHANCEMENTS.md §Error Recovery and Retry Logic](../src/chimera/FUTURE_ENHANCEMENTS.md#error-recovery-and-retry-logic)

## Acceptance Criteria

- [ ] Implement exponential back-off retry with jitter for all adapter operations (`max_retries`, `base_delay_ms`, `max_delay_ms`)
- [ ] Distinguish retryable errors (network timeout, connection reset) from non-retryable (auth failure, not-found)
- [ ] Circuit breaker per adapter: trip after N consecutive failures, half-open after cooldown
- [ ] Retry budget: total time across retries ≤ configurable deadline
- [ ] Dead-letter queue for operations that exhaust retries (configurable: drop or DLQ)
- [ ] Metrics: retry count, circuit-breaker state, DLQ depth per adapter

## Labels

`reliability`, `module:chimera`, `priority:high`
""",
    },
    {
        "roadmap_id": "#3846",
        "title": "[performance][chimera] Batch Operation Enhancements",
        "labels": ["performance", "module:chimera", "priority:high", "status:open"],
        "body": """\
## Summary

Chimera executes all bulk insert/update/delete operations as individual
single-document API calls. This causes O(n) round-trips for large datasets
and is orders of magnitude slower than native bulk APIs offered by MongoDB,
Qdrant, and ThemisDB.

## Module

`src/chimera/` · `include/chimera/`

## Priority / Target Version

🟠 High · v1.1.0

## Detailed Implementation Description

See: [chimera/FUTURE_ENHANCEMENTS.md §Batch Operation Enhancements](../src/chimera/FUTURE_ENHANCEMENTS.md#batch-operation-enhancements)

## Acceptance Criteria

- [ ] `batchInsert(docs, batch_size)` uses MongoDB `insertMany()`, Qdrant `upsert_points()`, ThemisDB bulk import API
- [ ] `batchUpdate(filter, updates)` uses adapter-native bulk update operations
- [ ] `batchDelete(filter)` uses adapter-native bulk delete operations
- [ ] Configurable `batch_size` (default 1000); larger batches auto-split
- [ ] Benchmark: 10,000 document insert via `batchInsert` completes in ≤ 2 s on local instance
- [ ] Error handling: partial batch failure returns per-document error list

## Labels

`performance`, `module:chimera`, `priority:high`
""",
    },
    {
        "roadmap_id": "#3847",
        "title": "[persistence][maintenance] Schedule Persistence (RocksDB)",
        "labels": ["persistence", "module:maintenance", "priority:high", "status:open"],
        "body": """\
## Summary

`MaintenanceScheduler` stores all schedules in memory. After a process restart,
all scheduled maintenance windows (vacuum, compaction, backup) are lost and must
be manually reconfigured. This is not acceptable for production deployments.

## Module

`src/maintenance/` · `include/maintenance/`

## Priority / Target Version

🟠 High · v1.1.0

## Detailed Implementation Description

See: [maintenance/FUTURE_ENHANCEMENTS.md §Schedule Persistence (RocksDB)](../src/maintenance/FUTURE_ENHANCEMENTS.md#schedule-persistence-rocksdb)

## Acceptance Criteria

- [ ] Persist all schedule definitions to a dedicated RocksDB column family (`maintenance_schedules`)
- [ ] Reload schedules from RocksDB on startup before accepting any maintenance requests
- [ ] Schedule CRUD operations (`add`, `update`, `delete`) update both in-memory state and RocksDB atomically
- [ ] Schema versioning: handle RocksDB schema upgrades gracefully
- [ ] Unit test: write schedules, restart service, verify schedules are restored
- [ ] Write latency for schedule persistence: ≤ 5 ms per operation

## Labels

`persistence`, `module:maintenance`, `priority:high`
""",
    },
    {
        "roadmap_id": "#3848",
        "title": "[api][maintenance] Force-Run Endpoint: Window Override",
        "labels": ["api", "module:maintenance", "priority:high", "status:open"],
        "body": """\
## Summary

The maintenance module has no API to force-execute a scheduled task outside its
configured maintenance window. Operators need this for urgent maintenance
(emergency compaction, manual backup) without waiting for the next scheduled
window or modifying the schedule.

## Module

`src/maintenance/` · `include/maintenance/`

## Priority / Target Version

🟠 High · v1.1.0

## Detailed Implementation Description

See: [maintenance/FUTURE_ENHANCEMENTS.md §Force-Run Endpoint: Window Override](../src/maintenance/FUTURE_ENHANCEMENTS.md#force-run-endpoint-window-override)

## Acceptance Criteria

- [ ] Add `POST /admin/maintenance/{task_id}/force-run` REST endpoint
- [ ] Endpoint requires `system_admin` role (RBAC check via `requireAccess("system_admin")`)
- [ ] Force-run bypasses window check but respects task-level mutex (no concurrent execution of the same task)
- [ ] Force-run is logged in audit trail with operator identity and reason field
- [ ] Return 202 Accepted with task execution ID; `/admin/maintenance/runs/{id}` for status polling
- [ ] Unit test: verify force-run bypasses window check and is audited

## Labels

`api`, `module:maintenance`, `priority:high`
""",
    },
    {
        "roadmap_id": "#3849",
        "title": "[feature][temporal] Full System-Versioned Table Support",
        "labels": ["feature", "module:temporal", "priority:high", "status:open"],
        "body": """\
## Summary

ThemisDB lacks SQL:2011 system-versioned (temporal) table support. Without
this, tracking historical changes to records requires application-level
workarounds, and point-in-time queries are impossible at the database level.

## Module

`src/temporal/` · `include/temporal/`

## Priority / Target Version

🟠 High · v1.1.0

## Detailed Implementation Description

See: [temporal/FUTURE_ENHANCEMENTS.md §Full System-Versioned Table Support](../src/temporal/FUTURE_ENHANCEMENTS.md#full-system-versioned-table-support)

## Acceptance Criteria

- [ ] Automatic history table creation and management (`history_table_name` in `Config`)
- [ ] Transparent version tracking on all DML operations (`insert`, `update`, `upsert`, `deleteRow`)
- [ ] System-generated transaction time columns (`sys_time` with auto-assigned `now()`)
- [ ] Efficient storage of historical versions with configurable `retention_period`
- [ ] `enforceRetentionPolicy()` background cleanup respects ≤ 100 ms per batch
- [ ] Integration with table DDL operations (`createVersionedTable` static factory with JSON schema)
- [ ] History table write overhead: < 15% over non-temporal insert
- [ ] AQL `FOR SYSTEM_TIME AS OF <ts>` query syntax integration (Phase 3b)

## Labels

`feature`, `module:temporal`, `priority:high`
""",
    },
    {
        "roadmap_id": "#3850",
        "title": "[correctness][temporal] Temporal Conflict Detection and Resolution",
        "labels": ["correctness", "module:temporal", "priority:high", "status:open"],
        "body": """\
## Summary

The temporal module does not detect or resolve conflicts when concurrent
transactions write to overlapping time periods in a system-versioned table.
Without conflict detection, temporal data can become inconsistent under
concurrent write workloads.

## Module

`src/temporal/` · `include/temporal/`

## Priority / Target Version

🟠 High · v1.1.0

## Detailed Implementation Description

See: [temporal/FUTURE_ENHANCEMENTS.md §Temporal Conflict Detection and Resolution](../src/temporal/FUTURE_ENHANCEMENTS.md#temporal-conflict-detection-and-resolution)

## Acceptance Criteria

- [ ] Detect overlapping period writes for the same key in the same system-versioned table
- [ ] On conflict: return structured `TemporalConflictError` with conflicting version details
- [ ] Support configurable resolution strategies: `REJECT` (default), `LAST_WRITE_WINS`, `MERGE_USER_DEFINED`
- [ ] Conflict check runs within the same `RocksDB WriteBatch` as the DML operation (atomic)
- [ ] Unit test: two concurrent writers for overlapping periods → exactly one succeeds or custom resolver invoked
- [ ] Conflict detection adds ≤ 5 ms latency per write on single-node deployment

## Labels

`correctness`, `module:temporal`, `priority:high`
""",
    },
    {
        "roadmap_id": "#3851",
        "title": "[correctness][temporal] Snapshot Isolation for Temporal Queries",
        "labels": ["correctness", "module:temporal", "priority:high", "status:open"],
        "body": """\
## Summary

Time-travel queries (`FOR SYSTEM_TIME AS OF <ts>`) do not guarantee snapshot
isolation. A concurrent write completing during query execution can cause the
query to read partially updated data, violating the "immutable snapshot"
semantic expected from temporal queries.

## Module

`src/temporal/` · `include/temporal/`

## Priority / Target Version

🟠 High · v1.1.0

## Detailed Implementation Description

See: [temporal/FUTURE_ENHANCEMENTS.md §Snapshot Isolation](../src/temporal/FUTURE_ENHANCEMENTS.md#snapshot-isolation)

## Acceptance Criteria

- [ ] Time-travel queries acquire a RocksDB snapshot at the specified `AS OF` timestamp
- [ ] All reads during the query use this snapshot exclusively — no write-lock acquisition
- [ ] Snapshot is released immediately after the query completes
- [ ] Concurrent writes after snapshot creation are invisible to the time-travel query
- [ ] Unit test: write row, take snapshot, update row, run time-travel query → sees pre-update value
- [ ] Time-travel queries must not acquire write locks; confirmed by TSAN / lock analysis

## Labels

`correctness`, `module:temporal`, `priority:high`
""",
    },
    {
        "roadmap_id": "#3852",
        "title": "[feature][temporal] Application-Versioned Tables (Bi-Temporal)",
        "labels": ["feature", "module:temporal", "priority:high", "status:open"],
        "body": """\
## Summary

ThemisDB lacks support for application-versioned (valid-time) tables — the
second axis of the SQL:2011 bi-temporal model. Without valid-time support,
scenarios such as retroactive corrections, future-dated records, and regulatory
"as-reported" vs. "as-of" queries cannot be implemented.

## Module

`src/temporal/` · `include/temporal/`

## Priority / Target Version

🟠 High · v1.2.0

## Detailed Implementation Description

See: [temporal/FUTURE_ENHANCEMENTS.md §Application-Versioned Tables (Bi-Temporal)](../src/temporal/FUTURE_ENHANCEMENTS.md#application-versioned-tables-bi-temporal)

## Acceptance Criteria

- [ ] `BiTemporalTable` class supporting user-controlled valid-time periods (`valid_from`, `valid_to`)
- [ ] `insertWithValidTime(key, doc, valid_from, valid_to)` and `updateForValidTime(key, doc, period)`
- [ ] `BiTemporalTable::queryBiTemporal(key, transaction_time, valid_time)` combining both axes
- [ ] Temporal foreign keys: `TemporalForeignKey::validate()` — period-aware referential integrity
- [ ] Temporal uniqueness constraints: reject inserts overlapping an existing period for the same key
- [ ] Gap and overlap detection: `findGaps()` and `findOverlaps()` helpers
- [ ] AQL `FOR APPLICATION_TIME FROM … TO …` query syntax integration

## Labels

`feature`, `module:temporal`, `priority:high`
""",
    },
    {
        "roadmap_id": "#3853",
        "title": "[feature][temporal] Time-Travel Query Engine",
        "labels": ["feature", "module:temporal", "priority:high", "status:open"],
        "body": """\
## Summary

ThemisDB has no query engine support for time-travel queries. Operators cannot
query the state of the database at a past point in time using SQL:2011 syntax
(`AS OF`, `FROM … TO`, `BETWEEN … AND`). This is required for audit, compliance,
and debugging workflows.

## Module

`src/temporal/` · `include/temporal/temporal_query_engine.h`

## Priority / Target Version

🟠 High · v1.2.0

## Detailed Implementation Description

See: [temporal/FUTURE_ENHANCEMENTS.md §Time-Travel Query Engine](../src/temporal/FUTURE_ENHANCEMENTS.md#time-travel-query-engine)

## Acceptance Criteria

- [ ] `TemporalQueryEngine::executeTemporalQuery(table, query, spec)` with `spec` encoding `AS_OF`, `FROM_TO`, `BETWEEN_AND`
- [ ] AQL surface syntax: `FOR x IN t FOR SYSTEM_TIME AS OF "2025-01-01T00:00:00Z" RETURN x`
- [ ] Time-travel queries use RocksDB snapshots (read-only, no write locks)
- [ ] Support both system-versioned and application-versioned table queries
- [ ] Unit test: insert v1, update to v2, query `AS OF (between v1 and v2 timestamp)` → returns v1
- [ ] Time-travel query performance: 80-95% of current table queries for point-in-time lookups

## Labels

`feature`, `module:temporal`, `priority:high`
""",
    },
    {
        "roadmap_id": "#3854",
        "title": "[performance][temporal] Temporal Indexes",
        "labels": ["performance", "module:temporal", "priority:high", "status:open"],
        "body": """\
## Summary

Time-travel queries currently require O(n) scan of the entire history table.
Temporal indexes (B-tree on transaction time, interval-tree on valid-time
periods) are needed to achieve acceptable query performance for tables with
large version histories.

## Module

`src/temporal/` · `include/temporal/temporal_index_manager.h`

## Priority / Target Version

🟠 High · v1.2.0

## Detailed Implementation Description

See: [temporal/FUTURE_ENHANCEMENTS.md §Temporal Indexes](../src/temporal/FUTURE_ENHANCEMENTS.md#temporal-indexes)

## Acceptance Criteria

- [ ] `TemporalIndexManager::createTemporalIndex(table, name, spec)` supporting `BTREE` (system time) and `INTERVAL_TREE` (valid time) index types
- [ ] Index entries updated atomically with history table writes (same `WriteBatch`)
- [ ] `AS OF` point queries use B-tree index for O(log n) lookup
- [ ] `FROM … TO` range queries use interval-tree for efficient overlap detection
- [ ] `EXPLAIN` output includes temporal index usage
- [ ] Benchmark: point-in-time lookup on 1M-version table completes ≤ 10 ms with index

## Labels

`performance`, `module:temporal`, `priority:high`
""",
    },
    {
        "roadmap_id": "#3855",
        "title": "[security] AQLInjectionDetector: AST-Level Validation",
        "labels": ["security", "injection", "module:security", "priority:high", "status:open"],
        "body": """\
## Summary

`AQLInjectionDetector` uses regex-based pattern matching to detect AQL
injection. Regex-based detection is inherently incomplete and can be bypassed
by adversarial encodings, whitespace manipulation, or novel injection patterns.
AST-level validation is required for reliable protection.

## Module

`src/security/` · `include/security/aql_injection_detector.h`

## Priority / Target Version

🟠 High · v1.4.0

## Detailed Implementation Description

See: [security/FUTURE_ENHANCEMENTS.md §AQLInjectionDetector AST-Level Validation](../src/security/FUTURE_ENHANCEMENTS.md#aqlinjectdetector-ast-level-validation)

## Acceptance Criteria

- [ ] Parse the AQL query into an AST using the existing `AQLParser` before executing any injection scan
- [ ] Validate the AST against an allowlist of permitted node types, operation types, and collection access patterns
- [ ] Reject queries containing dynamically constructed collection names, `FUNCTION()` calls not in whitelist, or `LET` assignments that shadow system variables
- [ ] Fall back to regex scan only if the parser itself fails (parse error)
- [ ] Detection must be fail-closed: parser exception → reject query, not allow
- [ ] Add test cases: known regex-bypass payloads that are caught at AST level
- [ ] Validation overhead: ≤ 2 ms per query (parser is already called by the query path)

## Labels

`security`, `injection`, `module:security`, `priority:high`
""",
    },
    {
        "roadmap_id": "#3856",
        "title": "[gpu][stub-replacement] query_accelerator.cpp: Replace CPU Fallback Stubs with Real CUDA/HIP",
        "labels": ["gpu", "stub-replacement", "performance", "module:gpu", "priority:high", "status:open"],
        "body": """\
## Summary

`src/performance/phase3/query_accelerator.cpp` contains 5 GPU kernel stubs that
always fall back to CPU implementations with a `// TODO: implement GPU` comment.
All GPU-accelerated query operations (hash join, aggregation, sort, filter,
projection) are silently executed on the CPU, providing no acceleration.

## Module

`src/performance/phase3/` · `src/gpu/`

## Priority / Target Version

🟠 High · v1.4.0

## Detailed Implementation Description

See: [gpu/FUTURE_ENHANCEMENTS.md §query_accelerator.cpp](../src/gpu/FUTURE_ENHANCEMENTS.md#query_acceleratorcpp-replace-cpu-fallback-stubs-with-real-cudahip-dispatch)

## Acceptance Criteria

- [ ] Implement CUDA kernel for hash join: `cudaHashJoin(left, right, result)` with configurable hash table size
- [ ] Implement CUDA kernel for aggregation: `cudaAggregate(input, group_keys, agg_funcs, result)`
- [ ] Implement CUDA kernel for sort: `cudaSort(input, sort_keys, result)` using thrust::sort or CUB
- [ ] Implement HIP equivalents for AMD GPU support
- [ ] Runtime dispatch: select CUDA vs. HIP based on detected GPU vendor at startup
- [ ] CPU fallback path preserved for non-GPU deployments, with `spdlog::info` logging
- [ ] Benchmark: GPU hash join on 10M rows: ≥ 4x speedup vs. CPU baseline on RTX-class GPU
- [ ] All existing query correctness tests pass (CPU and GPU paths produce identical results)

## Labels

`gpu`, `stub-replacement`, `performance`, `module:gpu`, `priority:high`
""",
    },
    {
        "roadmap_id": "#3857",
        "title": "[gpu][stub-replacement][index] GPU Vector Index: CUDA and HIP Backend Implementation",
        "labels": ["gpu", "stub-replacement", "module:index", "priority:high", "status:open"],
        "body": """\
## Summary

`src/index/gpu_vector_index.cpp` implements a GPU-accelerated HNSW vector index
but CUDA and HIP dispatch functions are never actually called — the code falls
through to CPU HNSW for all operations. Vector similarity search workloads get
no GPU acceleration.

## Module

`src/index/` · `include/index/gpu_vector_index.h`

## Priority / Target Version

🟠 High · v1.4.0

## Detailed Implementation Description

See: [index/FUTURE_ENHANCEMENTS.md §GPU Vector Index CUDA and HIP Backend Implementation](../src/index/FUTURE_ENHANCEMENTS.md#gpu-vector-index-cuda-and-hip-backend-implementation)

## Acceptance Criteria

- [ ] Implement `cudaHNSWBuild(vectors, M, ef_construction)` CUDA kernel for HNSW graph construction
- [ ] Implement `cudaHNSWSearch(query, k, ef_search)` CUDA kernel for k-NN search
- [ ] Implement HIP equivalents for AMD GPU support
- [ ] Runtime device detection: select CUDA/HIP/CPU backend at index construction time
- [ ] Remove silent k > kMaxK clamping (or make it an explicit error + log)
- [ ] Benchmark: k-NN search on 1M 1536-dim vectors: ≥ 8x speedup vs. CPU on RTX-class GPU
- [ ] GPU and CPU search results agree within cosine similarity tolerance 1e-4

## Labels

`gpu`, `stub-replacement`, `module:index`, `priority:high`
""",
    },
    {
        "roadmap_id": "#3858",
        "title": "[gpu][stub-replacement][geo] CUDA and OpenCL Implementation in gpu_backend_production.cpp",
        "labels": ["gpu", "stub-replacement", "module:geo", "priority:high", "status:open"],
        "body": """\
## Summary

`src/geo/gpu_backend_production.cpp` exposes GPU-accelerated geospatial distance
and containment operations but all kernel functions return placeholder results.
Geospatial distance matrix and polygon containment are computed on CPU despite
the GPU backend being selected.

## Module

`src/geo/` · `include/geo/gpu_backend_production.h`

## Priority / Target Version

🟠 High · v1.4.0

## Detailed Implementation Description

See: [geo/FUTURE_ENHANCEMENTS.md §CUDA and OpenCL Implementation in gpu_backend_production.cpp](../src/geo/FUTURE_ENHANCEMENTS.md#cuda-and-opencl-implementation-in-gpu_backend_productioncpp)

## Acceptance Criteria

- [ ] Implement CUDA geospatial distance kernel: WGS84 Haversine, batch up to 1M points → distance matrix
- [ ] Implement CUDA polygon containment kernel: point-in-polygon test, batch up to 1M points
- [ ] Implement OpenCL equivalents for cross-platform GPU support (AMD/Intel)
- [ ] Deterministic FP tolerance: results agree with CPU within ≤ 1e-6 for all WGS84 inputs
- [ ] Handle invalid geometry and NaN coordinates with structured errors (not silent NaN propagation)
- [ ] Benchmark: distance matrix for 1M point pairs: ≥ 8x speedup vs. CPU baseline on RTX-class GPU

## Labels

`gpu`, `stub-replacement`, `module:geo`, `priority:high`
""",
    },
    {
        "roadmap_id": "#3859",
        "title": "[correctness][aql] Post-Generation AQL Validation in translateNLToAQL()",
        "labels": ["correctness", "module:aql", "priority:high", "status:open"],
        "body": """\
## Summary

`llm_aql_handler.cpp:translateNLToAQL()` validates the LLM-generated query
using `AQLSyntaxHighlighter::annotateErrors()` which **only logs warnings** —
it never rejects or sanitises the output. `AQLQueryValidator::validate()` is
never invoked on LLM-generated queries. A structurally invalid query silently
reaches the caller and may be executed against the database.

## Module

`src/aql/` · `src/aql/llm_aql_handler.cpp`

## Priority / Target Version

🟠 High · v1.6.0

## Detailed Implementation Description

See: [aql/FUTURE_ENHANCEMENTS.md §1 Post-Generation AQL Validation](../src/aql/FUTURE_ENHANCEMENTS.md#1--post-generation-aql-validation-in-translatenlttoaql)

## Acceptance Criteria

- [ ] In `translateNLToAQL()`, after markdown-fence stripping, call `AQLQueryValidator::validate(aql_query)`; if any issue has severity `ERROR`, throw `LLMException(INVALID_RESPONSE, ...)`
- [ ] Apply same fix to `translateNLToAQLStreaming()` and `translateNLToAQLWithExamples()`
- [ ] Add retry path: if validation fails and retries remain, re-invoke LLM with error annotation as feedback
- [ ] Expose `TranslationValidationMode` enum (`WARN_ONLY`, `REJECT_ON_ERROR`, `RETRY_ON_ERROR`) on `LLMAQLHandler`
- [ ] Unit test: NL query that causes mock LLM to return broken AQL → `translateNLToAQL` throws
- [ ] Validation overhead: ≤ 1 ms per generated query

## Labels

`correctness`, `module:aql`, `priority:high`
""",
    },
    {
        "roadmap_id": "#3860",
        "title": "[thread-safety][correctness][aql] Eliminate Thread Leak in LLMTimeoutManager::executeWithTimeout()",
        "labels": ["thread-safety", "correctness", "module:aql", "priority:high", "status:open"],
        "body": """\
## Summary

`LLMTimeoutManager::executeWithTimeout()` (in `include/aql/llm_timeout_manager.h`)
calls `worker.detach()` when a timeout fires. The comment explicitly acknowledges
the leak. Under sustained load, a burst of LLM timeouts accumulates many
detached threads, each consuming ~8 MB stack and holding references to the
plugin manager. This is a resource-exhaustion DoS vector.

## Module

`src/aql/` · `include/aql/llm_timeout_manager.h`

## Priority / Target Version

🟠 High · v1.6.0

## Detailed Implementation Description

See: [aql/FUTURE_ENHANCEMENTS.md §2 Eliminate Thread Leak](../src/aql/FUTURE_ENHANCEMENTS.md#2--eliminate-thread-leak-in-llmtimeoutmanagerexecutewithtimeout)

## Acceptance Criteria

- [ ] Replace `std::thread` + `std::packaged_task` with `std::jthread` (C++20) and `std::stop_token`
- [ ] On timeout: call `request_stop()` and transfer jthread ownership to a thin background cleanup thread that joins when the worker finishes
- [ ] Apply same fix to `executeWithCancelToken()`
- [ ] Zero leaked threads after 1,000 sequential timeout events in the test suite
- [ ] Test: after `executeWithTimeout()` throws `TIMEOUT`, worker thread terminates within `timeout + 500 ms`
- [ ] Document `TimeoutConfig` defaults in struct: `infer=300s`, `rag=600s`, `embed=60s`, `model_load=900s`

## Labels

`thread-safety`, `correctness`, `module:aql`, `priority:high`
""",
    },
    {
        "roadmap_id": "#3861",
        "title": "[reliability][aql] Per-Operation-Type Circuit Breakers",
        "labels": ["reliability", "module:aql", "priority:high", "status:open"],
        "body": """\
## Summary

`LLMAQLHandler::Impl` uses a single `sharding::CircuitBreaker` shared across
`executeInfer()`, `executeRAG()`, and `executeEmbed()`. When `executeInfer`
accumulates 5 failures, the breaker trips and **all** LLM operations are
blocked — even RAG and EMBED, which might be functioning normally. This
creates unnecessary blast radius on INFER failures.

## Module

`src/aql/` · `src/aql/llm_aql_handler.cpp`

## Priority / Target Version

🟠 High · v1.6.0

## Detailed Implementation Description

See: [aql/FUTURE_ENHANCEMENTS.md §3 Per-Operation-Type Circuit Breakers](../src/aql/FUTURE_ENHANCEMENTS.md#3--per-operation-type-circuit-breakers)

## Acceptance Criteria

- [ ] Replace single `circuit_breaker_` with `std::unordered_map<std::string, sharding::CircuitBreaker>` keyed by `"infer"`, `"rag"`, `"embed"`, `"finetune"`
- [ ] Each `execute*()` method looks up its own breaker
- [ ] Per-command `CircuitBreaker::Config` injectable via `LLMAQLHandler::Config`
- [ ] Add `getCircuitBreakerStates()` method for observability; expose via `LLM STATS` command
- [ ] Existing `metrics.recordCircuitBreakerState("infer", "open")` extended to all command types
- [ ] Test: INFER breaker trip does not affect RAG or EMBED operations

## Labels

`reliability`, `module:aql`, `priority:high`
""",
    },
    {
        "roadmap_id": "#3862",
        "title": "[correctness][aql] Bounded Conversation History with Context-Window Budget",
        "labels": ["correctness", "module:aql", "priority:high", "status:open"],
        "body": """\
## Summary

`aql_conversation_context.cpp` retains all conversation history without a size
bound. For long-running sessions, the accumulated history can exceed the LLM
context window, causing truncation (silent loss of early context) or API errors.
There is no token-budget enforcement or eviction strategy.

## Module

`src/aql/` · `src/aql/aql_conversation_context.cpp` · `include/aql/aql_conversation_context.h`

## Priority / Target Version

🟠 High · v1.6.0

## Detailed Implementation Description

See: [aql/FUTURE_ENHANCEMENTS.md §9 Bounded Conversation History](../src/aql/FUTURE_ENHANCEMENTS.md#9--bounded-conversation-history-with-context-window-budget)

## Acceptance Criteria

- [ ] Add `ConversationContext::Config::max_tokens` (default 8,192) and `max_turns` (default 20) limits
- [ ] Track token count estimate for each message (word count * 1.3 approximation until accurate tokeniser available)
- [ ] On budget overflow: evict oldest non-system turns until budget is satisfied (sliding window)
- [ ] System prompt is never evicted; it is excluded from the rolling budget
- [ ] `summarize_on_overflow = true` config option: summarise evicted turns via LLM before discarding
- [ ] Unit test: add 100 turns; verify context size never exceeds `max_tokens` after each add
- [ ] Overflow eviction: ≤ 1 ms for 100-turn history

## Labels

`correctness`, `module:aql`, `priority:high`
""",
    },
]

# ---------------------------------------------------------------------------
# Combined list
# ---------------------------------------------------------------------------

ALL_ISSUES = []
if not BATCH or BATCH == "A":
    ALL_ISSUES.extend(BATCH_A_CRITICAL)
if not BATCH or BATCH == "B":
    ALL_ISSUES.extend(BATCH_B_HIGH_IMMEDIATE)


# ---------------------------------------------------------------------------
# GitHub API helpers
# ---------------------------------------------------------------------------

def _headers() -> dict:
    return {
        "Authorization": f"Bearer {GITHUB_TOKEN}",
        "Accept": "application/vnd.github+json",
        "X-GitHub-Api-Version": "2022-11-28",
    }


def create_issue(title: str, body: str, labels: list) -> dict | None:
    """Create a GitHub issue. Returns the created issue dict or None on failure."""
    url = f"{GITHUB_API}/repos/{GITHUB_REPOSITORY}/issues"
    payload = json.dumps({"title": title, "body": body, "labels": labels}).encode()
    req = Request(url, data=payload, headers=_headers(), method="POST")
    try:
        with urlopen(req) as resp:
            return json.loads(resp.read())
    except HTTPError as exc:
        print(f"  ❌  HTTP {exc.code}: {exc.read().decode()}", file=sys.stderr)
        return None


def issue_exists(title: str) -> bool:
    """Return True if an open or closed issue with this exact title already exists."""
    for state in ("open", "closed"):
        url = (
            f"{GITHUB_API}/repos/{GITHUB_REPOSITORY}/issues"
            f"?state={state}&per_page=100"
        )
        req = Request(url, headers=_headers())
        try:
            with urlopen(req) as resp:
                issues = json.loads(resp.read())
                if any(i["title"] == title for i in issues):
                    return True
        except HTTPError:
            pass
    return False


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    if not GITHUB_TOKEN and not DRY_RUN:
        print(
            "❌  GITHUB_TOKEN not set.\n"
            "    Export it or set DRY_RUN=1 to preview.\n"
            "    Usage: GITHUB_TOKEN=ghp_… GITHUB_REPOSITORY=makr-code/ThemisDB "
            "python3 create-roadmap-issues.py",
            file=sys.stderr,
        )
        return 1

    batch_label = f" (Batch {BATCH})" if BATCH else " (All batches)"
    print(f"Repository : {GITHUB_REPOSITORY}")
    print(f"Dry-run    : {DRY_RUN}")
    print(f"Batch      : {BATCH or 'A+B'}{batch_label}")
    print(f"Issues     : {len(ALL_ISSUES)}")
    print()

    created = 0
    skipped = 0
    failed = 0

    for issue in ALL_ISSUES:
        rid = issue["roadmap_id"]
        title = issue["title"]
        labels = issue["labels"]
        body = issue["body"]

        print(f"→ [{rid}] {title}")

        if DRY_RUN:
            print(f"  [DRY-RUN] Would create with labels: {labels}")
            created += 1
            continue

        if issue_exists(title):
            print("  ⚠️  Already exists — skipping")
            skipped += 1
            continue

        result = create_issue(title, body, labels)
        if result:
            print(f"  ✅  Created: #{result['number']} → {result['html_url']}")
            created += 1
        else:
            print("  ❌  Failed to create issue")
            failed += 1

        # Respect GitHub secondary rate limit (1 req/s is safe)
        time.sleep(1)

    print()
    print("=" * 60)
    print(f"Created : {created}")
    print(f"Skipped : {skipped}")
    print(f"Failed  : {failed}")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
