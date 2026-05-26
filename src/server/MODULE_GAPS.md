# server Module — Implementation Gap Analysis

**Status:** In Progress  
**Last Updated:** 2026-05-26  

---

## 📊 Gap Summary

This module's gap analysis is pending. Run the gap audit to populate this document:

```bash
python tools/gap_audit_pipeline_v2.py
```

---

## ✅ Recent Remediation (2026-05-26) — W1-S06: LLM Handler + HTTP/3 Session — Exception Boundaries

**Scope:** `src/server/llm_api_handler.cpp`, `src/server/http3_session.cpp`  
**Ticket:** W1-S06 · Priority P2  

### Fixes Applied

#### 1. Silent `catch (...)` blocks — `llm_api_handler.cpp` (uncaught_exception / CWE-390)

**Root cause:** Two `catch (...)` blocks discarded exceptions silently without any diagnostic
output, making production issues hard to diagnose.

- **JWT validation** (`validateBearerToken`): `jwt_validator_->parseAndValidate(*token)`
  threw an unknown exception (e.g., from an expired/malformed token, or from an underlying
  OpenSSL error). The existing comment said "Token validation failed" but logged nothing.
- **Model listing** (`handleOpenAIListModels`): `plugin_mgr.listModels()` threw an unknown
  exception. The intent was to return an empty list rather than an error, but the failure
  was completely invisible in production logs.

**Fix:**
- JWT catch block: Added `THEMIS_DEBUG(...)` log ("treating token as invalid") so that
  misconfigured JWT validators surface in debug-level logs without flooding production logs.
- Model listing catch block: Added `THEMIS_WARN(...)` log so that model enumeration failures
  are visible at WARN level while preserving the empty-list fallback behaviour.
- Other `catch (...)` blocks (`std::stoi` / `std::stoul` in query-parameter parsing) are
  intentionally silent — the default value is safe and no diagnostic is needed.

#### 2. Null session guard — `Http3Handler::onPacketReceived()` CID migration path (null_dereference)

**Root cause:** After a QUIC connection-ID migration, `sessions_[cid_it->second]` was
retrieved as `auto session = sess_it->second`. Sessions are always stored via `make_shared`,
so the value is guaranteed non-null, but static analysers flagged the subsequent
`session->handlePacket(...)` as a potential null dereference because they cannot see into
the insertion path.

**Fix:** Added an explicit null check on `session` with an early return and an error log.
Added a Doxygen-compatible comment explaining the invariant.

#### 3. `cleanup_timer_` `[this]` capture lifetime documented (no_timeout / lifetime)

**Root cause:** `Http3Handler::start()` and `cleanupInactiveSessions()` use `[this]`
captures in `cleanup_timer_.async_wait(...)` callbacks. Static analysers flag these as
"no_timeout" or potential dangling-pointer callbacks.

**Assessment:**
- `Http3Handler::stop()` calls `cleanup_timer_.cancel()` which delivers
  `boost::asio::error::operation_aborted` to the pending callback. The lambda checks
  `if (!ec)` and will not call `cleanupInactiveSessions()` for aborted completions.
- The pattern is safe AS LONG AS `stop()` is called before the `Http3Handler` is destroyed
  and the io_context is stopped before the object's memory is freed.

**Fix:** Added an explicit lifetime-assumption comment to the `start()` timer setup.

### Gap Delta (estimated)

| Type | Before | After |
|---|---|---|
| uncaught_exception MEDIUM (llm) | 5 | 2 improved (log added); 3 intentionally silent (documented) |
| null_dereference HIGH (http3) | 5 | 1 explicit guard added; 4 false positives (guarded before use) |
| no_timeout CRITICAL (http3) | 2 | Documented as false positives (timer expiry IS set) |

---

## ✅ Recent Remediation (2026-05-26) — W1-S05: Cache Admin + SSE Manager — Shared Container Safety

**Scope:** `src/server/sse_connection_manager.cpp`, `src/server/cache_admin_api_handler.cpp`  
**Ticket:** W1-S05 · Priority P1  

### Fixes Applied

#### 1. Data race — SSE `backgroundPollTask()` buffer-full check (data_race / CWE-362)

**Root cause:** `backgroundPollTask()` first snapshotted active connections under a brief
`shared_lock<shared_mutex>`, releasing the lock, and then performed the buffer-full
early-exit check (`conn->buffered_events.size() >= config_.max_buffered_events`) on the
shared `Connection` struct **outside the lock**. Concurrent calls to
`pollEventsWithSequences()` or `pollRawEventsWithSequences()` hold the exclusive lock while
erasing elements from `conn->buffered_events`, creating a data race on the vector.

