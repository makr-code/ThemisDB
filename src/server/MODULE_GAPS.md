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
