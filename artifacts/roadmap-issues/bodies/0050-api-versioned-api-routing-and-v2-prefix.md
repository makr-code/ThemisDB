### Context

This issue implements the roadmap item 'Versioned API Routing and `/v2/` Prefix' for the api domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: Versioned API Routing and `/v2/` Prefix

### Goal

Deliver the scoped changes for Versioned API Routing and `/v2/` Prefix in src/api/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### Versioned API Routing and `/v2/` Prefix
**Priority:** High
**Target Version:** v1.8.0

Current REST routes use unversioned paths (e.g., `/documents/{id}`). Introduce a `/v1/` prefix retroactively (with redirect from unversioned) and implement `/v2/` routes that support bulk operations, streaming query results, and async job tracking.

**Implementation Notes:**
- `[x]` Add `RouteVersionRouter` middleware in `include/server/route_version_router.h`; unversioned paths redirect 301 to `/v1/`; wired in `src/server/http_server.cpp`.
- `[x]` `/v1/` routes: exact current behaviour; unversioned paths redirect 301 to `/v1/` via `RouteVersionRouter::getRedirectTarget()`.
- `[x]` `/v2/documents` — bulk insert endpoint accepting `application/x-ndjson` body (newline-delimited JSON documents, up to 10,000 per request); implemented in `EntityApiHandler::handleBulkNdjson()`.
- `[x]` `/v2/query/stream` — SSE endpoint implemented via `QueryApiHandler::handleQueryStreamSse()`; registered as `Route::QueryStreamSseGet`.
- `[x]` `/v2/jobs/{id}` — async job status for long-running queries; store job state in `cache::AdaptiveQueryCache` with TTL = 1 hour.

**Performance Targets:**
- Bulk insert of 10,000 256-byte documents in < 500 ms end-to-end (network excluded).
- SSE streaming first-byte latency < 5 ms after query planning completes.

---

### Acceptance Criteria

- [ ] Add `RouteVersionRouter` middleware in `include/server/route_version_router.h`; unversioned paths redirect 301 to `/v1/`; wired in `src/server/http_server.cpp`.
- [ ] `/v1/` routes: exact current behaviour; unversioned paths redirect 301 to `/v1/` via `RouteVersionRouter::getRedirectTarget()`.
- [ ] `/v2/documents` — bulk insert endpoint accepting `application/x-ndjson` body (newline-delimited JSON documents, up to 10,000 per request); implemented in `EntityApiHandler::handleBulkNdjson()`.
- [ ] `/v2/query/stream` — SSE endpoint implemented via `QueryApiHandler::handleQueryStreamSse()`; registered as `Route::QueryStreamSseGet`.
- [ ] `/v2/jobs/{id}` — async job status for long-running queries; store job state in `cache::AdaptiveQueryCache` with TTL = 1 hour.
- [ ] Bulk insert of 10,000 256-byte documents in < 500 ms end-to-end (network excluded).
- [ ] SSE streaming first-byte latency < 5 ms after query planning completes.

### Relationships

- Roadmap row: #50 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/api/FUTURE_ENHANCEMENTS.md#versioned-api-routing-and-v2-prefix
- Source key: roadmap:50:api:v1.8.0:versioned-api-routing-and-v2-prefix

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:50:api:v1.8.0:versioned-api-routing-and-v2-prefix -->
<!-- roadmap-ref: row=50;module=api;target=v1.8.0 -->
<!-- roadmap-detail: src/api/FUTURE_ENHANCEMENTS.md#versioned-api-routing-and-v2-prefix -->