**Fix:** Moved the buffer-full predicate into the snapshot loop inside the
`shared_lock` scope. The check now reads `conn->buffered_events.size()` under the shared
lock, which is safe because the write side (`backgroundPollTask()` write path) acquires the
exclusive lock before modifying `buffered_events`. Added an explanatory comment.

#### 2. False positives documented — `cache_admin_api_handler.cpp` (data_race)

**Scanner flags:** 16 CRITICAL data_race alerts on `cache_->getHealthStatus()`,
`cache_->invalidate()`, `cache_->getTenantStats()`, etc.

**Assessment:** `AdaptiveQueryCache` is a fully thread-safe implementation with internal
synchronisation across `l1_mutex_` (shared_mutex), `l2_mutex_`, `l3_mutex_`,
`tenant_mutex_`, and `coordinator_mutex_`. All public methods acquire the appropriate lock
before accessing internal state. No external lock is needed in `CacheAdminApiHandler`.
The scanner flags arise because the tool cannot trace into the shared_ptr target to inspect
its internal locking discipline.

**Fix:** Added a thread-safety clarification comment above the endpoint handlers section
documenting the invariant. No code change required.

#### 3. False positives documented — SSE iterator_invalidation (iterator_invalidation)

**Scanner flags:** 2 CRITICAL iterator_invalidation alerts at `connections_.find()` calls in
`unregisterConnection()` (L96) and `backgroundPollTask()` (L360).

**Assessment:** Both accesses are performed under the exclusive `connections_mutex_` lock and
the iterator is not retained after the `erase()` call. There is no iterator invalidation
risk. The scanner analyses the `erase` call without tracking that the invalidated iterator
is immediately discarded.

**Fix:** False positives — no code change required. The existing lock discipline is correct.

### Gap Delta (estimated)

| Type | Before | After |
|---|---|---|
| data_race CRITICAL (sse) | 5 | 1 real race fixed (L335); 4 false positives documented |
| data_race CRITICAL (cache_admin) | 16 | 0 real races; all 16 documented as false positives |
| iterator_invalidation CRITICAL (sse) | 2 | 0 real gaps; both documented as false positives |

---

## ✅ Recent Remediation (2026-05-26) — W1-S04: Retry / Timeout / Uncaught-Exception Hardening

**Scope:** `src/server/postgres_session.cpp`, `src/server/rpc/rpc_service_impl.cpp`  
**Ticket:** W1-S04 · Priority P1  

### Fixes Applied

#### 1. Buffer bounds validation — PostgreSQL Execute/Describe/Close messages (pointer_arithmetic / CWE-125)

**Root cause:** The wire-protocol dispatch loop in `onRead()` parsed the `'E'` (Execute),
`'D'` (Describe), and `'C'` (Close) messages by advancing `offset` after reading a
null-terminated portal/statement name, then immediately accessing `buffer_[offset]` (for
`'D'`/`'C'`) or `buffer_[offset] … buffer_[offset+3]` (for `'E'`) without verifying that
the remaining buffer is large enough. A malformed or truncated packet from an attacker or a
buggy client could cause an out-of-bounds read (OOB-R / CWE-125).

**Fix:**

```cpp
// Execute ('E'): guard before reading 4-byte maxRows
if (offset + 4 > bytes_transferred) {
    sendErrorResponse("ERROR", "08P01", "Malformed Execute message: missing maxRows field");
    break;
}
// Describe ('D') / Close ('C'): guard before reading type byte + name
if (offset + 2 > bytes_transferred) {
    sendErrorResponse("ERROR", "08P01", "Malformed Describe/Close message");
    break;
}
```

Also corrected the `int32_t maxRows` byte-shift expression to use `static_cast<uint8_t>`
casts to prevent sign-extension on platforms where `char` is signed.

#### 2. Uncaught exception chain — `translateQuery()` (uncaught_exception / CWE-248)

**Root cause:** `translateQuery()` calls `parseSelectQuery()`, `parseInsertQuery()`,
`parseUpdateQuery()`, `parseDeleteQuery()` — all of which throw `std::runtime_error` on
malformed input. Static analysis flagged the throw sites as "uncaught_exception" because
the callee functions lack internal try/catch blocks. All callers of `translateQuery()` are
already wrapped in `try { … } catch (const std::exception& e) { sendErrorResponse(…) }`,
so there is no actual abort risk. The documentation comment was missing, and the error
message for the unsupported-statement case was not including the statement text (making
debugging difficult).

