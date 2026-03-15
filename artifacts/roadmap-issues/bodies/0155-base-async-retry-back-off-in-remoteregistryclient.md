### Context

This issue implements the roadmap item 'Async Retry Back-Off in `RemoteRegistryClient`' for the base domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.3.0.

Primary detail section: Async Retry Back-Off in `RemoteRegistryClient`

### Goal

Deliver the scoped changes for Async Retry Back-Off in `RemoteRegistryClient` in src/base/ and complete the linked detail section in a release-ready state for v1.3.0.

### Detailed Scope

### Async Retry Back-Off in `RemoteRegistryClient`
**Priority:** Medium
**Target Version:** v1.3.0

`remote_registry_client.cpp` uses `std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms))` in both `httpGet` (line 309) and `httpGetBinary` (line 394) retry loops. This blocks the calling thread — potentially a server I/O thread — for up to 16 s.

**Implementation Notes:**
- `[x]` Replace blocking sleep with a `std::async`/future or a scheduler callback so the calling thread is released during back-off; use the existing `TaskScheduler` for delayed retry dispatch.
- `[x]` Add a `RemoteRegistryConfig::max_total_retry_time_ms` cap (default: 30 000 ms) to prevent retries from exceeding a caller's timeout budget.
- `[x]` Expose retry attempt count and last error in a `RemoteRegistryClient::lastRequestStats()` struct for observability.

---

### Acceptance Criteria

- [x] Replace blocking sleep with a `std::async`/future or a scheduler callback so the calling thread is released during back-off; use the existing `TaskScheduler` for delayed retry dispatch.
- [x] Add a `RemoteRegistryConfig::max_total_retry_time_ms` cap (default: 30 000 ms) to prevent retries from exceeding a caller's timeout budget.
- [x] Expose retry attempt count and last error in a `RemoteRegistryClient::lastRequestStats()` struct for observability.

### Relationships

- Roadmap row: #155 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/base/FUTURE_ENHANCEMENTS.md#async-retry-back-off-in-remoteregistryclient
- Source key: roadmap:155:base:v1.3.0:async-retry-back-off-in-remoteregistryclient

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:155:base:v1.3.0:async-retry-back-off-in-remoteregistryclient -->
<!-- roadmap-ref: row=155;module=base;target=v1.3.0 -->
<!-- roadmap-detail: src/base/FUTURE_ENHANCEMENTS.md#async-retry-back-off-in-remoteregistryclient -->
