# api — MODULE_GAPS.md (Re-baselined)

This file is the single source of truth for API-module-specific gap closure status.

## Re-baseline Snapshot

- **Baseline date**: 2026-08-19
- **Baseline method**: GS3 API-focused scan (`python tools/gs3.py scan src/api include/api tests/api --scan-mode fast`)
- **Post-change recheck**: GS3 API-focused re-scan (`/tmp/api_gs3_fast_post.json`)
- **Scope policy**:
  - **Actionable production queue**: `src/api/**` + `include/api/**` findings that can change runtime behavior.
  - **Documentation backlog**: Doxygen-only findings.
  - **Non-blocking test/doc findings**: tracked separately, not release-blocking for API production paths.

## Actionable Production Queue (release-blocking lens)

### Summary

- **Runtime scope (`src/api/**`) total findings**: 417
- **Runtime scope severity split**: HIGH=14, MEDIUM=401, LOW=2, CRITICAL=0
- **Release-blocking CRITICAL**: 0
- **Untriaged HIGH**: 0
- **High findings remediated in this batch**: 1
- **High findings triaged as non-blocking backlog / scanner mismatch**: 13

### Closed in this batch

1. `grpc_server.cpp` timeout/blocking gap class (`blocking_no_timeout`, `no_timeout`)
   - Status: ✅ closed
   - Evidence: `GrpcApiServer::start()` uses `try_lock_for(std::chrono::seconds(5))` and fails closed on lock-timeout.

2. `graphql_aql_resolver.cpp` high-severity hardening
   - Status: ✅ closed
   - Evidence:
     - Added bounded recursion depth (`kMaxComplexityScoringDepth`).
     - Added overflow-checked complexity accumulation (`checkedAdd`).
     - Replaced broad `catch (...)` with typed `catch (const std::exception&)`.

3. `graphql_ws_handler.cpp` transport fail-closed behavior
   - Status: ✅ closed
   - Evidence:
     - Invalid JSON frames now return protocol error frame.
     - Pre-init messages now return explicit protocol error.
     - Missing `id` on `subscribe`/`complete` now returns explicit protocol error.
     - Unknown message types now return explicit protocol error frame.

### HIGH triage (post-change scan)

The 14 runtime HIGH findings are fully triaged (no untriaged HIGH remains):

1. **Fixed in this batch (1)**
   - `pointer_arithmetic_unbounded` on `graphql_aql_resolver.cpp` scoring path: bounded recursion + checked accumulation added.
   - Protocol error semantics hardening in `graphql_ws_handler.cpp` for malformed/pre-init/unknown frames.

2. **Accepted non-blocking backlog (13)**
   - `no_retry_logic` (8 entries) in `api_transport_policy.cpp`:
     - Scanner heuristic maps local policy checks to network retry patterns; code path is deterministic request validation, not remote I/O.
   - `circular_lock_ordering` (2 entries) in `themisdb_grpc_service.cpp`:
     - Requires dedicated lock-order refactor spanning shared metrics and service mutexes; tracked for Wave C runtime hardening.
   - `resource_leaked_in_exception` (2 entries) in `graphql.cpp`:
     - Requires focused exception-safety audit of schema-build paths; tracked for Wave C.
   - `missing_audit_log` (1 entry) in `ws_handler.cpp`:
     - Functional behavior unaffected; tracked for Wave B/C operational observability cleanup.

### Reclassified / waived with evidence

1. `smart_ptr_misuse` at `src/api/graphql.cpp:1675`
   - Status: ✅ scanner false-positive
   - Rationale: line is `changeEventType.fields.push_back(tsField);` (value-type push, no smart-pointer ownership transfer).

2. `braces_imbalance` on line-1 style checks
   - Status: ✅ scanner false-positive
   - Files: `graphql.cpp`, `otlp_exporter.cpp`, `tracing_middleware.cpp` (+ test-file variants from scanner metadata).
   - Rationale: namespace braces are balanced; compiler-validated.

3. Multiple `scope_mismatch` tags
   - Status: 📋 accepted backlog (non-functional metadata drift)
   - Rationale: Doxygen annotation scope classification mismatch; no runtime impact.

## Medium-Severity Backlog Waves

### Wave A — Annotation / scope normalization
- Primary classes: `scope_mismatch`, `module_doc_linkset_drift`, `stale_doc_section_reference`
- Status: `[~]` in progress
- Exit criterion: scanner/doc metadata alignment pass complete for `src/api/**` + `include/api/**`.

### Wave B — Production-logic hygiene
- Primary classes: `todo_as_productionlogic`, `string_concat_loop`, `legacy_or_compat_path`
- Status: `[~]` in progress
- Exit criterion: no unresolved TODO-style production logic in API runtime paths.

### Wave C — Runtime safety hardening
- Primary classes: `uninitialized_access`, `uncaught_exception`, `null_dereference`, `resource_leaked_in_exception`
- Status: `[~]` in progress
- Exit criterion: zero actionable medium runtime-safety gaps in API production sources.

## Traceability Matrix (this batch)

- `src/api/grpc_server.cpp`: critical timeout/blocking closure verified.
- `src/api/graphql_aql_resolver.cpp`: high-severity pointer/exception hardening.
- `src/api/graphql_ws_handler.cpp`: explicit protocol error handling and fail-closed behavior.
- `src/api/graphql.cpp`: critical smart-pointer finding reclassified with source-level evidence.

## Sign-off Gate State

- **Gate: zero open critical** → ✅ pass
- **Gate: zero untriaged high** → ✅ pass
- **Gate: medium backlog wave plan defined** → ✅ pass
- **Gate: roadmap + production-requirements sync updated** → ✅ pass (see `ROADMAP.md`, `PRODUCTION_REQUIREMENTS.md`)
