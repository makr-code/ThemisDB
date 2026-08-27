# Server Module - Verified Gap Analysis (L0.5)

> Last Updated: 2026-06-25T14:00:24Z
> Source: gap_scan_results_verified_L0.5_full.json
> Verification Method: Semantic code pattern analysis with false-positive elimination
> Verification Status: L0.5 Verified (22,160 total verified gaps across all modules, 6.8% false-positive removal applied)
> Wave 1 CRITICAL remediation applied: 2026-08-25 (see section below)

## Wave 1 CRITICAL Batch Fixed

> **Date:** 2026-08-25
> **Branch:** copilot/select-important-core-modules
> **Engineer:** Copilot Wave-1 CRITICAL Gap Remediation Agent

### Summary of Fixes Applied

| Category | Count Fixed | Files Touched | Fix Pattern |
|---|---:|---|---|
| `thread_join_no_timeout` / `blocking_no_timeout` | 8 | `http_server.cpp` | `stop()` and `wait()` converted to `std::async`-based timed join with 10 s deadline; stragglers reported and abandoned |
| `thread_join_no_timeout` | 1 | `mqtt_client_service.cpp` | `io_thread_.join()` wrapped in timed `std::async` join |
| `data_race` (monitoring handler pointer) | 10 | `http_server.cpp` | `monitoring_api_` snapshotted under `api_handlers_mutex_` into `monitoring_api_snap`; all switch-case uses updated |
| `data_race` (static one-shot init) | 3 | `http_server.cpp` | Three `static llm::DocsAssistant` / `static bool initialized` patterns replaced with `std::call_once` + per-endpoint `once_flag` |
| `no_timeout` (synchronous socket write) | 4 | `mqtt_client_service.cpp` | `SO_SNDTIMEO` applied via `boost::asio::socket_base::send_timeout` before each synchronous `asio::write` call |
| `missing_audit_log` (authorize without log) | 2 | `shard_repair_api_handler.cpp`, `rope_api_handler.cpp` | `THEMIS_INFO/WARN("[AUDIT] …")` added immediately after `authorize()` covering ALLOW and DENY branches |
| **Total CRITICAL fixes** | **28** | 4 files | — |

### False Positives Confirmed (no code change required)

| Scanner Finding | Location | Reason |
|---|---|---|
| `smart_ptr_misuse` lines 2446, 2478 | `http_server.cpp` | Scanner matched JavaScript `new Date()` / `new Error()` inside C-string literals as raw `new T` |
| `new_without_raii` lines 158, 162 | `shard_repair_api_handler.cpp` | Same JS-string false positive (`new Error()`) |
| `new_without_raii` lines 293, 294 | `replication_topology_api_handler.cpp` | Same JS-string false positive |
| `missing_audit_log` lines 7084-10122 | `http_server.cpp` | Authorize calls are routed through `requireScope()` / `requireAccess()` which both have audit logging at lines 10073-10081 |
| `missing_audit_log` lines 154-313 | `session_api_handler.cpp` | `auditAuthorizationDecision()` is called immediately after every `authorize()` invocation |
| `data_race` (local lambdas) | `query_api_handler.cpp` | Scanner flagged `[&]` captures of stack-local variables as data races; these are function-local and never shared across threads |
| `data_race` on `registry_` | `import_api_handler.cpp` | `ImportJobRegistry` has its own internal `std::mutex` guarding all public methods (confirmed in `importer_interface.h`) |
| `array_bounds` line 487 | `mqtt_session.cpp` | `buffer_` is `std::array<char,8192>`; access at `[0]` is guarded by `bytes_transferred < 2` check immediately above |
| `blocking_no_timeout` lines 787, 801, 821 | `mqtt_session.cpp` | `weak_ptr::lock()` is non-blocking; scanner misclassified it |

### Remaining CRITICAL Gaps (post-Wave 1)

| Severity | Count (pre-Wave 1) | Count (post-Wave 1) |
|---|---:|---:|
| Critical | 186 | ~158 |

Approximate breakdown of remaining ~158:
- `missing_audit_log` (other files not in this wave): ~12
- `data_race` (other modules): ~53 (model_integrity_gap, llm_api_handler, etc.)
- `model_integrity_gap` (LLM handler): ~10 — requires HMAC/SHA-256 model verification gate
- `iterator_invalidation` in query_api_handler: ~3
- Remaining `no_timeout` in other handlers: ~6

These are planned for Wave 2 / Wave B remediation.

### Regression Tests Added

- `tests/server/test_wave1_critical_gaps.cpp` — 7 focused unit tests covering:
  - Timed-join: quick-exit threads join cleanly; stuck threads counted as stragglers
  - `std::call_once`: invoked exactly once under concurrency; result consistent
  - Handler-snapshot pattern: concurrent readers see consistent pointer
  - Audit-log pattern: ALLOW and DENY branches both produce records
  - Send-timeout constants are positive

---

## Executive Summary

- **Total Verified Gaps**: 2,172
- **Actionable Gaps (Critical + High)**: 654 (30.1%)
- **Affected Source Files**: 111
- **Previous Scan (2026-06-04)**: 1,793 findings → **Current Verified**: 2,172 findings (+21% additional gaps identified)
- **Wave 1 CRITICAL Fixed (2026-08-25)**: 28 gaps across 4 source files

**Batch 3 Wave Correlation (2026-08-14):**
- **Wave A Gaps** (~400 IMPL gaps): HTTP timeout enforcement, graceful shutdown drain, rate-limit fail-closed, protocol retry semantics
- **Wave B Gaps** (~300 IMPL gaps): Distributed rate-limit state, GraphQL federation, adaptive backpressure
- **DOC Gaps** (~1,500 gaps): Thread-safety model documentation, fail-closed behavior documentation, auth flow documentation

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | ~158 (was 186; 28 fixed in Wave 1 — see section above) |
| High | 468 |
| Medium | 1013 |
| Low | 8 |

## Category Summary

| Category | Count |
|---|---:|
| hardcoded_path | 258 |
| copy_overhead | 139 |
| uncaught_exception | 131 |
| generic_catch | 119 |
| null_dereference | 118 |
| unnecessary_copy | 108 |
| string_concat_loop | 94 |
| data_race | 76 |
| resource_leaked_in_exception | 64 |
| db_connection_leak | 53 |
| manual_cleanup | 33 |
| missing_audit_log | 32 |
| no_retry_logic | 32 |
| missing_correlation_id | 30 |
| explicit_delete | 28 |
| legacy_or_compat_path | 27 |
| missing_latency_metric | 27 |
| delete_without_nullptr | 25 |
| delete_no_nullptr | 22 |
| uninitialized_access | 21 |
| unordered_container_iter | 21 |
| no_timeout | 19 |
| range_temporary | 19 |
| pointer_arithmetic_unbounded | 17 |
| catch_all_swallow | 16 |
| insecure_model_url | 14 |
| o_n_squared | 13 |
| smart_ptr_misuse | 13 |
| stale_doc_section_reference | 13 |
| unspecified_consistency | 13 |
| missing_health_check | 12 |
| blocking_no_timeout | 11 |
| model_integrity_gap | 11 |
| iterator_invalidation | 10 |
| array_bounds_violation | 8 |
| unvalidated_llm_output | 8 |
| arithmetic_overflow | 7 |
| map_vs_unordered_map | 7 |
| missing_vector_reserve | 7 |
| missing_resource_limits | 6 |
| new_without_delete | 6 |
| new_without_raii | 6 |
| primitive_no_volatile | 6 |
| repeated_search | 6 |
| deadlock_risk | 5 |
| duplicate_qualified_signature | 5 |
| fp_exact_comparison | 5 |
| memory_order | 5 |
| unstructured_log | 5 |
| allocation_loop | 4 |
| array_bounds | 4 |
| lock_contention | 4 |
| nested_loop_find | 4 |
| unchecked_array_index | 4 |
| hardcoded_output | 3 |
| lock_in_loop | 3 |
| manual_cleanup_in_destructor | 3 |
| thread_join_no_timeout | 3 |
| timestamp_sorting_unstable | 3 |
| unsanitized_llm_input | 3 |
| conditional_initialization_use | 2 |
| exception_in_destructor | 2 |
| expensive_copy | 2 |
| missing_dtor | 2 |
| module_doc_linkset_drift | 2 |
| shift_overflow | 2 |
| command_injection | 1 |
| expensive_inner_op | 1 |
| missing_adr_reference | 1 |
| missing_consensus | 1 |
| missing_trace_point | 1 |
| missing_version_tracking | 1 |
| posix_only_api | 1 |
| prompt_injection | 1 |
| size_assumption | 1 |
| stale_read_undocumented | 1 |
| uninitialized_array | 1 |
| unsafe_singleton | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| server/http_server.cpp | 231 | 28 | 109 | 94 | 0 |
| server/query_api_handler.cpp | 149 | 34 | 40 | 75 | 0 |
| server/postgres_session.cpp | 131 | 7 | 18 | 106 | 0 |
| server/import_wizard_builder.cpp | 118 | 0 | 2 | 116 | 0 |
| server/task_scheduler_api_handler.cpp | 113 | 6 | 4 | 98 | 5 |
| server/monitoring_api_handler.cpp | 69 | 6 | 4 | 59 | 0 |
| server/llm_api_handler.cpp | 67 | 15 | 39 | 13 | 0 |
| server/mcp_server.cpp | 63 | 3 | 34 | 26 | 0 |
| server/http3_session.cpp | 61 | 7 | 20 | 34 | 0 |
| server/rpc/rpc_service_impl.cpp | 50 | 1 | 35 | 14 | 0 |
| server/lora_api_handler.cpp | 39 | 4 | 21 | 14 | 0 |
| server/mqtt_client_service.cpp | 39 | 5 | 6 | 28 | 0 |
| server/entity_api_handler.cpp | 33 | 4 | 12 | 17 | 0 |
| server/changefeed_api_handler.cpp | 27 | 2 | 5 | 20 | 0 |
| server/mqtt_session.cpp | 27 | 8 | 1 | 18 | 0 |
| server/rate_limiter_v2.cpp | 24 | 4 | 20 | 0 | 0 |
| server/shard_repair_api_handler.cpp | 24 | 7 | 3 | 14 | 0 |
| server/async_job_api_handler.cpp | 21 | 2 | 17 | 2 | 0 |
| server/replication_topology_api_handler.cpp | 21 | 6 | 7 | 8 | 0 |
| server/http2_session.cpp | 20 | 0 | 5 | 15 | 0 |
| server/rope_api_handler.cpp | 20 | 4 | 11 | 5 | 0 |
| server/vector_api_handler.cpp | 18 | 2 | 1 | 15 | 0 |
| server/voice_api_handler.cpp | 18 | 2 | 5 | 11 | 0 |
| server/api_gateway.cpp | 16 | 1 | 9 | 6 | 0 |
| server/auth_middleware.cpp | 16 | 2 | 5 | 9 | 0 |
| server/distributed_gateway.cpp | 15 | 1 | 6 | 8 | 0 |
| server/websocket_session.cpp | 15 | 0 | 7 | 8 | 0 |
| server/graph_api_handler.cpp | 14 | 1 | 8 | 5 | 0 |
| server/rpc/snapshot_transfer_handler.cpp | 14 | 1 | 11 | 2 | 0 |
| server/spatial_api_handler.cpp | 14 | 0 | 2 | 12 | 0 |
| server/health_error_service.cpp | 13 | 4 | 6 | 3 | 0 |
| server/tenant_manager.cpp | 13 | 1 | 9 | 3 | 0 |
| server/export_api_handler.cpp | 12 | 3 | 1 | 8 | 0 |
| server/policy_engine.cpp | 12 | 2 | 2 | 8 | 0 |
| server/feedback_api_handler.cpp | 11 | 0 | 2 | 9 | 0 |
| server/audit_api_handler.cpp | 9 | 0 | 2 | 7 | 0 |
| server/bpmn_api_handler.cpp | 9 | 2 | 5 | 2 | 0 |
| server/ethics_api_handler.cpp | 9 | 0 | 3 | 6 | 0 |
| server/import_api_handler.cpp | 9 | 6 | 3 | 0 | 0 |
| server/profiling_api_handler.cpp | 9 | 0 | 0 | 9 | 0 |
| server/llm_grpc_service.cpp | 8 | 2 | 0 | 6 | 0 |
| server/pki_api_handler.cpp | 7 | 0 | 0 | 7 | 0 |
| server/prompt_engineering_grpc_service.cpp | 7 | 0 | 0 | 7 | 0 |
| server/rpc/differential_update_engine.cpp | 7 | 0 | 0 | 7 | 0 |
| server/wal_grpc_service.cpp | 7 | 0 | 2 | 5 | 0 |
| server/wasm_handler_registry.cpp | 7 | 4 | 3 | 0 | 0 |
| server/chunked_response_writer.cpp | 6 | 0 | 0 | 6 | 0 |
| server/diff_api_handler.cpp | 6 | 0 | 0 | 6 | 0 |
| server/grpc_web_proxy_handler.cpp | 6 | 0 | 3 | 3 | 0 |
| server/oauth2_provider.cpp | 6 | 0 | 6 | 0 | 0 |
| server/rpc/blob_transfer_handler.cpp | 6 | 0 | 2 | 4 | 0 |
| server/schema_api_handler.cpp | 6 | 0 | 1 | 5 | 0 |
| server/session_api_handler.cpp | 6 | 5 | 1 | 0 | 0 |
| server/themis_core_grpc_service.cpp | 6 | 0 | 0 | 6 | 0 |
| server/compliance_reporting_api_handler.cpp | 5 | 1 | 2 | 2 | 0 |
| server/opa_adapter.cpp | 5 | 1 | 1 | 3 | 0 |
| server/saml_auth_provider.cpp | 5 | 0 | 0 | 5 | 0 |
| server/sse_connection_manager.cpp | 5 | 1 | 4 | 0 | 0 |
| server/content_api_handler.cpp | 4 | 0 | 0 | 4 | 0 |
| server/geo_topology_api_handler.cpp | 4 | 0 | 4 | 0 | 0 |
| server/policy_manager_api_handler.cpp | 4 | 1 | 2 | 1 | 0 |
| server/ranger_adapter.cpp | 4 | 0 | 1 | 3 | 0 |
| server/rate_limiter.cpp | 4 | 1 | 1 | 2 | 0 |
| server/branch_api_handler.cpp | 3 | 0 | 3 | 0 | 0 |
| server/cache_admin_api_handler.cpp | 3 | 1 | 1 | 1 | 0 |
| server/cdn_cache_middleware.cpp | 3 | 2 | 0 | 1 | 0 |
| server/pii_api_handler.cpp | 3 | 0 | 0 | 3 | 0 |
| server/policy_template_api_handler.cpp | 3 | 1 | 1 | 1 | 0 |
| server/policy_validation_api_handler.cpp | 3 | 1 | 1 | 1 | 0 |
| server/policy_versioning_api_handler.cpp | 3 | 1 | 1 | 1 | 0 |
| server/reports_api_handler.cpp | 3 | 0 | 0 | 3 | 0 |
| server/review_scheduling_api_handler.cpp | 3 | 1 | 1 | 1 | 0 |
| server/update_api_handler.cpp | 3 | 1 | 2 | 0 | 0 |
| server/api_key_mgmt_handler.cpp | 2 | 0 | 1 | 1 | 0 |
| server/distributed_txn_api_handler.cpp | 2 | 0 | 1 | 1 | 0 |
| server/http3_datagram.cpp | 2 | 0 | 2 | 0 | 0 |
| server/http_type_adapter.cpp | 2 | 0 | 0 | 2 | 0 |
| server/index_api_handler.cpp | 2 | 0 | 0 | 2 | 0 |
| server/pitr_grpc_service.cpp | 2 | 0 | 0 | 2 | 0 |
| server/response_transformer.cpp | 2 | 0 | 2 | 0 | 0 |
| server/saga_api_handler.cpp | 2 | 0 | 0 | 2 | 0 |
| server/sharding_metrics_handler.cpp | 2 | 0 | 2 | 0 | 0 |
| server/snapshot_api_handler.cpp | 2 | 0 | 1 | 1 | 0 |
| server/transaction_api_handler.cpp | 2 | 0 | 2 | 0 | 0 |
| server/workload_fingerprint_engine.cpp | 2 | 0 | 2 | 0 | 0 |
| server/ARCHITECTURE.md | 1 | 0 | 0 | 0 | 1 |
| server/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| server/ROADMAP.md | 1 | 0 | 0 | 0 | 1 |
| server/api_auth_config.cpp | 1 | 0 | 1 | 0 | 0 |
| server/api_version.cpp | 1 | 0 | 1 | 0 | 0 |
| server/cache_api_handler.cpp | 1 | 0 | 1 | 0 | 0 |
| server/continuous_query_api_handler.cpp | 1 | 0 | 0 | 1 | 0 |
| server/cost_based_rate_limiter.cpp | 1 | 0 | 1 | 0 | 0 |
| server/openapi_route_registry.cpp | 1 | 1 | 0 | 0 | 0 |
| server/policy_api_handler.cpp | 1 | 0 | 1 | 0 | 0 |
| server/prompt_engineering_api_handler.cpp | 1 | 0 | 1 | 0 | 0 |
| server/retention_api_handler.cpp | 1 | 0 | 1 | 0 | 0 |
| server/smart_routing.cpp | 1 | 1 | 0 | 0 | 0 |
| server/timeseries_api_handler.cpp | 1 | 0 | 1 | 0 | 0 |
| server/wal_api_handler.cpp | 1 | 1 | 0 | 0 | 0 |

## Full Scanner Findings

### server/http_server.cpp
Total findings: 231

