# API Module Contract

Datum: 2026-08-03  
**Status:** Active  
**Module:** api (HTTP/gRPC/GraphQL transport)  
**Primary:** include/api/api_transport_contracts.h, src/api/ROADMAP.md

## Public API Surface

| API | Namespace | Input Contract | Output Contract | Errors | Thread-Safety | Ownership/Lifetime | Notes |
|---|---|---|---|---|---|---|---|
| `ITransportContract::validate()` | `themis::api` | Request msg (non-null), Middleware state (initialized) | bool: true if passes all 5 policy checks | None (pre-validated) | ✅ Multi-reader | N/A (validation only) | P0; GATE-API-03 ≤1µs |
| `TransportPolicyMiddleware::handle()` | `themis::api` | HttpRequest*, middleware config | HttpResponse (400/403/500 on policy violation, else passes through) | Never throws (fail-closed design) | ✅ Thread-safe via internal mutex | Request pointer borrowed; Response created fresh | P0; GATE-API-01 ≤5µs (GET) |
| `ApiErrorTaxonomy::toHttpStatus()` | `themis::api` | TransportFailureClass enum (9 values) | HTTP status code (400/403/500/503) | None | ✅ Lock-free (enum mapping) | N/A (pure function) | P0; GATE-API-05 ≤1µs |
| `ApiErrorTaxonomy::toMessage()` | `themis::api` | ErrorCode (12 auth + 3 transport types) | std::string (error message, safe for client) | None | ✅ Lock-free | String owned by return value | P0; GATE-API-06 ≤2µs |
| `IAPIVersionRouter::route()` | `themis::api` | Method (GET/POST/..), Version (1/2/3), Url path | RouteEntry (handler, version descriptor, capability flags) | NotFoundError if route undefined | ✅ Multi-reader (route table locked) | RouteEntry borrowed; valid until next route() call | P0; GATE-API-02 ≤3µs |
| `registerVersion()` | `themis::api` | VersionDescriptor, handler impl | void | ConflictError if version exists | 🔒 Single-threaded during init (must call before serving) | Version stored in global registry | Internal API (fluid) |
| `lockableHttpHandler()` | `themis::api` | IHttpHandler*, lock strategy (ReadLock/WriteLock) | ScopedLock wrapper | None | ✅ Thread-safe (lock guard) | Returns guard; owner is caller | Utility for middleware chains |
| `parseContentType()` | `themis::api` | Header string (e.g., "application/json; charset=utf-8") | ContentType enum + charset | ValueError if unparseable | ✅ Lock-free (stateless) | N/A (returns enum) | Helper; P1 |
| `degradedModeHandler()` | `themis::api` | Query (cached/synthetic), options | Partial result (reduced fidelity, still consistent) | TimeoutError if server overloaded; GracefulDegradation status | ✅ Multi-reader (readonly queries) | Result borrowed from cache; immutable | P1 (fallback path) |

## Preconditions & Postconditions

| API | Preconditions | Postconditions |
|---|---|---|
| `handle()` | Middleware initialized; HttpRequest complete (all headers/body present) | Either error response sent OR forwarded to inner handler; never both |
| `route()` | Version registered via `registerVersion()` before first call | Route entry guaranteed valid for lifetime of application |
| `toHttpStatus()` | Enum value in [0, 8] (9 transport failure classes) | Returned status in {400, 403, 500, 503}; deterministic |
| `toMessage()` | Error code in registered range | Message safe for external clients (no internal stack traces) |

## Invariants & State

| Invariant | Enforcement |
|---|---|
| Policy middleware never passes invalid requests | Checked before any inner handler invocation |
| Version table locked after init | Verified in integration tests (test_api_phase4_concurrency.cpp) |
| Error taxonomy is bijective | Unit test: test_api_error_taxonomy_completeness.cpp |
| Degraded mode respects consistency | Verified in wave5+ resilience tests |

## Security & Validation

| Aspect | Rule |
|---|---|
| Malformed payload | Return 400 immediately (fail-closed) |
| Path length | Max 2048 chars; return 414 if exceeded |
| Header size | Max 32 KB; return 431 if exceeded |
| Content-Type validation | Enforced for PUT/POST; return 415 if missing/invalid |
| Rate limiting | Delegated to middleware; 503 on quota exceeded |

## Concurrency & Locking

| Scenario | Behavior | Test |
|---|---|---|
| 32 concurrent GET requests | All served in parallel; each ≤5µs | test_api_phase4_concurrency.cpp (32×50 matrix) |
| Mixed GET/POST under load | POST serialized per policy; GETs remain concurrent | test_api_mixed_load.cpp |
| Version table update during serving | Blocked until all in-flight requests complete | N/A (version table set once at startup) |

## Performance Commitments (Release Gates)

| Gate | Latency | Test |
|---|---|---|
| GATE-API-01 | GET parse & route ≤5 µs | benchmarks/bench_api_release_gates.cpp |
| GATE-API-02 | Route lookup ≤3 µs | (subset of GATE-API-01) |
| GATE-API-03 | Policy validation ≤1 µs | bench_api_release_gates.cpp |
| GATE-API-04 | Error serialization ≤2 µs | (taxonomy mapping) |
| GATE-API-05 | HTTP status mapping ≤1 µs | (deterministic enum) |
| GATE-API-06 | Error message format ≤2 µs | (string template fill) |

## Evolution & Deprecation

| Item | Status | Notes |
|---|---|---|
| HTTP 2.0 support | Planned Q1 2027 | Currently HTTP/1.1 only |
| WebSocket transport | Beta (optional) | Flag: `TRANSPORT_WEBSOCKET_ENABLED` |
| GraphQL federation | Q2 2027 roadmap | Requires schema stitching |

---

**Zuletzt geprueft (API contracts):** 2026-08-03