**Fix:**
- Added an explanatory comment to `translateQuery()` documenting the exception contract
  (exceptions propagate to caller catch blocks that convert them to PostgreSQL
  `ErrorResponse` messages with SQLSTATE 42601).
- Changed the unsupported-statement error from `"Unsupported SQL statement type"` to
  `"Unsupported SQL statement type: <first-32-chars>"` to improve diagnostics.

#### 3. Lambda-scope storage null guard — `handleGetCollectionMetadata()` (null_dereference)

**Root cause:** `handleGetCollectionMetadata()` validates `storage` with `if (!storage)` at
the function level, then later constructs a `json result` using a immediately-invoked lambda
`[&] { … }()` that calls `storage->scanPrefix(…)`. Static analysers that evaluate the
lambda body independently (without visibility into the outer scope's null check) flagged
this as a potential null dereference.

**Fix:** Added an explicit `if (storage)` guard inside the lambda body with a comment
explaining that `storage` was already validated above. The lambda now compiles as trivially
safe from any analysis perspective.

### Retry / Timeout Assessment

| Component | Retry applicability | Timeout applicability |
|---|---|---|
| `postgres_session.cpp` | Not applicable — session-layer protocol handler; individual queries are stateful with the client and cannot be silently retried. Transient query-engine errors produce `ErrorResponse` (SQLSTATE 08006) + `ReadyForQuery` so the client can retry at its own discretion. | Handled by the Boost.Asio socket timer inherited from the parent `Session` (`armReadTimer`). Individual SQL parse steps complete in O(query-length) time and cannot block indefinitely. |
| `rpc_service_impl.cpp` | Handler calls are fully synchronous; retry belongs at the gRPC/HTTP client level. Per-handler errors are returned as structured `RPCErrorCode` responses. | gRPC deadline propagation is the responsibility of the transport layer (ThemisRPCServer). No in-handler retry budget is needed. |

### Gap Delta (estimated)

| Type | Before | After |
|---|---|---|
| pointer_arithmetic HIGH (postgres) | 18 | Reduced: 3 Execute/Describe/Close OOB guards added |
| uncaught_exception HIGH (postgres) | 11 | Documented as properly caught at callers; error message improved |
| null_dereference HIGH (rpc) | 2 | 1 storage lambda guard added; 1 already guarded (false positive documented) |

---

## ✅ Recent Remediation (2026-05-26) — W1-S03: Null/Pointer-Guard Standardisation

**Scope:** `src/server/mcp_server.cpp`, `src/server/voice_api_handler.cpp`,
           `include/server/voice_api_handler.h`  
**Ticket:** W1-S03 · Priority P1  

### Fixes Applied

#### 1. Null `std::function` guard — MCP tool/resource/prompt dispatch (null_dereference / CWE-476)

**Root cause:** `McpServer::handleToolsCall()`, `McpServer::handleResourcesRead()`, and
`McpServer::handlePromptsGet()` dispatched through `it->second.handler(…)` without first
checking whether the stored `std::function` was non-empty. A default-constructed or
explicitly-emptied handler would cause `std::bad_function_call` (silently caught by the
outer `catch` block, but producing an opaque `-32000 Tool execution failed` JSON-RPC error
rather than the more informative `-32601 handler not available` error). The same risk
existed in the post-approval re-dispatch path (`McpServer::handleAiApprove()`).

**Fix:**

```cpp
// Before each handler call:
if (!it->second.handler) {
    return createError(-32601, "Tool handler not available: " + name);
}
```

Applies to:
- `handleToolsCall()` — `ToolHandler`
- `handleResourcesRead()` — `ResourceHandler`
- `handlePromptsGet()` — `PromptHandler`
- `handleAiApprove()` post-approval re-dispatch — `ToolHandler`

#### 2. Defensive null guard — MCP transport pointer usage (null_dereference / CWE-476)

**Root cause:** `McpServer::start()` created transport objects with `std::make_shared` and
immediately called methods on the result without an explicit null check. `std::make_shared`
never returns `nullptr` (it throws `std::bad_alloc` on OOM), so the practical risk was
zero, but the pattern was flagged by the static analyser and the invariant was not explicit.

**Fix:** Added explicit `if (transport_ptr_)` guards after each `make_shared` call.
Includes an `spdlog::error` fallback path documenting the unreachable branch.

#### 3. Constructor null guard — `VoiceApiHandler::voice_assistant_` (null_dereference / CWE-476)

**Root cause:** `VoiceApiHandler` stores `voice_assistant_` as a `shared_ptr` taken from the
caller. All handler methods (handleSynthesize, handleVoiceCommand, handleStreamCommand,
handleWakeWordDetect, handleRecordCall, handleGenerateProtocol, handleGetSession,
handleUpdateSessionContext, handleDeleteSession, handleGetVoices, handleCreateMacro,
handleListMacros, handleGetMacro, handleUpdateMacro, handleDeleteMacro, handleListRecordings,
handleGetRecording, handleSearchTranscripts, handleStats, handleAuthEnroll, handleAuthVerify,
handleAuthAuthenticate, handleAuthIdentify, handleAuthListProfiles, handleAuthDeleteProfile)
all dereference `voice_assistant_` without a null check. Passing `nullptr` to the constructor
would cause a hard crash on the first request to any of these endpoints.

**Fix:**

```cpp
// In constructor body, immediately after member initialisation:
if (!voice_assistant_) {
    throw std::invalid_argument(
        "VoiceApiHandler: voice_assistant must be a non-null shared_ptr");
}
```

Failing fast at construction time surfaces the programming error at the call site rather
than producing a segfault deep inside a request handler. Matching `@throws` Doxygen
annotation added to the header declaration.

### Gap Delta (estimated, mcp_server.cpp + voice_api_handler.cpp)

| Type | Before | After |
|---|---|---|
| null_dereference (HIGH) | 7 (mcp) + 7 (voice) = 14 | 7 mcp → 0 real; voice guarded at ctor |
| pointer_arithmetic (HIGH) | 16 (mcp) + 25 (voice) = 41 | Structural false-positives documented; real risks guarded |

### Remaining False Positives (documented, not fixed)

The scanner flags the following patterns as `null_dereference` or `pointer_arithmetic`
even though they are correctly guarded:

- `(*body)["options"]`, `(*body)["custom_fields"]` in voice_api_handler.cpp — all preceded
  by `body->contains(…)` guard; `body` is `std::optional<json>` not a raw pointer.
- `db_->isOpen()` in mcp_server.cpp at L1544/1556/1565 — all inside blocks already guarded
  by `if (!db_ || !db_->isOpen())` at function entry.
- Range-for structured bindings on `clients_` and `sessions_` maps in mcp_server.cpp — these
  are value accesses on unordered_map, not unsafe pointer arithmetic.
- `result.metadata.*` field accesses in toolLLMComplete at L1720–1725 — value-type struct
  member access, not pointer dereference.

---



**Scope:** `src/server/http_server.cpp`, `include/server/http_server.h`  
**Ticket:** W1-S02 · Priority P0  

### Fixes Applied

#### 1. Data Race: `config_.request_timeout_ms` (data_race / CWE-362)
- **Root cause**: `handleConfig()` (POST /config hot-reload path) wrote `config_.request_timeout_ms`
  without synchronization, while `Session::armReadTimer()` and `SslSession::armReadTimer()` read it
  concurrently on worker threads.
- **Fix**: Added `std::atomic<uint32_t> request_timeout_ms_live_` to `HttpServer`.
  Initialized from `config_.request_timeout_ms` in the constructor. Hot-reload uses
  `request_timeout_ms_live_.store(…)` and `armReadTimer()` reads via `.load(…, relaxed)`.
  The timeout value is captured in the lambda (not re-loaded from the server pointer) to avoid
  an additional race in the async callback.

#### 2. Data Race: Feature flag hot-reload (data_race / CWE-362)
- **Root cause**: `handleConfig()` wrote `config_.feature_semantic_cache`,
  `config_.feature_llm_store`, `config_.feature_cdc`, and `config_.feature_timeseries` without
  synchronization; concurrent handler methods (e.g., `handleLlmInteractionPost`, `handleCapabilities`,
  `applyGovernanceHeaders`) read these same fields on other worker threads.
- **Fix**: Added four `std::atomic<bool>` shadow fields (`feature_*_live_`). All hot-reload
  writes now use `.store(…, relaxed)` and all concurrent handler reads use `.load(…, relaxed)`.
  Initialization-time reads in the constructor (before threads start) continue to access
  `config_.feature_*` directly without atomics, which is safe since no threads are running yet.

#### 3. Missing Write-Phase Timeout (no_timeout / CWE-400)
- **Root cause**: `Session::doWrite()` and `SslSession::doWrite()` initiated `async_write`
  without arming the I/O timer, allowing a slow or adversarial client that stops reading to
  hold a socket indefinitely.
- **Fix**: `armReadTimer()` is now called at the start of `doWrite()` and `cancelReadTimer()`
  is called at the start of `onWrite()` in both `Session` and `SslSession`. The existing timer
  is safely reusable because `cancelReadTimer()` is always called in `onRead()` before `doWrite()`
  is ever reached. Updated the `read_timer_` doc-comment to reflect its dual read/write role.

#### 4. Iterator Invalidation Risk: `audit_rate_buckets_` (iterator_invalidation)
- **Root cause**: `enforceAuditRateLimit()` used `operator[]` to look up the rate bucket, which
  implicitly inserts a default element and can trigger a rehash, invalidating all existing iterators
  (even though no dangling iterator was held in this specific case, the pattern is scanner-flagged
  and carries latent risk when the code is extended).
- **Fix**: Replaced `audit_rate_buckets_[key]` with `audit_rate_buckets_.try_emplace(key)` which
  makes the insertion intent explicit and returns a stable iterator to the (possibly new) element
  via structured bindings. All access remains under `audit_rate_mutex_`.

### Gap Delta for `http_server.cpp` (CRITICAL)
| Type | Before | After (estimated) |
|---|---|---|
| data_race | 150 (server module) | Reduced: 5 hot spots eliminated |
| no_timeout | 25 (server module) | Reduced: 2 write-phase timeouts added |
| iterator_invalidation | 56 (server module) | Reduced: 1 operator[] rehash risk removed |

---

## ✅ Recent Remediation (2026-05-19)

- Removed token-value logging from:
  - `src/server/auth_middleware.cpp` (`AuthMiddleware::authorize`)
  - `src/server/http_server.cpp` (`HttpServer::handlePiiDeleteByUuid`)
- Removed Authorization header value logging and temporary `[AUTH-DBG]` stderr diagnostics from `src/server/http_server.cpp` (`HttpServer::requireAccess`).
- Removed detailed auth decision logging (`user_id` / `reason`) and token-validation diagnostics in `src/server/http_server.cpp` PII-delete authorization paths.
- Removed startup `validateToken` debug block in `HttpServer` constructor that ran on every server start and logged `user_id`/`reason` without operational value (GAP-011/CWE-532 clean-up).
- Added STUB/SIMULATION NOTE to HTTP/2 server-push `ResponseBuffer` raw `new` usage documenting the nghttp2 C API constraint and the planned migration to shared_ptr (see `FUTURE_ENHANCEMENTS.md §HTTP2 BufferManagement`).
- Security impact: avoids leaking bearer token fragments and internal validation state into logs (CWE-532 hardening).
- Related test coverage: `tests/test_auth_middleware.cpp`
  - `AuthMiddlewareGap013Test.DeniedReason_DoesNotEchoPresentedToken`
  - `AuthMiddlewareGap013Test.InsufficientScope_ReasonDoesNotEchoToken`
  - `AuthMiddlewareGap013Test.ValidateToken_ReasonDoesNotEchoToken`
  - `AuthMiddlewareGap013Test.ConcurrentDenyRequests_NoCrossContamination`

---

## 🚀 How to Use This Documentation

Once generated, this file will contain:

- **Gap Statistics:** Count of unimplemented paths, TODOs, STUBs, etc.
- **Critical Issues:** What needs to be fixed first
- **Implementation Roadmap:** Phases and priorities
- **Affected Files:** Which source files have gaps
- **GitHub Issues:** Links to related GitHub issues
- **Next Steps:** Action items for developers

---

## 📍 Location

This documentation is in the module directory for easy access:
```
src/server/MODULE_GAPS.md  ← You are here
```

Developers working on this module can reference this file directly.

---

## 🔄 How It's Updated

The documentation is automatically generated and updated by the gap audit pipeline:

```bash
# Full pipeline (scan + update headers + generate docs)
python tools/gap_audit_pipeline_v2.py

# Just generate module docs
python tools/module_doc_generator.py . ai_working ai_working/module_gaps
```

After each run, this file is updated with fresh analysis.

---

**Format:** THEMIS_MODULE_GAPS_v1  
**Generator:** ThemisDB Gap Audit Pipeline v2  
**Auto-Generated:** Yes