- Line 1600: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: : "ADAPTIVE_THROTTLE_TRIGGERED";

                THEMIS_WARN("[RateLimiter] anomaly type={} ip={} detail={}",

                    type_str, ev.ip, ev.detail);

                if (auto audit = weak_audit.lock()) {

                    nlohmann::json entry;

                    entry["event"]  = "rate_limiter_anomaly";

                    entry["type"]   = type_str;
- Line 1600: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: if (auto audit = weak_audit.lock()) {
- Line 2205: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: THEMIS_INFO("Waiting for worker threads to finish...");

    for (auto& thread : threads_) {

        if (thread.joinable()) {

            thread.join();

        }

    }

    threads_.clear();
- Line 2205: severity=CRITICAL; category=no_timeout
  Description: thread_join without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: thread.join();
- Line 2205: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: thread.join();
- Line 2221: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

}



void HttpServer::wait() {

    for (auto& thread : threads_) {

        if (thread.joinable()) {

            thread.join();
- Line 2221: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: void HttpServer::wait() {
- Line 2224: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: void HttpServer::wait() {

    for (auto& thread : threads_) {

        if (thread.joinable()) {

            thread.join();

        }

    }

}
- Line 2224: severity=CRITICAL; category=no_timeout
  Description: thread_join without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: thread.join();
- Line 2224: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: thread.join();
- Line 2446: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: THEMIS_WARN("Max connections ({}) reached - rejecting new connection",
- Line 2478: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: THEMIS_WARN("TLS enabled but SSL context unavailable; rejecting new connection");
- Line 4770: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: response = monitoring_api_->handleVersion(req);
- Line 4778: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: response = monitoring_api_->handleStats(req);
- Line 4786: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: response = monitoring_api_->handleCapabilities(req);
- Line 4817: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: response = monitoring_api_->handleMetrics(req);
- Line 4847: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: response = monitoring_api_->handleMetricsHtml(req);
- Line 4877: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: response = monitoring_api_->handlePluginMetrics(req);
- Line 4892: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: response = monitoring_api_->handleObservabilityAlerts(req);
- Line 4906: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: response = monitoring_api_->handleObservabilityAlertSilence(req);
- Line 4921: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: response = monitoring_api_->handleObservabilityHealth(req);
- Line 4936: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: response = monitoring_api_->handleLicenseStatus(req);
- Line 7084: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto authz = auth_->authorize(*token, "task:register");
- Line 7277: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto authz = auth_->authorize(*token, "task:execute");
- Line 9696: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto ar = auth_->authorize(*token, scope);
- Line 9821: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto decision = policy_engine_->authorize(user_id, std::string(action), resource, client_ip);
- Line 9985: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto decision = policy_engine_->authorize(user_id, "pii.write", path_only, client_ip);
- Line 10122: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto decision = policy_engine_->authorize(user_id, "pii.write", path_only, client_ip);
- Line 671: severity=HIGH; category=catch_all_swallow
  Description: catch(...) block swallows errors without rethrowing or explicit handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_design_error_rules
  Context: catch(...) { ... }
- Line 770: severity=HIGH; category=catch_all_swallow
  Description: catch(...) block swallows errors without rethrowing or explicit handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_design_error_rules
  Context: catch(...) { ... }
- Line 1165: severity=HIGH; category=catch_all_swallow
  Description: catch(...) block swallows errors without rethrowing or explicit handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_design_error_rules
  Context: catch(...) { ... }
- Line 1232: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: feedback_api_handler_->setLiveFeedbackCollector(live_feedback_collector_);
- Line 1233: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: feedback_api_handler_->setLearningOrchestrator(continuous_learning_orchestrator_);
- Line 1260: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: schema_api_handler_->setStatisticsCollector(stats_collector_.get());
- Line 1261: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: schema_api_handler_->setSchemaConstraints(schema_constraints_.get());
- Line 1262: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: schema_api_handler_->setSchemaVersionManager(schema_version_mgr_.get());
- Line 1263: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: schema_api_handler_->setIndexRecommender(index_recommender_.get());
- Line 1264: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: schema_api_handler_->setAuditLog(schema_audit_log_.get());
- Line 1268: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: schema_api_handler_->setColumnLineageTracker(column_lineage_tracker_.get());
- Line 1907: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ssl_ctx_->use_certificate_chain_file(config_.tls_cert_path);
- Line 1908: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ssl_ctx_->use_private_key_file(config_.tls_key_path, boost::asio::ssl::context::pem);
- Line 1914: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: SSL_CTX_set_cipher_list(ssl_ctx_->native_handle(), config_.tls_cipher_list.c_str());
- Line 1919: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: SSL_CTX_set_cipher_list(ssl_ctx_->native_handle(),
- Line 1925: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ssl_ctx_->set_options(
- Line 1940: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ssl_ctx_->load_verify_file(config_.tls_ca_cert_path);
- Line 1941: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ssl_ctx_->set_verify_mode(
- Line 1951: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ssl_ctx_->set_verify_mode(boost::asio::ssl::verify_none);
- Line 2063: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: http3_handler_->start();
- Line 2108: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 2116: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: http3_handler_->stop();
- Line 2130: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: while (active_requests_.load(std::memory_order_acquire) > 0
- Line 2132: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
- Line 2134: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto remaining = active_requests_.load(std::memory_order_acquire);
- Line 2251: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: new_ctx->use_certificate_chain_file(config_.tls_cert_path);
- Line 2251: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 2252: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: new_ctx->use_private_key_file(config_.tls_key_path, boost::asio::ssl::context::pem);
- Line 2255: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: SSL_CTX_set_cipher_list(new_ctx->native_handle(), config_.tls_cipher_list.c_str());
- Line 2257: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: SSL_CTX_set_cipher_list(new_ctx->native_handle(),
- Line 2257: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 2261: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: new_ctx->set_options(
- Line 2271: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: new_ctx->load_verify_file(config_.tls_ca_cert_path);
- Line 2272: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: new_ctx->set_verify_mode(
- Line 2278: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 2280: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: new_ctx->set_verify_mode(boost::asio::ssl::verify_none);
- Line 2446: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 3024: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Backward compatibility alias
- Line 3708: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

    auto span = Tracer::startSpanFromHeaders("http_request", req_headers);

    span.setAttribute("http.method", std::string(http::to_string(req.method())));

    span.setAttribute("http.target", std::string(req.target()));

    

    auto start = std::chrono::steady_clock::now();
- Line 3967: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!tenant_guard->acquireQuerySlot()) {
- Line 4427: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: themis::llm::InferenceRequest llm_request;

                        llm_request.prompt = query;

                        llm_request.model_id = payload.value("model", std::string{"default"});

                        llm_request.max_tokens = max_tokens;

                        llm_request.temperature = static_cast<float>(payload.value("temperature", 0.7));

                        if (!lora_id.empty()) {
- Line 4429: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: llm_request.prompt = query;

                        llm_request.model_id = payload.value("model", std::string{"default"});

                        llm_request.max_tokens = max_tokens;

                        llm_request.temperature = static_cast<float>(payload.value("temperature", 0.7));

                        if (!lora_id.empty()) {

                            llm_request.lora_adapter_id = lora_id;

                        }
- Line 4433: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (!lora_id.empty()) {

                            llm_request.lora_adapter_id = lora_id;

                        }

                        llm_request.metadata["rag_mode"] = rag_mode;

                        if (payload.contains("rag_tensor_slots")) {

                            llm_request.metadata["rag_tensor_slots"] = payload["rag_tensor_slots"];

                        }
- Line 4435: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

                        llm_request.metadata["rag_mode"] = rag_mode;

                        if (payload.contains("rag_tensor_slots")) {

                            llm_request.metadata["rag_tensor_slots"] = payload["rag_tensor_slots"];

                        }

                        if (payload.contains("rag_tensor_slot_chars")) {

                            llm_request.metadata["rag_tensor_slot_chars"] = payload["rag_tensor_slot_chars"];
- Line 4438: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: llm_request.metadata["rag_tensor_slots"] = payload["rag_tensor_slots"];

                        }

                        if (payload.contains("rag_tensor_slot_chars")) {

                            llm_request.metadata["rag_tensor_slot_chars"] = payload["rag_tensor_slot_chars"];

                        }



                        auto llm_response = plugin_mgr.generateRAG(rag_context, llm_request);
- Line 5785: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: snapshot_api_handler_->handleCreateTag(httplib_req, httplib_res);
- Line 5798: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: snapshot_api_handler_->handleListTags(httplib_req, httplib_res);
- Line 5811: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: snapshot_api_handler_->handleGetTag(httplib_req, httplib_res);
- Line 5824: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: snapshot_api_handler_->handleDeleteTag(httplib_req, httplib_res);
- Line 5837: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: snapshot_api_handler_->handleGetStats(httplib_req, httplib_res);
- Line 5852: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: diff_api_handler_->handleGetDiff(httplib_req, httplib_res);
- Line 5865: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: diff_api_handler_->handleGetCacheStats(httplib_req, httplib_res);
- Line 5878: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: diff_api_handler_->handleClearCache(httplib_req, httplib_res);
- Line 5893: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: pitr_api_handler_->handleRestore(httplib_req, httplib_res);
- Line 5906: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: pitr_api_handler_->handlePreview(httplib_req, httplib_res);
- Line 5919: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: pitr_api_handler_->handleGetProgress(httplib_req, httplib_res);
- Line 5932: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: branch_api_handler_->handleCreateBranch(httplib_req, httplib_res);
- Line 5943: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: branch_api_handler_->handleListBranches(httplib_req, httplib_res);
- Line 5954: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: branch_api_handler_->handleGetActiveBranch(httplib_req, httplib_res);
- Line 5965: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: branch_api_handler_->handleGetStats(httplib_req, httplib_res);
- Line 5976: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: branch_api_handler_->handleGetBranch(httplib_req, httplib_res);
- Line 5987: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: branch_api_handler_->handleSwitchBranch(httplib_req, httplib_res);
- Line 5998: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: branch_api_handler_->handleDeleteBranch(httplib_req, httplib_res);
- Line 6009: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: branch_api_handler_->handleMergeBranches(httplib_req, httplib_res);
- Line 6022: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: merge_api_handler_->handleMerge(httplib_req, httplib_res);
- Line 6033: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: merge_api_handler_->handleMergePreview(httplib_req, httplib_res);
- Line 6044: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: merge_api_handler_->handleMergeByTag(httplib_req, httplib_res);
- Line 6055: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: merge_api_handler_->handleCanFastForward(httplib_req, httplib_res);
- Line 6367: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: response = feedback_api_handler_->handleCreateFeedback(req);
- Line 6376: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: response = feedback_api_handler_->handleListFeedback(req);
- Line 6391: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: response = feedback_api_handler_->handleGetFeedback(req, id);
- Line 6407: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: response = feedback_api_handler_->handleUpdateFeedback(req, id);
- Line 6423: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: response = feedback_api_handler_->handleDeleteFeedback(req, id);
- Line 6439: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: response = feedback_api_handler_->handleGetAdapterFeedback(req, adapter_id);
- Line 6449: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: response = feedback_api_handler_->handleGetStatistics(req);
- Line 6784: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: mvcc_api_handler_->handleGetKey(httplib_req, httplib_res);
- Line 6786: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: mvcc_api_handler_->handlePutKey(httplib_req, httplib_res);
- Line 6808: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: mvcc_api_handler_->handleListVersions(httplib_req, httplib_res);
- Line 6810: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: mvcc_api_handler_->handleGcVersions(httplib_req, httplib_res);
- Line 6823: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: mvcc_api_handler_->handleGetClock(httplib_req, httplib_res);
- Line 6835: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: mvcc_api_handler_->handleGetStats(httplib_req, httplib_res);
- Line 6845: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: response = graphql_api_handler_->handlePost(req);
- Line 6850: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: response = graphql_api_handler_->handleSchemaGet(req);
- Line 6883: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: response = serverless_fn_handler_->handleRegister(req);
- Line 6887: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: response = serverless_fn_handler_->handleList(req);
- Line 6922: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: response = serverless_fn_handler_->handleInvoke(req, id);
- Line 6924: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: response = serverless_fn_handler_->handleVersions(req, id);
- Line 6926: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: response = serverless_fn_handler_->handleGet(req, id);
- Line 6928: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: response = serverless_fn_handler_->handleUpdate(req, id);
- Line 6930: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: response = serverless_fn_handler_->handleDelete(req, id);
- Line 7014: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: response = udf_api_handler_->handleRegister(req);
- Line 7017: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: response = udf_api_handler_->handleList(req);
- Line 7025: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: response = udf_api_handler_->handleGet(req, udf_name);
- Line 7034: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: response = udf_api_handler_->handleDelete(req, udf_name);
- Line 7691: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (auto& seg : {std::string("page"), std::string("page_size"),
- Line 9532: severity=HIGH; category=conditional_initialization_use
  Description: Use outside conditional block may use uninitialized value
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Variable initialized conditionally
- Line 9564: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: feature_semantic_cache_live_.store(enabled, std::memory_order_relaxed);
- Line 9760: severity=HIGH; category=catch_all_swallow
  Description: catch(...) block swallows errors without rethrowing or explicit handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_design_error_rules
  Context: catch(...) { ... }
- Line 10142: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Fallback to pseudonymizer erase/soft-delete
- Line 11685: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Modern browsers ignore X-XSS-Protection; set to 0 to avoid legacy behavior
- Line 11830: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const uint32_t timeout_ms = server_->request_timeout_ms_live_.load(std::memory_order_relaxed);
- Line 11927: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // legacy /v2/changes protocol only.  The new /v2/cdc/stream
- Line 12123: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const uint32_t timeout_ms = server_->request_timeout_ms_live_.load(std::memory_order_relaxed);
- Line 12256: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // legacy /v2/changes protocol only.  The new /v2/cdc/stream
- Line 12853: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: user_id = ctx->user_id;
- Line 12897: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Fallback to legacy rate limiter
- Line 13084: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const bool cap_semantic_cache = feature_semantic_cache_live_.load(std::memory_order_relaxed);
- Line 13085: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const bool cap_llm_store      = feature_llm_store_live_.load(std::memory_order_relaxed);
- Line 13860: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['                    try {', '                        uint64_t end_pos = std::stoull(rv.substr(dash + 1));', '                        length = (end_pos >= offset) ? (end_pos - offset + 1) : 0;', '                    } catch (...) {}', '                }']
- Line 671: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // GAP-011 fixed: log only token length, never prefix/suffix bytes.

            THEMIS_INFO("Auth check after addToken: validateToken(token_len={}) -> authorized={} user_id='{}' reason='{}'",

                       cfg.token.size(), v.authorized, v.user_id, v.reason);

        } catch (...) {}

    }

    // Read-only token

    if (auto t = themis_get_env("THEMIS_TOKEN_READONLY")) {
- Line 671: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 770: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

        // Optional: override audit rate limit via env var for tests

        if (const char* lim = std::getenv("THEMIS_AUDIT_RATE_LIMIT")) {

            try { audit_rate_limit_per_minute_ = static_cast<uint32_t>(std::stoul(lim)); } catch (...) {}

        } else {

            audit_rate_limit_per_minute_ = config_.audit_rate_limit_per_minute;

        }
- Line 770: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { audit_rate_limit_per_minute_ = static_cast<uint32_t>(std::stoul(lim)); } catch (...) {}
- Line 923: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: cache::CacheHitRateSloMonitor::Config slo_cfg;

            // Use sensible defaults; operators can override via environment variables.

            if (const char* warn_thr = std::getenv("THEMIS_CACHE_SLO_WARN")) {

                try { slo_cfg.warning_threshold = std::stod(warn_thr); } catch (...) {}

            }

            if (const char* crit_thr = std::getenv("THEMIS_CACHE_SLO_CRIT")) {

                try { slo_cfg.critical_threshold = std::stod(crit_thr); } catch (...) {}
- Line 923: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { slo_cfg.warning_threshold = std::stod(warn_thr); } catch (...) {}
- Line 926: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try { slo_cfg.warning_threshold = std::stod(warn_thr); } catch (...) {}

            }

            if (const char* crit_thr = std::getenv("THEMIS_CACHE_SLO_CRIT")) {

                try { slo_cfg.critical_threshold = std::stod(crit_thr); } catch (...) {}

            }

            auto cache_slo = std::make_shared<cache::CacheHitRateSloMonitor>(

                slo_cfg, alertmanager_);
- Line 926: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { slo_cfg.critical_threshold = std::stod(crit_thr); } catch (...) {}
- Line 965: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (auto cc = themis_get_env("THEMIS_RANGER_CLIENT_CERT")) rcfg.client_cert_path = *cc;

        if (auto ck = themis_get_env("THEMIS_RANGER_CLIENT_KEY")) rcfg.client_key_path = *ck;

        if (auto ct = themis_get_env("THEMIS_RANGER_CONNECT_TIMEOUT_MS")) {

            try { rcfg.connect_timeout_ms = std::stol(*ct); } catch (...) {}

        }

        if (auto rt = themis_get_env("THEMIS_RANGER_REQUEST_TIMEOUT_MS")) {

            try { rcfg.request_timeout_ms = std::stol(*rt); } catch (...) {}
- Line 965: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { rcfg.connect_timeout_ms = std::stol(*ct); } catch (...) {}
- Line 968: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try { rcfg.connect_timeout_ms = std::stol(*ct); } catch (...) {}

        }

        if (auto rt = themis_get_env("THEMIS_RANGER_REQUEST_TIMEOUT_MS")) {

            try { rcfg.request_timeout_ms = std::stol(*rt); } catch (...) {}

        }

        if (auto mr = themis_get_env("THEMIS_RANGER_MAX_RETRIES")) {

            try { rcfg.max_retries = std::stoi(*mr); } catch (...) {}
- Line 968: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { rcfg.request_timeout_ms = std::stol(*rt); } catch (...) {}
- Line 971: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try { rcfg.request_timeout_ms = std::stol(*rt); } catch (...) {}

        }

        if (auto mr = themis_get_env("THEMIS_RANGER_MAX_RETRIES")) {

            try { rcfg.max_retries = std::stoi(*mr); } catch (...) {}

        }

        if (auto rb = themis_get_env("THEMIS_RANGER_RETRY_BACKOFF_MS")) {

            try { rcfg.retry_backoff_ms = std::stol(*rb); } catch (...) {}
- Line 971: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { rcfg.max_retries = std::stoi(*mr); } catch (...) {}
- Line 974: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try { rcfg.max_retries = std::stoi(*mr); } catch (...) {}

        }

        if (auto rb = themis_get_env("THEMIS_RANGER_RETRY_BACKOFF_MS")) {

            try { rcfg.retry_backoff_ms = std::stol(*rb); } catch (...) {}

        }

        try {

            ranger_client_ = std::make_unique<themis::server::RangerClient>(std::move(rcfg));
- Line 974: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { rcfg.retry_backoff_ms = std::stol(*rb); } catch (...) {}
- Line 1165: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (auto interval = themis_get_env("THEMIS_UPDATE_CHECK_INTERVAL")) {

                try {

                    update_config.check_interval = std::chrono::seconds(std::stoul(*interval));

                } catch (...) {}

            }

            if (auto auto_update = themis_get_env("THEMIS_AUTO_UPDATE_ENABLED")) {

                update_config.auto_update_enabled = (*auto_update == "true" || *auto_update == "1");
- Line 1165: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 1527: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: opa_cfg.policy_path = *path;

        }

        if (auto tms = themis_get_env("THEMIS_OPA_TIMEOUT_MS")) {

            try { opa_cfg.timeout_ms = std::stol(*tms); } catch (...) {}

        }

        try {

            opa_adapter_ = std::make_unique<themis::OpaAdapter>(opa_cfg);
- Line 1527: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { opa_cfg.timeout_ms = std::stol(*tms); } catch (...) {}
- Line 1579: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: rate_config.bucket_capacity = limit;

            rate_config.refill_rate = static_cast<double>(limit) / 60.0;

            THEMIS_INFO("Rate limit set to {} req/min from environment", limit);

        } catch (...) {

            THEMIS_WARN("Invalid THEMIS_RATE_LIMIT_PER_MINUTE value, using default");

        }

    }
- Line 1579: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 1873: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::lock_guard<std::mutex> lock(max_body_bytes_mutex_);

        if (auto v = themis_get_env("THEMIS_MAX_BODY_BYTES")) {

            try { max_body_bytes_ = static_cast<size_t>(std::stoull(*v)); }

            catch (...) { THEMIS_WARN("Invalid THEMIS_MAX_BODY_BYTES value, using default 10MB"); }

        } else {

            // fall back to config max_request_size_mb if provided

            if (config_.max_request_size_mb > 0) {
- Line 1873: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: catch (...) { THEMIS_WARN("Invalid THEMIS_MAX_BODY_BYTES value, using default 10MB"); }
- Line 1919: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: SSL_CTX_set_cipher_list(ssl_ctx_->native_handle(),
- Line 1982: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (const char* env_port = std::getenv("THEMIS_HEALTH_PORT")) {

                try {

                    health_port = static_cast<uint16_t>(std::stoi(env_port));

                } catch (...) {

                    THEMIS_WARN("Invalid THEMIS_HEALTH_PORT value, using default {}", health_port);

                }

            }
- Line 1982: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 2184: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: storage_->close(); // This flushes and closes cleanly
- Line 2257: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: SSL_CTX_set_cipher_list(new_ctx->native_handle(),
- Line 2426: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool connection_slot_reserved = false;
- Line 2450: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: socket.close(close_ec);
- Line 2519: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: result += ' ';
- Line 3701: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::string> req_headers;
- Line 3777: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: header_bytes += field.name_string().size() + field.value().size() + 4; // 2 for ": " + 2 for "\r\n"
- Line 3778: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: header_bytes += field.name_string().size() + field.value().size() + 4; // 2 for ": " + 2 for "\r\n"
- Line 3912: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::string> headers;
- Line 4801: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];
- Line 7079: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto auth_header = req[http::field::authorization];
- Line 7272: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto auth_header = req[http::field::authorization];
- Line 7698: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (pos != std::string::npos) {

                        auto val = qs.substr(pos + key.size());

                        if (auto end = val.find('&'); end != std::string::npos) val = val.substr(0, end);

                        if (seg == "page") try { filter.page = std::stoi(val); } catch (...) {}

                        else if (seg == "page_size") try { filter.page_size = std::stoi(val); } catch (...) {}

                        else if (seg == "name") filter.name_filter = val;

                        else if (seg == "classification") filter.classification_filter = val;
- Line 7698: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: if (seg == "page") try { filter.page = std::stoi(val); } catch (...) {}
- Line 7699: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto val = qs.substr(pos + key.size());

                        if (auto end = val.find('&'); end != std::string::npos) val = val.substr(0, end);

                        if (seg == "page") try { filter.page = std::stoi(val); } catch (...) {}

                        else if (seg == "page_size") try { filter.page_size = std::stoi(val); } catch (...) {}

                        else if (seg == "name") filter.name_filter = val;

                        else if (seg == "classification") filter.classification_filter = val;

                    }
- Line 7699: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: else if (seg == "page_size") try { filter.page_size = std::stoi(val); } catch (...) {}
- Line 7778: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto& retention_api = *retention_api_;

            int limit = 100;

            if (auto qpos = std::string(req.target()).find("limit="); qpos != std::string::npos) {

                try { limit = std::stoi(std::string(req.target()).substr(qpos + 6)); } catch (...) {}

            }

            auto result = retention_api.getHistory(limit);

            response = makeResponse(http::status::ok, result.dump(), req);
- Line 7778: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { limit = std::stoi(std::string(req.target()).substr(qpos + 6)); } catch (...) {}
- Line 8617: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];
- Line 8663: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];
- Line 8695: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];
- Line 8727: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];
- Line 8997: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: out << "# HELP themis_content_blob_uncompressed_bytes_total Total uncompressed/original bytes observ
- Line 9001: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: out << "# HELP themis_content_blob_compression_ratio Histogram of compression ratios (original_size
- Line 9161: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: out << "themis_wal_apply_latency_seconds_sum " << (static_cast<double>(wal_latency_sum_us) / 1'000'0
- Line 9186: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: out << "themis_content_blob_compression_skipped_total{mime_prefix=\"image/\"} " << skipped_img << "\
- Line 9187: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: out << "themis_content_blob_compression_skipped_total{mime_prefix=\"video/\"} " << skipped_vid << "\
- Line 9188: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: out << "themis_content_blob_compression_skipped_total{mime_prefix=\"application/zip\"} " << skipped_
- Line 9217: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: out << "themis_content_blob_compression_skipped_total{mime_prefix=\"image/\"} 0\n";
- Line 9218: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: out << "themis_content_blob_compression_skipped_total{mime_prefix=\"video/\"} 0\n";
- Line 9219: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: out << "themis_content_blob_compression_skipped_total{mime_prefix=\"application/zip\"} 0\n";
- Line 9253: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: static std::unordered_map<std::string, std::string> parseQuery(const std::string& target) {
- Line 9273: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (s.empty()) return 0;

        bool numeric = std::all_of(s.begin(), s.end(), [](char c){ return c >= '0' && c <= '9'; });

        if (numeric) {

            try { return std::stoll(s); } catch (...) { return 0; }

        }

        // ISO8601 parsing

        // Expected: YYYY-MM-DDTHH:MM:SS[.fff][Z|±HH:MM]
- Line 9273: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { return std::stoll(s); } catch (...) { return 0; }
- Line 9297: severity=MEDIUM; category=expensive_copy
  Description: Unnecessary expensive copy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (zpos != std::string::npos) tzpos = zpos;
- Line 9322: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

                        tz_h = std::stoi(tzpart.substr(1,2));

                        tz_m = std::stoi(tzpart.substr(4,2));

                    } catch (...) { tz_h = tz_m = 0; tz_sign = 0; }

                }

            }

        }
- Line 9322: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) { tz_h = tz_m = 0; tz_sign = 0; }
- Line 9360: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: f.success_only = (v == "true" || v == "1" || v == "yes");

        }

        if (auto it = params.find("page"); it != params.end()) {

            try { f.page = std::max(1, std::stoi(it->second)); } catch (...) {}

        }

        if (auto it = params.find("page_size"); it != params.end()) {

            try {
- Line 9360: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { f.page = std::max(1, std::stoi(it->second)); } catch (...) {}
- Line 9400: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: f.success_only = (v == "true" || v == "1" || v == "yes");

        }

        if (auto it = params.find("page"); it != params.end()) {

            try { f.page = std::max(1, std::stoi(it->second)); } catch (...) {}

        }

        if (auto it = params.find("page_size"); it != params.end()) {

            try {
- Line 9400: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { f.page = std::max(1, std::stoi(it->second)); } catch (...) {}
- Line 9407: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: f.page_size = std::stoi(it->second);

                if (f.page_size < 1) f.page_size = 1;

                if (f.page_size > 10000) f.page_size = 10000; // allow larger for export

            } catch (...) {}

        }



        auto csv = audit_api.exportAuditLogsCsv(f);
- Line 9407: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 9437: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];
- Line 9523: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto lvl = lg["level"].get<std::string>();
- Line 9530: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto fmt = lg["format"].get<std::string>();
- Line 9546: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto timeout = body["request_timeout_ms"].get<uint32_t>();
- Line 9589: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto hours = body["cdc_retention_hours"].get<uint32_t>();
- Line 9674: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];
- Line 9733: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];
- Line 9760: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

                auto vres = auth_->validateToken(*token);

                THEMIS_INFO("requireAccess: validateToken -> authorized={} user_id='{}' reason='{}'", vres.authorized, vres.user_id, vres.reason);

            } catch (...) {}

            auto ar = auth_->authorize(*token, required_scope);

            if (audit_logger_) {

                audit_logger_->logSecurityEvent(
- Line 9760: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 9848: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];
- Line 9906: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];
- Line 10047: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];
- Line 10281: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

        if (!getParam("page").empty()) filter.page = std::stoi(getParam("page"));

        if (!getParam("page_size").empty()) filter.page_size = std::stoi(getParam("page_size"));

    } catch (...) {}



    std::string csv = pii_api.exportCsv(filter);

    http::response<http::string_body> res{http::status::ok, req.version()};
- Line 10281: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 10834: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, double> scores;
- Line 10873: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, double> scores;
- Line 11160: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto it = body["weights"].begin(); it != body["weights"].end(); ++it) {
- Line 11361: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto configObj = body["config"];
- Line 11948: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = request_[http::field::authorization];
- Line 12143: severity=MEDIUM; category=missing_health_check
  Description: Service initialization without nearby health/status handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void HttpServer::SslSession::start() {
- Line 12171: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: X509_free(cert);
- Line 12276: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = request_[http::field::authorization];
- Line 13856: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: rv = rv.substr(6);

            auto dash = rv.find('-');

            if (dash != std::string::npos) {

                try { offset = std::stoull(rv.substr(0, dash)); } catch (...) {}

                if (dash + 1 < rv.size()) {

                    try {

                        uint64_t end_pos = std::stoull(rv.substr(dash + 1));
- Line 13856: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { offset = std::stoull(rv.substr(0, dash)); } catch (...) {}

### server/query_api_handler.cpp
Total findings: 149

- Line 726: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::function<std::string(const std::shared_ptr<Expression>&, std::string&)> fieldFromFA = [&](const
- Line 763: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (!colL.empty() && rvL == var1 && bin->right->getType() == ASTNodeType::Literal) { auto lit = std:
- Line 764: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (!colL.empty() && rvL == var2 && bin->right->getType() == ASTNodeType::Literal) { auto lit = std:
- Line 765: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (bin->left->getType() == ASTNodeType::Literal) { std::string rv; std::string col = fieldFromFA(bi
- Line 806: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if ((*parse_result) && (*parse_result)->limit) { auto off = static_cast<size_t>(std::max<int64_t>(0,
- Line 831: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: resolveToLoopField = [&](const std::shared_ptr<Expression>& e)->std::optional<std::string> {
- Line 875: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto lit = std::static_pointer_cast<LiteralExpr>(be->right);
- Line 880: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto lit = std::static_pointer_cast<LiteralExpr>(be->left);
- Line 997: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: evalExprToLiteral = [&](std::shared_ptr<themis::query::Expression> expr, nlohmann::json& out)->bool
- Line 1132: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto parseSide = [&](std::shared_ptr<Expression> e, char& var, std::string& field) -> bool {
- Line 1143: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto parseSimpleFromExpr = [&](std::shared_ptr<Expression> expr, SimplePred& out)->bool {
- Line 1193: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto parseSide = [&](std::shared_ptr<Expression> e, char& var, std::string& field) -> bool {
- Line 1362: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator itp may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto itp = parent.find(cur);
- Line 1575: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::function<std::string(const std::shared_ptr<Expression>&, std::string&)> fieldFromFA = [&](const
- Line 1635: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto lit = std::static_pointer_cast<LiteralExpr>(bin->right);
- Line 1648: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto lit = std::static_pointer_cast<LiteralExpr>(bin->right);
- Line 1663: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto lit = std::static_pointer_cast<LiteralExpr>(bin->left);
- Line 1766: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto off = static_cast<size_t>(std::max<int64_t>(0, (*parse_result)->limit->offset));
- Line 1767: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto cnt = static_cast<size_t>(std::max<int64_t>(0, (*parse_result)->limit->count));
- Line 1959: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = parent.find(node);
- Line 2087: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = parent.find(cur);
- Line 2247: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto bin = std::static_pointer_cast<BinaryOpExpr>(filter->condition);
- Line 2252: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto fa = std::static_pointer_cast<FieldAccessExpr>(bin->left);
- Line 2253: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto lit = std::static_pointer_cast<LiteralExpr>(bin->right);
- Line 2354: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: evalExpr = [&](const std::shared_ptr<themis::query::Expression>& e) -> nlohmann::json {
- Line 2444: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: exprContainsFn = [&](const std::shared_ptr<themis::query::Expression>& expr, const std::string& name
- Line 2509: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: requested_count_for_cursor = static_cast<size_t>(std::max<int64_t>(1, (*parse_result)->limit->count)
- Line 2708: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto off = static_cast<size_t>(std::max<int64_t>(0, (*parse_result)->limit->offset));
- Line 2709: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto cnt = static_cast<size_t>(std::max<int64_t>(0, (*parse_result)->limit->count));
- Line 2741: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: cursor_meta["anchor_set"] = q.orderBy->cursor_pk.has_value();
- Line 2759: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto extractColumn = [&](const std::shared_ptr<themis::query::Expression>& expr)->std::string {
- Line 2893: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: extractColFromFA = [&](const std::shared_ptr<Expression>& expr, bool& rootedAtLoop)->std::optional<s
- Line 2906: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: containsFunction = [&](const std::shared_ptr<Expression>& expr, const std::string& name)->bool{
- Line 3224: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: requested_count = static_cast<size_t>(std::max<int64_t>(1, (*parse_result)->limit->count));
- Line 138: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto masking_policy = std::atomic_load_explicit(&masking_policy_, std::memory_order_acquire);
- Line 273: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto* stats_collector = stats_collector_.load(std::memory_order_acquire);
- Line 776: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto* stats_collector = stats_collector_.load(std::memory_order_acquire);
- Line 801: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: std::string retVar; if ((*parse_result)->return_node && (*parse_result)->return_node->expression) {
- Line 843: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = letMap.find(v->name);
- Line 915: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto* index_recommender = index_recommender_.load(std::memory_order_acquire);
- Line 978: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if ((*parse_result)->return_node && (*parse_result)->return_node->expression) {
- Line 980: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: auto* var = dynamic_cast<VariableExpr*>((*parse_result)->return_node->expression.get());
- Line 1063: severity=HIGH; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: char buf[32]; std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday); return std::string(buf);
- Line 1063: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: char buf[32]; std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", tm.tm_year+1900, tm.tm_mon+1, tm.tm_
- Line 1268: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: case SimplePred::Op::Eq:  return aval == lit;
- Line 1269: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: case SimplePred::Op::Neq: return aval != lit;
- Line 1279: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: case SimplePred::Op::Eq:  return av == lit;
- Line 1280: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: case SimplePred::Op::Neq: return av != lit;
- Line 1504: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (objVar->name == "v" || objVar->name == "e") return true;
- Line 1686: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto* stats_collector = stats_collector_.load(std::memory_order_acquire);
- Line 1731: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if ((*parse_result)->return_node && (*parse_result)->return_node->expression) {
- Line 1732: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (auto* v = dynamic_cast<VariableExpr*>((*parse_result)->return_node->expression.get())) {
- Line 2047: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (!blob && pk.find(':') == std::string::npos) {
- Line 2086: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = parent.find(cur);
- Line 2103: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (!blob && pk.find(':') == std::string::npos) {
- Line 2175: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto* stats_collector = stats_collector_.load(std::memory_order_acquire);
- Line 2214: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Provide "result" alias for compatibility with older clients/tests
- Line 2306: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto* stats_collector = stats_collector_.load(std::memory_order_acquire);
- Line 2379: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: for (const auto& [k, ce] : obj->fields) out[k] = evalExpr(ce);
- Line 2396: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: auto projected = evalExpr(jq.return_node->expression);
- Line 2571: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto* stats_collector = stats_collector_.load(std::memory_order_acquire);
- Line 2578: severity=HIGH; category=conditional_initialization_use
  Description: Use outside conditional block may use uninitialized value
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Variable initialized conditionally
- Line 2853: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto it = mp.find(a.var);
- Line 2942: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if ((*parse_result)->return_node && (*parse_result)->return_node->expression) {
- Line 2943: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: usesFulltextScore = containsFunction((*parse_result)->return_node->expression, "fulltext_score");
- Line 2956: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if ((*parse_result)->return_node && (*parse_result)->return_node->expression) {
- Line 2957: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: usesScoreFn = containsFunction((*parse_result)->return_node->expression, "bm25");
- Line 3184: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if ((*parse_result)->return_node && (*parse_result)->return_node->expression) {
- Line 3185: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (auto* v = dynamic_cast<VariableExpr*>((*parse_result)->return_node->expression.get())) {
- Line 3203: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if ((*parse_result)->return_node && (*parse_result)->return_node->expression) {
- Line 3204: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: auto out = evalExpr((*parse_result)->return_node->expression, e, env);
- Line 3264: severity=HIGH; category=catch_all_swallow
  Description: catch(...) block swallows errors without rethrowing or explicit handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_design_error_rules
  Context: catch(...) { ... }
- Line 3283: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Provide "result" alias for compatibility
- Line 3293: severity=HIGH; category=catch_all_swallow
  Description: catch(...) block swallows errors without rethrowing or explicit handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_design_error_rules
  Context: catch(...) { ... }
- Line 225: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: preds.push_back({p["column"].get<std::string>(), p["value"].get<std::string>()});
- Line 505: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::string schema_json(schema_bytes->begin(), schema_bytes->end());

                        schema = nlohmann::json::parse(schema_json);

                    }

                } catch (...) {}

                bool enabled = false;

                std::vector<std::string> fields;

                std::string context_type = "user";
- Line 505: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 510: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto coll = schema["collections"][table];
- Line 513: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (auto& f : coll["encryption"]["fields"]) if (f.is_string()) fields.push_back(f.get<std::string>(
- Line 536: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto enc_meta_str = obj[f + "_encrypted"].get<std::string>();
- Line 541: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto blob = themis::EncryptedBlob::fromJson(enc_meta);

                                std::vector<uint8_t> raw_key;

                                if (context_type == "group" && pki && obj.contains(f + "_group")) {

                                    std::string group_name; try { group_name = obj[f + "_group"].get<std::string>(); } catch (...) { group_name.clear(); }

                                    if (!group_name.empty()) {

                                        auto gdek = pki->getGroupDEK(group_name);

                                        std::vector<uint8_t> salt; std::string info = "field:" + f;
- Line 541: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: std::string group_name; try { group_name = obj[f + "_group"].get<std::string>(); } catch (...) { gro
- Line 566: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

                                            auto parsed = nlohmann::json::parse(plain_str);

                                            obj[f] = parsed;

                                        } catch (...) {

                                            obj[f] = plain_str;

                                        }

                                    } else {
- Line 566: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 729: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::vector<std::string> parts; parts.push_back(fa->field);
- Line 731: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: while (auto* fa2 = dynamic_cast<FieldAccessExpr*>(cur)) { parts.push_back(fa2->field); cur = fa2->ob
- Line 763: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (!colL.empty() && rvL == var1 && bin->right->getType() == ASTNodeType::Literal) { auto lit = std:
- Line 765: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (bin->left->getType() == ASTNodeType::Literal) { std::string rv; std::string col = fieldFromFA(bi
- Line 803: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (buildLeft) { for (const auto& e : rightVec) { auto k = getFieldStr(e, colRight); if (!k.has_value()) continue; auto range = hash.equal_range(*k); for (auto it = range.first; it != range.second; ++it) { const themis::BaseEntity& l = it->second; if (retVar == var1) out.push_back(l); else out.push_back(e); } } }
- Line 825: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::shared_ptr<themis::query::Expression>> letMap;
- Line 836: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: while (auto* fa2 = dynamic_cast<FieldAccessExpr*>(cur)) { parts.push_back(fa2->field); cur = fa2->object.get(); }
- Line 839: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::string col; for (auto it = parts.rbegin(); it != parts.rend(); ++it) { if (!col.empty()) col += "."; col += *it; }
- Line 876: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: eqPreds.push_back({*leftCol, litToString(lit->value)});
- Line 990: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: char var = '\0'; // 'v' or 'e'
- Line 1319: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

                    auto ent = themis::BaseEntity::deserialize(pk, *blob);

                    return ent.getFieldAsString(field);

                } catch (...) { return std::nullopt; }

            };

            auto getEFieldString = [&](const std::string& edgeId, const std::string& field)->std::optional<std::string>{

                if (field == "id") return edgeId;
- Line 1319: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) { return std::nullopt; }
- Line 1328: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

                    auto ent = themis::BaseEntity::deserialize(edgeId, *eblob);

                    return ent.getFieldAsString(field);

                } catch (...) { return std::nullopt; }

            };



            using namespace themis::query;
- Line 1328: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) { return std::nullopt; }
- Line 1364: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: pathEdges.push_back(itp->second.edgeId);
- Line 1365: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: pathNodes.push_back(itp->second.parent);
- Line 1580: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: parts.push_back(fa->field);
- Line 1583: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: parts.push_back(fa2->field);
- Line 1817: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto valOpt = ent.getFieldAsString(p.field);

                            if (!valOpt) return false;

                            if (!cmp(*valOpt, p.literal, p.op)) return false;

                        } catch (...) { return false; }

                    }

                }

                return true;
- Line 1817: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) { return false; }
- Line 1860: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (!valOpt) return false;

                        if (!cmp(*valOpt, p.literal, p.op)) return false;

                    }

                } catch (...) { return false; }

                return true;

            };
- Line 1860: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) { return false; }
- Line 1876: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto valOpt = ent.getFieldAsString(p.field);

                    if (!valOpt) return false;

                    return cmp(*valOpt, p.literal, p.op);

                } catch (...) { return false; }

            };



            // BFS mit Eltern-/Kanten-Tracking f�r e/p
- Line 1876: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) { return false; }
- Line 1961: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: resultEdgeIds.push_back(it->second.edgeId);
- Line 2055: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

                            auto entity = themis::BaseEntity::deserialize(pk, *blob);

                            res["entities"].push_back(entity.toJson());

                        } catch (...) {

                            res["entities"].push_back(nlohmann::json({{"_key", pk}}));

                        }

                    } else {
- Line 2055: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 2070: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

                            auto edgeEnt = themis::BaseEntity::deserialize(eid, *eblob);

                            res["entities"].push_back(edgeEnt.toJson());

                        } catch (...) {

                            res["entities"].push_back(nlohmann::json({{"_edge", eid}}));

                        }

                    } else {
- Line 2070: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 2089: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: edges.push_back(it->second.edgeId);
- Line 2111: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

                                auto ent = themis::BaseEntity::deserialize(pk, *blob);

                                jpath["vertices"].push_back(ent.toJson());

                            } catch (...) {

                                jpath["vertices"].push_back(nlohmann::json({{"_key", pk}}));

                            }

                        } else {
- Line 2111: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 2126: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

                                auto eent = themis::BaseEntity::deserialize(eid, *eblob);

                                jpath["edges"].push_back(eent.toJson());

                            } catch (...) {

                                jpath["edges"].push_back(nlohmann::json({{"_edge", eid}}));

                            }

                        } else {
- Line 2126: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 2203: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: themis::BaseEntity::Blob entity_blob(blob->begin(), blob->end());

                        auto entity = themis::BaseEntity::deserialize(key, entity_blob);

                        entities.push_back(nlohmann::json::parse(entity.toJson()));

                    } catch (...) {

                        // Skip malformed entities

                    }

                }
- Line 2203: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 2215: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: {"entities", applyMasking(entities, req)}

            };

            // Provide "result" alias for compatibility with older clients/tests

            try { response_body["result"] = response_body["entities"]; } catch (...) { /* ignore */ }

            

            if (explain) {

                response_body["query"] = aql_query;
- Line 2215: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { response_body["result"] = response_body["entities"]; } catch (...) { /* ignore */ }
- Line 2257: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: parts.push_back(fa->field);
- Line 2260: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: parts.push_back(fa2->field);
- Line 2268: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!col.empty()) col += ".";
- Line 2269: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!col.empty()) col += ".";
- Line 2352: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, nlohmann::json> letValues;
- Line 2559: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Ohne Sortwert kein sicherer Anker

                                        early_empty_due_to_cursor = true;

                                    }

                                } catch (...) {

                                    early_empty_due_to_cursor = true;

                                }

                            }
- Line 2559: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 2771: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!col.empty()) col += ".";
- Line 2772: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!col.empty()) col += ".";
- Line 2800: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::unordered_map<std::string, AggState>> acc;
- Line 2813: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (dv.has_value()) { out = *dv; return true; }

                auto sv = e.getFieldAsString(col);

                if (sv.has_value()) {

                    try { out = std::stod(*sv); return true; } catch (...) { /* ignore */ }

                }

                return false;

            };
- Line 2813: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { out = std::stod(*sv); return true; } catch (...) { /* ignore */ }
- Line 3101: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: else if (a.is_boolean()) out += (a.get<bool>()?"true":"false");
- Line 3196: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, nlohmann::json> env;
- Line 3284: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: {"entities", applyMasking(entities, req)}

            };

            // Provide "result" alias for compatibility

            try { response_body["result"] = response_body["entities"]; } catch (...) { /* ignore */ }

        }

        

        if (explain) {
- Line 3284: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { response_body["result"] = response_body["entities"]; } catch (...) { /* ignore */ }
- Line 3293: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (!plan_json.is_null()) {

                // Markiere, wenn LET-Filter vor der Übersetzung extrahiert wurden (MVP-Sonderpfad)

                if (letFilterHandled) {

                    try { plan_json["let_pre_extracted"] = true; } catch (...) { /* noop */ }

                }

                response_body["plan"] = plan_json;

            }
- Line 3293: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { plan_json["let_pre_extracted"] = true; } catch (...) { /* noop */ }
- Line 3488: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];
- Line 3524: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];
- Line 3581: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: decoded += ' ';
- Line 3582: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: decoded += ' ';
- Line 3606: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (n < lo) n = lo;

                if (n > hi) n = hi;

                return n;

            } catch (...) { return def; }

        };

        max_seconds  = extractInt("max_seconds",  30,    1,    60);

        heartbeat_ms = extractInt("heartbeat_ms", 15000, 100, 60000);
- Line 3606: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) { return def; }
- Line 3626: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_fwd = req[http::field::authorization];
- Line 3640: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: json result;

        try {

            result = json::parse(aql_resp.body());

        } catch (...) {

            result = json::object();

        }
- Line 3640: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### server/postgres_session.cpp
Total findings: 131

- Line 123: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto close_fn = [this]() {

        closeSocket();

    };

    if (auto self = weak_self.lock()) {

        asio::dispatch(socket_.get_executor(), [self, close_fn]() {

            close_fn();

        });
- Line 123: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: if (auto self = weak_self.lock()) {
- Line 276: severity=CRITICAL; category=db_connection_leak
  Description: Database connection from state never released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: TransactionState state = transactionState_.load(std::memory_order_acquire);
- Line 930: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator ltrim may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto ltrim = field.find_first_not_of(" \t");
- Line 931: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator rtrim may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto rtrim = field.find_last_not_of(" \t");
- Line 1567: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: message.insert(message.end(), payload.begin(), payload.end());



    auto weak_self = weak_from_this();

    if (auto self = weak_self.lock()) {

        asio::dispatch(socket_.get_executor(), [self, message = std::move(message)]() mutable {

            self->enqueueWrite(std::move(message));

        });
- Line 1567: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: if (auto self = weak_self.lock()) {
- Line 149: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (ec || stopped_.load(std::memory_order_acquire)) {
- Line 171: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (ec || stopped_.load(std::memory_order_acquire)) {
- Line 190: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: switch (transactionState_.load(std::memory_order_acquire)) {
- Line 301: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Handle schema queries (pg_catalog, information_schema) for BI tool compatibility
- Line 366: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 549: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: size_t dotPos = colName.find('.');
- Line 725: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: size_t dotPos = colName.find('.');
- Line 787: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: size_t dotPos = colName.find('.');
- Line 854: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!copyInProgress_.load(std::memory_order_acquire)) {
- Line 863: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 868: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(copyMutex_);
- Line 878: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!copyInProgress_.load(std::memory_order_acquire)) {
- Line 1290: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (ec || stopped_.load(std::memory_order_acquire)) {
- Line 1302: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (inStartup_.load(std::memory_order_acquire)) {
- Line 1521: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (ec || stopped_.load(std::memory_order_acquire)) {
- Line 1578: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (stopped_.load(std::memory_order_acquire)) {
- Line 1626: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Standard PostgreSQL types required for BI tool compatibility
- Line 2167: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: size_t eqPos = assignment.find('=');
- Line 32: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: result += "''";  // PostgreSQL escapes single quotes by doubling
- Line 33: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: result += "''";  // PostgreSQL escapes single quotes by doubling
- Line 35: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: result += "\\\\";  // Escape backslashes
- Line 35: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: result += "\\\\";  // Escape backslashes
- Line 86: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: throw;

        } catch (const std::exception& e) {

            std::cerr << "[PostgresSession] " << context << ": " << e.what() << "\n";

        } catch (...) {

            std::cerr << "[PostgresSession] " << context << ": unknown exception\n";

        }

    }
- Line 86: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 142: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: socket_.close(ec);
- Line 155: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

            sendErrorResponse("ERROR", "57014", "Connection timed out while waiting for client message");

            stop();

        } catch (...) {

            logCurrentException("Read-timeout handler error");

            stop();

        }
- Line 155: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 177: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

            sendErrorResponse("ERROR", "57014", "Connection timed out while sending response");

            stop();

        } catch (...) {

            logCurrentException("Write-timeout handler error");

            stop();

        }
- Line 177: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 202: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<std::string, std::string>& params) {
- Line 258: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void PostgresSession::handleQuery(const std::string& query) {
- Line 455: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void PostgresSession::handleExecute(const std::string& portal, int32_t maxRows) {
- Line 547: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: fields.push_back({"?column?", 0, 0, 25, -1, -1, 0});
- Line 554: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: fields.push_back({colName, 0, 0, 25, -1, -1, 0});
- Line 600: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: row_vals.push_back(
- Line 604: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: row_vals.push_back("");
- Line 722: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: fields.push_back({"?column?", 0, 0, 25, -1, -1, 0}); // text type
- Line 730: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: fields.push_back({colName, 0, 0, 25, -1, -1, 0}); // text type
- Line 735: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: fields.push_back({"?column?", 0, 0, 25, -1, -1, 0});
- Line 785: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: fields.push_back({"?column?", 0, 0, 25, -1, -1, 0});
- Line 792: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: fields.push_back({colName, 0, 0, 25, -1, -1, 0});
- Line 796: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: fields.push_back({"?column?", 0, 0, 25, -1, -1, 0});
- Line 911: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: field += '"';
- Line 912: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: field += '"';
- Line 1032: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void PostgresSession::sendReadyForQuery(char transactionStatus) {
- Line 1051: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back((field.tableOid >> 24) & 0xFF);
- Line 1052: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back((field.tableOid >> 16) & 0xFF);
- Line 1053: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back((field.tableOid >> 8) & 0xFF);
- Line 1054: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back(field.tableOid & 0xFF);
- Line 1057: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back((field.columnAttrNumber >> 8) & 0xFF);
- Line 1058: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back(field.columnAttrNumber & 0xFF);
- Line 1061: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back((field.dataTypeOid >> 24) & 0xFF);
- Line 1062: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back((field.dataTypeOid >> 16) & 0xFF);
- Line 1063: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back((field.dataTypeOid >> 8) & 0xFF);
- Line 1064: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back(field.dataTypeOid & 0xFF);
- Line 1067: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back((field.dataTypeSize >> 8) & 0xFF);
- Line 1068: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back(field.dataTypeSize & 0xFF);
- Line 1071: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back((field.typeModifier >> 24) & 0xFF);
- Line 1072: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back((field.typeModifier >> 16) & 0xFF);
- Line 1073: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back((field.typeModifier >> 8) & 0xFF);
- Line 1074: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back(field.typeModifier & 0xFF);
- Line 1077: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back((field.formatCode >> 8) & 0xFF);
- Line 1078: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back(field.formatCode & 0xFF);
- Line 1089: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back((colCount >> 8) & 0xFF);
- Line 1090: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back(colCount & 0xFF);
- Line 1095: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back((len >> 24) & 0xFF);
- Line 1096: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back((len >> 16) & 0xFF);
- Line 1097: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back((len >> 8) & 0xFF);
- Line 1098: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back(len & 0xFF);
- Line 1114: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back((colCount >> 8) & 0xFF);
- Line 1115: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back(colCount & 0xFF);
- Line 1120: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back((len >> 24) & 0xFF);
- Line 1121: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back((len >> 16) & 0xFF);
- Line 1122: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back((len >> 8) & 0xFF);
- Line 1123: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back(len & 0xFF);
- Line 1163: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back((typeOid >> 24) & 0xFF);
- Line 1164: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back((typeOid >> 16) & 0xFF);
- Line 1165: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back((typeOid >> 8) & 0xFF);
- Line 1166: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back(typeOid & 0xFF);
- Line 1192: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back((numColumns >> 8) & 0xFF);
- Line 1193: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back(numColumns & 0xFF);
- Line 1197: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back((format >> 8) & 0xFF);
- Line 1198: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back(format & 0xFF);
- Line 1214: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back((numColumns >> 8) & 0xFF);
- Line 1215: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back(numColumns & 0xFF);
- Line 1218: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back((format >> 8) & 0xFF);
- Line 1219: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back(format & 0xFF);
- Line 1235: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back((numColumns >> 8) & 0xFF);
- Line 1236: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back(numColumns & 0xFF);
- Line 1239: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back((format >> 8) & 0xFF);
- Line 1240: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back(format & 0xFF);
- Line 1263: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back('S');
- Line 1268: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back('C');
- Line 1273: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: payload.push_back('M');
- Line 1411: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: params.push_back("NULL");
- Line 1526: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool shouldContinue = false;
- Line 1543: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } catch (const std::exception& e) {

                std::cerr << "[PostgresSession] Write completion handler error: " << e.what() << "\n";

                stop();

            } catch (...) {

                logCurrentException("Write completion handler error");

                stop();

            }
- Line 1543: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 1597: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool PostgresSession::isSchemaQuery(const std::string& query) {
- Line 1610: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void PostgresSession::handleSchemaQuery(const std::string& query) {
- Line 1720: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::cerr << "[PostgresSession] pg_attribute query: document parse error: " << e.what() << "\n";
- Line 1761: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: PostgresSession::QueryInfo PostgresSession::parseSelectQuery(const std::string& query) {
- Line 1920: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i > 0) cypher += ", ";
- Line 1921: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i > 0) cypher += ", ";
- Line 1926: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: cypher += "count(n)";
- Line 1932: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: cypher += "count(n." + col + ")";
- Line 1937: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: cypher += "sum(n." + col + ")";
- Line 1942: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: cypher += "avg(n." + col + ")";
- Line 1947: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: cypher += "min(n." + col + ")";
- Line 1959: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i > 0) cypher += ", ";
- Line 1960: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i > 0) cypher += ", ";
- Line 1963: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: cypher += "n";
- Line 1965: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: cypher += "n." + col;
- Line 1991: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::string PostgresSession::parseInsertQuery(const std::string& query) {
- Line 2079: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i > 0) cypher += ", ";
- Line 2080: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i > 0) cypher += ", ";
- Line 2089: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::string PostgresSession::parseUpdateQuery(const std::string& query) {
- Line 2152: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: assignments.push_back(cypherSetClause.substr(start, i - start));
- Line 2153: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: assignments.push_back(cypherSetClause.substr(start, i - start));
- Line 2158: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: assignments.push_back(cypherSetClause.substr(start));
- Line 2183: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i > 0) cypherSetClause += ", ";
- Line 2184: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i > 0) cypherSetClause += ", ";
- Line 2194: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::string PostgresSession::parseDeleteQuery(const std::string& query) {
- Line 2240: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::string PostgresSession::translateQuery(const std::string& postgresQuery) {

### server/import_wizard_builder.cpp
Total findings: 118

- Line 33: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    html += "<!DOCTYPE html>\\n<html lang=\\"en\\">\\n<head>\\n";', '    html += "<meta charset=\\"UTF-8\\">\\n";', '    html += "<meta name=\\"viewport\\" content=\\"width=device-width,initial-scale=1\\">\\n";', '    html += "<title>ThemisDB Import Wizard</title>\\n";', '    html += "<style>\\n";']
- Line 233: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 34: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<title>ThemisDB Import Wizard</title>\n";
- Line 116: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</style>\n</head>\n<body>\n";
- Line 120: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<h1>&#128190; ThemisDB Import Wizard</h1>\n";
- Line 121: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<p class=\"sub\">Import data from PostgreSQL or S3 into ThemisDB in a few guided steps.</p>
- Line 125: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <div class=\"step active\" id=\"step-tab-1\">1&nbsp;Source</div>\n";
- Line 126: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <div class=\"step\" id=\"step-tab-2\">2&nbsp;Connection</div>\n";
- Line 127: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <div class=\"step\" id=\"step-tab-3\">3&nbsp;Options</div>\n";
- Line 128: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <div class=\"step\" id=\"step-tab-4\">4&nbsp;Review</div>\n";
- Line 129: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <div class=\"step\" id=\"step-tab-5\">5&nbsp;Progress</div>\n";
- Line 130: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</div>\n";
- Line 134: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<h2>Choose a data source</h2>\n";
- Line 137: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    <div class=\"icon\">&#128036;</div>\n";
- Line 138: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    <div class=\"name\">PostgreSQL</div>\n";
- Line 139: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    <div class=\"desc\">pg_dump file or connection string</div>\n";
- Line 140: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  </div>\n";
- Line 142: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    <div class=\"icon\">&#9729;</div>\n";
- Line 143: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    <div class=\"name\">S3 / Object Storage</div>\n";
- Line 144: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    <div class=\"desc\">s3://bucket/key (CSV, TSV, JSONL)</div>\n";
- Line 145: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  </div>\n";
- Line 146: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</div>\n";
- Line 147: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<div class=\"btn-row\"><button class=\"btn btn-primary\" onclick=\"goStep(2)\">Next &rarr;<
- Line 148: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</div>\n";  // panel-1
- Line 152: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<h2>Configure source</h2>\n";
- Line 154: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <label for=\"pg-path\">pg_dump file path or PostgreSQL connection string</label>\n";
- Line 155: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <input id=\"pg-path\" type=\"text\" placeholder=\"/path/to/dump.sql  or  postgresql://use
- Line 156: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <p style=\"font-size:0.78rem;color:#7a7aaa;margin-top:4px\">The path is resolved on the T
- Line 157: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</div>\n";
- Line 159: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <label for=\"s3-path\">S3 URL</label>\n";
- Line 160: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <input id=\"s3-path\" type=\"text\" placeholder=\"s3://my-bucket/export/dump.csv\">\n";
- Line 161: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <p style=\"font-size:0.78rem;color:#7a7aaa;margin-top:4px\">Use a trailing / to import an
- Line 162: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</div>\n";
- Line 163: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<div id=\"step2-error\" class=\"alert alert-error\" style=\"display:none\"></div>\n";
- Line 165: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <button class=\"btn btn-secondary\" onclick=\"goStep(1)\">&larr; Back</button>\n";
- Line 166: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <button class=\"btn btn-primary\" onclick=\"goStep(3)\">Next &rarr;</button>\n";
- Line 167: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</div>\n";
- Line 168: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</div>\n";  // panel-2
- Line 172: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<h2>Import options</h2>\n";
- Line 173: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<label for=\"opt-namespace\">Target namespace</label>\n";
- Line 175: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<label for=\"opt-batch\">Batch size (rows per transaction)</label>\n";
- Line 179: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <label for=\"opt-dryrun\">Dry-run (validate without writing data)</label>\n";
- Line 180: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</div>\n";
- Line 183: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <label for=\"opt-skip-dup\">Skip duplicate records</label>\n";
- Line 184: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</div>\n";
- Line 187: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <label for=\"opt-continue-err\">Continue on row errors</label>\n";
- Line 188: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</div>\n";
- Line 189: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<details><summary>&#9881; Advanced options</summary>\n";
- Line 190: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<label for=\"opt-include\">Include tables (comma-separated, empty = all)</label>\n";
- Line 192: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<label for=\"opt-exclude\">Exclude tables (comma-separated)</label>\n";
- Line 194: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<label for=\"opt-conflict\">Conflict strategy</label>\n";
- Line 196: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <option value=\"0\" selected>Overwrite (replace existing)</option>\n";
- Line 197: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <option value=\"1\">Skip (keep existing)</option>\n";
- Line 198: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <option value=\"2\">Merge (field-level, incoming wins)</option>\n";
- Line 199: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <option value=\"3\">Error (abort on first conflict)</option>\n";
- Line 200: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</select>\n";
- Line 201: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</details>\n";
- Line 203: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <button class=\"btn btn-secondary\" onclick=\"goStep(2)\">&larr; Back</button>\n";
- Line 204: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <button class=\"btn btn-primary\" onclick=\"goStep(4)\">Review &rarr;</button>\n";
- Line 205: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</div>\n";
- Line 206: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</div>\n";  // panel-3
- Line 210: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<h2>Review &amp; start import</h2>\n";
- Line 212: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "background:#0f1829;border-radius:4px;padding:14px\"></div>\n";
- Line 214: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <button class=\"btn btn-secondary\" onclick=\"goStep(3)\">&larr; Back</button>\n";
- Line 215: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <button class=\"btn btn-primary\" id=\"btn-start\" onclick=\"startImport()\">&#9654; Star
- Line 216: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</div>\n";
- Line 217: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</div>\n";  // panel-4
- Line 221: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<h2>Import progress</h2>\n";
- Line 224: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    <span id=\"prog-stage\" style=\"font-size:0.85rem;color:#aaaacc;font-style:italic\">Ini
- Line 225: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    <span id=\"prog-badge\" class=\"status-badge s-running\">running</span>\n";
- Line 226: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  </div>\n";
- Line 227: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <div class=\"progress-bar-wrap\"><div class=\"progress-bar\" id=\"prog-bar\" style=\"widt
- Line 229: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "<span id=\"prog-current\">0</span> / <span id=\"prog-total\">?</span> records</div>\n";
- Line 230: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</div>\n";
- Line 231: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<div id=\"result-box\" style=\"display:none;margin-top:18px\"></div>\n";
- Line 233: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <button class=\"btn btn-secondary\" id=\"btn-new\" onclick=\"resetWizard()\">&#8635; New
- Line 234: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <button class=\"btn btn-danger\" id=\"btn-cancel\" onclick=\"cancelImport()\">&#9632; Can
- Line 235: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</div>\n";
- Line 236: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</div>\n";  // panel-5
- Line 238: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</div>\n";  // card
- Line 242: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<h2>Recent import jobs</h2>\n";
- Line 243: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<div id=\"jobs-list\"><em style=\"color:#555\">Loading&#8230;</em></div>\n";
- Line 245: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "<button class=\"btn btn-secondary\" style=\"font-size:0.8rem\" onclick=\"loadJobs()\">&#8635; Refre
- Line 246: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "</div>\n";
- Line 247: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</div>\n";  // jobs-panel
- Line 268: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: html += "    var p=document.getElementById('panel-'+i);\n";
- Line 293: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  function row(k,v){return '<tr><td style=\"color:#7a7aaa;padding:3px 8px 3px 0;white-space
- Line 294: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "<td style=\"color:#d0d0e8;padding:3px 0\">'+v+'</td></tr>';}\n";
- Line 295: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  html+=row('Source type',currentSource==='postgresql'?'PostgreSQL':'S3 / Object Storage');
- Line 296: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  html+=row('Source path','<code>'+escHtml(path)+'</code>');\n";
- Line 297: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  html+=row('Target namespace','<code>'+escHtml(ns)+'</code>');\n";
- Line 299: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  html+=row('Dry-run',dr?'<span style=\"color:#ffcc00\">Yes (no data written)</span>':'No')
- Line 302: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  if(inc) html+=row('Include tables','<code>'+escHtml(inc)+'</code>');\n";
- Line 303: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  if(exc) html+=row('Exclude tables','<code>'+escHtml(exc)+'</code>');\n";
- Line 305: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  html+='</table>';\n";
- Line 311: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  return String(s).replace(/&/g,'&amp;').replace(/</g,'<').replace(/>/g,'>');\n";
- Line 337: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  var url=(currentSource==='postgresql')?'/api/v1/import/postgresql':'/api/v1/import/s3';\n
- Line 340: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  fetch(url,{method:'POST',headers:{'Content-Type':'application/json'},body:body})\n";
- Line 354: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  fetch('/api/v1/import/'+currentJobId+'/status')\n";
- Line 375: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  var pct=(tot>0)?Math.min(100,Math.round(cur/tot*100)):0;\n";
- Line 392: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    box.innerHTML+='<strong>Import complete</strong><br>';\n";
- Line 393: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    box.innerHTML+='Imported: <b>'+s.imported_records+'</b> &nbsp; ';\n";
- Line 394: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    box.innerHTML+='Skipped: <b>'+s.skipped_records+'</b> &nbsp; ';\n";
- Line 395: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    box.innerHTML+='Failed: <b>'+s.failed_records+'</b> &nbsp; ';\n";
- Line 396: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    box.innerHTML+='Time: <b>'+(s.elapsed_seconds||0).toFixed(2)+'s</b>';\n";
- Line 397: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    box.innerHTML+='</div>';\n";
- Line 399: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    box.innerHTML='<div class=\"alert alert-error\">Import '+data.status+'.</div>';\n";
- Line 407: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  fetch('/api/v1/import/'+currentJobId+'/cancel',{method:'POST'})\n";
- Line 428: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  fetch('/api/v1/import/jobs')\n";
- Line 432: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "      if(!jobs.length){el.innerHTML='<em style=\"color:#555\">No jobs yet.</em>';return;}\n
- Line 437: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "        html+='<span class=\"job-id\" title=\"'+escHtml(j.id)+'\">'+escHtml(j.id)+'</span>'
- Line 438: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "        html+='<span class=\"status-badge s-'+(j.status||'pending')+'\">'+escHtml(j.status)
- Line 440: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "          html+='<span class=\"job-stats\">'+stats.imported_records+' imported, '+stats.fai
- Line 442: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "          html+='<span class=\"job-stage\">'+escHtml(j.stage||'')+'</span>';\n";
- Line 444: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "        html+='</div>';\n";
- Line 447: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    }).catch(function(e){document.getElementById('jobs-list').innerHTML='<em style=\"color:
- Line 452: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</script>\n";
- Line 453: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</body>\n</html>\n";

### server/task_scheduler_api_handler.cpp
Total findings: 113

- Line 602: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: html += "  return new Date(iso).toLocaleString();\n";
- Line 602: severity=CRITICAL; category=new_without_raii
  Description: Raw new() without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: html += "\n";

    html += "function formatDate(iso) {\n";

    html += "  if (!iso) return '–';\n";

    html += "  return new Date(iso).toLocaleString();\n";

    html += "}\n";

    html += "\n";

    html += "async function loadTasks() {\n";
- Line 602: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: html += "  return new Date(iso).toLocaleString();\n";
- Line 646: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: html += "  document.getElementById('refresh-indicator').textContent = 'Updated ' + new Date().toLoca
- Line 646: severity=CRITICAL; category=new_without_raii
  Description: Raw new() without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: html += "async function loadAll() {\n";

    html += "  document.getElementById('refresh-indicator').textContent = 'Refreshing…';\n";

    html += "  await Promise.all([loadStats(), loadTasks()]);\n";

    html += "  document.getElementById('refresh-indicator').textContent = 'Updated ' + new Date().toLocaleTimeString();\n";

    html += "}\n";

    html += "\n";

    html += "async function runTask(id) {\n";
- Line 646: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: html += "  document.getElementById('refresh-indicator').textContent = 'Updated ' + new Date().toLoca
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3365 feat(scheduler): expose mul... (2026-03-12) | #3362 feat(scheduler): ex
- Line 440: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    html += "<!DOCTYPE html>\\n<html lang=\\"en\\">\\n<head>\\n";', '    html += "<meta charset=\\"UTF-8\\">\\n";', '    html += "<meta name=\\"viewport\\" content=\\"width=device-width, initial-scale=1\\">\\n";', '    html += "<title>ThemisDB – Task Scheduler</title>\\n";', '    html += "<style>\\n";']
- Line 602: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 646: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 371: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

        if (v.is_string()) {

            try { auto iv = std::stoll(v.get<std::string>()); return iv > 0 ? static_cast<size_t>(iv) : def; }

            catch (...) { return def; }

        }

        return def;

    };
- Line 371: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: catch (...) { return def; }
- Line 403: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: int64_t ms = 0;

        const auto& v = query_params["start_time_ms"];

        if (v.is_number_integer()) ms = v.get<int64_t>();

        else if (v.is_string()) { try { ms = std::stoll(v.get<std::string>()); } catch (...) {} }

        if (ms > 0) params.start_time = std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));

    }

    if (query_params.contains("end_time_ms") && !query_params["end_time_ms"].is_null()) {
- Line 403: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: else if (v.is_string()) { try { ms = std::stoll(v.get<std::string>()); } catch (...) {} }
- Line 410: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: int64_t ms = 0;

        const auto& v = query_params["end_time_ms"];

        if (v.is_number_integer()) ms = v.get<int64_t>();

        else if (v.is_string()) { try { ms = std::stoll(v.get<std::string>()); } catch (...) {} }

        if (ms > 0) params.end_time = std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));

    }
- Line 410: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: else if (v.is_string()) { try { ms = std::stoll(v.get<std::string>()); } catch (...) {} }
- Line 414: severity=MEDIUM; category=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: params.sort_by = scheduler::AuditQueryParams::SortBy::TIMESTAMP_DESC;
- Line 437: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: html += "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n";
- Line 441: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<title>ThemisDB – Task Scheduler</title>\n";
- Line 486: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</style>\n</head>\n<body>\n";
- Line 489: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <h1>&#x23F2; Task Scheduler</h1>\n";
- Line 490: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <span class=\"badge\">ThemisDB</span>\n";
- Line 491: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</header>\n";
- Line 497: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-registered\">–</div><div class=\"lbl\"
- Line 498: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-active\">–</div><div class=\"lbl\">Act
- Line 499: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-running\">–</div><div class=\"lbl\">Ru
- Line 500: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-total\">–</div><div class=\"lbl\">Exec
- Line 501: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-failed\">–</div><div class=\"lbl\">Fai
- Line 502: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-status\">–</div><div class=\"lbl\">Sch
- Line 503: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</div>\n";
- Line 507: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <button class=\"btn-primary\" onclick=\"openCreateDialog()\">&#43; New Task</button>\n";
- Line 508: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <button class=\"btn-secondary\" onclick=\"loadAll()\">&#8635; Refresh</button>\n";
- Line 509: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <span id=\"refresh-indicator\"></span>\n";
- Line 510: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</div>\n";
- Line 514: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <th>ID / Name</th><th>Type</th><th>Trigger</th><th>Status</th>\n";
- Line 515: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <th>Executions</th><th>Last Error</th><th>Next Run</th><th>Actions</th>\n";
- Line 516: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</tr></thead>\n";
- Line 517: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<tbody id=\"task-table-body\"><tr><td colspan=\"8\" style=\"text-align:center;color:#64748b
- Line 518: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</table>\n";
- Line 520: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</div>\n"; // end .container
- Line 523: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<div class=\"toast\" id=\"toast\"></div>\n";
- Line 527: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <h2 id=\"dialog-title\">New Task</h2>\n";
- Line 529: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <label>Name</label>\n";
- Line 531: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <label>Description</label>\n";
- Line 533: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <label>Type</label>\n";
- Line 535: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    <option value=\"aql_query\">AQL Query</option>\n";
- Line 536: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    <option value=\"function\">Function</option>\n";
- Line 537: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  </select>\n";
- Line 539: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    <label>AQL Query</label>\n";
- Line 540: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    <textarea id=\"f-aql\" placeholder=\"FOR d IN collection RETURN d\"></textarea>\n";
- Line 541: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  </div>\n";
- Line 543: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    <label>Function Name</label>\n";
- Line 545: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  </div>\n";
- Line 546: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <label>Trigger</label>\n";
- Line 548: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    <option value=\"interval\">Interval</option>\n";
- Line 549: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    <option value=\"cron\">Cron</option>\n";
- Line 550: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    <option value=\"manual\">Manual</option>\n";
- Line 551: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  </select>\n";
- Line 553: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    <label>Interval (seconds)</label>\n";
- Line 555: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  </div>\n";
- Line 557: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    <label>Cron Expression</label>\n";
- Line 558: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    <input id=\"f-cron\" type=\"text\" placeholder=\"*/5 * * * *\">\n";
- Line 559: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  </div>\n";
- Line 560: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <label>Timeout (seconds)</label>\n";
- Line 562: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  <label>Max Retries</label>\n";
- Line 565: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    <button class=\"btn-secondary\" onclick=\"closeDialog()\">Cancel</button>\n";
- Line 566: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    <button class=\"btn-primary\" onclick=\"saveTask()\">Save</button>\n";
- Line 567: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  </div>\n";
- Line 568: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</dialog>\n";
- Line 572: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "const API = '/api/tasks';\n";
- Line 575: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  const opts = { method, headers: {'Content-Type':'application/json'} };\n";
- Line 589: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  const s = await api('GET', API + '/stats');\n";
- Line 609: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    tbody.innerHTML = '<tr><td colspan=\"8\" style=\"text-align:center;color:#64748b\">No t
- Line 616: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "      ? '<span class=\"tag tag-aql\">AQL</span>'\n";
- Line 617: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "      : '<span class=\"tag tag-fn\">Fn</span>';\n";
- Line 619: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "      ? `<button class=\"btn-warning\" onclick=\"disableTask('${t.id}')\">Pause</button>`\n
- Line 620: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "      : `<button class=\"btn-success\" onclick=\"enableTask('${t.id}')\">Resume</button>`;\
- Line 622: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "      <td><b>${escHtml(t.id)}</b><br><small style=\"color:#94a3b8\">${escHtml(t.name)}</sma
- Line 623: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "      <td>${typeTag}</td>\n";
- Line 624: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "      <td>${escHtml(t.trigger_type || '–')}</td>\n";
- Line 625: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "      <td class=\"${statusClass}\">${statusText}</td>\n";
- Line 626: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "      <td>${t.successful_executions ?? 0} / ${t.total_executions ?? 0}</td>\n";
- Line 627: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "      <td style=\"color:#f87171;font-size:.8rem\">${escHtml(t.last_error || '')}</td>\n";
- Line 628: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "      <td style=\"font-size:.8rem\">${formatDate(t.next_run)}</td>\n";
- Line 630: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "        <button class=\"btn-primary\" onclick=\"runTask('${t.id}')\">&#9654; Run</button>\n
- Line 632: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "        <button class=\"btn-secondary\" onclick=\"editTask('${t.id}')\">&#9998; Edit</butto
- Line 633: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "        <button class=\"btn-danger\" onclick=\"deleteTask('${t.id}')\">&#x1F5D1;</button>\n
- Line 634: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "      </td>\n";
- Line 635: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    </tr>`;\n";
- Line 640: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  return String(s).replace(/&/g,'&amp;').replace(/</g,'<').replace(/>/g,'>').replace(/\"/g,
- Line 650: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  const r = await api('POST', API + '/' + id + '/execute');\n";
- Line 656: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  const r = await api('POST', API + '/' + id + '/enable');\n";
- Line 662: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  const r = await api('POST', API + '/' + id + '/disable');\n";
- Line 669: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  const r = await api('DELETE', API + '/' + id);\n";
- Line 692: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  const t = await api('GET', API + '/' + id);\n";
- Line 702: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  document.getElementById('f-interval').value = Math.round((t.interval_ms || 300000) / 1000
- Line 704: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "  document.getElementById('f-timeout').value = Math.round((t.timeout_ms || 600000) / 1000);
- Line 711: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: html += "  document.getElementById('task-dialog').close();\n";
- Line 744: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "    ? await api('PUT', API + '/' + id, body)\n";
- Line 755: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "// Auto-refresh every 30 seconds\n";
- Line 758: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</script>\n";
- Line 759: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</body>\n</html>\n";
- Line 861: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto ms = j["interval_ms"].get<int64_t>();
- Line 867: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto secs = j["interval_seconds"].get<int64_t>();
- Line 873: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto ms = j["timeout_ms"].get<int64_t>();
- Line 879: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto secs = j["timeout_seconds"].get<int64_t>();
- Line 915: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto task_ids = request["task_ids"].get<std::vector<std::string>>();
- Line 981: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto it = req["extra_labels"].begin(); it != req["extra_labels"].end(); ++it) {
- Line 507: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: html += "  <button class=\"btn-primary\" onclick=\"openCreateDialog()\">&#43; New Task</button>\n";
- Line 565: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: html += "    <button class=\"btn-secondary\" onclick=\"closeDialog()\">Cancel</button>\n";
- Line 674: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: html += "function openCreateDialog() {\n";
- Line 710: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: html += "function closeDialog() {\n";
- Line 748: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: html += "    closeDialog();\n";

### server/monitoring_api_handler.cpp
Total findings: 69

- Line 117: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: bool alive = (is_running_ == nullptr) || is_running_->load(std::memory_order_relaxed);
- Line 144: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: bool server_running = (is_running_ == nullptr) || is_running_->load(std::memory_order_relaxed);
- Line 446: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const auto loop_context = continuous_learning_orchestrator_->serializeLoopContext();
- Line 660: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const std::string loop_context = continuous_learning_orchestrator_->serializeLoopContext();
- Line 1421: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: loop_context = continuous_learning_orchestrator_->serializeLoopContext();
- Line 1472: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator next may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto next = arr.find("},{", cur);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3149 feat(api): Complete OpenAPI... (2026-03-12) | #2831 [config] Wire Prome
- Line 488: severity=HIGH; category=catch_all_swallow
  Description: catch(...) block swallows errors without rethrowing or explicit handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_design_error_rules
  Context: catch(...) { ... }
- Line 932: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Config path resolution metrics (hit rate, miss rate, legacy fallback rate)
- Line 1396: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['        html += "<!DOCTYPE html>\\n<html lang=\\"en\\">\\n<head>\\n";', '        html += "<meta charset=\\"UTF-8\\">\\n";', '        html += "<meta name=\\"viewport\\" content=\\"width=device-width, initial-scale=1\\">\\n";', '        html += "<title>ThemisDB Metrics</title>\\n";', '        html += "<style>\\n";']
- Line 631: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: out += "# HELP themis_build_info ThemisDB build information\n";
- Line 674: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: out += "# HELP themis_continuous_learning_loop_signal_value Latest loop signal value\n";
- Line 683: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: out += "# HELP themis_continuous_learning_loop_live_signal Loop uses live provider signal (1=yes,0=f
- Line 704: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: out += "themis_continuous_learning_loop_signal_value" + labels +
- Line 720: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

            } catch (const std::exception& e) {

                THEMIS_WARN("Failed to collect continuous learning metrics: {}", e.what());

            } catch (...) {

                THEMIS_WARN("Unknown error while collecting continuous learning metrics");

            }

        }
- Line 720: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 761: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto it = r["files_per_level"].begin(); it != r["files_per_level"].end(); ++it) {
- Line 771: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (level.size() > 1 && (level[0] == 'L' || level[0] == 'l')) {

                    try {

                        return std::stoi(level.substr(1));

                    } catch (...) {

                    }

                }

                return -1;
- Line 771: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 785: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: out += "rocksdb_files_level{level=\"" + level + "\"} " + std::to_string(val) + "\n";
- Line 786: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += "rocksdb_files_level{level=\"" + level + "\"} " + std::to_string(val) + "\n";
- Line 832: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: out += "\n# HELP themis_plugin_loads_total Total number of plugin loads\n";
- Line 835: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: out += "themis_plugin_loads_total{plugin=\"" + plugin_name + "\"} 1\n";
- Line 836: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += "themis_plugin_loads_total{plugin=\"" + plugin_name + "\"} 1\n";
- Line 842: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: out += "themis_plugin_reloads_total{plugin=\"" + plugin_name + "\"} "
- Line 843: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += "themis_plugin_reloads_total{plugin=\"" + plugin_name + "\"} "
- Line 850: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: out += "themis_plugin_errors_total{plugin=\"" + plugin_name + "\"} "
- Line 851: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += "themis_plugin_errors_total{plugin=\"" + plugin_name + "\"} "
- Line 858: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: out += "themis_plugin_function_calls_total{plugin=\"" + plugin_name + "\"} "
- Line 859: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += "themis_plugin_function_calls_total{plugin=\"" + plugin_name + "\"} "
- Line 867: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: out += "themis_plugin_load_duration_seconds_sum{plugin=\"" + plugin_name + "\"} "
- Line 868: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += "themis_plugin_load_duration_seconds_sum{plugin=\"" + plugin_name + "\"} "
- Line 870: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += "themis_plugin_load_duration_seconds_count{plugin=\"" + plugin_name + "\"} 1\n";
- Line 876: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: out += "themis_plugin_memory_bytes{plugin=\"" + plugin_name + "\"} "
- Line 877: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += "themis_plugin_memory_bytes{plugin=\"" + plugin_name + "\"} "
- Line 885: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: out += "themis_plugin_call_latency_milliseconds{plugin=\"" + plugin_name
- Line 886: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += "themis_plugin_call_latency_milliseconds{plugin=\"" + plugin_name
- Line 888: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += "themis_plugin_call_latency_milliseconds{plugin=\"" + plugin_name
- Line 890: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += "themis_plugin_call_latency_milliseconds_sum{plugin=\"" + plugin_name
- Line 892: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += "themis_plugin_call_latency_milliseconds_count{plugin=\"" + plugin_name
- Line 900: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } catch (const std::exception& e) {

            // If plugin metrics fail, log and continue without them

            THEMIS_WARN("Failed to collect plugin metrics: {}", e.what());

        } catch (...) {

            // Catch any other exceptions to prevent metrics collection from breaking /metrics endpoint

            THEMIS_WARN("Unknown error while collecting plugin metrics");

        }
- Line 900: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 943: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: themis::config::ConfigMetricsExporter::updateMetricsCollector();

        } catch (const std::exception& e) {

            THEMIS_WARN("Failed to collect config path resolution metrics: {}", e.what());

        } catch (...) {

            THEMIS_WARN("Unknown error while collecting config path resolution metrics");

        }
- Line 943: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 1384: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: case '&': escaped += "&amp;"; break;
- Line 1385: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '&': escaped += "&amp;"; break;
- Line 1386: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '<': escaped += "&lt;"; break;
- Line 1387: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '>': escaped += "&gt;"; break;
- Line 1388: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '"': escaped += "&quot;"; break;
- Line 1397: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<title>ThemisDB Metrics</title>\n";
- Line 1408: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<h1>ThemisDB Metrics Dashboard</h1>\n";
- Line 1409: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<p class=\"sub\">Version: <b>" + version + "</b> &nbsp;|&nbsp; ";
- Line 1411: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<a href=\"/metrics\">Raw Prometheus</a></p>\n";
- Line 1412: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<table>\n<tr><th>Metric</th><th class=\"val\">Value</th></tr>\n";
- Line 1413: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: html += "<tr><td>" + name + "</td><td class=\"val\">" + val + "</td></tr>\n";
- Line 1414: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<tr><td>" + name + "</td><td class=\"val\">" + val + "</td></tr>\n";
- Line 1414: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: html += "<tr><td>" + name + "</td><td class=\"val\">" + val + "</td></tr>\n";
- Line 1416: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</table>\n";
- Line 1428: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<h2>Continuous Learning Loops</h2>\n";
- Line 1474: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: loop_items.push_back(arr.substr(cur));
- Line 1477: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: loop_items.push_back(arr.substr(cur, next - cur + 1));
- Line 1484: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<p><em>No loop results yet.</em></p>\n";
- Line 1496: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "</tr>\n";
- Line 1510: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: html += "<tr>";
- Line 1513: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<td class=\"val\">" + escape_html(sig_val) + "</td>";
- Line 1520: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "<td class=\"val\">" + escape_html(mdelta) + "</td>";
- Line 1523: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</tr>\n";
- Line 1525: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</table>\n";
- Line 1529: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: html += "</body>\n</html>\n";

### server/llm_api_handler.cpp
Total findings: 67

- Line 190: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return handleLoadModel(req);
- Line 192: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return handleUnloadModel(req);
- Line 407: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: rag_mode = json_value_to<std::string>(body->at("rag_mode"));
- Line 569: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: llm_request.metadata["rag_tensor_slots"] = body->at("rag_tensor_slots");
- Line 572: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: llm_request.metadata["rag_tensor_slot_chars"] = body->at("rag_tensor_slot_chars");
- Line 898: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: http::response<http::string_body> LLMApiHandler::handleLoadModel(
- Line 900: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto span = Tracer::startSpan("handleLoadModel");
- Line 927: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: bool loaded = plugin_mgr.loadModel(model_id, path);
- Line 930: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: loaded = plugin_mgr.loadModel(model_id, path);
- Line 949: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: THEMIS_ERROR("LLMApiHandler::handleLoadModel: {}", e.what());
- Line 958: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: http::response<http::string_body> LLMApiHandler::handleUnloadModel(
- Line 960: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto span = Tracer::startSpan("handleUnloadModel");
- Line 982: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: plugin_mgr.unloadModel(model_id);
- Line 998: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: logCurrentException("LLMApiHandler::handleUnloadModel");
- Line 1979: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto perm = policy_engine_->checkInferencePermission(header_map);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #5405 W1-S06: Close remaining unc... (2026-05-28) | #4187 feat(llm): OpenAI-c
- Line 156: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: return lora_handler_->handleRequest(req);
- Line 291: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

        llm::InferenceRequest llm_request;

        llm_request.prompt = prompt;

        llm_request.model_id = model_id.empty() ? std::string("default") : model_id;

        llm_request.max_tokens = max_tokens;

        llm_request.temperature = static_cast<float>(temperature);

        if (!lora_id.empty()) {
- Line 293: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: llm_request.prompt = prompt;

        llm_request.model_id = model_id.empty() ? std::string("default") : model_id;

        llm_request.max_tokens = max_tokens;

        llm_request.temperature = static_cast<float>(temperature);

        if (!lora_id.empty()) {

            llm_request.lora_adapter_id = lora_id;

        }
- Line 299: severity=HIGH; category=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto llm_response = plugin_mgr.generate(llm_request);
- Line 299: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto llm_response = plugin_mgr.generate(llm_request);
- Line 306: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: const auto tokens_generated = llm_response.tokens_generated;
- Line 309: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: const int safe_tokens_generated = tokens_generated > 0 ? tokens_generated : 1;
- Line 331: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: "LLMApiHandler::handleInference success: model='{}' prompt_len={} tokens_generated={} inference_time_ms={:.2f} lora='{}'",
- Line 563: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Prepare inference request

        llm::InferenceRequest llm_request;

        llm_request.prompt = query;

        llm_request.model_id = model_id.empty() ? std::string("default") : model_id;

        llm_request.max_tokens = effective_generation_max_tokens;

        llm_request.temperature = static_cast<float>(temperature);

        llm_request.lora_adapter_id = lora_id;
- Line 567: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: llm_request.max_tokens = effective_generation_max_tokens;

        llm_request.temperature = static_cast<float>(temperature);

        llm_request.lora_adapter_id = lora_id;

        llm_request.metadata["rag_mode"] = rag_mode;

        if (body->contains("rag_tensor_slots")) {

            llm_request.metadata["rag_tensor_slots"] = body->at("rag_tensor_slots");

        }
- Line 569: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: llm_request.lora_adapter_id = lora_id;

        llm_request.metadata["rag_mode"] = rag_mode;

        if (body->contains("rag_tensor_slots")) {

            llm_request.metadata["rag_tensor_slots"] = body->at("rag_tensor_slots");

        }

        if (body->contains("rag_tensor_slot_chars")) {

            llm_request.metadata["rag_tensor_slot_chars"] = body->at("rag_tensor_slot_chars");
- Line 572: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: llm_request.metadata["rag_tensor_slots"] = body->at("rag_tensor_slots");

        }

        if (body->contains("rag_tensor_slot_chars")) {

            llm_request.metadata["rag_tensor_slot_chars"] = body->at("rag_tensor_slot_chars");

        }



        // Call LLMPluginManager for RAG inference
- Line 576: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto llm_response = plugin_mgr.generateRAG(rag_context, llm_request);
- Line 596: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: "LLMApiHandler::handleRAG success: query_len={} collection='{}' top_k={} docs_retrieved={} tokens_generated={} inference_time_ms={:.2f} cache_hit={} rag_mode='{}' lora='{}'",
- Line 868: severity=HIGH; category=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: http::response<http::string_body> LLMApiHandler::handleListModels(
- Line 898: severity=HIGH; category=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: http::response<http::string_body> LLMApiHandler::handleLoadModel(
- Line 914: severity=HIGH; category=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return createErrorResponse(http::status::bad_request, "Missing 'model_id' field");
- Line 921: severity=HIGH; category=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return createErrorResponse(http::status::bad_request, "Invalid load model parameters", e.what());
- Line 958: severity=HIGH; category=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: http::response<http::string_body> LLMApiHandler::handleUnloadModel(
- Line 973: severity=HIGH; category=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return createErrorResponse(http::status::bad_request, "Missing 'model_id' field");
- Line 976: severity=HIGH; category=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return createErrorResponse(http::status::bad_request, "Invalid unload model parameters", e.what());
- Line 999: severity=HIGH; category=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return createErrorResponse(http::status::internal_server_error, "Failed to unload model");
- Line 1003: severity=HIGH; category=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: http::response<http::string_body> LLMApiHandler::handleModelInfo(
- Line 1042: severity=HIGH; category=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return createErrorResponse(http::status::internal_server_error, "Model info retrieval failed");
- Line 1046: severity=HIGH; category=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: http::response<http::string_body> LLMApiHandler::handleIngestModel(
- Line 1064: severity=HIGH; category=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return createErrorResponse(http::status::bad_request, "Missing 'model_id' field");
- Line 1092: severity=HIGH; category=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return createErrorResponse(http::status::internal_server_error, "Model ingestion failed");
- Line 1521: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: {"content_preview", doc.text_content.substr(0, 200) + "..."}

            });

        }

        response_data["relevant_documents"] = docs_array;

        

        return createJsonResponse(response_data);
- Line 1745: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Convert to JSON response

        json response_data = stored.toJson();

        response_data["message"] = "Feedback recorded successfully";

        

        return createJsonResponse(response_data, http::status::created);
- Line 2018: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // write chunks incrementally; here we buffer them for compatibility
- Line 2030: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: spdlog::info(

                "LLMApiHandler::handleOpenAIChatCompletions stream start: model='{}' prompt_len={} request_max_tokens={}",

                model_id.empty() ? std::string{"default"} : model_id,

                llm_request.prompt.size(),

                llm_request.max_tokens);



            llm_request.stream_callback = [&](const std::string& token) {
- Line 2033: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: llm_request.prompt.size(),

                llm_request.max_tokens);



            llm_request.stream_callback = [&](const std::string& token) {

                sse_body += llm::OpenAICompatAdapter::buildStreamChunk(

                    token, completion_id, model_id, created);

            };
- Line 2040: severity=HIGH; category=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: plugin_mgr.generate(llm_request);
- Line 2082: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: spdlog::info(

                "LLMApiHandler::handleOpenAIChatCompletions non-stream start: model='{}' prompt_len={} request_max_tokens={}",

                model_id.empty() ? std::string{"default"} : model_id,

                llm_request.prompt.size(),

                llm_request.max_tokens);



            llm::InferenceResponse llm_response;
- Line 2088: severity=HIGH; category=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: llm_response = plugin_mgr.generate(llm_request);
- Line 2088: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: llm_response = plugin_mgr.generate(llm_request);
- Line 2111: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: "LLMApiHandler::handleOpenAIChatCompletions non-stream complete: model='{}' tokens_generated={} inference_time_ms={:.2f}",
- Line 2131: severity=HIGH; category=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: http::response<http::string_body> LLMApiHandler::handleOpenAIListModels(
- Line 78: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto content = entity["content"].get<std::string>();
- Line 84: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto content = entity["text"].get<std::string>();
- Line 90: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto content = entity["body"].get<std::string>();
- Line 103: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto source = entity["source"].get<std::string>();
- Line 109: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto source = entity["id"].get<std::string>();
- Line 184: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return handleEmbed(req);
- Line 491: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: const std::vector<float> query_vec = plugin_mgr.embed(query);
- Line 619: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: http::response<http::string_body> LLMApiHandler::handleEmbed(
- Line 696: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (raw[i] == '+') { decoded += ' '; ++i; }
- Line 697: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (raw[i] == '+') { decoded += ' '; ++i; }
- Line 1974: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::string> header_map;
- Line 2040: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: plugin_mgr.generate(llm_request);
- Line 2088: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: llm_response = plugin_mgr.generate(llm_request);

### server/mcp_server.cpp
Total findings: 63

- Line 2831: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Windows implementation using PeekNamedPipe for non-blocking stdin

    std::weak_ptr<StdioTransport> weak_self = weak_from_this();

    asio::post(io_context_, [weak_self]() {

        auto self = weak_self.lock();

        if (!self) return;



        HANDLE h_stdin = GetStdHandle(STD_INPUT_HANDLE);
- Line 2831: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: auto self = weak_self.lock();
- Line 3163: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = sessions_.find(session_id);
- Line 103: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 119: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 343: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Fall back to legacy path if new one doesn't exist
- Line 489: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: json McpServer::handleRequest(const json& request) {

    try {

        // Validate JSON-RPC 2.0 request

        if (!request.contains("jsonrpc") || request["jsonrpc"] != "2.0") {

            return createError(-32600, "Invalid Request: missing or invalid 'jsonrpc' field");

        }
- Line 493: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return createError(-32600, "Invalid Request: missing or invalid 'jsonrpc' field");

        }



        if (!request.contains("method")) {

            return createError(-32600, "Invalid Request: missing 'method' field");

        }
- Line 498: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



        std::string method = request["method"];

        json params = request.contains("params") ? request["params"] : json::object();



        // Route to appropriate handler

        if (method == "initialize") {
- Line 723: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: {"description", "AI Safety: preview the delete without executing it (Phase 1 guard)"}}}
- Line 723: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: {"properties", {

                {"key", {{"type", "string"}}},

                {"dry_run", {{"type", "boolean"}, {"default", false},

                             {"description", "AI Safety: preview the delete without executing it (Phase 1 guard)"}}}

            }},

            {"required", {"key"}}

        },
- Line 723: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: {"description", "AI Safety: preview the delete without executing it (Phase 1 guard)"}}}
- Line 862: severity=HIGH; category=missing_trace_point
  Description: Critical function query without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: {"description", "Search query (e.g., 'gpu', 'lora', 'storage')"}
- Line 947: severity=HIGH; category=command_injection
  Description: sql_injection_concat: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: "blocked: keyword='{}' pos={} query='{}'",
- Line 963: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }



            // AI Safety Layer — Schichten 1 & 2: DOG + HILG (ASL-4/5)

            // For AQL write/delete operations: require approval before execution.

            if (const auto guard_resp = checkOperationGuard(

                    "query", args,

                    /*ai_session_id=*/"", /*caller_role=*/"")) {
- Line 963: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // For AQL write/delete operations: require approval before execution.
- Line 1234: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: {"message", "Failed to delete entity"},
- Line 1234: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: } else {

            return {

                {"status", "error"},

                {"message", "Failed to delete entity"},

                {"key", key}

            };

        }
- Line 1234: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: {"message", "Failed to delete entity"},
- Line 2081: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (size_t i = 0; i < std::min(results.size(), size_t(3)); ++i) {
- Line 2102: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (size_t i = 0; i < std::min(results.size(), size_t(3)); ++i) {
- Line 2822: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!is_running_.load(std::memory_order_acquire)) return;
- Line 2840: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: while (self->is_running_.load(std::memory_order_acquire)) {
- Line 2891: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: } else if (!self->is_running_.load(std::memory_order_acquire)) {
- Line 2922: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 2933: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: while (self->is_running_.load(std::memory_order_acquire)) {
- Line 2980: severity=HIGH; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << data << std::flush;
- Line 3019: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!is_running_.load(std::memory_order_acquire)) return;
- Line 3057: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!is_running_.load(std::memory_order_acquire)) return;
- Line 3071: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!is_running_.load(std::memory_order_acquire)) return;
- Line 3079: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!ec && self->is_running_.load(std::memory_order_acquire)) {
- Line 3122: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!is_running_.load(std::memory_order_acquire)) return;
- Line 3137: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!is_running_.load(std::memory_order_acquire)) return;
- Line 3176: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!is_running_.load(std::memory_order_acquire)) return;
- Line 3202: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!is_running_.load(std::memory_order_acquire)) return;
- Line 3223: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!is_running_.load(std::memory_order_acquire)) return;
- Line 3231: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!ec && self->is_running_.load(std::memory_order_acquire)) {
- Line 71: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Phase 2 (ASL-4)' that was not found in 'src/security/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/security/ROADMAP.md § Phase 2 (ASL-4)
- Line 84: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Phase 3 (ASL-9)' that was not found in 'src/security/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/security/ROADMAP.md § Phase 3 (ASL-9)
- Line 151: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Phase 2 (ASL-7)' that was not found in 'src/security/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/security/ROADMAP.md § Phase 2 (ASL-7)
- Line 363: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Phase 4 (ASL-12)' that was not found in 'src/security/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/security/ROADMAP.md § Phase 4 (ASL-12)
- Line 524: severity=MEDIUM; category=missing_health_check
  Description: Service initialization without nearby health/status handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: json McpServer::handleInitialize(const json& params) {
- Line 862: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: {"description", "Search query (e.g., 'gpu', 'lora', 'storage')"}
- Line 887: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: json McpServer::toolQuery(const json& args) {
- Line 1234: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: {"message", "Failed to delete entity"},
- Line 1301: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto ft_config = args["fulltext_config"];
- Line 1682: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: messages.push_back({
- Line 1837: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: {"status", "success"},

            {"error", metadata.toJSON()}

        };

    } catch (...) {

        // Search by query

        auto results = registry.searchErrors(query);
- Line 1837: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 2020: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: answer += fmt::format("**{}** ({} error types)\n", category, errors.size());
- Line 2046: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i > 0) docs_str += ", ";
- Line 2047: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i > 0) docs_str += ", ";
- Line 2279: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: json McpServer::promptSimpleQuery(const std::string& name, const json& args) {
- Line 2294: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: json McpServer::promptComplexQuery(const std::string& name, const json& args) {
- Line 2447: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Phase 3 (ASL-8)' that was not found in 'src/security/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/security/ROADMAP.md § Phase 3 (ASL-8)
- Line 2586: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Phase 3 (ASL-10)' that was not found in 'src/security/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/security/ROADMAP.md § Phase 3 (ASL-10)
- Line 2722: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Phase 3 (ASL-11)' that was not found in 'src/security/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/security/ROADMAP.md § Phase 3 (ASL-11)
- Line 2778: severity=MEDIUM; category=missing_health_check
  Description: Service initialization without nearby health/status handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void StdioTransport::start() {
- Line 2779: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool expected = false;
- Line 2800: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'MCP StdioTransport Platform Support' that was not found in 'src/server/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/server/FUTURE_ENHANCEMENTS.md §"MCP StdioTransport Platform Support"
- Line 2808: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: fn = stdioReadFnStorage();

        }

        if (fn) {

            try { fn(); } catch (...) {}

        } else {

            spdlog::warn("MCP stdio transport: Unsupported platform, stdin reading not implemented");

        }
- Line 2808: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { fn(); } catch (...) {}
- Line 2996: severity=MEDIUM; category=missing_health_check
  Description: Service initialization without nearby health/status handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void SseTransport::start() {

### server/http3_session.cpp
Total findings: 61

- Line 40: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: cid->data[i] = static_cast<uint8_t>(rd() & 0xFFu);
- Line 230: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: THEMIS_INFO("HTTP/3 rejecting new QUIC from {} (HTTP/2 fallback active)", client_ip);
- Line 235: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: THEMIS_INFO("HTTP/3 new QUIC connection from {}", session_key);
- Line 317: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 6 > array 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: const uint8_t first = data[0];
- Line 317: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 6 > array size 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: //   Bytes 6..6+dcid_len-1: DCID

    if (len < 6) {

        return {};

    }



    const uint8_t first = data[0];

    const bool long_header = (first & 0x80) != 0;



    if (long_header) {

        // Long header format: version (4 bytes) then DCID length (1 byte) then DCID

        uint8_t dcid_len = data[5];
- Line 322: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 6 > array 5
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: uint8_t dcid_len = data[5];
- Line 322: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 6 > array size 5
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: const uint8_t first = data[0];

    const bool long_header = (first & 0x80) != 0;



    if (long_header) {

        // Long header format: version (4 bytes) then DCID length (1 byte) then DCID

        uint8_t dcid_len = data[5];

        if (dcid_len == 0 || dcid_len > 20) {

            return {};

        }

        if (len < static_cast<size_t>(6 + dcid_len)) {

            return {};
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #5367 Resolve PR merge conflicts ... (2026-05-27) | #3291 [network] QUIC/HTTP
- Line 89: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: Http3Handler::~Http3Handler() {
- Line 99: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 156: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!running_.load(std::memory_order_acquire) || !socket_.is_open()) {
- Line 168: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (running_.load(std::memory_order_acquire) && socket_.is_open()) {
- Line 215: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 230: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 254: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!running_.load(std::memory_order_acquire)) {
- Line 263: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (ec || !running_.load(std::memory_order_acquire)) {
- Line 273: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (running_.load(std::memory_order_acquire)) {
- Line 375: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: Http3Session::~Http3Session() {
- Line 391: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 421: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 453: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 473: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 476: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 696: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 697: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 839: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: vec[0].base = (uint8_t*)body_ptr->data();
- Line 840: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: vec[0].len = body_ptr->size();
- Line 59: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: THEMIS_ERROR("{}: unknown exception", context);

    } catch (const std::exception& e) {

        THEMIS_ERROR("{}: {}", context, e.what());

    } catch (...) {

        THEMIS_ERROR("{}: non-standard exception", context);

    }

}
- Line 59: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 136: severity=MEDIUM; category=missing_health_check
  Description: Service initialization without nearby health/status handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void Http3Handler::start() {
- Line 166: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: [this](boost::system::error_code ec, std::size_t bytes_transferred) {

            try {

                onReceive(ec, bytes_transferred);

            } catch (...) {

                logCurrentException("HTTP/3 receive callback failed");

                if (running_.load(std::memory_order_acquire) && socket_.is_open()) {

                    doAccept();
- Line 166: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 269: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

            cleanupInactiveSessions();

        } catch (...) {

            logCurrentException("HTTP/3 cleanup timer error");

        }
- Line 269: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 387: severity=MEDIUM; category=missing_health_check
  Description: Service initialization without nearby health/status handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void Http3Session::start() {
- Line 491: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (!ec) {

            try {

                onTimeout();

            } catch (...) {

                logCurrentException("HTTP/3 idle timeout handler error");

            }

        }
- Line 491: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 527: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (!ec) {

            try {

                onTimeout();

            } catch (...) {

                logCurrentException("HTTP/3 idle timeout handler error");

            }

        }
- Line 527: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 791: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::string> response_headers;
- Line 802: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_map<std::string, std::string>& headers) {
- Line 897: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: self->metrics_.handshake_end_us - self->metrics_.handshake_start_us);

        }

        return 0;

    } catch (...) {

        logCurrentException("HTTP/3 handshakeCompletedCallback failed");

        return NGTCP2_ERR_CALLBACK_FAILURE;

    }
- Line 897: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 929: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



        return 0;

    } catch (...) {

        logCurrentException("HTTP/3 recvStreamDataCallback failed");

        return NGTCP2_ERR_CALLBACK_FAILURE;

    }
- Line 929: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 948: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: nghttp3_conn_add_ack_offset(self->http3_conn_, stream_id, datalen);

        }

        return 0;

    } catch (...) {

        logCurrentException("HTTP/3 ackStreamDataCallback failed");

        return NGTCP2_ERR_CALLBACK_FAILURE;

    }
- Line 948: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 965: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

        self->streams_.erase(stream_id);

        return 0;

    } catch (...) {

        logCurrentException("HTTP/3 streamCloseCallback failed");

        return NGTCP2_ERR_CALLBACK_FAILURE;

    }
- Line 965: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 981: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

        generateConnectionIdCallback(cid);

        return 0;

    } catch (...) {

        logCurrentException("HTTP/3 getNewConnectionIdCallback failed");

        return NGTCP2_ERR_CALLBACK_FAILURE;

    }
- Line 981: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 1001: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return NGTCP2_ERR_CALLBACK_FAILURE;

        }

        return self->feedCryptoData(level, data, datalen);

    } catch (...) {

        logCurrentException("HTTP/3 recvCryptoDataCallback failed");

        return NGTCP2_ERR_CALLBACK_FAILURE;

    }
- Line 1001: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 1120: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto& stream = self->streams_[stream_id];

        stream.body.append(reinterpret_cast<const char*>(data), datalen);

        return 0;

    } catch (...) {

        logCurrentException("HTTP/3 http3RecvDataCallback failed");

        return NGHTTP3_ERR_CALLBACK_FAILURE;

    }
- Line 1120: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 1157: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



        return 0;

    } catch (...) {

        logCurrentException("HTTP/3 http3DecodHeaderCallback failed");

        return NGHTTP3_ERR_CALLBACK_FAILURE;

    }
- Line 1157: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 1177: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: stream.headers_complete = true;

        stream.stream_id = stream_id;

        return 0;

    } catch (...) {

        logCurrentException("HTTP/3 http3EndHeadersCallback failed");

        return NGHTTP3_ERR_CALLBACK_FAILURE;

    }
- Line 1177: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 1194: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: self->processStream(stream_id);

        return 0;

    } catch (...) {

        logCurrentException("HTTP/3 http3EndStreamCallback failed");

        return NGHTTP3_ERR_CALLBACK_FAILURE;

    }
- Line 1194: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### server/rpc/rpc_service_impl.cpp
Total findings: 50

- Line 3314: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto auth_result = auth.authorize(token, required_scope);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #5399 fix(rpc): complete deadline... (2026-05-27) | #3449 [RPC] Implement Bat
- Line 471: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 605: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 642: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: "Request deadline exceeded during delete cascade scan"
- Line 642: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (deadline_exceeded) {

            return createError(

                themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,

                "Request deadline exceeded during delete cascade scan"

            );

        }
- Line 642: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: "Request deadline exceeded during delete cascade scan"
- Line 653: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: " reference this entity. Use cascade=true to delete them."
- Line 653: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: "Referential integrity violation: " +

                    std::to_string(direct_children.size()) + " child entit" +

                    (direct_children.size() == 1 ? "y" : "ies") +

                    " reference this entity. Use cascade=true to delete them."

            );

        }
- Line 653: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: " reference this entity. Use cascade=true to delete them."
- Line 657: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Collect all descendants via BFS for cascade delete
- Line 672: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: "Request deadline exceeded during delete cascade traversal"
- Line 672: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (shouldCheckDeadline(bfs_visited) && isDeadlineExceeded(deadline)) {

                    return createError(

                        themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,

                        "Request deadline exceeded during delete cascade traversal"

                    );

                }
- Line 672: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: "Request deadline exceeded during delete cascade traversal"
- Line 679: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: size_t first_colon  = curr_key.find(':');
- Line 696: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: "Request deadline exceeded during delete cascade scan"
- Line 696: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (deadline_exceeded) {

                    return createError(

                        themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,

                        "Request deadline exceeded during delete cascade scan"

                    );

                }

                for (const auto& gc_key : grandchildren) {
- Line 696: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: "Request deadline exceeded during delete cascade scan"
- Line 714: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: "Request deadline exceeded during delete cascade write"
- Line 714: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (shouldCheckDeadline(deleted_items) && isDeadlineExceeded(deadline)) {

                return createError(

                    themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,

                    "Request deadline exceeded during delete cascade write"

                );

            }

            if (!storage->del(*it)) {
- Line 714: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: "Request deadline exceeded during delete cascade write"
- Line 720: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: "Failed to delete child entity during cascade: " + *it
- Line 720: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (!storage->del(*it)) {

                return createError(

                    themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,

                    "Failed to delete child entity during cascade: " + *it

                );

            }

            ++deleted_count;
- Line 720: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: "Failed to delete child entity during cascade: " + *it
- Line 730: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: "Request deadline exceeded during delete write"
- Line 730: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (isDeadlineExceeded(deadline)) {

            return createError(

                themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT,

                "Request deadline exceeded during delete write"

            );

        }

        if (!storage->del(key)) {
- Line 730: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: "Request deadline exceeded during delete write"
- Line 736: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: "Failed to delete entity from database"
- Line 736: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (!storage->del(key)) {

            return createError(

                themis::plugins::rpc::RPCErrorCode::INTERNAL_ERROR,

                "Failed to delete entity from database"

            );

        }

        ++deleted_count;
- Line 736: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: "Failed to delete entity from database"
- Line 918: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // Set version: Client provides version in entity, or 0 for new entities
- Line 1719: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 2197: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 2315: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 3265: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::min(backoff, remaining));
- Line 3283: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // for backward compatibility. In production, auth should always be configured.
- Line 642: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: "Request deadline exceeded during delete cascade scan"
- Line 672: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: "Request deadline exceeded during delete cascade traversal"
- Line 696: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: "Request deadline exceeded during delete cascade scan"
- Line 1029: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: json ThemisRPCService::handleQuery(const json& params) {
- Line 1081: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: prefix += model + ":";
- Line 1314: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: json ThemisRPCService::handleGeoQuery(const json& params) {
- Line 1380: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto bbox_json = params["bbox"];
- Line 1440: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto center = params["center"];
- Line 1543: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: json ThemisRPCService::handleTimeSeriesQuery(const json& params) {
- Line 2356: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: json ThemisRPCService::handlePaginatedQuery(const json& params) {
- Line 3242: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << " (attempt " << attempt << "/" << max_attempts << "): " << e.what() << "\n";
- Line 3243: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

            std::cerr << "[ThemisRPCService] Retrying method '" << method << "' after exception"

                      << " (attempt " << attempt << "/" << max_attempts << "): " << e.what() << "\n";

        } catch (...) {

            const std::string error_message =

                currentExceptionMessage("Unknown internal error during RPC dispatch");

            if (!retryable_method || attempt == max_attempts) {
- Line 3243: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 3253: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << " (attempt " << attempt << "/" << max_attempts << "): " << error_message << "\n";

### server/lora_api_handler.cpp
Total findings: 39

- Line 342: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto& td = body->at("training_data");
- Line 458: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto& td = body->at("additional_training_data");
- Line 785: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: memory_mb = static_cast<double>(adapter_opt->memory_bytes) / (1024.0 * 1024.0);
- Line 935: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto handle   = inference_engine_->submit(eng_req);
- Line 180: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: {"message", "Model registered successfully"}

        };

        

        if (!architecture.empty()) response_data["architecture"] = architecture;

        if (!description.empty()) response_data["description"] = description;

        

        return createJsonResponse(response_data, http::status::created);
- Line 181: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: };

        

        if (!architecture.empty()) response_data["architecture"] = architecture;

        if (!description.empty()) response_data["description"] = description;

        

        return createJsonResponse(response_data, http::status::created);
- Line 248: severity=HIGH; category=catch_all_swallow
  Description: catch(...) block swallows errors without rethrowing or explicit handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_design_error_rules
  Context: catch(...) { ... }
- Line 259: severity=HIGH; category=catch_all_swallow
  Description: catch(...) block swallows errors without rethrowing or explicit handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_design_error_rules
  Context: catch(...) { ... }
- Line 295: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

    

    try {

        // In production, this would delete from model registry

        

        return createJsonResponse(json::object(), http::status::no_content);
- Line 295: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // In production, this would delete from model registry
- Line 302: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: "Failed to delete model",
- Line 302: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: } catch (const std::exception& e) {

        return createErrorResponse(

            http::status::internal_server_error,

            "Failed to delete model",

            e.what()

        );

    }
- Line 302: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: "Failed to delete model",
- Line 418: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

        

        json response_data = adapter_info->toJSON();

        response_data["status"] = adapter_info->is_loaded ? "ready" : "stored";

        response_data["created_at"] = std::chrono::system_clock::to_time_t(

            adapter_info->metadata.created_at

        );
- Line 419: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: json response_data = adapter_info->toJSON();

        response_data["status"] = adapter_info->is_loaded ? "ready" : "stored";

        response_data["created_at"] = std::chrono::system_clock::to_time_t(

            adapter_info->metadata.created_at

        );
- Line 530: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: "Failed to delete adapter: " + adapter_id
- Line 530: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: return createErrorResponse(

                http::status::not_found,

                "Adapter not found",

                "Failed to delete adapter: " + adapter_id

            );

        }
- Line 530: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: "Failed to delete adapter: " + adapter_id
- Line 539: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: "Failed to delete adapter",
- Line 539: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: } catch (const std::exception& e) {

        return createErrorResponse(

            http::status::internal_server_error,

            "Failed to delete adapter",

            e.what()

        );

    }
- Line 539: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: "Failed to delete adapter",
- Line 575: severity=HIGH; category=catch_all_swallow
  Description: catch(...) block swallows errors without rethrowing or explicit handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_design_error_rules
  Context: catch(...) { ... }
- Line 586: severity=HIGH; category=catch_all_swallow
  Description: catch(...) block swallows errors without rethrowing or explicit handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_design_error_rules
  Context: catch(...) { ... }
- Line 871: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: {"progress",   latest->progress}

        };

        if (!latest->error_message.empty()) {

            response_data["error"] = latest->error_message;

        }



        return createJsonResponse(response_data);
- Line 924: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Build an EnhancedInferenceRequest from the LoRA query parameters.

            llm::InferenceEngineEnhanced::EnhancedInferenceRequest eng_req;

            eng_req.base_request.prompt     = prompt;

            eng_req.base_request.model_id   = model_id.empty() ? "default" : model_id;

            eng_req.base_request.max_tokens = max_tokens;

            eng_req.base_request.temperature = static_cast<float>(temperature);

            if (!adapter_id.empty()) {
- Line 15: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: #include "llm/lora_framework/lora_storage_service.h"
- Line 16: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: #include "llm/lora_framework/lora_training_service.h"
- Line 248: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: limit_end == std::string_view::npos ? std::string_view::npos : limit_end - limit_pos - 6)};

                try {

                    limit = std::stoul(limit_str);

                } catch (...) {}

            }

            

            // Parse offset
- Line 248: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 259: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: offset_end == std::string_view::npos ? std::string_view::npos : offset_end - offset_pos - 7)};

                try {

                    offset = std::stoul(offset_str);

                } catch (...) {}

            }

        }
- Line 259: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 575: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: limit_end == std::string_view::npos ? std::string_view::npos : limit_end - limit_pos - 6)};

                try {

                    limit = std::stoul(limit_str);

                } catch (...) {}

            }

            

            // Parse offset
- Line 575: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 586: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: offset_end == std::string_view::npos ? std::string_view::npos : offset_end - offset_pos - 7)};

                try {

                    offset = std::stoul(offset_str);

                } catch (...) {}

            }

            

            // Parse base_model filter
- Line 586: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 1130: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (parsed.is_object()) {

            return parsed;

        }

    } catch (...) {

        return std::nullopt;

    }
- Line 1130: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 1292: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto created_ns = metadata_json["created_at"].get<uint64_t>();
- Line 1300: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto updated_ns = metadata_json["updated_at"].get<uint64_t>();

### server/mqtt_client_service.cpp
Total findings: 39

- Line 257: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: asio::write(*asio_->ssl_stream, asio::buffer(pkt), ec);
- Line 266: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: asio::write(asio_->socket, asio::buffer(pkt), ec);
- Line 286: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: if (io_thread_.joinable()) io_thread_.join();
- Line 434: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: asio::write(*asio_->ssl_stream, asio::buffer(pkt), we);
- Line 438: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: asio::write(asio_->socket, asio::buffer(pkt), we);
- Line 57: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (size_t i = 0; i < std::min(len, size_t{4}); ++i) {
- Line 403: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // timeout handler so neither captures a dangling stack reference.

    auto connected = std::make_shared<std::atomic<bool>>(false);



    asio_->socket.async_connect(

        *results.begin(),

        [this, connected](boost::system::error_code ec2) {

            asio_->connect_timer.cancel(); // cancel connection timeout
- Line 551: severity=HIGH; category=catch_all_swallow
  Description: catch(...) block swallows errors without rethrowing or explicit handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_design_error_rules
  Context: catch(...) { ... }
- Line 567: severity=HIGH; category=catch_all_swallow
  Description: catch(...) block swallows errors without rethrowing or explicit handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_design_error_rules
  Context: catch(...) { ... }
- Line 839: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Extract collection name from metadata (key "collection") or use "default"

    std::string collection = "default";

    if (event.metadata.contains("collection") &&

        event.metadata["collection"].is_string()) {

        collection = event.metadata["collection"].get<std::string>();

    }
- Line 840: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::string collection = "default";

    if (event.metadata.contains("collection") &&

        event.metadata["collection"].is_string()) {

        collection = event.metadata["collection"].get<std::string>();

    }



    return topic_prefix_ + collection + "/" + type_str;
- Line 2: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: * ThemisDB | File: mqtt_client_service.cpp | Version: 0.0.12 | Last Modified: 2026-05-31 12:17:24
- Line 10: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: #include "server/mqtt_client_service.h"
- Line 151: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: vheader.push_back(qos & 0x03);
- Line 236: severity=MEDIUM; category=missing_health_check
  Description: Service initialization without nearby health/status handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void MqttClientService::start() {
- Line 237: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool expected = false;
- Line 245: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool expected = true;
- Line 260: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: asio_->ssl_stream->lowest_layer().close(ec);
- Line 268: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: asio_->socket.close(ec);
- Line 274: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: asio_->ssl_stream->lowest_layer().close(ec);
- Line 280: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: asio_->socket.close(ec);
- Line 388: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: asio_->socket.close(ec);
- Line 551: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: h = handler_;

    }

    if (h) {

        try { h->onConnected(cid); } catch (...) {}

    }



    doWrite(); // Flush any queued publishes
- Line 551: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { h->onConnected(cid); } catch (...) {}
- Line 567: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: h = handler_;

    }

    if (h) {

        try { h->onMessage(topic, payload, qos); } catch (...) {}

    }

}
- Line 567: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { h->onMessage(topic, payload, qos); } catch (...) {}
- Line 679: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: asio_->ssl_stream->lowest_layer().close(ec);
- Line 685: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: asio_->socket.close(ec);
- Line 695: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: h = handler_;

        }

        if (h) {

            try { h->onDisconnected(reason); } catch (...) {}

        }

    }
- Line 695: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { h->onDisconnected(reason); } catch (...) {}
- Line 714: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

        asio_->ssl_ctx = std::make_unique<boost::asio::ssl::context>(

            boost::asio::ssl::context::tlsv12_client);

    } catch (...) {

        scheduleReconnect();

        return;

    }
- Line 714: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 775: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: SSL_set_tlsext_host_name(asio_->ssl_stream->native_handle(),
- Line 786: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: asio_->ssl_stream->lowest_layer().close(ce);
- Line 801: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: , topic_prefix_(service.getConfig().cdc_topic_prefix)
- Line 802: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: , qos_(service.getConfig().cdc_qos) {}
- Line 804: severity=MEDIUM; category=missing_health_check
  Description: Service initialization without nearby health/status handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool MqttCDCTransport::start() {
- Line 819: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::string    payload = j.dump();

        std::string    topic   = topicForEvent(event);

        return service_.publish(topic, payload, qos_, false);

    } catch (...) {

        return false;

    }

}
- Line 819: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### server/entity_api_handler.cpp
Total findings: 33

- Line 155: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto authz = auth_->authorize(*token_opt, scope);
- Line 587: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: auto write_result = strategy->write(
- Line 880: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator pos may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto pos = key.find(':');
- Line 1136: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ct_it->value().find("application/x-ndjson") == std::string_view::npos) {
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #2800 [cdc] Change event enrichme... (2026-03-12) | #2726 [api] Batch operati
- Line 726: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Geo MVP: Remove from spatial index before delete (best-effort)
- Line 737: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: THEMIS_WARN("Geo index delete hook failed for {}:{}: {}", table, pk, e.what());
- Line 737: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

            } catch (const std::exception& e) {

                // Log but don't fail the request - geo index is best-effort

                THEMIS_WARN("Geo index delete hook failed for {}:{}: {}", table, pk, e.what());

            }

        }
- Line 737: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_WARN("Geo index delete hook failed for {}:{}: {}", table, pk, e.what());
- Line 754: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: "Index/Storage delete failed: " + st.message, req);
- Line 754: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (!st.ok) {

            span.setStatus(false, st.message);

            return makeErrorResponse(http::status::internal_server_error,

                "Index/Storage delete failed: " + st.message, req);

        }



        // Record CDC event if changefeed enabled
- Line 754: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: "Index/Storage delete failed: " + st.message, req);
- Line 976: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: } else { // delete
- Line 987: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: THEMIS_WARN("Geo index delete hook failed for {}:{}: {}", vop.table, vop.pk, e.what());
- Line 987: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: vop.table, vop.pk, *old_blob);

                            }

                        } catch (const std::exception& e) {

                            THEMIS_WARN("Geo index delete hook failed for {}:{}: {}", vop.table, vop.pk, e.what());

                        }

                    }
- Line 987: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_WARN("Geo index delete hook failed for {}:{}: {}", vop.table, vop.pk, e.what());
- Line 108: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];
- Line 142: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];
- Line 247: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto coll = schema["collections"][table];
- Line 252: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (auto& f : coll["encryption"]["fields"]) if (f.is_string()) fields.push_back(f.get<std::string>(
- Line 267: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto enc_meta_str = entity_json[f + "_encrypted"].get<std::string>();
- Line 274: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (context_type == "group" && pki && entity_json.contains(f + "_group")) {

                                    // Group context (MVP: first group / single string)

                                    std::string group_name;

                                    try { group_name = entity_json[f + "_group"].get<std::string>(); } catch (...) {

                                        THEMIS_WARN("Group name cast failed for field {}: skipping group context", f);

                                        group_name.clear();

                                    }
- Line 274: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { group_name = entity_json[f + "_group"].get<std::string>(); } catch (...) {
- Line 394: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto coll = schema["collections"][table];
- Line 407: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (f.is_string()) fields.push_back(f.get<std::string>());
- Line 850: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: errors.push_back({
- Line 851: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: errors.push_back({
- Line 859: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: errors.push_back({
- Line 871: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: errors.push_back({
- Line 882: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: errors.push_back({
- Line 894: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: errors.push_back({
- Line 1174: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: errors.push_back({
- Line 1209: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: errors.push_back({

### server/changefeed_api_handler.cpp
Total findings: 27

- Line 1025: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto auth_result = auth_->authorize(*token, required_scope);
- Line 1120: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto auth_result = auth_->authorize(*token, required_scope);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #2846 [cdc] GDPR-aware PII field ... (2026-03-12) | #2791 feat(cache): Adapti
- Line 251: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 451: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Activation (legacy): `THEMIS_ENABLE_SSE` + `keep_alive=true` + `sse_manager_ != nullptr`
- Line 538: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 555: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 314: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: v = 60;

                    }

                    max_seconds = v;

                } catch (...) {

                    THEMIS_DEBUG("changefeed: ignoring invalid max_seconds query param");

                }

            }
- Line 314: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 330: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (v < 100) v = 100; // minimum 100ms

                    if (v > 60000) v = 60000;

                    heartbeat_ms_override = v;

                } catch (...) {

                    THEMIS_DEBUG("changefeed: ignoring invalid heartbeat_ms query param");

                }

            }
- Line 330: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 350: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: v = 120000;

                    }

                    retry_ms = v;

                } catch (...) {

                    THEMIS_DEBUG("changefeed: ignoring invalid retry_ms query param");

                }

            }
- Line 350: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 370: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: v = 1000;

                    }

                    max_events_per_poll = static_cast<size_t>(v);

                } catch (...) {

                    THEMIS_DEBUG("changefeed: ignoring invalid max_events query param");

                }

            }
- Line 370: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 406: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (v >= 0) {

                        ack_timeout_override = std::chrono::milliseconds(v);

                    }

                } catch (...) {

                    THEMIS_DEBUG("changefeed: ignoring invalid ack_timeout_ms query param");

                }

            }
- Line 406: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 420: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

                    uint64_t last_id = std::stoull(std::string(h.value()));

                    if (from_seq == 0) from_seq = last_id;

                } catch (...) {

                    THEMIS_DEBUG("changefeed: ignoring invalid Last-Event-ID header value");

                    break;

                }
- Line 420: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 849: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto v = body["max_age_hours"].get<uint32_t>();
- Line 857: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto v = body["max_event_count"].get<uint64_t>();
- Line 865: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto v = body["max_size_bytes"].get<uint64_t>();
- Line 873: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto v = body["cleanup_interval_minutes"].get<uint32_t>();
- Line 996: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];
- Line 1055: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::string> headers_map;
- Line 1091: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];
- Line 1140: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::string> headers_map;

### server/mqtt_session.cpp
Total findings: 27

- Line 487: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 5 > array 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: const uint8_t packetType = static_cast<uint8_t>(buffer_[0]);
- Line 487: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 5 > array size 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: doRead(); // Need more data

                return;

            }

            

            // Parse fixed header

            const uint8_t packetType = static_cast<uint8_t>(buffer_[0]);

            const uint8_t messageType = static_cast<uint8_t>((packetType >> 4) & 0x0Fu);

            

            // Decode remaining length (variable length encoding)

            size_t multiplier = 1;

            size_t remainingLength = 0;
- Line 787: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: for (const auto& [filter, sessions] : subscriptions_) {

        if (topicMatches(filter, topic)) {

            for (auto& sessionWeak : sessions) {

                if (auto session = sessionWeak.lock()) {

                    session->sendPublish(topic, payload, qos, retain);

                }

            }
- Line 787: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: if (auto session = sessionWeak.lock()) {
- Line 801: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Round-robin delivery to one session in the group (thread-safe)

                if (!sessions.empty()) {

                    size_t idx = sharedSubscriptionRoundRobin_.fetch_add(1, std::memory_order_relaxed) % sessions.size();

                    if (auto session = sessions[idx].lock()) {

                        session->sendPublish(topic, payload, qos, retain);

                    }

                }
- Line 801: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: if (auto session = sessions[idx].lock()) {
- Line 821: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::unordered_set<MqttSession*> seen;

    for (auto& [topic, session_vec] : subscriptions_) {

        for (auto& weak_session : session_vec) {

            auto session = weak_session.lock();

            if (!session) continue;

            if (!seen.insert(session.get()).second) continue;  // already counted
- Line 821: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: auto session = weak_session.lock();
- Line 534: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['                            // Read packet ID (2 bytes)', '                            packetId = static_cast<uint16_t>(', '                                (static_cast<uint16_t>(static_cast<uint8_t>(buffer_[payloadOffset])) << 8) |', '                                static_cast<uint16_t>(static_cast<uint8_t>(buffer_[payloadOffset + 1]))', '                            );']
- Line 42: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

        metrics_.disconnectCount++;

        stop();

    } catch (...) {

        // Destructors must not throw.

    }

}
- Line 42: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 52: severity=MEDIUM; category=missing_health_check
  Description: Service initialization without nearby health/status handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void MqttSession::start() {
- Line 66: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: wsStream_->close(websocket::close_code::normal, ec);
- Line 69: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: socket_.close(ec);
- Line 314: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: packet.push_back(static_cast<uint8_t>(0x20u)); // CONNACK packet type
- Line 318: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: packet.push_back(static_cast<uint8_t>(3u));    // Remaining length
- Line 319: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: packet.push_back(static_cast<uint8_t>(sessionPresent ? 1u : 0u));
- Line 321: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: packet.push_back(static_cast<uint8_t>(0u));    // Properties length
- Line 323: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: packet.push_back(static_cast<uint8_t>(2u));    // Remaining length
- Line 324: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: packet.push_back(static_cast<uint8_t>(sessionPresent ? 1u : 0u));
- Line 335: severity=MEDIUM; category=shift_overflow
  Description: Shift operation detected (verify shift count < bitwidth)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    // Build MQTT PUBLISH packet', '    std::vector<uint8_t> packet;', '    uint8_t flags = static_cast<uint8_t>(qos << 1);', '    if (retain) {', '        flags = static_cast<uint8_t>(flags | 0x01u);']
- Line 384: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: packet.push_back(static_cast<uint8_t>((topicLen >> 8) & 0xFFu));
- Line 385: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: packet.push_back(static_cast<uint8_t>(topicLen & 0xFFu));
- Line 392: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: packet.push_back(static_cast<uint8_t>((packetId >> 8) & 0xFFu));
- Line 393: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: packet.push_back(static_cast<uint8_t>(packetId & 0xFFu));
- Line 801: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (auto session = sessions[idx].lock()) {
- Line 818: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<MqttSession*> seen;

### server/rate_limiter_v2.cpp
Total findings: 24

- Line 300: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: size_t slot_idx;

    {

        std::unique_lock<std::mutex> lk(redis_pool_.pool_mu);

        redis_pool_.pool_cv.wait(lk, [this]() {

            return !redis_pool_.available.empty();

        });

        slot_idx = redis_pool_.available.front();
- Line 300: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: redis_pool_.pool_cv.wait(lk, [this]() {
- Line 453: severity=CRITICAL; category=db_connection_leak
  Description: Database connection from current never released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: size_t current = tokens.load(std::memory_order_acquire);
- Line 498: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: THEMIS_WARN("PerClientRateLimiter: Max clients ({}) reached, rejecting new client: {}",
- Line 120: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (config_.backend == Backend::REDIS && redis_healthy_.load(std::memory_order_acquire)) {
- Line 186: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: redis_healthy_.load(std::memory_order_acquire);
- Line 193: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto& [prio, bucket] : buckets_) {
- Line 194: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(bucket->mutex);
- Line 223: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (slot.ctx && !slot.ctx->err) return true;  // Already healthy.
- Line 228: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (!slot.ctx || slot.ctx->err) {
- Line 231: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: slot.ctx ? slot.ctx->errstr : "null context");
- Line 307: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    }', '', '    auto& slot = redis_pool_.slots[slot_idx];', '    std::string key = redisKey(config_.bucket_id, prio);', '    int result = redisExecEvalsha(slot, key, capacity, refill_rate, consume_count);']
- Line 321: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (slot.ctx && !slot.ctx->err && !config_.redis.auth.empty()) {
- Line 331: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (slot.ctx && !slot.ctx->err) {
- Line 372: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (!slot.ctx || slot.ctx->err || !slot.script_loaded) return -1;
- Line 389: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (!reply || slot.ctx->err) {
- Line 391: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: slot.ctx ? slot.ctx->errstr : "null context");
- Line 420: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (redis_healthy_.load(std::memory_order_acquire)) return;
- Line 445: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 446: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 458: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: std::memory_order_acquire)) {
- Line 498: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 503: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 522: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: lock.unlock(); // Unlock before trying to acquire tokens

### server/shard_repair_api_handler.cpp
Total findings: 24

- Line 158: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: << "function fmtTime(v){return v?new Date(v).toLocaleString():'-';}"
- Line 158: severity=CRITICAL; category=new_without_raii
  Description: Raw new() without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: << "const jobs=document.getElementById('jobs');"

         << "const raw=document.getElementById('raw');"

         << "const flash=document.getElementById('flash');"

         << "function fmtTime(v){return v?new Date(v).toLocaleString():'-';}"

         << "function badge(status){return `<span class=\"badge ${status}\">${status}</span>`;}"

         << "function setFlash(msg){flash.textContent=msg;setTimeout(()=>{if(flash.textContent===msg)flash.textContent='';},4000);}"

         << "async function load(){const res=await fetch('/v1/admin/repair/health');const data=await res.json();raw.textContent=JSON.stringify(data,null,2);summary.innerHTML='';const cards=[['Status',data.status],['Engine',data.engine_running?'running':'stopped'],['Scans',data.metrics?.total_scans ?? 0],['Repairs OK',data.metrics?.total_repairs_successful ?? 0],['Repairs Failed',data.metrics?.total_repairs_failed ?? 0],['Active Jobs',(data.active_jobs||[]).length]];cards.forEach(([label,val])=>{const el=document.createElement('div');el.className='card';el.innerHTML=`<div>${label}</div><div class=\"metric\">${val}</div>`;summary.appendChild(el);});shards.innerHTML=(data.shards||[]).map(s=>`<tr><td>${s.shard_id||'-'}</td><td>${badge(s.status||'healthy')}</td><td>${s.documents_scanned}</td><td>${s.documents_healthy}</td><td>${s.documents_degraded}</td><td>${s.documents_unrecoverable}</td><td>${s.last_error||''}</td></tr>`).join('');jobs.innerHTML=(data.active_jobs||[]).map(j=>`<tr><td>${j.job_id}</td><td>${j.shard_id||'-'}</td><td>${j.document_id||'-'}</td><td>${j.is_full_scan?'yes':'no'}</td><td>${fmtTime(j.submitted_at_unix_ms)}</td><td>${j.completed?'yes':'no'}</td><td>${j.success?'yes':'no'}</td></tr>`).join('');}"
- Line 158: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: << "function fmtTime(v){return v?new Date(v).toLocaleString():'-';}"
- Line 162: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: << "async function post(url,body){const res=await fetch(url,{method:'POST',headers:{'Content-Type':'
- Line 162: severity=CRITICAL; category=new_without_raii
  Description: Raw new() without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: << "function badge(status){return `<span class=\"badge ${status}\">${status}</span>`;}"

         << "function setFlash(msg){flash.textContent=msg;setTimeout(()=>{if(flash.textContent===msg)flash.textContent='';},4000);}"

         << "async function load(){const res=await fetch('/v1/admin/repair/health');const data=await res.json();raw.textContent=JSON.stringify(data,null,2);summary.innerHTML='';const cards=[['Status',data.status],['Engine',data.engine_running?'running':'stopped'],['Scans',data.metrics?.total_scans ?? 0],['Repairs OK',data.metrics?.total_repairs_successful ?? 0],['Repairs Failed',data.metrics?.total_repairs_failed ?? 0],['Active Jobs',(data.active_jobs||[]).length]];cards.forEach(([label,val])=>{const el=document.createElement('div');el.className='card';el.innerHTML=`<div>${label}</div><div class=\"metric\">${val}</div>`;summary.appendChild(el);});shards.innerHTML=(data.shards||[]).map(s=>`<tr><td>${s.shard_id||'-'}</td><td>${badge(s.status||'healthy')}</td><td>${s.documents_scanned}</td><td>${s.documents_healthy}</td><td>${s.documents_degraded}</td><td>${s.documents_unrecoverable}</td><td>${s.last_error||''}</td></tr>`).join('');jobs.innerHTML=(data.active_jobs||[]).map(j=>`<tr><td>${j.job_id}</td><td>${j.shard_id||'-'}</td><td>${j.document_id||'-'}</td><td>${j.is_full_scan?'yes':'no'}</td><td>${fmtTime(j.submitted_at_unix_ms)}</td><td>${j.completed?'yes':'no'}</td><td>${j.success?'yes':'no'}</td></tr>`).join('');}"

         << "async function post(url,body){const res=await fetch(url,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body||{})});const data=await res.json();if(!res.ok){throw new Error(data.message||data.error||'request failed');}return data;}"

         << "document.getElementById('refreshBtn').onclick=()=>load().catch(e=>setFlash(e.message));"

         << "document.getElementById('scanBtn').onclick=async()=>{const r=await post('/v1/admin/repair/scan',{});setFlash(`Full scan queued: ${r.job_id}`);load();};"

         << "document.getElementById('repairBtn').onclick=async()=>{const shardId=document.getElementById('shardId').value.trim();const r=await post('/v1/admin/repair',{shard_id:shardId});setFlash(`Repair queued: ${r.job_id}`);load();};"
- Line 162: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: << "async function post(url,body){const res=await fetch(url,{method:'POST',headers:{'Content-Type':'
- Line 207: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto ar = auth.authorize(*token, required_scope);
- Line 115: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['         << "<html lang=\\"en\\">\\n"', '         << "<head><meta charset=\\"utf-8\\">\\n"', '         << "<meta name=\\"viewport\\" content=\\"width=device-width,initial-scale=1\\">\\n"', '         << "<title>Themis Repair Dashboard</title>\\n"', '         << "<style>"']
- Line 158: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 345: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: const std::string job_id = extractJobId(std::string(req.target()));
- Line 116: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "<title>Themis Repair Dashboard</title>\n"
- Line 137: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "</style></head>\n"
- Line 140: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "<p class=\"sub\">Shard health, repair jobs, and anti-entropy triggers.</p>"
- Line 141: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "<div id=\"flash\"></div>"
- Line 142: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "<section class=\"grid\" id=\"summary\"></section>"
- Line 144: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "<button id=\"scanBtn\">Start Full Scan</button>"
- Line 146: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "<button id=\"repairBtn\" class=\"secondary\">Queue Shard Repair</button>"
- Line 147: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "<button id=\"refreshBtn\">Refresh</button>"
- Line 149: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "<h2>Shard Health</h2><table><thead><tr><th>Shard</th><th>Status</th><th>Scanned</th><th>Healthy<
- Line 150: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "<h2>Active Jobs</h2><table><thead><tr><th>Job</th><th>Shard</th><th>Document</th><th>Full Scan</
- Line 151: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "<h2>Raw JSON</h2><pre id=\"raw\">loading...</pre>"
- Line 159: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "function badge(status){return `<span class=\"badge ${status}\">${status}</span>`;}"
- Line 161: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "async function load(){const res=await fetch('/v1/admin/repair/health');const data=await res.json
- Line 193: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];

### server/async_job_api_handler.cpp
Total findings: 21

- Line 565: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto status_result = registry_->requestCancel(job_id);
- Line 576: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return makeJsonResponse(http::status::conflict,
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4285 feat(server): Versioned API... (2026-03-17) | #2763 [api] Async job API
- Line 138: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 142: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto it = jobs_.begin(); it != jobs_.end(); ) {
- Line 144: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> rlock(rec.mu);
- Line 156: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::shared_ptr<AsyncJobRecord> AsyncJobRegistry::get(const std::string& id) const {
- Line 175: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = jobs_.find(id);
- Line 199: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = jobs_.find(id);
- Line 224: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 226: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto it = jobs_.begin(); it != jobs_.end(); ) {
- Line 228: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> rlock(rec.mu);
- Line 273: severity=HIGH; category=posix_only_api
  Description: POSIX-only API getpid( without platform guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::to_string(static_cast<long long>(::getpid())) +
- Line 293: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: f.wait_for(std::chrono::seconds(2));
- Line 358: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (job->cancel_requested.load(std::memory_order_acquire)) {
- Line 370: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (job->cancel_requested.load(std::memory_order_acquire)) {
- Line 428: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: f.wait_for(std::chrono::seconds(0)) ==
- Line 527: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: const std::string target(req.target());
- Line 553: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: const std::string target(req.target());
- Line 56: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool isValidAsyncQuery(std::string_view query) {
- Line 446: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_hdr = req[http::field::authorization];

### server/replication_topology_api_handler.cpp
Total findings: 21

- Line 293: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: << "if(!t.ok)throw new Error('topology '+t.status);\n"
- Line 293: severity=CRITICAL; category=new_without_raii
  Description: Raw new() without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: << "async function load(){try{\n"

         << "const t=await fetch(API_BASE+'/api/v1/replication/topology');\n"

         << "const h=await fetch(API_BASE+'/api/v1/replication/health');\n"

         << "if(!t.ok)throw new Error('topology '+t.status);\n"

         << "if(!h.ok)throw new Error('health '+h.status);\n"

         << "document.getElementById('topology').textContent=JSON.stringify(await t.json(),null,2);\n"

         << "document.getElementById('health').textContent=JSON.stringify(await h.json(),null,2);\n"
- Line 293: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: << "if(!t.ok)throw new Error('topology '+t.status);\n"
- Line 294: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: << "if(!h.ok)throw new Error('health '+h.status);\n"
- Line 294: severity=CRITICAL; category=new_without_raii
  Description: Raw new() without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: << "const t=await fetch(API_BASE+'/api/v1/replication/topology');\n"

         << "const h=await fetch(API_BASE+'/api/v1/replication/health');\n"

         << "if(!t.ok)throw new Error('topology '+t.status);\n"

         << "if(!h.ok)throw new Error('health '+h.status);\n"

         << "document.getElementById('topology').textContent=JSON.stringify(await t.json(),null,2);\n"

         << "document.getElementById('health').textContent=JSON.stringify(await h.json(),null,2);\n"

         << "errorEl.style.display='none';\n"
- Line 294: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: << "if(!h.ok)throw new Error('health '+h.status);\n"
- Line 97: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: http::response<http::string_body> ReplicationTopologyApiHandler::handleTopologyGet(
- Line 172: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: http::response<http::string_body> ReplicationTopologyApiHandler::handleHealthGet(
- Line 225: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: http::response<http::string_body> ReplicationTopologyApiHandler::handleUiGet(
- Line 230: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: const std::string target{req.target()};
- Line 279: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['         << "<html lang=\\"en\\">\\n"', '         << "<head><meta charset=\\"utf-8\\">\\n"', '         << "<meta name=\\"viewport\\" content=\\"width=device-width,initial-scale=1\\">\\n"', '         << "<title>Themis Replication Topology</title>\\n"', '         << "<style>body{font-family:system-ui,sans-serif;margin:16px}"']
- Line 293: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 294: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 280: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "<title>Themis Replication Topology</title>\n"
- Line 284: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "#error{color:#b91c1c;margin:8px 0;display:none}</style></head>\n"
- Line 285: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "<body><h1>Replication Topology</h1><div id=\"error\"></div>\n"
- Line 286: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "<h2>Topology</h2><pre id=\"topology\">loading...</pre>\n"
- Line 287: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "<h2>Health</h2><pre id=\"health\">loading...</pre>\n"
- Line 291: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "const t=await fetch(API_BASE+'/api/v1/replication/topology');\n"
- Line 292: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "const h=await fetch(API_BASE+'/api/v1/replication/health');\n"
- Line 299: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "load();setInterval(load,5000);</script></body></html>\n";

### server/http2_session.cpp
Total findings: 20

- Line 152: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 161: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 248: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: const uint32_t timeout_ms = server_->hot_request_timeout_ms_.load(std::memory_order_acquire);
- Line 278: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: const uint32_t timeout_ms = server_->hot_request_timeout_ms_.load(std::memory_order_acquire);
- Line 599: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(push_mutex_);
- Line 29: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: static const unsigned char alpn_proto_list[] = "\x02h2\x08http/1.1";
- Line 113: severity=MEDIUM; category=missing_health_check
  Description: Service initialization without nearby health/status handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void Http2Session::start() {
- Line 265: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: self->stream_.lowest_layer().close(close_ec);
- Line 295: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: self->stream_.lowest_layer().close(close_ec);
- Line 531: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::string> response_headers;
- Line 548: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_map<std::string, std::string>& headers) {
- Line 553: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: nva.push_back({
- Line 563: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: nva.push_back({
- Line 605: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: nva.push_back({
- Line 613: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: nva.push_back({
- Line 621: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: nva.push_back({
- Line 629: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: nva.push_back({
- Line 653: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: response_nva.push_back({
- Line 668: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: response_nva.push_back({
- Line 679: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: response_nva.push_back({

### server/rope_api_handler.cpp
Total findings: 20

- Line 193: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto config_opt = vector_index_->getRotaryEmbeddingConfig();
- Line 810: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto config_opt = vector_index_->getRotaryEmbeddingConfig();
- Line 891: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // auth_->authorize(); deny with HTTP 403 when the scope is not granted.
- Line 904: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto ar = auth_->authorize(*token, permission);
- Line 69: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto span = Tracer::startSpan("handleRopeConfigPost");

    span.setAttribute("http.method", "POST");

    span.setAttribute("http.path", std::string(req.target()));

    

    try {

        // Extract index_name from path
- Line 172: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto span = Tracer::startSpan("handleRopeConfigGet");

    span.setAttribute("http.method", "GET");

    span.setAttribute("http.path", std::string(req.target()));

    

    try {

        // Extract index_name from path
- Line 236: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto span = Tracer::startSpan("handleRopeConfigDelete");

    span.setAttribute("http.method", "DELETE");

    span.setAttribute("http.path", std::string(req.target()));

    

    try {

        // Extract index_name from path
- Line 272: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: THEMIS_ERROR("RoPE config delete error: {}", e.what());
- Line 272: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: } catch (const std::exception& e) {

        span.setStatus(false, e.what());

        THEMIS_ERROR("RoPE config delete error: {}", e.what());

        return makeErrorResponse(http::status::internal_server_error, e.what(), req);

    }

}
- Line 272: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_ERROR("RoPE config delete error: {}", e.what());
- Line 289: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto span = Tracer::startSpan("handleRopeAddPost");

    span.setAttribute("http.method", "POST");

    span.setAttribute("http.path", std::string(req.target()));

    

    try {

        // Extract index_name from path
- Line 414: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto span = Tracer::startSpan("handleRopeAddRelationalPost");

    span.setAttribute("http.method", "POST");

    span.setAttribute("http.path", std::string(req.target()));

    

    try {

        // Extract index_name from path
- Line 538: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto span = Tracer::startSpan("handleRopeSearchPost");

    span.setAttribute("http.method", "POST");

    span.setAttribute("http.path", std::string(req.target()));

    

    try {

        // Extract index_name from path
- Line 652: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto span = Tracer::startSpan("handleRopeBatchAddPost");

    span.setAttribute("http.method", "POST");

    span.setAttribute("http.path", std::string(req.target()));

    

    try {

        // Extract index_name from path
- Line 790: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto span = Tracer::startSpan("handleRopeStatsGet");

    span.setAttribute("http.method", "GET");

    span.setAttribute("http.path", std::string(req.target()));

    

    try {

        // Extract index_name from path
- Line 366: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: vec.push_back(v.get<float>());
- Line 490: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: vec.push_back(v.get<float>());
- Line 581: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: query_vector.push_back(val.get<float>());
- Line 732: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: vec.push_back(v.get<float>());
- Line 892: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];

### server/vector_api_handler.cpp
Total findings: 18

- Line 762: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Extract Bearer token and use auth_->authorize() to check the required
- Line 777: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto ar = auth_->authorize(*token, permission);
- Line 205: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Legacy Format
- Line 123: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: queryVector.push_back(val.get<float>());
- Line 170: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: "Field 'cursor' exceeds maximum allowed length", req);

                }

                offset = static_cast<size_t>(std::stoull(cur));

            } catch (...) {

                offset = 0;

            }

        }
- Line 170: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 302: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto coll = schema_json["collections"][object_name];
- Line 304: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto ecfg = coll["encryption"];
- Line 307: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& f : ecfg["fields"]) if (f.is_string()) vector_enc_fields.push_back(f.get<std::strin
- Line 312: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto itf = coll["fields"].begin(); itf != coll["fields"].end(); ++itf) {
- Line 315: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: vector_enc_fields.push_back(itf.key());
- Line 317: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (itf.value().is_object() && itf.value().value("encrypt", false)) {

                                        vector_enc_fields.push_back(itf.key());

                                    }

                                } catch (...) { /* ignore */ }

                            }

                            vector_enc_enabled = !vector_enc_fields.empty();

                        }
- Line 317: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) { /* ignore */ }
- Line 324: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

                }

            }

        } catch (...) {

            vector_enc_enabled = false; // fail-safe

        }
- Line 324: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 356: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto fit = it["fields"].begin(); fit != it["fields"].end(); ++fit) {
- Line 765: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];
- Line 790: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];

### server/voice_api_handler.cpp
Total findings: 18

- Line 1058: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: metadata.meeting_id = body->value("meeting_id", "");
- Line 1434: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto macros = voice_assistant_->macroManager().listMacros("", tag_filter);
- Line 88: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 1987: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: host_lower == "::1" ||

        host_lower.find("[::1]") != std::string::npos ||

        host_lower.find("::ffff:127.") != std::string::npos) {

        throw std::invalid_argument("Access to localhost is not allowed");

    }

    

    // Block cloud metadata endpoints (common in AWS, GCP, Azure)
- Line 2022: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (response_future.wait_for(std::chrono::seconds(70)) == std::future_status::timeout) {
- Line 2023: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Wait with timeout (70s = 10s connect + 60s request + 10s buffer)

    if (response_future.wait_for(std::chrono::seconds(70)) == std::future_status::timeout) {

        throw std::runtime_error("Audio download timed out");

    }

    

    auto response = response_future.get();
- Line 2030: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Check if download was successful

    if (!response.isSuccess()) {

        throw std::runtime_error(

            "Failed to download audio: HTTP " + std::to_string(response.status_code)

        );

    }
- Line 74: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

                octets[i] = octet;

            }

        } catch (...) {

            // std::stoi can throw invalid_argument or out_of_range

            return false;

        }
- Line 74: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 1210: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto it = step_json["parameters"].begin(); it != step_json["parameters"].end(); ++it) {
- Line 1234: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto it = j["parameters"].begin(); it != j["parameters"].end(); ++it) {
- Line 1327: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back(parseStep(sj));
- Line 1490: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back(parseStep(sj));
- Line 1785: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];
- Line 1852: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: ) {

    try {

        return json::parse(req.body());

    } catch (...) {

        return std::nullopt;

    }

}
- Line 1852: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 1969: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: utils::URLComponents components;

    try {

        components = utils::parseURL(url);

    } catch (...) {

        throw std::invalid_argument("Invalid URL format");

    }
- Line 1969: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### server/api_gateway.cpp
Total findings: 16

- Line 48: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator first may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto first = s.find_first_not_of(" \t");
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4146 feat(server): API Versionin... (2026-03-13) | #2991 feat(api): Integrat
- Line 255: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: federated_queries_++;

    

    if (!config_.enable_query_federation) {

        throw Error(static_cast<int>(ErrorCode::FeatureDisabled), 

                   "Query federation is not enabled");

    }
- Line 260: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

    

    if (!shard_router_) {

        throw Error(static_cast<int>(ErrorCode::ConfigurationError), 

                   "Shard router not configured for query federation");

    }

    auto& shard_router = *shard_router_;
- Line 521: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (ctx && !ctx->user_id.empty()) {
- Line 522: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: client_id = ctx->user_id;  // Use JWT subject as client ID
- Line 588: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 823: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Priority 2: Accept-Version header (legacy)
- Line 898: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Check if endpoint is deprecated
- Line 909: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Add API-Deprecated header (issue-specified format: "v1.0 (remove YYYY-MM-DD)")
- Line 133: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];
- Line 512: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];
- Line 558: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];
- Line 776: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::string> labels = {
- Line 946: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: R"(^/v(\d+(?:\.\d+){0,2})(?=/|$))"
- Line 964: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: R"(^/v\d+(?:\.\d+){0,2}(?=/|$))"

### server/auth_middleware.cpp
Total findings: 16

- Line 183: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: AuthMiddleware::AuthResult AuthMiddleware::authorize(std::string_view token, std::string_view required_scope) const {
- Line 613: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authenticate" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto claims = mtls_auth.authenticate(std::string(cert_pem));
- Line 170: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = role_scope_map_.find(role);
- Line 200: severity=HIGH; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // inputs (which would require zero-padding and may confuse static
- Line 307: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::find(it->second.begin(), it->second.end(), required_scope) != it->second.end()) {
- Line 317: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::find(it->second.begin(), it->second.end(), required_scope) != it->second.end()) {
- Line 541: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::find(it->second.begin(), it->second.end(), required_scope) != it->second.end()) {
- Line 159: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::vector<std::string>> mapping)
- Line 213: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!scopes_list.empty()) scopes_list += ",";
- Line 342: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> granted_scopes(claims.scopes.begin(),
- Line 414: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto claims = jwt_validator_->parseAndValidate(std::string(token));

            metrics_.jwt_validation_success_total++;

            return AuthResult::OK(claims.sub, claims.tenant_id, claims.groups);

        } catch (...) {

            metrics_.jwt_validation_failed_total++;

            // GAP-013: Log JWT validation failures at WARN for auditability (CWE-778).

            // Previously logged at DEBUG, which means auth failures were invisible
- Line 414: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 528: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i > 0) roles_str += ", ";
- Line 529: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i > 0) roles_str += ", ";
- Line 698: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::vector<std::string>> mapping;
- Line 704: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: scopes.push_back(s.as<std::string>());

### server/distributed_gateway.cpp
Total findings: 15

- Line 163: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: seen.insert(node.node_id);
- Line 273: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (node.has_value() && node->node_id != config_.node_id) {
- Line 279: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: key, node->node_id);
- Line 308: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 313: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return future.get();
- Line 367: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 471: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string key = std::string(req.target());
- Line 96: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto it = j["rate_limits"].begin(); it != j["rate_limits"].end(); ++it) {
- Line 135: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ConsistentHashRing::removeNode(const std::string& node_id)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: void ConsistentHashRing::removeNode(const std::string& node_id) {
- Line 159: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ConsistentHashRing::nodeCount()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: std::size_t ConsistentHashRing::nodeCount() const {
- Line 161: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> seen;
- Line 213: severity=MEDIUM; category=missing_health_check
  Description: Service initialization without nearby health/status handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void DistributedGateway::start() {
- Line 364: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Stale entry – ignore (idempotent apply)
- Line 485: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto upgrade = req[http::field::upgrade];
- Line 496: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto accept = req[http::field::accept];

### server/websocket_session.cpp
Total findings: 15

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4184 feat(cdc): WebSocket Change... (2026-03-13) | #3316 [WIP] Add WebSocket
- Line 201: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: auto responses = cdc_stream_handler_->handleFrame(msg);
- Line 696: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (!handler->hasSubscriptions()) continue;
- Line 698: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: auto frames = handler->pollEvents(*changefeed_);
- Line 702: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: auto redeliveries = handler->checkRedelivery();
- Line 719: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Legacy /v2/changes polling path.
- Line 722: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 509: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ws_tls_->close(websocket::close_code::internal_error, close_ec);
- Line 511: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ws_plain_->close(websocket::close_code::internal_error, close_ec);
- Line 545: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ws_tls_->close(websocket::close_code::internal_error, close_ec);
- Line 547: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ws_plain_->close(websocket::close_code::internal_error, close_ec);
- Line 555: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void WebSocketSession::close() {
- Line 583: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ws_tls_->close(websocket::close_code::normal, ec);
- Line 585: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ws_plain_->close(websocket::close_code::normal, ec);
- Line 854: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: session->close();

### server/graph_api_handler.cpp
Total findings: 14

- Line 73: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto [status, visited] = graph_index_->bfs(start_vertex, static_cast<int>(max_depth));
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #450 [REFACTOR] Extract GraphApi... (2026-03-11)
- Line 235: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: "Failed to delete edge: " + status.message, req);
- Line 235: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: span.setAttribute("error", "edge_deletion_failed");

            span.setStatus(false, status.message);

            return makeErrorResponse(http::status::internal_server_error,

                "Failed to delete edge: " + status.message, req);

        }



        // Delete edge entity from storage
- Line 235: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: "Failed to delete edge: " + status.message, req);
- Line 265: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: THEMIS_ERROR("Edge delete error: {}", e.what());
- Line 265: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: } catch (const std::exception& e) {

        span.recordError(e.what());

        span.setStatus(false);

        THEMIS_ERROR("Edge delete error: {}", e.what());

        return makeErrorResponse(http::status::internal_server_error, e.what(), req);

    }

}
- Line 265: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_ERROR("Edge delete error: {}", e.what());
- Line 729: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Normalize legacy/empty exports to an object JSON so clients can round-trip
- Line 380: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: body += "themis_graph_latency_ms_bucket{le=\"";
- Line 381: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: body += "themis_graph_latency_ms_bucket{le=\"";
- Line 383: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: body += "\"} ";
- Line 385: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: body += '\n';
- Line 989: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (pv.is_string()) pverts.push_back(pv.get<std::string>());

### server/rpc/snapshot_transfer_handler.cpp
Total findings: 14

- Line 71: severity=CRITICAL; category=missing_dtor
  Description: Class SnapshotTransferHandler allocates resources but has no destructor
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct SnapshotTransferHandler
- Line 83: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: ~Impl() {
- Line 85: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete checkpoint_;
- Line 85: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: ~Impl() {

        if (checkpoint_) {

            delete checkpoint_;

        }

    }
- Line 113: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 139: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::recursive_directory_iterator(snapshot_dir_)) {
- Line 419: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::recursive_directory_iterator(snapshot_dir_)) {
- Line 550: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: THEMIS_ERROR("LZ4: Failed to allocate memory: {}", e.what());
- Line 604: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: THEMIS_ERROR("Snappy: Failed to allocate memory: {}", e.what());
- Line 643: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: THEMIS_ERROR("LZ4: Failed to allocate memory: {}", e.what());
- Line 731: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::recursive_directory_iterator(snapshot_dir_)) {
- Line 760: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::recursive_directory_iterator(dir)) {
- Line 141: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: files.push_back(entry.path());
- Line 733: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: files.push_back(entry.path());

### server/spatial_api_handler.cpp
Total findings: 14

- Line 163: severity=HIGH; category=catch_all_swallow
  Description: catch(...) block swallows errors without rethrowing or explicit handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_design_error_rules
  Context: catch(...) { ... }
- Line 173: severity=HIGH; category=catch_all_swallow
  Description: catch(...) block swallows errors without rethrowing or explicit handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_design_error_rules
  Context: catch(...) { ... }
- Line 64: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto cfg = j["config"];
- Line 66: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto bounds = cfg["total_bounds"];
- Line 163: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

                                geom_info = geo::EWKBParser::parseGeoJSON(geo_val.dump());

                                parse_ok = true;

                            } catch (...) {}

                        }

                    } else if (geo_val.is_string()) {

                        const std::string& geo_str = geo_val.get<std::string>();
- Line 163: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 173: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

                                geom_info = geo::EWKBParser::parseGeoJSON(geo_str);

                                parse_ok = true;

                            } catch (...) {}

                        }

                        if (!parse_ok) {

                            try {
- Line 173: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 179: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

                                geom_info = geo::EWKBParser::parseWKT(geo_str);

                                parse_ok = true;

                            } catch (...) {}

                        }

                    }

                    if (!parse_ok) {
- Line 179: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 321: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto gpu_json_str = geo::getGpuSpatialBackendStatsJson();

            auto gpu_stats = json::parse(gpu_json_str);

            response["gpu_backend"] = gpu_stats;

        } catch (...) {

            response["gpu_backend"] = nullptr;

        }
- Line 321: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 354: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: result += ' ';
- Line 363: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::unordered_map<std::string, std::string> SpatialApiHandler::parseQuery(const std::string& target) {

### server/health_error_service.cpp
Total findings: 13

- Line 51: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 159: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: http::read(socket, buffer, req, ec);
- Line 174: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: http::write(socket, error_res, ec);
- Line 182: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: http::write(socket, res, ec);
- Line 135: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
- Line 139: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 244: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: error_handler_->handleGetErrors(handler_req, handler_res);
- Line 256: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: error_handler_->handleGetCategories(handler_req, handler_res);
- Line 269: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: error_handler_->handleSearchErrors(handler_req, handler_res);
- Line 285: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: error_handler_->handleGetError(handler_req, handler_res);
- Line 2: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: * ThemisDB | File: health_error_service.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
- Line 10: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: #include "server/health_error_service.h"
- Line 89: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: acceptor_->close(ec);

### server/tenant_manager.cpp
Total findings: 13

- Line 630: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator tenantIt may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto tenantIt = tenants_.find(tid);
- Line 128: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 129: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 132: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 137: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 204: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: THEMIS_WARN("TenantManager: Cannot delete default tenant");
- Line 204: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Don't allow deleting default tenant

    if (tid == config_.default_tenant_id) {

        THEMIS_WARN("TenantManager: Cannot delete default tenant");

        return false;

    }
- Line 204: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_WARN("TenantManager: Cannot delete default tenant");
- Line 662: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // Escape label value safely using a new string
- Line 662: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 509: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void TenantManager::recordQuery(std::string_view tenant_id) {
- Line 666: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: tid += '\\';
- Line 667: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: tid += '\\';

### server/export_api_handler.cpp
Total findings: 12

- Line 113: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto* exporter = static_cast<exporters::IExporter*>(plugin->getInstance());
- Line 474: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 32 > array 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: query = conditions[0];
- Line 474: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 32 > array size 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

    }

    

    // Build final query

    if (!conditions.empty()) {

        query = conditions[0];

        for (size_t i = 1; i < conditions.size(); ++i) {

            query += " AND " + conditions[i];

        }

    }
- Line 113: severity=HIGH; category=unsafe_singleton
  Description: Singleton access without thread-safety mechanism
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto* exporter = static_cast<exporters::IExporter*>(plugin->getInstance());
- Line 262: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: exported_file.close();
- Line 384: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::string ExportApiHandler::buildAqlQuery(const json& request_json) {
- Line 427: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: conditions.push_back("category='" + theme + "'");
- Line 433: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: conditions.push_back("domain='" + domain + "'");
- Line 439: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: conditions.push_back("subject='" + subject + "'");
- Line 446: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: conditions.push_back("created_at>='" + from_date + "'");
- Line 476: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: query += " AND " + conditions[i];
- Line 505: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto auth_header = req[http::field::authorization];

### server/policy_engine.cpp
Total findings: 12

- Line 203: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: "replaced all policies, new count=" + std::to_string(count));
- Line 243: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: PolicyEngine::Decision PolicyEngine::authorize(const std::string& user_id,
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #5123 docs(server): update VCCDB ... (2026-05-14) | #3154 [governance] Implem
- Line 212: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: {

        std::lock_guard<std::mutex> lock(mutex_);

        if (config_.max_policies > 0 && policies_.size() >= config_.max_policies) {

            throw std::length_error(

                "PolicyEngine: max_policies limit (" +

                std::to_string(config_.max_policies) + ") reached");

        }
- Line 70: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto eff = n["effect"].as<std::string>("allow");
- Line 84: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: for (const auto& ua : n["allowed_user_agent_patterns"]) p.allowed_user_agent_patterns.push_back(ua.as<std::string>());

                    }

                    return p;

                } catch (...) {

                    return std::nullopt;

                }

            };
- Line 84: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 134: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto mtime = std::filesystem::last_write_time(path);

            last_loaded_mtime_ = std::chrono::time_point_cast<std::chrono::system_clock::duration>(

                mtime - decltype(mtime)::clock::now() + std::chrono::system_clock::now());

        } catch (...) {

            last_loaded_mtime_ = std::chrono::system_clock::now();

        }

        return true;
- Line 134: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 360: severity=MEDIUM; category=expensive_copy
  Description: Unnecessary expensive copy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (user_agent->find(pat) != std::string::npos) { ok = true; break; }
- Line 398: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: p.time_window_utc_hours_end   = j.value("time_window_utc_hours_end",   -1);

        if (j.contains("allowed_user_agent_patterns")) for (const auto& ua : j["allowed_user_agent_patterns"]) p.allowed_user_agent_patterns.push_back(ua.get<std::string>());

        return p;

    } catch (...) {

        return std::nullopt;

    }

}
- Line 398: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### server/feedback_api_handler.cpp
Total findings: 11

- Line 143: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: {"training_weight", feedback.training_weight}

                };

                if (feedback.model_response_id.has_value()) {

                    metadata["model_response_id"] = *feedback.model_response_id;

                }

                if (feedback.cache_key.has_value()) {

                    metadata["cache_key"] = *feedback.cache_key;
- Line 146: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata["model_response_id"] = *feedback.model_response_id;

                }

                if (feedback.cache_key.has_value()) {

                    metadata["cache_key"] = *feedback.cache_key;

                }



                feedback_collector_->recordFeedback(
- Line 121: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: auto stored = storage_service.createFeedback(feedback);
- Line 214: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: auto feedback_list = storage_service.listFeedback(filter);
- Line 273: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: auto feedback = storage_service.getFeedback(id);
- Line 336: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool success = storage_service.updateFeedback(id, feedback);
- Line 347: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: auto updated = storage_service.getFeedback(id);
- Line 389: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool success = storage_service.deleteFeedback(id);
- Line 459: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: auto feedback_list = storage_service.getFeedbackForAdapter(adapter_id, limit);
- Line 521: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: auto stats = storage_service.getStatistics(adapter_id);
- Line 579: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: llm::lora::FeedbackFilter FeedbackAPIHandler::parseFilterFromQuery(const std::string& query) const {

### server/audit_api_handler.cpp
Total findings: 9

- Line 55: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 127: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 37: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: AuditLogEntry::toJson()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: nlohmann::json AuditLogEntry::toJson() const {
- Line 76: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto ciphertext_b64 = payload["ciphertext_b64"].get<std::string>();
- Line 117: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (!event_data.empty()) {

            event = nlohmann::json::parse(event_data);

        }

    } catch (...) {

        // If parsing fails, treat as raw string

    }
- Line 117: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 189: severity=MEDIUM; category=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: // Sort by timestamp descending (newest first)
- Line 238: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (c == '"') escaped += "\"\"";
- Line 239: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (c == '"') escaped += "\"\"";

### server/bpmn_api_handler.cpp
Total findings: 9

- Line 113: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // auth_->authorize() which checks that the token contains the required scope.
- Line 114: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto ar = auth_->authorize(*token, scope);
- Line 188: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Parse request body

        json request = json::parse(req.body());

        

        std::string process_key = request.value("process_definition_key", "");

        json variables = request.value("variables", json::object());

        std::string business_key = request.value("business_key", "");
- Line 189: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: json request = json::parse(req.body());

        

        std::string process_key = request.value("process_definition_key", "");

        json variables = request.value("variables", json::object());

        std::string business_key = request.value("business_key", "");

        

        if (process_key.empty()) {
- Line 190: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::string process_key = request.value("process_definition_key", "");

        json variables = request.value("variables", json::object());

        std::string business_key = request.value("business_key", "");

        

        if (process_key.empty()) {

            return makeErrorResponse(http::status::bad_request, "Missing process_definition_key", req);
- Line 307: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Parse request body

        json request = json::parse(req.body());

        json variables = request.value("variables", json::object());

        

        // Task ID format: "instance_id:node_id"

        std::string instance_id;
- Line 478: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto tsIt = token.visit_timestamps.find(node);
- Line 59: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];
- Line 94: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];

### server/ethics_api_handler.cpp
Total findings: 9

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3632 fix(build): register 40+ mi... (2026-03-12) | #946 [FEATURE] Ethics AI
- Line 536: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: query::AQLParser parser;

    auto parse_result = parser.parse(resolved_query);

    if (!parse_result.has_value()) {

        throw std::runtime_error("AQL parse error: " + parse_result.error().message());

    }



    // Translate the AST to a QueryEngine query
- Line 542: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Translate the AST to a QueryEngine query

    auto translation = AQLTranslator::translate(*parse_result);

    if (!translation.success) {

        throw std::runtime_error("AQL translation error: " + translation.error_message);

    }



    // Execute and return results as JSON array
- Line 101: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto philosophies = body["philosophy_schools"].get<std::vector<std::string>>();
- Line 144: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto philosophies = body["philosophy_schools"].get<std::vector<std::string>>();
- Line 371: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto philosophies = body["philosophy_schools"].get<std::vector<std::string>>();
- Line 428: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: prom += "# TYPE " + prefix + " gauge\n";
- Line 517: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (c == '\'') { escaped += "''"; } else { escaped += c; }
- Line 518: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (c == '\'') { escaped += "''"; } else { escaped += c; }

### server/import_api_handler.cpp
Total findings: 9

- Line 310: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto job = registry_->getJsonSnapshot(job_id);
- Line 322: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto snapshot = registry_->getRunningAndJsonSnapshot(job_id);
- Line 338: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto updated = registry_->getJsonSnapshot(job_id);
- Line 345: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto jobs = registry_->allJsonSnapshots();
- Line 353: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto jobs = registry_->allJsonSnapshots();
- Line 489: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto source_path_opt = registry_->getSourcePathSnapshot(job_id);
- Line 195: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: jsonOk(res, handle->toJson());
- Line 248: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: jsonOk(res, handle->toJson());
- Line 303: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: jsonOk(res, handle->toJson());

### server/profiling_api_handler.cpp
Total findings: 9

- Line 268: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto qp = body["query_profiler"];
- Line 274: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto threshold_ms = qp["slow_query_threshold_ms"].get<int>();
- Line 288: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto sp = body["storage_profiler"];
- Line 292: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto threshold_ms = sp["slow_op_threshold_ms"].get<int>();
- Line 306: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto an = body["analyzer"];
- Line 309: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto threshold_ms = an["slow_query_threshold_ms"].get<int>();
- Line 317: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto cache_hit_rate = an["cache_hit_rate_threshold"].get<double>();
- Line 405: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

        value = std::stoi(value_str);

        return true;

    } catch (...) {

        value = default_value;

        return false;

    }
- Line 405: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### server/llm_grpc_service.cpp
Total findings: 8

- Line 180: severity=CRITICAL; category=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: internal_req.prompt = request->query();
- Line 546: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto* stats = response->mutable_cache_stats();
- Line 2: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: * ThemisDB | File: llm_grpc_service.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
- Line 10: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: #include "server/llm_grpc_service.h"
- Line 53: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: while (s.size() % 4) s += '=';
- Line 78: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto exp = claims["exp"].get<int64_t>();
- Line 148: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto internal_resp = plugin_mgr.generate(internal_req);
- Line 407: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: temp_file.close();

### server/pki_api_handler.cpp
Total findings: 7

- Line 78: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: out.push_back((uint8_t)((val>>valb)&0xFF));
- Line 118: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: SigningResult res = signing_service.sign(data, key_id);
- Line 148: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto data_b64 = body["data_b64"].get<std::string>();
- Line 157: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool ok = signing_service.verify(data, sig, key_id);
- Line 390: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto qualified_sig = body["qualified_signature"];
- Line 535: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto keys = hsm_provider_->listKeys();

                status["hsm_keys_count"] = keys.size();

                status["hsm_status"] = "connected";

            } catch (...) {

                status["hsm_status"] = "error";

            }

        }
- Line 535: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### server/prompt_engineering_grpc_service.cpp
Total findings: 7

- Line 2: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: * ThemisDB | File: prompt_engineering_grpc_service.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
- Line 11: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: * @file prompt_engineering_grpc_service.cpp
- Line 28: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'gRPC Service' that was not found in 'src/prompt_engineering/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/prompt_engineering/FUTURE_ENHANCEMENTS.md §"gRPC Service"
- Line 29: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'gRPC service lifecycle' that was not found in 'src/server/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/server/ROADMAP.md §gRPC service lifecycle
- Line 32: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: #include "server/prompt_engineering_grpc_service.h"
- Line 75: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } catch (const std::exception& e) {

            THEMIS_ERROR("Prompt gRPC service accessor callback failed: {}", e.what());

            service_ptr_ = nullptr;

        } catch (...) {

            THEMIS_ERROR("Prompt gRPC service accessor callback failed: unknown error");

            service_ptr_ = nullptr;

        }
- Line 75: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### server/rpc/differential_update_engine.cpp
Total findings: 7

- Line 38: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: boundaries.push_back(i);
- Line 43: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: boundaries.push_back(data.size());  // End
- Line 102: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, uint32_t> base_hashes;
- Line 152: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<uint32_t, std::string> ExtractChunks(
- Line 156: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<uint32_t, std::string> chunks;
- Line 183: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: manifest.push_back(info);
- Line 197: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<uint32_t, const ChunkInfo*> by_index;

### server/wal_grpc_service.cpp
Total findings: 7

- Line 191: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return grpc::Status::OK;

            }



            if (!request.entries_compressed().empty()) {

                std::vector<uint8_t> compressed(request.entries_compressed().begin(), request.entries_compressed().end());

                auto decompressed = themis::utils::zstd_decompress(compressed);

                if (decompressed.empty()) {
- Line 192: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



            if (!request.entries_compressed().empty()) {

                std::vector<uint8_t> compressed(request.entries_compressed().begin(), request.entries_compressed().end());

                auto decompressed = themis::utils::zstd_decompress(compressed);

                if (decompressed.empty()) {

                    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Failed to decompress entries_compressed");
- Line 2: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: * ThemisDB | File: wal_grpc_service.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
- Line 10: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: #include "server/wal_grpc_service.h"
- Line 231: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'WAL gRPC Replication' that was not found in 'src/sharding/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/sharding/FUTURE_ENHANCEMENTS.md § "WAL gRPC Replication"
- Line 258: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } catch (const std::exception& e) {

            THEMIS_ERROR("WalGrpcService: service callback failed: {}", e.what());

            service_ptr_ = nullptr;

        } catch (...) {

            THEMIS_ERROR("WalGrpcService: service callback failed: unknown error");

            service_ptr_ = nullptr;

        }
- Line 258: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### server/wasm_handler_registry.cpp
Total findings: 7

- Line 119: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 4 > array size 3
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::vector<uint8_t> result;

    result.reserve((encoded.size() / 4) * 3);



    int i = 0;

    unsigned char char4[4];

    unsigned char char3[3];

    int len = static_cast<int>(encoded.size());



    int idx = 0;

    while (idx < len && encoded[idx] != '=' && isBase64(

               static_cast<unsigned char>(encoded[idx]))) {
- Line 134: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 3 > array size 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: const char* pos = std::find(kBase64Chars,

                                            kBase64Chars + 64,

                                            static_cast<char>(char4[j]));

                char4[j] = static_cast<unsigned char>(pos - kBase64Chars);

            }

            char3[0] = static_cast<unsigned char>( (char4[0] << 2) | (char4[1] >> 4));

            char3[1] = static_cast<unsigned char>(((char4[1] & 0x0f) << 4) | (char4[2] >> 2));

            char3[2] = static_cast<unsigned char>(((char4[2] & 0x03) << 6) |  char4[3]);

            for (int j = 0; j < 3; ++j) {

                result.push_back(char3[j]);

            }
- Line 135: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 3 > array size 1
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: kBase64Chars + 64,

                                            static_cast<char>(char4[j]));

                char4[j] = static_cast<unsigned char>(pos - kBase64Chars);

            }

            char3[0] = static_cast<unsigned char>( (char4[0] << 2) | (char4[1] >> 4));

            char3[1] = static_cast<unsigned char>(((char4[1] & 0x0f) << 4) | (char4[2] >> 2));

            char3[2] = static_cast<unsigned char>(((char4[2] & 0x03) << 6) |  char4[3]);

            for (int j = 0; j < 3; ++j) {

                result.push_back(char3[j]);

            }

            i = 0;
- Line 136: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 3 > array size 2
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: static_cast<char>(char4[j]));

                char4[j] = static_cast<unsigned char>(pos - kBase64Chars);

            }

            char3[0] = static_cast<unsigned char>( (char4[0] << 2) | (char4[1] >> 4));

            char3[1] = static_cast<unsigned char>(((char4[1] & 0x0f) << 4) | (char4[2] >> 2));

            char3[2] = static_cast<unsigned char>(((char4[2] & 0x03) << 6) |  char4[3]);

            for (int j = 0; j < 3; ++j) {

                result.push_back(char3[j]);

            }

            i = 0;

        }
- Line 129: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: const char* pos = std::find(kBase64Chars,
- Line 147: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const char* pos = std::find(kBase64Chars,
- Line 147: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: const char* pos = std::find(kBase64Chars,

### server/chunked_response_writer.cpp
Total findings: 6

- Line 49: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: body += "0\r\n\r\n";
- Line 99: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: current_chunk += '\n';
- Line 152: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: chunk_data += '\n';
- Line 153: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: chunk_data += '\n';
- Line 182: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: size_t chunk_size = 0;

        try {

            chunk_size = std::stoul(size_str, nullptr, 16);

        } catch (...) {

            THEMIS_WARN("decodeChunkedBody: failed to parse chunk size '{}'; aborting decode", size_str);

            break;

        }
- Line 182: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### server/diff_api_handler.cpp
Total findings: 6

- Line 148: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

            size_t limit = std::stoull(req.get_param_value("limit"));

            options.limit = limit;

        } catch (...) {

            throw std::invalid_argument("Invalid limit parameter");

        }

    }
- Line 148: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 158: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

            size_t offset = std::stoull(req.get_param_value("offset"));

            options.offset = offset;

        } catch (...) {

            throw std::invalid_argument("Invalid offset parameter");

        }

    }
- Line 158: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 176: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Try to parse as milliseconds first

    try {

        return std::stoll(str);

    } catch (...) {

        spdlog::debug("DiffApiHandler::parseTimestamp: '{}' is not a numeric millisecond timestamp, trying ISO 8601", str);

    }
- Line 176: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### server/grpc_web_proxy_handler.cpp
Total findings: 6

- Line 172: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: : grpc::InsecureChannelCredentials();

    auto channel = grpc::CreateChannel(config_.backend_address, creds);

    channel_holder_ = channel;

    stub_holder_    = std::make_shared<grpc::GenericStub>(channel);

#endif

    // STUB/SIMULATION NOTE:

    // Purpose: Allow GrpcWebProxyHandler to be instantiated without gRPC.
- Line 298: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: #ifdef THEMIS_ENABLE_GRPC

    ensureChannel();



    auto* stub = static_cast<grpc::GenericStub*>(stub_holder_.get());



    grpc::ClientContext ctx;
- Line 350: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: {

        grpc::CompletionQueue cq;

        auto call = stub->PrepareUnaryCall(&ctx, method, request_buf, &cq);

        call->StartCall();

        call->Finish(&response_buf, &status, reinterpret_cast<void*>(1));
- Line 188: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'gRPC-Web Proxy Activation' that was not found in 'src/server/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/server/FUTURE_ENHANCEMENTS.md §"gRPC-Web Proxy Activation"
- Line 324: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: unit == 'm' || unit == 'u' || unit == 'n') {

                ctx.set_deadline(deadline);

            }

        } catch (...) {

            // Ignore malformed grpc-timeout; use default deadline

        }

    } else if (config_.deadline_ms > 0) {
- Line 324: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### server/oauth2_provider.cpp
Total findings: 6

- Line 84: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: : config_(config)

{

    if (config_.oidc.issuer_url.empty()) {

        throw auth::AuthException(auth::AuthError(

            auth::AuthErrorCode::AUTH_CONFIG_INVALID,

            "OAuth2Provider configuration error",

            "oidc.issuer_url must not be empty"
- Line 91: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: ));

    }

    if (config_.oidc.client_id.empty()) {

        throw auth::AuthException(auth::AuthError(

            auth::AuthErrorCode::AUTH_CONFIG_INVALID,

            "OAuth2Provider configuration error",

            "oidc.client_id must not be empty"
- Line 98: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: ));

    }

    if (config_.redirect_uri.empty()) {

        throw auth::AuthException(auth::AuthError(

            auth::AuthErrorCode::AUTH_CONFIG_INVALID,

            "OAuth2Provider configuration error",

            "redirect_uri must not be empty"
- Line 230: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: curl_easy_cleanup(curl);



    if (res != CURLE_OK) {

        throw std::runtime_error(

            std::string("libcurl error: ") + curl_easy_strerror(res));

    }

    return response;
- Line 441: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 460: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### server/rpc/blob_transfer_handler.cpp
Total findings: 6

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #970 [P1] Implement checkpoint/r... (2026-03-11) | #104 RPC Framework with gR
- Line 397: severity=HIGH; category=catch_all_swallow
  Description: catch(...) block swallows errors without rethrowing or explicit handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_design_error_rules
  Context: catch(...) { ... }
- Line 259: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: output_file_.close();
- Line 397: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (!bridged.empty()) {

                    return bridged;

                }

            } catch (...) {

            }

        }
- Line 397: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 408: severity=MEDIUM; category=shift_overflow
  Description: Shift operation detected (verify shift count < bitwidth)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['                for (int j = 0; j < 8; ++j) {', '                    const uint32_t mask = (crc & 1u) ? 0xFFFFFFFFu : 0u;', '                    crc = (crc >> 1) ^ (0xEDB88320u & mask);', '                }', '            }']

### server/schema_api_handler.cpp
Total findings: 6

- Line 679: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& s : is.getStatistics(std::string_view(table_name))) {
- Line 1155: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: errors.push_back({{"error", "Table schema missing 'name' field"}});
- Line 1156: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: errors.push_back({{"error", "Table schema missing 'name' field"}});
- Line 1162: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: errors.push_back({{"table", schema.name},
- Line 1172: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: imported.push_back(schema.name);
- Line 1175: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: errors.push_back({{"error", std::string("Parse error: ") + ex.what()}});

### server/session_api_handler.cpp
Total findings: 6

- Line 154: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto auth_result = auth_->authorize(bearer_token, "auth:sessions");
- Line 217: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto auth_result = auth_->authorize(bearer_token, "auth:sessions");
- Line 259: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto auth_result = auth_->authorize(bearer_token, "auth:sessions");
- Line 269: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto admin_result = auth_->authorize(bearer_token, "admin:all");
- Line 313: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto auth_result = auth_->authorize(bearer_token, "auth:sessions");
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #2811 [auth] Wire session revocat... (2026-03-12) | #2770 [auth] Implement se

### server/themis_core_grpc_service.cpp
Total findings: 6

- Line 2: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: * ThemisDB | File: themis_core_grpc_service.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
- Line 10: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: #include "server/themis_core_grpc_service.h"
- Line 24: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: // path.  This mirrors the pattern used by WalGrpcService / wal_grpc_service.cpp.
- Line 127: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'gRPC Core Service Activation' that was not found in 'src/server/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/server/FUTURE_ENHANCEMENTS.md §"gRPC Core Service Activation"
- Line 143: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } catch (const std::exception& e) {

            THEMIS_ERROR("ThemisCoreServiceImpl: service-instance callback failed: {}", e.what());

            service_ptr_ = nullptr;

        } catch (...) {

            THEMIS_ERROR("ThemisCoreServiceImpl: service-instance callback failed: unknown error");

            service_ptr_ = nullptr;

        }
- Line 143: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### server/compliance_reporting_api_handler.cpp
Total findings: 5

- Line 313: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto auth_result = auth.authorize(*token, required_scope);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3154 [governance] Implement comp... (2026-03-12) | #1075 Implement GAP-004 P
- Line 284: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Backward compatibility: If no auth configured or disabled, allow access but log a warning
- Line 199: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: entries.push_back(
- Line 293: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];

### server/opa_adapter.cpp
Total findings: 5

- Line 50: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #5123 docs(server): update VCCDB ... (2026-05-14) | #3076 feat(governance): I
- Line 28: severity=MEDIUM; category=missing_health_check
  Description: Service initialization without nearby health/status handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void ensure_curl_global_init() {
- Line 91: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // non-empty).  Treat this as "allow" consistent with OPA's convention

        // where undefined / empty results indicate denial.

        if (result.is_object() && !result.empty()) return true;

    } catch (...) {

        // Parse failure → treat as unavailable

    }

    return std::nullopt;
- Line 91: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### server/saml_auth_provider.cpp
Total findings: 5

- Line 248: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << " index=\"1\"/>\n";
- Line 256: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: xml << "  </md:SPSSODescriptor>\n";
- Line 264: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "  </md:Organization>\n";
- Line 270: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "  </md:ContactPerson>\n";
- Line 273: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: xml << "</md:EntityDescriptor>\n";

### server/sse_connection_manager.cpp
Total findings: 5

- Line 325: severity=CRITICAL; category=missing_dtor
  Description: Class PollTarget allocates resources but has no destructor
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct PollTarget
- Line 93: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::unique_lock<std::shared_mutex> lock(connections_mutex_);
- Line 364: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // Query new events since last sequence — without holding connections_mutex_.
- Line 364: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 389: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // new events to preserve the hard max_buffered_events bound.

### server/content_api_handler.cpp
Total findings: 4

- Line 71: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];
- Line 371: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, double> scores;
- Line 407: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, double> scores;
- Line 745: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto it = body["weights"].begin(); it != body["weights"].end(); ++it) {

### server/geo_topology_api_handler.cpp
Total findings: 4

- Line 131: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::find(zones_arr.begin(), zones_arr.end(), s.zone) == zones_arr.end()) {
- Line 131: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::find(zones_arr.begin(), zones_arr.end(), s.zone) == zones_arr.end()) {
- Line 189: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (ratio == 0.0) {
- Line 261: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### server/policy_manager_api_handler.cpp
Total findings: 4

- Line 462: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto auth_result = auth.authorize(*token, required_scope);
- Line 226: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 433: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Backward compatibility: If no auth configured or disabled, allow access but log a warning
- Line 442: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];

### server/ranger_adapter.cpp
Total findings: 4

- Line 118: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(backoff));
- Line 142: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: resource_prefixes.push_back(path["value"].get<std::string>());
- Line 145: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& v : path["values"]) if (v.is_string()) resource_prefixes.push_back(v.get<std::strin
- Line 149: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (resource_prefixes.empty()) resource_prefixes.push_back("/");

### server/rate_limiter.cpp
Total findings: 4

- Line 191: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: THEMIS_DEBUG("Created new rate limit bucket: key={}, capacity={}, rate={}/min",
- Line 191: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 28: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: TokenBucket::refill()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: void TokenBucket::refill() {
- Line 338: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RateLimiter::reset()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: void RateLimiter::reset() {

### server/branch_api_handler.cpp
Total findings: 3

- Line 229: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: sendError(res, 400, "Failed to delete branch. Branch may be active or not fully merged.");
- Line 229: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: bool success = branch_manager_.deleteBranch(branch_name, force);

    if (!success) {

        sendError(res, 400, "Failed to delete branch. Branch may be active or not fully merged.");

        return;

    }
- Line 229: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: sendError(res, 400, "Failed to delete branch. Branch may be active or not fully merged.");

### server/cache_admin_api_handler.cpp
Total findings: 3

- Line 167: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto ar = auth_->authorize(*token, required_scope);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4329 Implement SLO monitor laten... (2026-03-18) | #2789 [cache] Admin HTTP
- Line 153: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];

### server/cdn_cache_middleware.cpp
Total findings: 3

- Line 220: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::string_view inm  = inm_it->value();
- Line 221: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::string_view etag = etag_it->value();
- Line 81: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: oss << "W/\"" << std::hex << std::setfill('0') << std::setw(16) << h << "\"";

### server/pii_api_handler.cpp
Total findings: 3

- Line 97: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto span = Tracer::startSpan("getMapping");

        json j = json::parse(value);

        return PiiMapping::fromJson(j);

    } catch (...) {

        return std::nullopt;

    }

}
- Line 97: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 162: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: csv += r.value("original_uuid", ""); csv += ",";

### server/policy_template_api_handler.cpp
Total findings: 3

- Line 241: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto auth_result = auth.authorize(*token, required_scope);
- Line 213: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Backward compatibility: If no auth configured or disabled, allow access but log a warning
- Line 222: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];

### server/policy_validation_api_handler.cpp
Total findings: 3

- Line 179: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto auth_result = auth.authorize(*token, required_scope);
- Line 150: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Backward compatibility: If no auth configured or disabled, allow access but log a warning
- Line 159: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];

### server/policy_versioning_api_handler.cpp
Total findings: 3

- Line 375: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto auth_result = auth.authorize(*token, required_scope);
- Line 347: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Backward compatibility: If no auth configured or disabled, allow access but log a warning
- Line 356: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];

### server/reports_api_handler.cpp
Total findings: 3

- Line 46: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto lvl = j["level"].get<std::string>();
- Line 59: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto ts = j["ts"].get<std::string>();
- Line 63: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto ts = j["timestamp"].get<std::string>();

### server/review_scheduling_api_handler.cpp
Total findings: 3

- Line 239: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto auth_result = auth.authorize(*token, required_scope);
- Line 211: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Backward compatibility: If no auth configured or disabled, allow access but log a warning
- Line 220: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_header = req[http::field::authorization];

### server/update_api_handler.cpp
Total findings: 3

- Line 103: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: config_json["is_running"] = checker_->isRunning();
- Line 133: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 141: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### server/api_key_mgmt_handler.cpp
Total findings: 2

- Line 216: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 124: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (p.is_string()) permissions.push_back(p.get<std::string>());

### server/distributed_txn_api_handler.cpp
Total findings: 2

- Line 308: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string_view path  = req.target();
- Line 162: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto        op      = body["operation"];

### server/http3_datagram.cpp
Total findings: 2

- Line 94: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['', '    const uint8_t* payload    = data + consumed;', '    const size_t   payload_len = len - consumed;', '', '    // Look up and invoke handler.']
- Line 99: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lk(contexts_mutex_);

### server/http_type_adapter.cpp
Total findings: 2

- Line 42: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: out += ' ';
- Line 43: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += ' ';

### server/index_api_handler.cpp
Total findings: 2

- Line 86: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto configObj = body["config"];
- Line 132: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: columns.push_back(c.get<std::string>());

### server/pitr_grpc_service.cpp
Total findings: 2

- Line 2: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: * ThemisDB | File: pitr_grpc_service.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
- Line 10: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: #include "server/pitr_grpc_service.h"

### server/response_transformer.cpp
Total findings: 2

- Line 22: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 65: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### server/saga_api_handler.cpp
Total findings: 2

- Line 147: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto sig = j["signature"];
- Line 226: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: hash.push_back(byte.get<uint8_t>());

### server/sharding_metrics_handler.cpp
Total findings: 2

- Line 78: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: oss << "# HELP themisdb_slo_error_budget Remaining error budget (0-1)\n";
- Line 102: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: double global_error_budget = slo_monitor.getGlobalErrorBudget();

### server/snapshot_api_handler.cpp
Total findings: 2

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #385 Phase 1 & 2: Implement Name... (2026-03-11) | #384 [WIP] Add Named Snaps
- Line 101: severity=MEDIUM; category=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: sort_by = "timestamp";

### server/transaction_api_handler.cpp
Total findings: 2

- Line 204: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: BaseEntity entity = BaseEntity::fromJson(key, data.dump());

                status = txn->optimisticPut(table, entity, expected_version);

            } else if (op_type == "optimistic_erase") {

                // OCC: delete entity only if version matches expected_version

                if (!op.contains("expected_version") || !op["expected_version"].is_number_unsigned()) {

                    errors_array.push_back({{"index", i},

                        {"error", "optimistic_erase requires 'expected_version' (unsigned int)"}});
- Line 204: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // OCC: delete entity only if version matches expected_version

### server/workload_fingerprint_engine.cpp
Total findings: 2

- Line 122: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['        confidence = 0.0;', '    } else {', '        pattern = kPatternMap[domIdx];', '', '        // Confidence as dominance against the runner-up class.']
- Line 127: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['        // This keeps confidence expressive even when the normalized 4-way', '        // distribution is softened by residual MIXED mass.', '        double first = vec[domIdx];', '        double second = 0.0;', '        for (std::size_t i = 0; i < vec.size(); ++i) {']

### server/ARCHITECTURE.md
Total findings: 1

- Line 1: severity=LOW; category=missing_adr_reference
  Description: Architecture doc missing ADR references: adr_003, adr_007, adr_008
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_module_governance_rules
  Context: Add explicit ADR links/references for module-critical design decisions

### server/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### server/ROADMAP.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'ROADMAP.md' is missing expected cross-links: ARCHITECTURE.md, FUTURE_ENHANCEMENTS.md, README.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### server/api_auth_config.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #5123 docs(server): update VCCDB ... (2026-05-14) | #3104 feat(api): Implemen

### server/api_version.cpp
Total findings: 1

- Line 105: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Support current major version and previous major version for backward compatibility

### server/cache_api_handler.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #446 [REFACTOR] Extract Cache Op... (2026-03-11)

### server/continuous_query_api_handler.cpp
Total findings: 1

- Line 287: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: constexpr int     kPollTimeoutSec  = 60;

### server/cost_based_rate_limiter.cpp
Total findings: 1

- Line 58: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### server/openapi_route_registry.cpp
Total findings: 1

- Line 197: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto tag_description = [](const std::string& t) -> std::string {

### server/policy_api_handler.cpp
Total findings: 1

- Line 47: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto& ranger_client = *ranger_client_;

    try {

        std::string err;

        auto jsonOpt = ranger_client.fetchPolicies(&err);

        if (!jsonOpt) {

            return makeErrorResponse(http::status::bad_gateway, std::string("Ranger fetch failed: ") + err, req);

        }

### server/prompt_engineering_api_handler.cpp
Total findings: 1

- Line 128: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### server/retention_api_handler.cpp
Total findings: 1

- Line 75: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (policy.name.find(filter.name_filter) == std::string::npos) {

### server/smart_routing.cpp
Total findings: 1

- Line 202: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (sc < cc || (sc == cc && s->cached_avg_latency < chosen->cached_avg_latency)) {

### server/timeseries_api_handler.cpp
Total findings: 1

- Line 368: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### server/wal_api_handler.cpp
Total findings: 1

- Line 54: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (hdr == req.end() || hdr->value() != wal_shared_secret_) {

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
