# server Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: server
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 3475
- Actionable Findings (Critical + High): 1406
- Affected Files: 115

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 295 |
| High | 1111 |
| Medium | 2069 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| container | 645 |
| performance_patterns | 547 |
| reliability | 510 |
| platform | 479 |
| security | 368 |
| memory | 238 |
| raii | 127 |
| concurrency | 123 |
| llm_ai_safety | 97 |
| observability | 68 |
| performance | 68 |
| exception_safety | 66 |
| audit_logging | 46 |
| legacy_duplication | 32 |
| determinism | 29 |
| distributed_consistency | 16 |
| type_conversion | 9 |
| input_validation | 4 |
| uninitialized | 3 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/server/http_server.cpp | 538 | 61 | 280 | 197 | 0 |
| src/server/query_api_handler.cpp | 285 | 40 | 81 | 164 | 0 |
| src/server/import_wizard_builder.cpp | 220 | 0 | 2 | 218 | 0 |
| src/server/task_scheduler_api_handler.cpp | 204 | 4 | 15 | 180 | 5 |
| src/server/postgres_session.cpp | 180 | 8 | 47 | 125 | 0 |
| src/server/llm_api_handler.cpp | 136 | 15 | 69 | 52 | 0 |
| src/server/voice_api_handler.cpp | 135 | 2 | 43 | 90 | 0 |
| src/server/mcp_server.cpp | 112 | 8 | 63 | 41 | 0 |
| src/server/monitoring_api_handler.cpp | 74 | 2 | 12 | 60 | 0 |
| src/server/rpc/rpc_service_impl.cpp | 73 | 1 | 26 | 46 | 0 |
| src/server/http3_session.cpp | 70 | 10 | 32 | 28 | 0 |
| src/server/mqtt_client_service.cpp | 57 | 7 | 21 | 29 | 0 |
| src/server/shard_repair_api_handler.cpp | 56 | 7 | 6 | 43 | 0 |
| src/server/mqtt_session.cpp | 55 | 7 | 14 | 34 | 0 |
| src/server/websocket_session.cpp | 43 | 1 | 31 | 11 | 0 |
| src/server/policy_engine.cpp | 40 | 2 | 4 | 34 | 0 |
| src/server/async_job_api_handler.cpp | 38 | 7 | 18 | 13 | 0 |
| src/server/entity_api_handler.cpp | 37 | 4 | 6 | 27 | 0 |
| src/server/lora_api_handler.cpp | 37 | 4 | 10 | 23 | 0 |
| src/server/vector_api_handler.cpp | 37 | 2 | 8 | 27 | 0 |
| src/server/http2_session.cpp | 32 | 3 | 12 | 17 | 0 |
| src/server/graph_api_handler.cpp | 30 | 1 | 8 | 21 | 0 |
| src/server/rope_api_handler.cpp | 30 | 4 | 10 | 16 | 0 |
| src/server/distributed_gateway.cpp | 28 | 3 | 9 | 16 | 0 |
| src/server/schema_api_handler.cpp | 28 | 0 | 1 | 27 | 0 |
| src/server/tenant_manager.cpp | 28 | 4 | 16 | 8 | 0 |
| src/server/import_api_handler.cpp | 27 | 6 | 10 | 11 | 0 |
| src/server/changefeed_api_handler.cpp | 26 | 4 | 6 | 16 | 0 |
| src/server/content_api_handler.cpp | 26 | 1 | 0 | 25 | 0 |
| src/server/replication_topology_api_handler.cpp | 26 | 4 | 7 | 15 | 0 |
| src/server/sse_connection_manager.cpp | 25 | 7 | 15 | 3 | 0 |
| src/server/bpmn_api_handler.cpp | 22 | 2 | 9 | 11 | 0 |
| src/server/rate_limiter_v2.cpp | 22 | 3 | 16 | 3 | 0 |
| src/server/rpc/differential_update_engine.cpp | 22 | 1 | 0 | 21 | 0 |
| src/server/saga_api_handler.cpp | 22 | 0 | 2 | 20 | 0 |
| src/server/feedback_api_handler.cpp | 20 | 0 | 7 | 13 | 0 |
| src/server/auth_middleware.cpp | 19 | 2 | 7 | 10 | 0 |
| src/server/buffer_binary_protocol.cpp | 19 | 0 | 19 | 0 | 0 |
| src/server/profiling_api_handler.cpp | 19 | 0 | 3 | 16 | 0 |
| src/server/saml_auth_provider.cpp | 18 | 2 | 0 | 16 | 0 |
| src/server/api_gateway.cpp | 17 | 1 | 8 | 8 | 0 |
| src/server/llm_grpc_service.cpp | 17 | 1 | 9 | 7 | 0 |
| src/server/pki_api_handler.cpp | 17 | 0 | 1 | 16 | 0 |
| src/server/ethics_api_handler.cpp | 16 | 0 | 3 | 13 | 0 |
| src/server/ranger_adapter.cpp | 16 | 0 | 2 | 14 | 0 |
| src/server/smart_routing.cpp | 16 | 7 | 2 | 7 | 0 |
| src/server/api_key_mgmt_handler.cpp | 15 | 1 | 7 | 7 | 0 |
| src/server/geo_topology_api_handler.cpp | 15 | 0 | 4 | 11 | 0 |
| src/server/prompt_engineering_api_handler.cpp | 15 | 3 | 4 | 8 | 0 |
| src/server/rpc/snapshot_transfer_handler.cpp | 15 | 1 | 9 | 5 | 0 |
| src/server/wasm_handler_registry.cpp | 15 | 0 | 4 | 11 | 0 |
| src/server/export_api_handler.cpp | 14 | 2 | 4 | 8 | 0 |
| src/server/health_error_service.cpp | 14 | 4 | 8 | 2 | 0 |
| src/server/index_api_handler.cpp | 14 | 0 | 1 | 13 | 0 |
| src/server/audit_api_handler.cpp | 13 | 0 | 3 | 10 | 0 |
| src/server/spatial_api_handler.cpp | 13 | 0 | 4 | 9 | 0 |
| src/server/timeseries_api_handler.cpp | 13 | 0 | 6 | 7 | 0 |
| src/server/policy_versioning_api_handler.cpp | 12 | 1 | 2 | 9 | 0 |
| src/server/oauth2_provider.cpp | 11 | 2 | 9 | 0 | 0 |
| src/server/wal_grpc_service.cpp | 11 | 0 | 5 | 6 | 0 |
| src/server/chunked_response_writer.cpp | 10 | 0 | 0 | 10 | 0 |
| src/server/graphql_api_handler.cpp | 10 | 0 | 4 | 6 | 0 |
| src/server/session_api_handler.cpp | 10 | 5 | 3 | 2 | 0 |
| src/server/transaction_api_handler.cpp | 10 | 0 | 0 | 10 | 0 |
| src/server/maintenance_api_handler.cpp | 9 | 2 | 3 | 4 | 0 |
| src/server/rate_limiter.cpp | 9 | 4 | 3 | 2 | 0 |
| src/server/response_transformer.cpp | 9 | 1 | 2 | 6 | 0 |
| src/server/retention_api_handler.cpp | 9 | 0 | 3 | 6 | 0 |
| src/server/compliance_reporting_api_handler.cpp | 8 | 1 | 1 | 6 | 0 |
| src/server/http3_datagram.cpp | 8 | 1 | 7 | 0 | 0 |
| src/server/opa_adapter.cpp | 8 | 1 | 5 | 2 | 0 |
| include/server/examples/workload_fingerprint_example.cpp | 6 | 0 | 0 | 6 | 0 |
| src/server/branch_api_handler.cpp | 6 | 0 | 1 | 5 | 0 |
| src/server/buffer_api_handler.cpp | 6 | 0 | 3 | 3 | 0 |
| src/server/distributed_txn_api_handler.cpp | 6 | 0 | 1 | 5 | 0 |
| src/server/openapi_route_registry.cpp | 6 | 1 | 0 | 5 | 0 |
| src/server/pii_api_handler.cpp | 6 | 0 | 1 | 5 | 0 |
| src/server/policy_manager_api_handler.cpp | 6 | 1 | 2 | 3 | 0 |
| src/server/policy_template_api_handler.cpp | 6 | 1 | 2 | 3 | 0 |
| src/server/rpc/blob_transfer_handler.cpp | 6 | 0 | 1 | 5 | 0 |
| src/server/serverless_function_api_handler.cpp | 6 | 2 | 0 | 4 | 0 |
| src/server/wal_api_handler.cpp | 6 | 1 | 0 | 5 | 0 |
| src/server/cache_admin_api_handler.cpp | 5 | 1 | 0 | 4 | 0 |
| src/server/grpc_web_proxy_handler.cpp | 5 | 0 | 3 | 2 | 0 |
| src/server/policy_validation_api_handler.cpp | 5 | 1 | 1 | 3 | 0 |
| src/server/review_scheduling_api_handler.cpp | 5 | 1 | 1 | 3 | 0 |
| src/server/snapshot_api_handler.cpp | 5 | 0 | 1 | 4 | 0 |
| src/server/cdn_cache_middleware.cpp | 4 | 2 | 0 | 2 | 0 |
| src/server/classification_api_handler.cpp | 4 | 0 | 0 | 4 | 0 |
| src/server/error_api_handler.cpp | 4 | 0 | 0 | 4 | 0 |
| src/server/rate_limiting_middleware.cpp | 4 | 0 | 2 | 2 | 0 |
| src/server/reports_api_handler.cpp | 4 | 0 | 0 | 4 | 0 |
| src/server/request_coalescing.cpp | 4 | 3 | 0 | 1 | 0 |
| src/server/themis_core_grpc_service.cpp | 4 | 0 | 1 | 3 | 0 |
| src/server/adaptive_rate_limiter.cpp | 3 | 1 | 0 | 2 | 0 |
| src/server/cache_api_handler.cpp | 3 | 0 | 3 | 0 | 0 |
| src/server/continuous_query_api_handler.cpp | 3 | 0 | 1 | 2 | 0 |
| src/server/cost_based_rate_limiter.cpp | 3 | 1 | 2 | 0 | 0 |
| src/server/diff_api_handler.cpp | 3 | 0 | 0 | 3 | 0 |
| src/server/mvcc_api_handler.cpp | 3 | 0 | 2 | 1 | 0 |
| src/server/pitr_api_handler.cpp | 3 | 0 | 0 | 3 | 0 |
| src/server/pitr_grpc_service.cpp | 3 | 0 | 2 | 1 | 0 |
| src/server/prompt_api_handler.cpp | 3 | 0 | 1 | 2 | 0 |
| src/server/prompt_engineering_grpc_service.cpp | 3 | 0 | 0 | 3 | 0 |
| src/server/update_api_handler.cpp | 3 | 1 | 2 | 0 | 0 |
| src/server/api_security_audit.cpp | 2 | 0 | 0 | 2 | 0 |
| src/server/hot_reload_api_handler.cpp | 2 | 0 | 0 | 2 | 0 |
| src/server/http3_production_config.cpp | 2 | 1 | 1 | 0 | 0 |
| src/server/http_type_adapter.cpp | 2 | 0 | 0 | 2 | 0 |
| src/server/merge_api_handler.cpp | 2 | 0 | 0 | 2 | 0 |
| src/server/sharding_metrics_handler.cpp | 2 | 0 | 2 | 0 | 0 |
| src/server/udf_api_handler.cpp | 2 | 0 | 0 | 2 | 0 |
| src/server/workload_fingerprint_engine.cpp | 2 | 0 | 2 | 0 | 0 |
| src/server/api_version.cpp | 1 | 0 | 1 | 0 | 0 |
| src/server/policy_api_handler.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### src/server/http_server.cpp
Total findings: 538

- Line 546: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cf_result = storage_->getOrCreateColumnFamily("pii_mappings");
- Line 632: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: adaptive_index_ = std::make_shared<AdaptiveIndexManager>(storage_->getRawDB());
- Line 1562: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (auto audit = weak_audit.lock()) {
- Line 2158: severity=CRITICAL; category=no_timeout
  Description: thread_join without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: thread.join();
- Line 2174: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: void HttpServer::wait() {
- Line 2177: severity=CRITICAL; category=no_timeout
  Description: thread_join without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: thread.join();
- Line 2290: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: THEMIS_WARN("Max connections ({}) reached - rejecting new connection",
- Line 2322: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: THEMIS_WARN("TLS enabled but SSL context unavailable; rejecting new connection");
- Line 3587: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: std::string_view incoming_corr = (corr_it != req.end()) ? std::string_view(corr_it->value()) : "";
- Line 3588: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: correlation_id = tracing_middleware_->processRequest(incoming_corr);
- Line 3663: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto checkSegment = [&](const std::string& prefix) -> std::optional<http::response<http::string_body
- Line 3844: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: http::response<http::string_body> response = ethics_api_->handle(req, target);
- Line 4521: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = monitoring_api_->handleHealthCheck(req);
- Line 4524: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = monitoring_api_->handleLiveness(req);
- Line 4527: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = monitoring_api_->handleReadiness(req);
- Line 4530: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = monitoring_api_->handleOpenApi(req);
- Line 4533: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = monitoring_api_->handleVersion(req);
- Line 4536: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = monitoring_api_->handleStats(req);
- Line 4539: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = monitoring_api_->handleCapabilities(req);
- Line 4565: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = monitoring_api_->handleMetrics(req);
- Line 4590: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = monitoring_api_->handleMetricsHtml(req);
- Line 4615: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = monitoring_api_->handlePluginMetrics(req);
- Line 4625: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = monitoring_api_->handleObservabilityAlerts(req);
- Line 4634: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = monitoring_api_->handleObservabilityAlertSilence(req);
- Line 4644: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = monitoring_api_->handleObservabilityHealth(req);
- Line 4654: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = monitoring_api_->handleLicenseStatus(req);
- Line 4868: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = cache_api_->handleQuery(req);
- Line 4903: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = cache_api_->handlePut(req);
- Line 4910: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = cache_api_->handleStats(req);
- Line 4917: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = cache_admin_api_->handleHealth(req);
- Line 4924: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = cache_admin_api_->handleStats(req);
- Line 4931: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = cache_admin_api_->handleEvictKey(req);
- Line 4938: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = cache_admin_api_->handleEvictTenant(req);
- Line 4945: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = cache_admin_api_->handleCircuitBreakerReset(req);
- Line 4952: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = cache_admin_api_->handleCircuitBreakerStatus(req);
- Line 4959: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = cache_admin_api_->handleWarmup(req);
- Line 4966: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = cache_admin_api_->handleSnapshot(req);
- Line 4973: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = cache_admin_api_->handleListTenants(req);
- Line 4980: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = cache_admin_api_->handleTenantStats(req);
- Line 4987: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = cache_admin_api_->handleUpdateTenantQuota(req);
- Line 4994: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = cache_admin_api_->handlePiiEvict(req);
- Line 6676: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto authz = auth_->authorize(*token, "task:register");
  Confidence: band=very_high; score=0.99
- Line 6869: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto authz = auth_->authorize(*token, "task:execute");
  Confidence: band=very_high; score=0.99
- Line 9043: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator bucket_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto bucket_it = audit_rate_buckets_.begin();
- Line 9288: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto ar = auth_->authorize(*token, scope);
  Confidence: band=very_high; score=0.99
- Line 9370: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: auth
  Context: std::cerr << "[AUTH-DBG] validateToken -> authorized=" << (vres.authorized?"true":"false")
  Confidence: band=very_high; score=0.92
- Line 9374: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto ar = auth_->authorize(*token, required_scope);
  Confidence: band=very_high; score=0.99
- Line 9376: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: auth
  Context: std::cerr << "[AUTH-DBG] authorize -> authorized=" << (ar.authorized?"true":"false")
  Confidence: band=very_high; score=0.92
- Line 9406: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: auth
  Context: std::cerr << "[AUTH-DBG] before_policy_check -> user_id='" << user_id << "' action='" << action << "' resource='" << resource << "'\n";
  Confidence: band=very_high; score=0.92
- Line 9433: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto decision = policy_engine_->authorize(user_id, std::string(action), resource, client_ip);
  Confidence: band=very_high; score=0.99
- Line 9540: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto ar = auth_->authorize(*token, "pii:reveal");
  Confidence: band=very_high; score=0.99
- Line 9542: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: ar = auth_->authorize(*token, "admin");
  Confidence: band=very_high; score=0.99
- Line 9579: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto decision = policy_engine_->authorize(user_id, "pii.write", path_only, client_ip);
  Confidence: band=very_high; score=0.99
- Line 9663: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto ar = auth_->authorize(*token, "pii:write");
  Confidence: band=very_high; score=0.99
- Line 9664: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: THEMIS_INFO("PII Delete: authorize('pii:write') -> authorized={}", ar.authorized);
  Confidence: band=very_high; score=0.99
- Line 9666: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: THEMIS_INFO("PII Delete: trying fallback authorize('admin')");
  Confidence: band=very_high; score=0.99
- Line 9667: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: ar = auth_->authorize(*token, "admin");
  Confidence: band=very_high; score=0.99
- Line 9668: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: THEMIS_INFO("PII Delete: authorize('admin') -> authorized={}", ar.authorized);
  Confidence: band=very_high; score=0.99
- Line 9701: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto decision = policy_engine_->authorize(user_id, "pii.write", path_only, client_ip);
  Confidence: band=very_high; score=0.99
- Line 10212: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator pos may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto pos = path.find("/chunks");
- Line 12448: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto result = rate_limiting_middleware_->check(client_key, path);
- Line 0: severity=HIGH; category=uncategorized
  Context: ['                    try {', '                        uint64_t end_pos = std::stoull(rv.substr(dash + 1));', '                        length = (end_pos >= offset) ? (end_pos - offset + 1) : 0;', '                    } catch (...) {}', '                }']
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
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Context: Variable initialized conditionally
  Confidence: band=high; score=0.81
- Line 549: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: pii_api_ = std::make_unique<PIIApiHandler>(storage_->getRawDB(), pii_cf_handle_);
- Line 556: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: pii_api_ = std::make_unique<PIIApiHandler>(storage_->getRawDB(), nullptr);
- Line 556: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: pii_api_ = std::make_unique<PIIApiHandler>(storage_->getRawDB(), nullptr);
- Line 699: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: content_manager_->registerProcessor(std::unique_ptr<themis::content::IContentProcessor>(text_process
- Line 699: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: content_manager_->registerProcessor(std::unique_ptr<themis::content::IContentProcessor>(text_process
- Line 1101: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::shared_ptr<QueryEngine>(ethics_query_engine_.get(), [](QueryEngine*) {}),
- Line 1222: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: schema_api_handler_->setStatisticsCollector(stats_collector_.get());
- Line 1223: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: schema_api_handler_->setSchemaConstraints(schema_constraints_.get());
- Line 1224: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: schema_api_handler_->setSchemaVersionManager(schema_version_mgr_.get());
- Line 1225: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: schema_api_handler_->setIndexRecommender(index_recommender_.get());
- Line 1226: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: schema_api_handler_->setAuditLog(schema_audit_log_.get());
- Line 1230: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: schema_api_handler_->setColumnLineageTracker(column_lineage_tracker_.get());
- Line 1374: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto response = qapi->handleQueryAql(inner);
- Line 1871: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: SSL_CTX_set_cipher_list(ssl_ctx_->native_handle(), config_.tls_cipher_list.c_str());
- Line 1876: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: SSL_CTX_set_cipher_list(ssl_ctx_->native_handle(),
- Line 1895: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("mTLS enabled but CA cert path not configured");
- Line 1923: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(std::string("TLS initialization failed: ") + e.what());
- Line 2020: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: http3_handler_->start();
- Line 2069: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: http3_handler_->stop();
- Line 2083: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (active_requests_.load(std::memory_order_acquire) > 0
- Line 2085: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
- Line 2087: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: auto remaining = active_requests_.load(std::memory_order_acquire);
- Line 2208: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: SSL_CTX_set_cipher_list(new_ctx->native_handle(), config_.tls_cipher_list.c_str());
- Line 2210: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: SSL_CTX_set_cipher_list(new_ctx->native_handle(),
- Line 2260: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: acceptor_.async_accept(
- Line 2665: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: MetadataIndexRecsGet,     // GET  /api/v1/metadata/index_recommendations[/:table]
- Line 2666: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: MetadataAuditGet,         // GET  /api/v1/metadata/audit[/:table]
- Line 2667: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: MetadataLineageGet,       // GET  /api/v1/metadata/lineage/:table[/:column]
- Line 2868: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward compatibility alias
  Confidence: band=high; score=0.8
- Line 3551: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: span.setAttribute("http.method", std::string(http::to_string(req.method())));
- Line 3552: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: span.setAttribute("http.target", std::string(req.target()));
- Line 3663: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto checkSegment = [&](const std::string& prefix) -> std::optional<http::response<http::string_body
- Line 3805: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!tenant_guard->acquireQuerySlot()) {
- Line 3844: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: http::response<http::string_body> response = ethics_api_->handle(req, target);
- Line 4213: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: llm_request.model_id = payload.value("model", std::string{"default"});
- Line 4214: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: llm_request.max_tokens = payload.value("max_tokens", 512);
- Line 4215: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: llm_request.temperature = static_cast<float>(payload.value("temperature", 0.7));
- Line 4253: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: llm_request.model_id = payload.value("model", std::string{"default"});
- Line 4254: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: llm_request.max_tokens = payload.value("max_tokens", 512);
- Line 4255: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: llm_request.temperature = static_cast<float>(payload.value("temperature", 0.7));
- Line 4309: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = assistant.query(query);
- Line 4328: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response_body["relevant_documents"] = docs_array;
- Line 4521: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = monitoring_api_->handleHealthCheck(req);
- Line 4524: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = monitoring_api_->handleLiveness(req);
- Line 4527: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = monitoring_api_->handleReadiness(req);
- Line 4530: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = monitoring_api_->handleOpenApi(req);
- Line 4533: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = monitoring_api_->handleVersion(req);
- Line 4536: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = monitoring_api_->handleStats(req);
- Line 4539: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = monitoring_api_->handleCapabilities(req);
- Line 4565: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = monitoring_api_->handleMetrics(req);
- Line 4590: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = monitoring_api_->handleMetricsHtml(req);
- Line 4615: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = monitoring_api_->handlePluginMetrics(req);
- Line 4625: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = monitoring_api_->handleObservabilityAlerts(req);
- Line 4634: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = monitoring_api_->handleObservabilityAlertSilence(req);
- Line 4644: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = monitoring_api_->handleObservabilityHealth(req);
- Line 4654: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = monitoring_api_->handleLicenseStatus(req);
- Line 4664: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = wal_api_->handleApply(req);
- Line 4675: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = admin_api_->handleBackup(req);
- Line 4683: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = admin_api_->handleRestore(req);
- Line 4686: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = entity_api_->handleGet(req);
- Line 4689: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = entity_api_->handlePut(req);
- Line 4692: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = entity_api_->handleDelete(req);
- Line 4695: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = entity_api_->handlePut(req);
- Line 4698: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = entity_api_->handleBatch(req);
- Line 4701: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = entity_api_->handleBulkNdjson(req);
- Line 4704: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = query_api_->handleQuery(req);
- Line 4721: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = query_api_->handleQueryAql(req);
- Line 4725: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = query_api_->handleQueryStreamSse(req);
- Line 4728: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = index_api_->handleCreate(req);
- Line 4731: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = index_api_->handleDrop(req);
- Line 4734: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = index_api_->handleStats(req);
- Line 4737: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = index_api_->handleRebuild(req);
- Line 4740: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = index_api_->handleReindex(req);
- Line 4745: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = spatial_api_->handleIndexCreate(req);
- Line 4749: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = spatial_api_->handleIndexRebuild(req);
- Line 4753: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = spatial_api_->handleIndexStats(req);
- Line 4757: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = spatial_api_->handleMetrics(req);
- Line 4763: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = graph_api_->handleTraverse(req);
- Line 4770: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = graph_api_->handleEdgeCreate(req);
- Line 4777: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = graph_api_->handleEdgeDelete(req);
- Line 4784: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = graph_api_->handleMetrics(req);
- Line 4791: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = graph_api_->handleMetricsPrometheus(req);
- Line 4798: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = graph_api_->handleIncrementalQueryRegister(req);
- Line 4805: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = graph_api_->handleIncrementalQueryUnregister(req);
- Line 4812: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = graph_api_->handleGraphChanges(req);
- Line 4819: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = graph_api_->handleCostModelCalibrate(req);
- Line 4826: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = graph_api_->handleCostModelExport(req);
- Line 4833: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = graph_api_->handleCostModelImport(req);
- Line 4840: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = graph_api_->handleQueryExplain(req);
- Line 4847: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = vector_api_->handleSearch(req);
- Line 4854: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = vector_api_->handleBatchInsert(req);
- Line 4861: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = vector_api_->handleDeleteByFilter(req);
- Line 4868: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = cache_api_->handleQuery(req);
- Line 4875: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = prompt_api_->handlePost(req);
- Line 4882: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = prompt_api_->handleList(req);
- Line 4889: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = prompt_api_->handleGet(req);
- Line 4896: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = prompt_api_->handlePut(req);
- Line 4903: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = cache_api_->handlePut(req);
- Line 4910: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = cache_api_->handleStats(req);
- Line 4917: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = cache_admin_api_->handleHealth(req);
- Line 4924: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = cache_admin_api_->handleStats(req);
- Line 4931: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = cache_admin_api_->handleEvictKey(req);
- Line 4938: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = cache_admin_api_->handleEvictTenant(req);
- Line 4945: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = cache_admin_api_->handleCircuitBreakerReset(req);
- Line 4952: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = cache_admin_api_->handleCircuitBreakerStatus(req);
- Line 4959: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = cache_admin_api_->handleWarmup(req);
- Line 4966: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = cache_admin_api_->handleSnapshot(req);
- Line 4973: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = cache_admin_api_->handleListTenants(req);
- Line 4980: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = cache_admin_api_->handleTenantStats(req);
- Line 4987: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = cache_admin_api_->handleUpdateTenantQuota(req);
- Line 4994: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = cache_admin_api_->handlePiiEvict(req);
- Line 5323: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = query_api_->handleQueryEnhanced(req);
- Line 5327: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = changefeed_api_->handleGet(req);
- Line 5334: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = changefeed_api_->handleStreamSse(req);
- Line 5341: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = changefeed_api_->handleStreamAck(req);
- Line 5348: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = changefeed_api_->handleStats(req);
- Line 5355: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = changefeed_api_->handleRetention(req);
- Line 5362: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = changefeed_api_->handleRetentionGet(req);
- Line 5369: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = changefeed_api_->handleRetentionPut(req);
- Line 5376: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = changefeed_api_->handleCompact(req);
- Line 5383: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = changefeed_api_->handleGdprRedact(req);
- Line 5395: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: snapshot_api_handler_->handleCreateTag(httplib_req, httplib_res);
- Line 5408: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: snapshot_api_handler_->handleListTags(httplib_req, httplib_res);
- Line 5421: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: snapshot_api_handler_->handleGetTag(httplib_req, httplib_res);
- Line 5434: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: snapshot_api_handler_->handleDeleteTag(httplib_req, httplib_res);
- Line 5447: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: snapshot_api_handler_->handleGetStats(httplib_req, httplib_res);
- Line 5462: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: diff_api_handler_->handleGetDiff(httplib_req, httplib_res);
- Line 5475: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: diff_api_handler_->handleGetCacheStats(httplib_req, httplib_res);
- Line 5488: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: diff_api_handler_->handleClearCache(httplib_req, httplib_res);
- Line 5503: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: pitr_api_handler_->handleRestore(httplib_req, httplib_res);
- Line 5516: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: pitr_api_handler_->handlePreview(httplib_req, httplib_res);
- Line 5529: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: pitr_api_handler_->handleGetProgress(httplib_req, httplib_res);
- Line 5542: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: branch_api_handler_->handleCreateBranch(httplib_req, httplib_res);
- Line 5553: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: branch_api_handler_->handleListBranches(httplib_req, httplib_res);
- Line 5564: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: branch_api_handler_->handleGetActiveBranch(httplib_req, httplib_res);
- Line 5575: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: branch_api_handler_->handleGetStats(httplib_req, httplib_res);
- Line 5586: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: branch_api_handler_->handleGetBranch(httplib_req, httplib_res);
- Line 5597: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: branch_api_handler_->handleSwitchBranch(httplib_req, httplib_res);
- Line 5608: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: branch_api_handler_->handleDeleteBranch(httplib_req, httplib_res);
- Line 5619: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: branch_api_handler_->handleMergeBranches(httplib_req, httplib_res);
- Line 5632: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: merge_api_handler_->handleMerge(httplib_req, httplib_res);
- Line 5643: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: merge_api_handler_->handleMergePreview(httplib_req, httplib_res);
- Line 5654: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: merge_api_handler_->handleMergeByTag(httplib_req, httplib_res);
- Line 5665: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: merge_api_handler_->handleCanFastForward(httplib_req, httplib_res);
- Line 5675: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = timeseries_api_->handlePut(req);
- Line 5683: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = timeseries_api_->handleQuery(req);
- Line 5691: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = timeseries_api_->handleAggregate(req);
- Line 5699: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = timeseries_api_->handleConfigGet(req);
- Line 5707: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = timeseries_api_->handleConfigPut(req);
- Line 5715: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = timeseries_api_->handleAggregatesGet(req);
- Line 5723: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = timeseries_api_->handleRetentionGet(req);
- Line 5731: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = timeseries_api_->handleMetricsGet(req);
- Line 5739: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = timeseries_api_->handlePrometheusRemoteWrite(req);
- Line 5746: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = index_api_->handleSuggestions(req);
- Line 5749: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = index_api_->handlePatterns(req);
- Line 5752: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = index_api_->handleRecordPattern(req);
- Line 5755: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = index_api_->handleClearPatterns(req);
- Line 5759: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = vector_api_->handleIndexSave(req);
- Line 5766: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = vector_api_->handleIndexLoad(req);
- Line 5773: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = vector_api_->handleIndexConfigGet(req);
- Line 5780: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = vector_api_->handleIndexConfigPut(req);
- Line 5787: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = vector_api_->handleIndexStats(req);
- Line 5794: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = vector_api_->handleIncrementalReindex(req);
- Line 5801: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = rope_api_->handleConfigPost(req);
- Line 5808: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = rope_api_->handleConfigGet(req);
- Line 5815: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = rope_api_->handleConfigDelete(req);
- Line 5822: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = rope_api_->handleAddPost(req);
- Line 5829: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = rope_api_->handleAddRelationalPost(req);
- Line 5836: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = rope_api_->handleSearchPost(req);
- Line 5843: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = rope_api_->handleBatchAddPost(req);
- Line 5850: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = rope_api_->handleStatsGet(req);
- Line 5929: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = export_api_->handleExportJsonlLlm(req);
- Line 5937: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = export_api_->handleExportStatus(req);
- Line 5948: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = update_api_->handleRequest(req);
- Line 5959: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = feedback_api_handler_->handleCreateFeedback(req);
- Line 5968: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = feedback_api_handler_->handleListFeedback(req);
- Line 5983: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = feedback_api_handler_->handleGetFeedback(req, id);
- Line 5999: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = feedback_api_handler_->handleUpdateFeedback(req, id);
- Line 6015: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = feedback_api_handler_->handleDeleteFeedback(req, id);
- Line 6031: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = feedback_api_handler_->handleGetAdapterFeedback(req, adapter_id);
- Line 6041: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = feedback_api_handler_->handleGetStatistics(req);
- Line 6061: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = transaction_api_->handleTransaction(req);
- Line 6064: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = transaction_api_->handleBegin(req);
- Line 6067: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = transaction_api_->handleCommit(req);
- Line 6070: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = transaction_api_->handleRollback(req);
- Line 6073: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = transaction_api_->handleStats(req);
- Line 6076: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = transaction_api_->handleGetVersion(req);
- Line 6081: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = transaction_api_->handleExplain(req);
- Line 6084: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = distributed_txn_api_->handleBegin(req);
- Line 6087: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = distributed_txn_api_->handleOperation(req);
- Line 6090: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = distributed_txn_api_->handleCommit(req);
- Line 6093: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = distributed_txn_api_->handleAbort(req);
- Line 6096: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = distributed_txn_api_->handleReadOnly(req);
- Line 6099: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = distributed_txn_api_->handleStatus(req);
- Line 6102: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = distributed_txn_api_->handleStats(req);
- Line 6120: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = content_api_->handleImport(req);
- Line 6124: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = content_api_->handleGet(req);
- Line 6127: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = content_api_->handleGetBlob(req);
- Line 6130: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = content_api_->handleGetChunks(req);
- Line 6145: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = content_api_->handleHybridSearch(req);
- Line 6148: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = content_api_->handleFusionSearch(req);
- Line 6151: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = content_api_->handleFulltextSearch(req);
- Line 6154: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = content_api_->handleContentFilterSchemaGet(req);
- Line 6157: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = content_api_->handleContentFilterSchemaPut(req);
- Line 6160: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = content_api_->handleConfigGet(req);
- Line 6163: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = content_api_->handleConfigPut(req);
- Line 6166: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = content_api_->handleEdgeWeightConfigGet(req);
- Line 6169: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = content_api_->handleEdgeWeightConfigPut(req);
- Line 6182: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = content_api_->handleEncryptionSchemaGet(req);
- Line 6195: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = content_api_->handleEncryptionSchemaPut(req);
- Line 6211: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = bpmn_api_->handleStartProcess(req);
- Line 6219: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = bpmn_api_->handleTaskComplete(req);
- Line 6227: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = bpmn_api_->handleQueryInstance(req);
- Line 6236: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = geo_topology_api_->handleTopologyGet(req);
- Line 6239: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = geo_topology_api_->handleRegionsGet(req);
- Line 6242: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = geo_topology_api_->handleHealthGet(req);
- Line 6245: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = geo_topology_api_->handleTopologyShardPost(req);
- Line 6248: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = geo_topology_api_->handleTopologyShardDelete(req);
- Line 6251: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = geo_topology_api_->handleConfigGet(req);
- Line 6254: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = geo_topology_api_->handleConfigPut(req);
- Line 6259: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = replication_topology_api_->handleTopologyGet(req);
- Line 6262: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = replication_topology_api_->handleHealthGet(req);
- Line 6265: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = replication_topology_api_->handleUiGet(req);
- Line 6334: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = policy_api_->handleImportRanger(req);
- Line 6353: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = policy_api_->handleExportRanger(req);
- Line 6376: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: mvcc_api_handler_->handleGetKey(httplib_req, httplib_res);
- Line 6378: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: mvcc_api_handler_->handlePutKey(httplib_req, httplib_res);
- Line 6400: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: mvcc_api_handler_->handleListVersions(httplib_req, httplib_res);
- Line 6402: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: mvcc_api_handler_->handleGcVersions(httplib_req, httplib_res);
- Line 6415: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: mvcc_api_handler_->handleGetClock(httplib_req, httplib_res);
- Line 6427: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: mvcc_api_handler_->handleGetStats(httplib_req, httplib_res);
- Line 6437: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = graphql_api_handler_->handlePost(req);
- Line 6442: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = graphql_api_handler_->handleSchemaGet(req);
- Line 6448: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = grpc_web_proxy_->handleOptions(req);
- Line 6452: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = grpc_web_proxy_->handleStatus(req);
- Line 6469: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = grpc_web_proxy_->handlePost(req, method_path);
- Line 6475: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = serverless_fn_handler_->handleRegister(req);
- Line 6479: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = serverless_fn_handler_->handleList(req);
- Line 6514: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = serverless_fn_handler_->handleInvoke(req, id);
- Line 6516: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = serverless_fn_handler_->handleVersions(req, id);
- Line 6518: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = serverless_fn_handler_->handleGet(req, id);
- Line 6520: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = serverless_fn_handler_->handleUpdate(req, id);
- Line 6522: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = serverless_fn_handler_->handleDelete(req, id);
- Line 6529: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = async_job_api_->handleSubmit(req);
- Line 6537: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = async_job_api_->handleList(req);
- Line 6545: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = async_job_api_->handleGetStatus(req);
- Line 6553: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = async_job_api_->handleCancel(req);
- Line 6606: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = udf_api_handler_->handleRegister(req);
- Line 6609: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = udf_api_handler_->handleList(req);
- Line 6617: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = udf_api_handler_->handleGet(req, udf_name);
- Line 6626: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = udf_api_handler_->handleDelete(req, udf_name);
- Line 7283: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto& seg : {std::string("page"), std::string("page_size"),
- Line 7662: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: span.setAttribute("http.status_code", static_cast<int64_t>(response.result_int()));
- Line 9156: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: feature_semantic_cache_live_.store(enabled, std::memory_order_relaxed);
- Line 9228: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: {"max_write_buffer_number", storage_->getConfig().max_write_buffer_number},
- Line 9229: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: {"min_write_buffer_number_to_merge", storage_->getConfig().min_write_buffer_number_to_merge},
- Line 9982: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response["interactions"] = json::array();
- Line 10088: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: bool success = llm_store_->updateMetadata(id, body_json);
- Line 10098: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response["message"] = "Metadata updated successfully";
- Line 11264: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Modern browsers ignore X-XSS-Protection; set to 0 to avoid legacy behavior
  Confidence: band=high; score=0.8
- Line 11384: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw; // Re-throw to allow caller to handle as service unavailable
- Line 11409: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: const uint32_t timeout_ms = server_->request_timeout_ms_live_.load(std::memory_order_relaxed);
- Line 11483: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: api::WsChangeHandler ws_handler(server_->auth_,
- Line 11506: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // legacy /v2/changes protocol only.  The new /v2/cdc/stream
  Confidence: band=high; score=0.8
- Line 11702: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: const uint32_t timeout_ms = server_->request_timeout_ms_live_.load(std::memory_order_relaxed);
- Line 11812: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: api::WsChangeHandler ws_handler(server_->auth_,
- Line 11835: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // legacy /v2/changes protocol only.  The new /v2/cdc/stream
  Confidence: band=high; score=0.8
- Line 12183: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: resp["indexes"] = stats_array;
- Line 12473: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Fallback to legacy rate limiter
  Confidence: band=high; score=0.8
- Line 12542: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: endpoints.push_back({"POST", "/api/aql",                  "AQL query (compat)"});
- Line 12660: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: const bool cap_semantic_cache = feature_semantic_cache_live_.load(std::memory_order_relaxed);
- Line 12661: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: const bool cap_llm_store      = feature_llm_store_live_.load(std::memory_order_relaxed);
- Line 12849: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: endpoints.push_back({"POST", "/api/v1/graphql",            "GraphQL query (v1)"});
- Line 661: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 699: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: content_manager_->registerProcessor(std::unique_ptr<themis::content::IContentProcessor>(text_process
- Line 760: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { audit_rate_limit_per_minute_ = static_cast<uint32_t>(std::stoul(lim)); } catch (...) {}
- Line 890: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { slo_cfg.warning_threshold = std::stod(warn_thr); } catch (...) {}
- Line 893: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { slo_cfg.critical_threshold = std::stod(crit_thr); } catch (...) {}
- Line 929: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { rcfg.connect_timeout_ms = std::stol(*ct); } catch (...) {}
- Line 932: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { rcfg.request_timeout_ms = std::stol(*rt); } catch (...) {}
- Line 935: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { rcfg.max_retries = std::stoi(*mr); } catch (...) {}
- Line 938: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { rcfg.retry_backoff_ms = std::stol(*rb); } catch (...) {}
- Line 943: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1129: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 1426: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: candidates.push_back(p);
- Line 1489: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { opa_cfg.timeout_ms = std::stol(*tms); } catch (...) {}
- Line 1523: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!wl_ip.empty()) rate_config.whitelist_ips.push_back(wl_ip);
- Line 1541: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1568: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { audit->logEvent(entry); } catch (...) {}
- Line 1627: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!origin.empty()) cors_allowed_origins_.push_back(origin);
- Line 1831: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { THEMIS_WARN("Invalid THEMIS_MAX_BODY_BYTES value, using default 10MB"); }
- Line 1876: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: SSL_CTX_set_cipher_list(ssl_ctx_->native_handle(),
  Confidence: band=high; score=0.74
- Line 1939: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1986: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads_.emplace_back([this, i] {
  Confidence: band=high; score=0.74
- Line 2063: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: acceptor_.close(ec);
- Line 2137: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: storage_->close(); // This flushes and closes cleanly
- Line 2210: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: SSL_CTX_set_cipher_list(new_ctx->native_handle(),
  Confidence: band=high; score=0.74
- Line 2294: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: socket.close(close_ec);
- Line 2363: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += ' ';
- Line 3398: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: static constexpr std::string_view kMaintStatus{"/api/v1/maintenance/status"};
- Line 3545: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> req_headers;
  Confidence: band=high; score=0.74
- Line 3618: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: header_bytes += field.name_string().size() + field.value().size() + 4; // 2 for ": " + 2 for "\r\n"
  Confidence: band=high; score=0.74
- Line 3619: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: header_bytes += field.name_string().size() + field.value().size() + 4; // 2 for ": " + 2 for "\r\n"
- Line 3750: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> headers;
  Confidence: band=medium; score=0.66
- Line 4321: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: docs_array.push_back({
  Confidence: band=high; score=0.74
- Line 4322: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: docs_array.push_back({
- Line 4550: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 5048: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shards_arr.push_back({
  Confidence: band=high; score=0.74
- Line 5049: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shards_arr.push_back({
- Line 5181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back({
  Confidence: band=high; score=0.74
- Line 5182: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back({
- Line 6671: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 6864: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 7290: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: if (seg == "page") try { filter.page = std::stoi(val); } catch (...) {}
- Line 7291: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: else if (seg == "page_size") try { filter.page_size = std::stoi(val); } catch (...) {}
- Line 7370: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { limit = std::stoi(std::string(req.target()).substr(qpos + 6)); } catch (...) {}
- Line 8032: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 8057: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { if (!req.body().empty()) body_json = json::parse(req.body()); } catch (...) {}
- Line 8082: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { body = json::parse(req.body()); } catch (...) {
- Line 8156: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { if (!req.body().empty()) body = json::parse(req.body()); } catch (...) {
- Line 8209: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 8220: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { body = json::parse(req.body()); } catch (...) {
- Line 8255: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 8287: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 8319: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 8335: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 8469: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 8589: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "# HELP themis_content_blob_uncompressed_bytes_total Total uncompressed/original bytes observ
- Line 8593: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "# HELP themis_content_blob_compression_ratio Histogram of compression ratios (original_size
- Line 8753: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "themis_wal_apply_latency_seconds_sum " << (static_cast<double>(wal_latency_sum_us) / 1'000'0
- Line 8753: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "themis_wal_apply_latency_seconds_sum " << (static_cast<double>(wal_latency_sum_us) / 1'000'0
- Line 8778: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "themis_content_blob_compression_skipped_total{mime_prefix=\"image/\"} " << skipped_img << "\
- Line 8778: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "themis_content_blob_compression_skipped_total{mime_prefix=\"image/\"} " << skipped_img << "\
- Line 8779: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "themis_content_blob_compression_skipped_total{mime_prefix=\"video/\"} " << skipped_vid << "\
- Line 8779: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "themis_content_blob_compression_skipped_total{mime_prefix=\"video/\"} " << skipped_vid << "\
- Line 8780: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "themis_content_blob_compression_skipped_total{mime_prefix=\"application/zip\"} " << skipped_
- Line 8780: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "themis_content_blob_compression_skipped_total{mime_prefix=\"application/zip\"} " << skipped_
- Line 8809: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "themis_content_blob_compression_skipped_total{mime_prefix=\"image/\"} 0\n";
- Line 8809: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "themis_content_blob_compression_skipped_total{mime_prefix=\"image/\"} 0\n";
- Line 8810: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "themis_content_blob_compression_skipped_total{mime_prefix=\"video/\"} 0\n";
- Line 8810: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "themis_content_blob_compression_skipped_total{mime_prefix=\"video/\"} 0\n";
- Line 8811: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "themis_content_blob_compression_skipped_total{mime_prefix=\"application/zip\"} 0\n";
- Line 8811: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "themis_content_blob_compression_skipped_total{mime_prefix=\"application/zip\"} 0\n";
- Line 8845: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: static std::unordered_map<std::string, std::string> parseQuery(const std::string& target) {
  Confidence: band=high; score=0.74
- Line 8865: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { return std::stoll(s); } catch (...) { return 0; }
- Line 8889: severity=MEDIUM; category=expensive_copy
  Description: Unnecessary expensive copy
  Remediation: Use const reference (const T&) or std::move if transfer is needed
  Context: if (zpos != std::string::npos) tzpos = zpos;
- Line 8914: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { tz_h = tz_m = 0; tz_sign = 0; }
- Line 8952: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { f.page = std::max(1, std::stoi(it->second)); } catch (...) {}
- Line 8959: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 8992: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { f.page = std::max(1, std::stoi(it->second)); } catch (...) {}
- Line 8999: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 9029: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 9078: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 9106: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 9115: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto lvl = lg["level"].get<std::string>();
  Confidence: band=high; score=0.74
- Line 9122: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto fmt = lg["format"].get<std::string>();
  Confidence: band=high; score=0.74
- Line 9138: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto timeout = body["request_timeout_ms"].get<uint32_t>();
  Confidence: band=high; score=0.74
- Line 9181: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto hours = body["cdc_retention_hours"].get<uint32_t>();
  Confidence: band=high; score=0.74
- Line 9266: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 9325: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 9344: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 9354: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 9372: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 9373: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 9378: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 9407: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 9460: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 9518: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 9641: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 9774: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 9860: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 9983: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response["interactions"].push_back(interaction.toJson());
  Confidence: band=high; score=0.74
- Line 9984: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response["interactions"].push_back(interaction.toJson());
- Line 10221: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(std::move(j));
  Confidence: band=high; score=0.74
- Line 10222: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(std::move(j));
- Line 10251: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resp.push_back({{"pk", pk}, {"score", score}});
  Confidence: band=high; score=0.74
- Line 10252: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: resp.push_back({{"pk", pk}, {"score", score}});
- Line 10261: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 10307: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resp.push_back({
  Confidence: band=high; score=0.74
- Line 10308: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: resp.push_back({
- Line 10328: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 10385: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vectorQuery.push_back(val.get<float>());
  Confidence: band=high; score=0.74
- Line 10386: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vectorQuery.push_back(val.get<float>());
- Line 10413: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> scores;
  Confidence: band=medium; score=0.66
- Line 10430: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fusedResults.emplace_back(pk, score);
  Confidence: band=high; score=0.74
- Line 10452: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> scores;
  Confidence: band=medium; score=0.66
- Line 10491: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resp.push_back({
  Confidence: band=high; score=0.74
- Line 10492: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: resp.push_back({
- Line 10518: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 10542: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 10566: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 10721: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 10739: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = body["weights"].begin(); it != body["weights"].end(); ++it) {
  Confidence: band=high; score=0.74
- Line 10751: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 10940: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto configObj = body["config"];
  Confidence: band=high; score=0.74
- Line 10945: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (s.is_string()) config.stopwords.push_back(s.get<std::string>());
  Confidence: band=high; score=0.74
- Line 10946: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (s.is_string()) config.stopwords.push_back(s.get<std::string>());
- Line 10986: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: columns.push_back(c.get<std::string>());
  Confidence: band=high; score=0.74
- Line 10987: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: columns.push_back(c.get<std::string>());
- Line 11419: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: self->socket_.close(close_ec);
- Line 11527: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = request_[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 11711: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: self->stream_.lowest_layer().close(close_ec);
- Line 11722: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void HttpServer::SslSession::start() {
  Confidence: band=medium; score=0.66
- Line 11750: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 11855: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = request_[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 12104: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resp.push_back(stat_obj);
  Confidence: band=high; score=0.74
- Line 12105: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: resp.push_back(stat_obj);
- Line 12176: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats_array.push_back({
  Confidence: band=high; score=0.74
- Line 12177: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats_array.push_back({
- Line 12247: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response.push_back(suggestion.toJson());
  Confidence: band=high; score=0.74
- Line 12248: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response.push_back(suggestion.toJson());
- Line 12289: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response.push_back(pattern.toJson());
  Confidence: band=high; score=0.74
- Line 12290: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response.push_back(pattern.toJson());
- Line 12742: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/merge/can-fast-forward", "Check fast-forward merge"});
- Line 12745: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/mvcc/keys/{key}", "Get key versions"});
- Line 12746: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/api/v1/mvcc/keys/{key}", "Put versioned key"});
- Line 12747: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/mvcc/keys/{key}/versions", "Get version history"});
- Line 12748: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"DELETE", "/api/v1/mvcc/keys/{key}/versions", "Delete versions"});
- Line 12749: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/mvcc/clock",      "Get HLC timestamp"});
- Line 12750: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/mvcc/stats",      "MVCC statistics"});
- Line 12755: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/ts/put",                "Store time-series data"});
- Line 12756: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/ts/query",              "Query time-series (beta)"});
- Line 12757: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/ts/aggregate",          "Aggregate time-series (beta)"});
- Line 12758: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/ts/config",             "Get time-series config"});
- Line 12759: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"PUT",  "/ts/config",             "Update time-series config"});
- Line 12760: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/ts/aggregates",         "List aggregates"});
- Line 12761: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/ts/retention",          "Get retention policy"});
- Line 12779: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/api/updates/check",     "Check for updates"});
- Line 12780: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/updates/config",    "Get update config"});
- Line 12781: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"PUT",  "/api/updates/config",    "Update update config"});
- Line 12788: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/auth/saml/login",    "SAML login initiator"});
- Line 12789: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/api/v1/auth/saml/acs",      "SAML assertion consumer"});
- Line 12790: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/api/v1/auth/saml/slo",      "SAML logout"});
- Line 12791: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/auth/saml/metadata", "SAML metadata"});
- Line 12795: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/api/pki/{key_id}/sign",     "Sign with PKI key"});
- Line 12796: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/api/pki/{key_id}/verify",   "Verify PKI signature"});
- Line 12797: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/api/pki/hsm/sign",          "HSM sign"});
- Line 12798: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/pki/hsm/keys",          "List HSM keys"});
- Line 12825: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/api/v1/export/jsonl_llm",   "Export to JSONL for LLM"});
- Line 12826: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/export/{id}/status", "Export job status"});
- Line 12829: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/api/keys",                  "Create API key"});
- Line 12830: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/keys",                  "List API keys"});
- Line 12831: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/keys/{id}",             "Get API key"});
- Line 12832: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"PUT",  "/api/keys/{id}",             "Update API key"});
- Line 12833: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"DELETE", "/api/keys/{id}",           "Delete API key"});
- Line 12836: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/auth/sessions",             "Create session"});
- Line 12837: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/auth/sessions",             "List sessions"});
- Line 12838: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"DELETE", "/auth/sessions/{id}",      "Delete session"});
- Line 12839: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"DELETE", "/auth/sessions",           "Revoke all other sessions"});
- Line 12842: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/api/v1/query/udfs",         "Register UDF"});
- Line 12843: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/query/udfs",         "List UDFs"});
- Line 12844: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/query/udfs/{name}",  "Get UDF"});
- Line 12910: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/information_schema", "INFORMATION_SCHEMA"});
- Line 12911: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/metadata/stats/{table}", "Table statistics"});
- Line 12912: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/api/v1/metadata/stats/{table}", "Update statistics"});
- Line 12913: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/metadata/constraints/{table}", "Table constraints"});
- Line 12914: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/metadata/index_recommendations", "Index recommendations"});
- Line 12915: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/metadata/audit",     "Metadata audit log"});
- Line 12916: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/metadata/lineage/{table}", "Column lineage"});
- Line 12917: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/api/v1/metadata/lineage",   "Track column lineage"});
- Line 12918: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"PUT",  "/api/v1/metadata/schema_import", "Import schema"});
- Line 12919: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/api/v1/metadata/constraints/validate/{table}", "Validate constraints"
- Line 12922: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/errors",             "List error codes"});
- Line 12923: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/errors/{code}",      "Get error documentation"});
- Line 12924: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/errors/categories",  "Error categories"});
- Line 12925: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/errors/search",      "Search errors"});
- Line 12928: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/api/v1/bpmn/process/start", "Start BPMN process"});
- Line 12929: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/api/v1/bpmn/task/{id}/complete", "Complete BPMN task"});
- Line 13408: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { offset = std::stoull(rv.substr(0, dash)); } catch (...) {}
- Line 13413: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}

### src/server/query_api_handler.cpp
Total findings: 285

- Line 714: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: std::function<std::string(const std::shared_ptr<Expression>&, std::string&)> fieldFromFA = [&](const
- Line 751: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (!colL.empty() && rvL == var1 && bin->right->getType() == ASTNodeType::Literal) { auto lit = std:
- Line 752: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (!colL.empty() && rvL == var2 && bin->right->getType() == ASTNodeType::Literal) { auto lit = std:
- Line 753: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (bin->left->getType() == ASTNodeType::Literal) { std::string rv; std::string col = fieldFromFA(bi
- Line 794: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if ((*parse_result) && (*parse_result)->limit) { auto off = static_cast<size_t>(std::max<int64_t>(0,
- Line 794: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator off may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: if ((*parse_result) && (*parse_result)->limit) { auto off = static_cast<size_t>(std::max<int64_t>(0,
- Line 819: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: resolveToLoopField = [&](const std::shared_ptr<Expression>& e)->std::optional<std::string> {
- Line 863: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lit = std::static_pointer_cast<LiteralExpr>(be->right);
- Line 868: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lit = std::static_pointer_cast<LiteralExpr>(be->left);
- Line 985: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: evalExprToLiteral = [&](std::shared_ptr<themis::query::Expression> expr, nlohmann::json& out)->bool
- Line 1120: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto parseSide = [&](std::shared_ptr<Expression> e, char& var, std::string& field) -> bool {
- Line 1131: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto parseSimpleFromExpr = [&](std::shared_ptr<Expression> expr, SimplePred& out)->bool {
- Line 1181: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto parseSide = [&](std::shared_ptr<Expression> e, char& var, std::string& field) -> bool {
- Line 1350: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator itp may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto itp = parent.find(cur);
- Line 1563: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: std::function<std::string(const std::shared_ptr<Expression>&, std::string&)> fieldFromFA = [&](const
- Line 1623: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lit = std::static_pointer_cast<LiteralExpr>(bin->right);
- Line 1636: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lit = std::static_pointer_cast<LiteralExpr>(bin->right);
- Line 1651: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lit = std::static_pointer_cast<LiteralExpr>(bin->left);
- Line 1754: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto off = static_cast<size_t>(std::max<int64_t>(0, (*parse_result)->limit->offset));
- Line 1755: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cnt = static_cast<size_t>(std::max<int64_t>(0, (*parse_result)->limit->count));
- Line 1759: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator first_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto first_it = out.begin() + static_cast<std::ptrdiff_t>(off);
- Line 1760: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator last_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto last_it = out.begin() + static_cast<std::ptrdiff_t>(last);
- Line 1947: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = parent.find(node);
- Line 2075: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = parent.find(cur);
- Line 2235: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto bin = std::static_pointer_cast<BinaryOpExpr>(filter->condition);
- Line 2240: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto fa = std::static_pointer_cast<FieldAccessExpr>(bin->left);
- Line 2241: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lit = std::static_pointer_cast<LiteralExpr>(bin->right);
- Line 2342: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: evalExpr = [&](const std::shared_ptr<themis::query::Expression>& e) -> nlohmann::json {
- Line 2432: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: exprContainsFn = [&](const std::shared_ptr<themis::query::Expression>& expr, const std::string& name
- Line 2497: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: requested_count_for_cursor = static_cast<size_t>(std::max<int64_t>(1, (*parse_result)->limit->count)
- Line 2696: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto off = static_cast<size_t>(std::max<int64_t>(0, (*parse_result)->limit->offset));
- Line 2697: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cnt = static_cast<size_t>(std::max<int64_t>(0, (*parse_result)->limit->count));
- Line 2706: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator first_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto first_it = sliced.begin() + static_cast<std::ptrdiff_t>(off);
- Line 2707: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator last_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto last_it = sliced.begin() + static_cast<std::ptrdiff_t>(last);
- Line 2729: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: cursor_meta["anchor_set"] = q.orderBy->cursor_pk.has_value();
- Line 2747: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto extractColumn = [&](const std::shared_ptr<themis::query::Expression>& expr)->std::string {
- Line 2841: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto it = mp.find(a.var);
- Line 2881: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: extractColFromFA = [&](const std::shared_ptr<Expression>& expr, bool& rootedAtLoop)->std::optional<s
- Line 2894: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: containsFunction = [&](const std::shared_ptr<Expression>& expr, const std::string& name)->bool{
- Line 3212: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: requested_count = static_cast<size_t>(std::max<int64_t>(1, (*parse_result)->limit->count));
- Line 0: severity=HIGH; category=uncategorized
  Context: Variable initialized conditionally
  Confidence: band=high; score=0.81
- Line 139: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: auto masking_policy = std::atomic_load_explicit(&masking_policy_, std::memory_order_acquire);
- Line 261: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: auto* stats_collector = stats_collector_.load(std::memory_order_acquire);
- Line 295: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: plan_json["estimates"] = nlohmann::json::array();
- Line 361: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: plan_json["estimates"] = nlohmann::json::array();
- Line 439: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: plan_json["estimates"] = nlohmann::json::array();
- Line 696: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (*parse_result && (*parse_result)->traversal == nullptr) {
- Line 714: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::function<std::string(const std::shared_ptr<Expression>&, std::string&)> fieldFromFA = [&](const
- Line 714: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::function<std::string(const std::shared_ptr<Expression>&, std::string&)> fieldFromFA = [&](const
- Line 718: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto* cur = fa->object.get();
- Line 719: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: while (auto* fa2 = dynamic_cast<FieldAccessExpr*>(cur)) { parts.push_back(fa2->field); cur = fa2->ob
- Line 736: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: collectPreds = [&](const std::shared_ptr<Expression>& e){
- Line 764: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: auto* stats_collector = stats_collector_.load(std::memory_order_acquire);
- Line 794: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if ((*parse_result) && (*parse_result)->limit) { auto off = static_cast<size_t>(std::max<int64_t>(0,
- Line 806: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if ((*parse_result) && (*parse_result)->traversal == nullptr && !(*parse_result)->for_nodes.empty())
- Line 819: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: resolveToLoopField = [&](const std::shared_ptr<Expression>& e)->std::optional<std::string> {
- Line 819: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: resolveToLoopField = [&](const std::shared_ptr<Expression>& e)->std::optional<std::string> {
- Line 824: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto* cur = fa->object.get();
- Line 825: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: while (auto* fa2 = dynamic_cast<FieldAccessExpr*>(cur)) { parts.push_back(fa2->field); cur = fa2->ob
- Line 831: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = letMap.find(v->name);
- Line 855: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: visit = [&](const std::shared_ptr<Expression>& ex){
- Line 903: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: auto* index_recommender = index_recommender_.load(std::memory_order_acquire);
- Line 985: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: evalExprToLiteral = [&](std::shared_ptr<themis::query::Expression> expr, nlohmann::json& out)->bool
- Line 985: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: evalExprToLiteral = [&](std::shared_ptr<themis::query::Expression> expr, nlohmann::json& out)->bool
- Line 1051: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: char buf[32]; std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday); return std::string(buf);
  Confidence: band=very_high; score=0.9
- Line 1051: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: char buf[32]; std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", tm.tm_year+1900, tm.tm_mon+1, tm.tm_
- Line 1120: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto parseSide = [&](std::shared_ptr<Expression> e, char& var, std::string& field) -> bool {
- Line 1120: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto parseSide = [&](std::shared_ptr<Expression> e, char& var, std::string& field) -> bool {
- Line 1123: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto* v = dynamic_cast<VariableExpr*>(fa->object.get());
- Line 1131: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto parseSimpleFromExpr = [&](std::shared_ptr<Expression> expr, SimplePred& out)->bool {
- Line 1131: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto parseSimpleFromExpr = [&](std::shared_ptr<Expression> expr, SimplePred& out)->bool {
- Line 1181: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto parseSide = [&](std::shared_ptr<Expression> e, char& var, std::string& field) -> bool {
- Line 1185: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto* v = dynamic_cast<VariableExpr*>(fa->object.get());
- Line 1256: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: case SimplePred::Op::Eq:  return aval == lit;
  Confidence: band=very_high; score=0.9
- Line 1257: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: case SimplePred::Op::Neq: return aval != lit;
  Confidence: band=very_high; score=0.9
- Line 1267: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: case SimplePred::Op::Eq:  return av == lit;
  Confidence: band=very_high; score=0.9
- Line 1268: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: case SimplePred::Op::Neq: return av != lit;
  Confidence: band=very_high; score=0.9
- Line 1399: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!fa->object) return false;  // Null-safety: Check shared_ptr is valid
- Line 1400: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto* v = dynamic_cast<const VariableExpr*>(fa->object.get());
- Line 1491: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (auto* objVar = dynamic_cast<VariableExpr*>(fa->object.get())) {
- Line 1492: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (objVar->name == "v" || objVar->name == "e") return true;
- Line 1495: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return usesVE(fa->object.get());
- Line 1544: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if ((*parse_result) && (*parse_result)->traversal == nullptr) {
- Line 1563: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::function<std::string(const std::shared_ptr<Expression>&, std::string&)> fieldFromFA = [&](const
- Line 1563: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::function<std::string(const std::shared_ptr<Expression>&, std::string&)> fieldFromFA = [&](const
- Line 1569: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto* cur = fa->object.get();
- Line 1572: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: cur = fa2->object.get();
- Line 1674: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: auto* stats_collector = stats_collector_.load(std::memory_order_acquire);
- Line 2030: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: res["entities"] = nlohmann::json::array();
- Line 2035: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (!blob && pk.find(':') == std::string::npos) {
- Line 2074: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = parent.find(cur);
- Line 2091: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (!blob && pk.find(':') == std::string::npos) {
- Line 2163: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: auto* stats_collector = stats_collector_.load(std::memory_order_acquire);
- Line 2202: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Provide "result" alias for compatibility with older clients/tests
  Confidence: band=high; score=0.8
- Line 2246: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto* cur = fa->object.get();
- Line 2249: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: cur = fa2->object.get();
- Line 2294: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: auto* stats_collector = stats_collector_.load(std::memory_order_acquire);
- Line 2328: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: storage_->scanPrefix(prefix, [&](std::string_view key, std::string_view value) -> bool {
- Line 2342: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: evalExpr = [&](const std::shared_ptr<themis::query::Expression>& e) -> nlohmann::json {
- Line 2342: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: evalExpr = [&](const std::shared_ptr<themis::query::Expression>& e) -> nlohmann::json {
- Line 2359: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto base = evalExpr(fa->object);
- Line 2367: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (const auto& [k, ce] : obj->fields) out[k] = evalExpr(ce);
- Line 2432: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: exprContainsFn = [&](const std::shared_ptr<themis::query::Expression>& expr, const std::string& name
- Line 2559: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: auto* stats_collector = stats_collector_.load(std::memory_order_acquire);
- Line 2617: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: plan_json["estimates"] = nlohmann::json::array();
- Line 2747: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto extractColumn = [&](const std::shared_ptr<themis::query::Expression>& expr)->std::string {
- Line 2752: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto* cur = fa->object.get();
- Line 2755: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: cur = fa2->object.get();
- Line 2841: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: const auto it = mp.find(a.var);
  Confidence: band=very_high; score=0.9
- Line 2881: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: extractColFromFA = [&](const std::shared_ptr<Expression>& expr, bool& rootedAtLoop)->std::optional<s
- Line 2881: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: extractColFromFA = [&](const std::shared_ptr<Expression>& expr, bool& rootedAtLoop)->std::optional<s
- Line 2884: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto* cur = fa->object.get();
- Line 2885: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: while (auto* fa2 = dynamic_cast<FieldAccessExpr*>(cur)) { parts.push_back(fa2->field); cur = fa2->ob
- Line 2894: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: containsFunction = [&](const std::shared_ptr<Expression>& expr, const std::string& name)->bool{
- Line 2978: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: evalExpr = [&](const std::shared_ptr<Expression>& expr,
- Line 2989: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto it = env.find(v->name); if (it != env.end()) return it->second; return nullptr; }
- Line 2999: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto base = evalExpr(fa->object, ent, env);
- Line 3001: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto it = base.find(fa->field); if (it != base.end()) return *it; return nullptr;
- Line 3158: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (const auto& kv : oc->fields) obj[kv.first] = evalExpr(kv.second, ent, env);
- Line 3271: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Provide "result" alias for compatibility
  Confidence: band=high; score=0.8
- Line 3424: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: enhanced_response["llm_context"] = json::array();
- Line 212: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: preds.push_back({p["column"].get<std::string>(), p["value"].get<std::string>()});
  Confidence: band=high; score=0.74
- Line 213: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: preds.push_back({p["column"].get<std::string>(), p["value"].get<std::string>()});
- Line 232: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rpreds.push_back(std::move(pr));
  Confidence: band=high; score=0.74
- Line 233: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rpreds.push_back(std::move(pr));
- Line 269: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::pair<themis::QueryEngine::Status, size_t> res;
- Line 292: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
  Confidence: band=high; score=0.74
- Line 293: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
- Line 296: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan_json["estimates"].push_back({
  Confidence: band=high; score=0.74
- Line 297: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan_json["estimates"].push_back({
- Line 316: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
  Confidence: band=high; score=0.74
- Line 317: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
- Line 335: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::pair<themis::QueryEngine::Status, std::vector<std::string>> res;
- Line 358: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
  Confidence: band=high; score=0.74
- Line 359: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
- Line 362: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan_json["estimates"].push_back({
  Confidence: band=high; score=0.74
- Line 363: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan_json["estimates"].push_back({
- Line 383: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
- Line 405: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: key_items.push_back(k);
  Confidence: band=high; score=0.74
- Line 406: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: key_items.push_back(k);
- Line 413: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::pair<themis::QueryEngine::Status, std::vector<themis::BaseEntity>> res;
- Line 436: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
  Confidence: band=high; score=0.74
- Line 437: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
- Line 440: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan_json["estimates"].push_back({
  Confidence: band=high; score=0.74
- Line 441: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan_json["estimates"].push_back({
- Line 460: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
  Confidence: band=high; score=0.74
- Line 461: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
- Line 482: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(e.toJson());
  Confidence: band=high; score=0.74
- Line 483: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entities.push_back(e.toJson());
- Line 493: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 498: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto coll = schema["collections"][table];
  Confidence: band=high; score=0.74
- Line 501: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto& f : coll["encryption"]["fields"]) if (f.is_string()) fields.push_back(f.get<std::string>(
- Line 513: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: try { obj = nlohmann::json::parse(e.toJson()); } catch (...) { entities.push_back(e.toJson()); continue; }
  Confidence: band=high; score=0.74
- Line 513: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: try { obj = nlohmann::json::parse(e.toJson()); } catch (...) { entities.push_back(e.toJson()); continue; }
  Confidence: band=high; score=0.74
- Line 514: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: try { obj = nlohmann::json::parse(e.toJson()); } catch (...) { entities.push_back(e.toJson()); conti
- Line 514: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { obj = nlohmann::json::parse(e.toJson()); } catch (...) { entities.push_back(e.toJson()); conti
- Line 521: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: bool encFlag = false; try { encFlag = obj[f + "_enc"].get<bool>(); } catch (...) { encFlag = false;
- Line 524: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto enc_meta_str = obj[f + "_encrypted"].get<std::string>();
  Confidence: band=high; score=0.74
- Line 529: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: std::string group_name; try { group_name = obj[f + "_group"].get<std::string>(); } catch (...) { gro
- Line 554: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 717: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: std::vector<std::string> parts; parts.push_back(fa->field);
- Line 719: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: while (auto* fa2 = dynamic_cast<FieldAccessExpr*>(cur)) { parts.push_back(fa2->field); cur = fa2->ob
- Line 768: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::pair<themis::QueryEngine::Status, std::vector<themis::BaseEntity>> res1;
- Line 777: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::pair<themis::QueryEngine::Status, std::vector<themis::BaseEntity>> res2;
- Line 792: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (buildLeft) { for (const auto& e : rightVec) { auto k = getFieldStr(e, colRight); if (!k.has_valu
- Line 793: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: else { for (const auto& e : leftVec) { auto k = getFieldStr(e, colLeft); if (!k.has_value()) continu
- Line 795: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nlohmann::json entities = nlohmann::json::array(); for (const auto& e : out) entities.push_back(e.to
- Line 813: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::shared_ptr<themis::query::Expression>> letMap;
  Confidence: band=medium; score=0.66
- Line 822: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: std::vector<std::string> parts; parts.push_back(fa->field);
  Confidence: band=high; score=0.74
- Line 823: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: std::vector<std::string> parts; parts.push_back(fa->field);
- Line 825: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: while (auto* fa2 = dynamic_cast<FieldAccessExpr*>(cur)) { parts.push_back(fa2->field); cur = fa2->ob
- Line 827: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: std::string col; for (auto it = parts.rbegin(); it != parts.rend(); ++it) { if (!col.empty()) col += "."; col += *it; }
  Confidence: band=high; score=0.74
- Line 978: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: char var = '\0'; // 'v' or 'e'
- Line 1307: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { return std::nullopt; }
- Line 1316: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { return std::nullopt; }
- Line 1349: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pathNodes.push_back(cur);
- Line 1352: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pathEdges.push_back(itp->second.edgeId);
- Line 1353: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pathNodes.push_back(itp->second.parent);
- Line 1568: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parts.push_back(fa->field);
- Line 1571: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parts.push_back(fa2->field);
- Line 1678: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::pair<themis::QueryEngine::Status, std::vector<themis::BaseEntity>> res1;
- Line 1691: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::pair<themis::QueryEngine::Status, std::vector<themis::BaseEntity>> res2;
- Line 1737: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (retVar == var1) out.push_back(l); else out.push_back(e);
  Confidence: band=high; score=0.74
- Line 1737: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (retVar == var1) out.push_back(l); else out.push_back(e);
  Confidence: band=high; score=0.74
- Line 1738: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (retVar == var1) out.push_back(l); else out.push_back(e);
- Line 1747: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (retVar == var1) out.push_back(e); else out.push_back(r);
- Line 1770: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& e : out) entities.push_back(e.toJson());
- Line 1805: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { return false; }
- Line 1823: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { return false; }
- Line 1848: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { return false; }
- Line 1864: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { return false; }
- Line 1944: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resultVertices.push_back(node);
  Confidence: band=high; score=0.74
- Line 1945: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: resultVertices.push_back(node);
- Line 2041: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: res["entities"].push_back(entity.toJson());
  Confidence: band=high; score=0.74
- Line 2042: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: res["entities"].push_back(entity.toJson());
- Line 2043: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2044: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: res["entities"].push_back(nlohmann::json({{"_key", pk}}));
- Line 2047: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: res["entities"].push_back(nlohmann::json({{"_key", pk}}));
- Line 2056: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: res["entities"].push_back(edgeEnt.toJson());
  Confidence: band=high; score=0.74
- Line 2057: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: res["entities"].push_back(edgeEnt.toJson());
- Line 2058: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2059: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: res["entities"].push_back(nlohmann::json({{"_edge", eid}}));
- Line 2062: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: res["entities"].push_back(nlohmann::json({{"_edge", eid}}));
- Line 2072: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vertices.push_back(cur);
  Confidence: band=high; score=0.74
- Line 2073: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vertices.push_back(cur);
- Line 2077: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edges.push_back(it->second.edgeId);
- Line 2079: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vertices.push_back(cur);
- Line 2097: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: jpath["vertices"].push_back(ent.toJson());
  Confidence: band=high; score=0.74
- Line 2098: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: jpath["vertices"].push_back(ent.toJson());
- Line 2099: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2100: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: jpath["vertices"].push_back(nlohmann::json({{"_key", pk}}));
- Line 2103: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: jpath["vertices"].push_back(nlohmann::json({{"_key", pk}}));
- Line 2112: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: jpath["edges"].push_back(eent.toJson());
  Confidence: band=high; score=0.74
- Line 2113: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: jpath["edges"].push_back(eent.toJson());
- Line 2114: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2115: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: jpath["edges"].push_back(nlohmann::json({{"_edge", eid}}));
- Line 2118: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: jpath["edges"].push_back(nlohmann::json({{"_edge", eid}}));
- Line 2122: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: res["entities"].push_back(std::move(jpath));
- Line 2167: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::pair<themis::QueryEngine::Status, std::vector<std::string>> statusKeys;
- Line 2189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(nlohmann::json::parse(entity.toJson()));
  Confidence: band=high; score=0.74
- Line 2190: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entities.push_back(nlohmann::json::parse(entity.toJson()));
- Line 2191: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2203: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { response_body["result"] = response_body["entities"]; } catch (...) { /* ignore */ }
- Line 2244: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: parts.push_back(fa->field);
  Confidence: band=high; score=0.74
- Line 2245: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parts.push_back(fa->field);
- Line 2248: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parts.push_back(fa2->field);
- Line 2256: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!col.empty()) col += ".";
  Confidence: band=high; score=0.74
- Line 2256: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!col.empty()) col += ".";
  Confidence: band=high; score=0.74
- Line 2257: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!col.empty()) col += ".";
- Line 2257: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!col.empty()) col += ".";
- Line 2273: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cq.predicates.push_back({col, litToString(lit->value)});
  Confidence: band=high; score=0.74
- Line 2274: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cq.predicates.push_back({col, litToString(lit->value)});
- Line 2313: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(result);
  Confidence: band=high; score=0.74
- Line 2314: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entities.push_back(result);
- Line 2340: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, nlohmann::json> letValues;
  Confidence: band=high; score=0.74
- Line 2372: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& ce : arr->elements) a.push_back(evalExpr(ce));
  Confidence: band=high; score=0.74
- Line 2373: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& ce : arr->elements) a.push_back(evalExpr(ce));
- Line 2384: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(projected);
  Confidence: band=high; score=0.74
- Line 2384: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(projected);
  Confidence: band=high; score=0.74
- Line 2385: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entities.push_back(projected);
- Line 2386: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2547: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2565: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::pair<themis::QueryEngine::Status, std::vector<themis::BaseEntity>> res;
- Line 2614: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
  Confidence: band=high; score=0.74
- Line 2615: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
- Line 2618: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan_json["estimates"].push_back({
  Confidence: band=high; score=0.74
- Line 2619: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan_json["estimates"].push_back({
- Line 2640: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
- Line 2754: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parts.push_back(fa2->field);
- Line 2759: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!col.empty()) col += ".";
  Confidence: band=high; score=0.74
- Line 2760: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!col.empty()) col += ".";
- Line 2783: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: aggs.push_back({a.varName, func, col});
  Confidence: band=high; score=0.74
- Line 2784: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: aggs.push_back({a.varName, func, col});
- Line 2788: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::unordered_map<std::string, AggState>> acc;
  Confidence: band=medium; score=0.66
- Line 2801: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { out = std::stod(*sv); return true; } catch (...) { /* ignore */ }
- Line 2850: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: groups.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 2850: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: groups.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 2851: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: groups.push_back(std::move(row));
- Line 2883: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: std::vector<std::string> parts; parts.push_back(fa->field);
- Line 2885: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: while (auto* fa2 = dynamic_cast<FieldAccessExpr*>(cur)) { parts.push_back(fa2->field); cur = fa2->ob
- Line 3089: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: else if (a.is_boolean()) out += (a.get<bool>()?"true":"false");
  Confidence: band=high; score=0.74
- Line 3162: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& el : ar->elements) arr.push_back(evalExpr(el, ent, env));
  Confidence: band=high; score=0.74
- Line 3162: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& el : ar->elements) arr.push_back(evalExpr(el, ent, env));
  Confidence: band=high; score=0.74
- Line 3163: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& el : ar->elements) arr.push_back(evalExpr(el, ent, env));
- Line 3179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& e : sliced) entities.push_back(e.toJson());
  Confidence: band=high; score=0.74
- Line 3180: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& e : sliced) entities.push_back(e.toJson());
- Line 3184: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, nlohmann::json> env;
  Confidence: band=medium; score=0.66
- Line 3192: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(std::move(out));
  Confidence: band=high; score=0.74
- Line 3192: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(std::move(out));
  Confidence: band=high; score=0.74
- Line 3192: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(std::move(out));
  Confidence: band=high; score=0.74
- Line 3193: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entities.push_back(std::move(out));
- Line 3195: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entities.push_back(e.toJson());
- Line 3226: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& e : sliced) page_items.push_back(e.toJson());
- Line 3252: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 3272: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { response_body["result"] = response_body["entities"]; } catch (...) { /* ignore */ }
- Line 3281: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { plan_json["let_pre_extracted"] = true; } catch (...) { /* noop */ }
- Line 3438: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: enhanced_response["llm_context"].push_back(llm_entry);
  Confidence: band=high; score=0.74
- Line 3439: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: enhanced_response["llm_context"].push_back(llm_entry);
- Line 3476: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 3512: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 3569: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: decoded += ' ';
  Confidence: band=high; score=0.74
- Line 3570: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: decoded += ' ';
- Line 3594: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { return def; }
- Line 3614: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_fwd = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 3628: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/import_wizard_builder.cpp
Total findings: 220

- Line 0: severity=HIGH; category=uncategorized
  Context: ['    html += "<!DOCTYPE html>\\n<html lang=\\"en\\">\\n<head>\\n";', '    html += "<meta charset=\\"UTF-8\\">\\n";', '    html += "<meta name=\\"viewport\\" content=\\"width=device-width,initial-scale=1\\">\\n";', '    html += "<title>ThemisDB Import Wizard</title>\\n";', '    html += "<style>\\n";']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 36: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<title>ThemisDB Import Wizard</title>\n";
- Line 118: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</style>\n</head>\n<body>\n";
- Line 122: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<h1>&#128190; ThemisDB Import Wizard</h1>\n";
- Line 123: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<p class=\"sub\">Import data from PostgreSQL or S3 into ThemisDB in a few guided steps.</p>
- Line 123: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<p class=\"sub\">Import data from PostgreSQL or S3 into ThemisDB in a few guided steps.</p>
- Line 127: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"step active\" id=\"step-tab-1\">1&nbsp;Source</div>\n";
- Line 127: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"step active\" id=\"step-tab-1\">1&nbsp;Source</div>\n";
- Line 127: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"step active\" id=\"step-tab-1\">1&nbsp;Source</div>\n";
- Line 128: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"step\" id=\"step-tab-2\">2&nbsp;Connection</div>\n";
- Line 128: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"step\" id=\"step-tab-2\">2&nbsp;Connection</div>\n";
- Line 128: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"step\" id=\"step-tab-2\">2&nbsp;Connection</div>\n";
- Line 129: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"step\" id=\"step-tab-3\">3&nbsp;Options</div>\n";
- Line 129: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"step\" id=\"step-tab-3\">3&nbsp;Options</div>\n";
- Line 129: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"step\" id=\"step-tab-3\">3&nbsp;Options</div>\n";
- Line 130: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"step\" id=\"step-tab-4\">4&nbsp;Review</div>\n";
- Line 130: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"step\" id=\"step-tab-4\">4&nbsp;Review</div>\n";
- Line 130: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"step\" id=\"step-tab-4\">4&nbsp;Review</div>\n";
- Line 131: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"step\" id=\"step-tab-5\">5&nbsp;Progress</div>\n";
- Line 131: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"step\" id=\"step-tab-5\">5&nbsp;Progress</div>\n";
- Line 131: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"step\" id=\"step-tab-5\">5&nbsp;Progress</div>\n";
- Line 132: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";
- Line 136: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<h2>Choose a data source</h2>\n";
- Line 139: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <div class=\"icon\">&#128036;</div>\n";
- Line 139: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <div class=\"icon\">&#128036;</div>\n";
- Line 140: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <div class=\"name\">PostgreSQL</div>\n";
- Line 140: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <div class=\"name\">PostgreSQL</div>\n";
- Line 141: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <div class=\"desc\">pg_dump file or connection string</div>\n";
- Line 141: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <div class=\"desc\">pg_dump file or connection string</div>\n";
- Line 142: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  </div>\n";
- Line 144: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <div class=\"icon\">&#9729;</div>\n";
- Line 144: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <div class=\"icon\">&#9729;</div>\n";
- Line 145: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <div class=\"name\">S3 / Object Storage</div>\n";
- Line 145: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <div class=\"name\">S3 / Object Storage</div>\n";
- Line 146: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <div class=\"desc\">s3://bucket/key (CSV, TSV, JSONL)</div>\n";
- Line 146: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <div class=\"desc\">s3://bucket/key (CSV, TSV, JSONL)</div>\n";
- Line 147: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  </div>\n";
- Line 148: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";
- Line 149: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div class=\"btn-row\"><button class=\"btn btn-primary\" onclick=\"goStep(2)\">Next &rarr;<
- Line 149: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div class=\"btn-row\"><button class=\"btn btn-primary\" onclick=\"goStep(2)\">Next &rarr;<
- Line 149: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div class=\"btn-row\"><button class=\"btn btn-primary\" onclick=\"goStep(2)\">Next &rarr;<
- Line 149: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div class=\"btn-row\"><button class=\"btn btn-primary\" onclick=\"goStep(2)\">Next &rarr;<
- Line 150: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";  // panel-1
- Line 154: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<h2>Configure source</h2>\n";
- Line 156: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label for=\"pg-path\">pg_dump file path or PostgreSQL connection string</label>\n";
- Line 156: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label for=\"pg-path\">pg_dump file path or PostgreSQL connection string</label>\n";
- Line 157: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <input id=\"pg-path\" type=\"text\" placeholder=\"/path/to/dump.sql  or  postgresql://use
- Line 157: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <input id=\"pg-path\" type=\"text\" placeholder=\"/path/to/dump.sql  or  postgresql://use
- Line 157: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <input id=\"pg-path\" type=\"text\" placeholder=\"/path/to/dump.sql  or  postgresql://use
- Line 157: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <input id=\"pg-path\" type=\"text\" placeholder=\"/path/to/dump.sql  or  postgresql://use
- Line 158: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <p style=\"font-size:0.78rem;color:#7a7aaa;margin-top:4px\">The path is resolved on the T
- Line 158: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <p style=\"font-size:0.78rem;color:#7a7aaa;margin-top:4px\">The path is resolved on the T
- Line 159: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";
- Line 161: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label for=\"s3-path\">S3 URL</label>\n";
- Line 161: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label for=\"s3-path\">S3 URL</label>\n";
- Line 162: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <input id=\"s3-path\" type=\"text\" placeholder=\"s3://my-bucket/export/dump.csv\">\n";
- Line 162: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <input id=\"s3-path\" type=\"text\" placeholder=\"s3://my-bucket/export/dump.csv\">\n";
- Line 162: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <input id=\"s3-path\" type=\"text\" placeholder=\"s3://my-bucket/export/dump.csv\">\n";
- Line 162: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <input id=\"s3-path\" type=\"text\" placeholder=\"s3://my-bucket/export/dump.csv\">\n";
- Line 163: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <p style=\"font-size:0.78rem;color:#7a7aaa;margin-top:4px\">Use a trailing / to import an
- Line 163: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <p style=\"font-size:0.78rem;color:#7a7aaa;margin-top:4px\">Use a trailing / to import an
- Line 164: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";
- Line 165: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div id=\"step2-error\" class=\"alert alert-error\" style=\"display:none\"></div>\n";
- Line 165: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div id=\"step2-error\" class=\"alert alert-error\" style=\"display:none\"></div>\n";
- Line 165: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div id=\"step2-error\" class=\"alert alert-error\" style=\"display:none\"></div>\n";
- Line 165: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div id=\"step2-error\" class=\"alert alert-error\" style=\"display:none\"></div>\n";
- Line 167: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-secondary\" onclick=\"goStep(1)\">&larr; Back</button>\n";
- Line 167: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-secondary\" onclick=\"goStep(1)\">&larr; Back</button>\n";
- Line 167: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-secondary\" onclick=\"goStep(1)\">&larr; Back</button>\n";
- Line 168: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-primary\" onclick=\"goStep(3)\">Next &rarr;</button>\n";
- Line 168: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-primary\" onclick=\"goStep(3)\">Next &rarr;</button>\n";
- Line 168: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-primary\" onclick=\"goStep(3)\">Next &rarr;</button>\n";
- Line 169: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";
- Line 170: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";  // panel-2
- Line 174: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<h2>Import options</h2>\n";
- Line 175: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<label for=\"opt-namespace\">Target namespace</label>\n";
- Line 175: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<label for=\"opt-namespace\">Target namespace</label>\n";
- Line 177: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<label for=\"opt-batch\">Batch size (rows per transaction)</label>\n";
- Line 177: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<label for=\"opt-batch\">Batch size (rows per transaction)</label>\n";
- Line 181: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label for=\"opt-dryrun\">Dry-run (validate without writing data)</label>\n";
- Line 181: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label for=\"opt-dryrun\">Dry-run (validate without writing data)</label>\n";
- Line 182: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";
- Line 185: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label for=\"opt-skip-dup\">Skip duplicate records</label>\n";
- Line 185: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label for=\"opt-skip-dup\">Skip duplicate records</label>\n";
- Line 186: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";
- Line 189: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label for=\"opt-continue-err\">Continue on row errors</label>\n";
- Line 189: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label for=\"opt-continue-err\">Continue on row errors</label>\n";
- Line 190: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";
- Line 191: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<details><summary>&#9881; Advanced options</summary>\n";
- Line 192: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<label for=\"opt-include\">Include tables (comma-separated, empty = all)</label>\n";
- Line 192: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<label for=\"opt-include\">Include tables (comma-separated, empty = all)</label>\n";
- Line 194: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<label for=\"opt-exclude\">Exclude tables (comma-separated)</label>\n";
- Line 194: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<label for=\"opt-exclude\">Exclude tables (comma-separated)</label>\n";
- Line 196: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<label for=\"opt-conflict\">Conflict strategy</label>\n";
- Line 196: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<label for=\"opt-conflict\">Conflict strategy</label>\n";
- Line 198: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <option value=\"0\" selected>Overwrite (replace existing)</option>\n";
- Line 198: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <option value=\"0\" selected>Overwrite (replace existing)</option>\n";
- Line 199: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <option value=\"1\">Skip (keep existing)</option>\n";
- Line 199: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <option value=\"1\">Skip (keep existing)</option>\n";
- Line 200: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <option value=\"2\">Merge (field-level, incoming wins)</option>\n";
- Line 200: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <option value=\"2\">Merge (field-level, incoming wins)</option>\n";
- Line 201: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <option value=\"3\">Error (abort on first conflict)</option>\n";
- Line 201: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <option value=\"3\">Error (abort on first conflict)</option>\n";
- Line 202: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</select>\n";
- Line 203: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</details>\n";
- Line 205: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-secondary\" onclick=\"goStep(2)\">&larr; Back</button>\n";
- Line 205: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-secondary\" onclick=\"goStep(2)\">&larr; Back</button>\n";
- Line 205: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-secondary\" onclick=\"goStep(2)\">&larr; Back</button>\n";
- Line 206: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-primary\" onclick=\"goStep(4)\">Review &rarr;</button>\n";
- Line 206: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-primary\" onclick=\"goStep(4)\">Review &rarr;</button>\n";
- Line 206: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-primary\" onclick=\"goStep(4)\">Review &rarr;</button>\n";
- Line 207: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";
- Line 208: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";  // panel-3
- Line 212: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<h2>Review &amp; start import</h2>\n";
- Line 214: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "background:#0f1829;border-radius:4px;padding:14px\"></div>\n";
- Line 216: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-secondary\" onclick=\"goStep(3)\">&larr; Back</button>\n";
- Line 216: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-secondary\" onclick=\"goStep(3)\">&larr; Back</button>\n";
- Line 216: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-secondary\" onclick=\"goStep(3)\">&larr; Back</button>\n";
- Line 217: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-primary\" id=\"btn-start\" onclick=\"startImport()\">&#9654; Star
- Line 217: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-primary\" id=\"btn-start\" onclick=\"startImport()\">&#9654; Star
- Line 217: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-primary\" id=\"btn-start\" onclick=\"startImport()\">&#9654; Star
- Line 217: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-primary\" id=\"btn-start\" onclick=\"startImport()\">&#9654; Star
- Line 218: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";
- Line 219: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";  // panel-4
- Line 223: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<h2>Import progress</h2>\n";
- Line 226: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <span id=\"prog-stage\" style=\"font-size:0.85rem;color:#aaaacc;font-style:italic\">Ini
- Line 226: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <span id=\"prog-stage\" style=\"font-size:0.85rem;color:#aaaacc;font-style:italic\">Ini
- Line 226: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <span id=\"prog-stage\" style=\"font-size:0.85rem;color:#aaaacc;font-style:italic\">Ini
- Line 227: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <span id=\"prog-badge\" class=\"status-badge s-running\">running</span>\n";
- Line 227: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <span id=\"prog-badge\" class=\"status-badge s-running\">running</span>\n";
- Line 227: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <span id=\"prog-badge\" class=\"status-badge s-running\">running</span>\n";
- Line 228: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  </div>\n";
- Line 229: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"progress-bar-wrap\"><div class=\"progress-bar\" id=\"prog-bar\" style=\"widt
- Line 229: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"progress-bar-wrap\"><div class=\"progress-bar\" id=\"prog-bar\" style=\"widt
- Line 229: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"progress-bar-wrap\"><div class=\"progress-bar\" id=\"prog-bar\" style=\"widt
- Line 229: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"progress-bar-wrap\"><div class=\"progress-bar\" id=\"prog-bar\" style=\"widt
- Line 229: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"progress-bar-wrap\"><div class=\"progress-bar\" id=\"prog-bar\" style=\"widt
- Line 231: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "<span id=\"prog-current\">0</span> / <span id=\"prog-total\">?</span> records</div>\n";
- Line 231: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "<span id=\"prog-current\">0</span> / <span id=\"prog-total\">?</span> records</div>\n";
- Line 231: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "<span id=\"prog-current\">0</span> / <span id=\"prog-total\">?</span> records</div>\n";
- Line 232: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";
- Line 233: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div id=\"result-box\" style=\"display:none;margin-top:18px\"></div>\n";
- Line 233: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div id=\"result-box\" style=\"display:none;margin-top:18px\"></div>\n";
- Line 233: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div id=\"result-box\" style=\"display:none;margin-top:18px\"></div>\n";
- Line 235: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-secondary\" id=\"btn-new\" onclick=\"resetWizard()\">&#8635; New
- Line 235: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-secondary\" id=\"btn-new\" onclick=\"resetWizard()\">&#8635; New
- Line 235: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-secondary\" id=\"btn-new\" onclick=\"resetWizard()\">&#8635; New
- Line 235: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-secondary\" id=\"btn-new\" onclick=\"resetWizard()\">&#8635; New
- Line 236: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-danger\" id=\"btn-cancel\" onclick=\"cancelImport()\">&#9632; Can
- Line 236: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-danger\" id=\"btn-cancel\" onclick=\"cancelImport()\">&#9632; Can
- Line 236: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-danger\" id=\"btn-cancel\" onclick=\"cancelImport()\">&#9632; Can
- Line 236: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-danger\" id=\"btn-cancel\" onclick=\"cancelImport()\">&#9632; Can
- Line 237: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";
- Line 238: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";  // panel-5
- Line 240: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";  // card
- Line 244: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<h2>Recent import jobs</h2>\n";
- Line 245: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div id=\"jobs-list\"><em style=\"color:#555\">Loading&#8230;</em></div>\n";
- Line 245: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div id=\"jobs-list\"><em style=\"color:#555\">Loading&#8230;</em></div>\n";
- Line 245: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div id=\"jobs-list\"><em style=\"color:#555\">Loading&#8230;</em></div>\n";
- Line 247: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "<button class=\"btn btn-secondary\" style=\"font-size:0.8rem\" onclick=\"loadJobs()\">&#8635; Refre
- Line 247: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "<button class=\"btn btn-secondary\" style=\"font-size:0.8rem\" onclick=\"loadJobs()\">&#8635; Refre
- Line 247: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "<button class=\"btn btn-secondary\" style=\"font-size:0.8rem\" onclick=\"loadJobs()\">&#8635; Refre
- Line 247: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "<button class=\"btn btn-secondary\" style=\"font-size:0.8rem\" onclick=\"loadJobs()\">&#8635; Refre
- Line 248: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "</div>\n";
- Line 249: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";  // jobs-panel
- Line 270: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: html += "    var p=document.getElementById('panel-'+i);\n";
  Confidence: band=high; score=0.74
- Line 295: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  function row(k,v){return '<tr><td style=\"color:#7a7aaa;padding:3px 8px 3px 0;white-space
- Line 295: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  function row(k,v){return '<tr><td style=\"color:#7a7aaa;padding:3px 8px 3px 0;white-space
- Line 296: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "<td style=\"color:#d0d0e8;padding:3px 0\">'+v+'</td></tr>';}\n";
- Line 296: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "<td style=\"color:#d0d0e8;padding:3px 0\">'+v+'</td></tr>';}\n";
- Line 297: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  html+=row('Source type',currentSource==='postgresql'?'PostgreSQL':'S3 / Object Storage');
- Line 298: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  html+=row('Source path','<code>'+escHtml(path)+'</code>');\n";
- Line 299: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  html+=row('Target namespace','<code>'+escHtml(ns)+'</code>');\n";
- Line 301: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  html+=row('Dry-run',dr?'<span style=\"color:#ffcc00\">Yes (no data written)</span>':'No')
- Line 301: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  html+=row('Dry-run',dr?'<span style=\"color:#ffcc00\">Yes (no data written)</span>':'No')
- Line 301: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  html+=row('Dry-run',dr?'<span style=\"color:#ffcc00\">Yes (no data written)</span>':'No')
- Line 304: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  if(inc) html+=row('Include tables','<code>'+escHtml(inc)+'</code>');\n";
- Line 305: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  if(exc) html+=row('Exclude tables','<code>'+escHtml(exc)+'</code>');\n";
- Line 307: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  html+='</table>';\n";
- Line 313: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  return String(s).replace(/&/g,'&amp;').replace(/</g,'<').replace(/>/g,'>');\n";
- Line 313: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  return String(s).replace(/&/g,'&amp;').replace(/</g,'<').replace(/>/g,'>');\n";
- Line 313: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  return String(s).replace(/&/g,'&amp;').replace(/</g,'<').replace(/>/g,'>');\n";
- Line 313: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  return String(s).replace(/&/g,'&amp;').replace(/</g,'<').replace(/>/g,'>');\n";
- Line 339: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  var url=(currentSource==='postgresql')?'/api/v1/import/postgresql':'/api/v1/import/s3';\n
- Line 339: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  var url=(currentSource==='postgresql')?'/api/v1/import/postgresql':'/api/v1/import/s3';\n
- Line 342: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  fetch(url,{method:'POST',headers:{'Content-Type':'application/json'},body:body})\n";
- Line 356: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  fetch('/api/v1/import/'+currentJobId+'/status')\n";
- Line 356: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  fetch('/api/v1/import/'+currentJobId+'/status')\n";
- Line 377: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  var pct=(tot>0)?Math.min(100,Math.round(cur/tot*100)):0;\n";
- Line 394: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    box.innerHTML+='<strong>Import complete</strong><br>';\n";
- Line 395: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    box.innerHTML+='Imported: <b>'+s.imported_records+'</b> &nbsp; ';\n";
- Line 396: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    box.innerHTML+='Skipped: <b>'+s.skipped_records+'</b> &nbsp; ';\n";
- Line 397: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    box.innerHTML+='Failed: <b>'+s.failed_records+'</b> &nbsp; ';\n";
- Line 398: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    box.innerHTML+='Time: <b>'+(s.elapsed_seconds||0).toFixed(2)+'s</b>';\n";
- Line 399: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    box.innerHTML+='</div>';\n";
- Line 401: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    box.innerHTML='<div class=\"alert alert-error\">Import '+data.status+'.</div>';\n";
- Line 401: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    box.innerHTML='<div class=\"alert alert-error\">Import '+data.status+'.</div>';\n";
- Line 409: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  fetch('/api/v1/import/'+currentJobId+'/cancel',{method:'POST'})\n";
- Line 409: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  fetch('/api/v1/import/'+currentJobId+'/cancel',{method:'POST'})\n";
- Line 409: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  fetch('/api/v1/import/'+currentJobId+'/cancel',{method:'POST'})\n";
- Line 430: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  fetch('/api/v1/import/jobs')\n";
- Line 434: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      if(!jobs.length){el.innerHTML='<em style=\"color:#555\">No jobs yet.</em>';return;}\n
- Line 434: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      if(!jobs.length){el.innerHTML='<em style=\"color:#555\">No jobs yet.</em>';return;}\n
- Line 439: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        html+='<span class=\"job-id\" title=\"'+escHtml(j.id)+'\">'+escHtml(j.id)+'</span>'
- Line 439: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        html+='<span class=\"job-id\" title=\"'+escHtml(j.id)+'\">'+escHtml(j.id)+'</span>'
- Line 439: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        html+='<span class=\"job-id\" title=\"'+escHtml(j.id)+'\">'+escHtml(j.id)+'</span>'
- Line 439: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        html+='<span class=\"job-id\" title=\"'+escHtml(j.id)+'\">'+escHtml(j.id)+'</span>'
- Line 440: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        html+='<span class=\"status-badge s-'+(j.status||'pending')+'\">'+escHtml(j.status)
- Line 440: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        html+='<span class=\"status-badge s-'+(j.status||'pending')+'\">'+escHtml(j.status)
- Line 440: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        html+='<span class=\"status-badge s-'+(j.status||'pending')+'\">'+escHtml(j.status)
- Line 442: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "          html+='<span class=\"job-stats\">'+stats.imported_records+' imported, '+stats.fai
- Line 442: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "          html+='<span class=\"job-stats\">'+stats.imported_records+' imported, '+stats.fai
- Line 444: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "          html+='<span class=\"job-stage\">'+escHtml(j.stage||'')+'</span>';\n";
- Line 444: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "          html+='<span class=\"job-stage\">'+escHtml(j.stage||'')+'</span>';\n";
- Line 446: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        html+='</div>';\n";
- Line 449: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    }).catch(function(e){document.getElementById('jobs-list').innerHTML='<em style=\"color:
- Line 449: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    }).catch(function(e){document.getElementById('jobs-list').innerHTML='<em style=\"color:
- Line 454: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</script>\n";
- Line 455: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</body>\n</html>\n";

### src/server/task_scheduler_api_handler.cpp
Total findings: 204

- Line 604: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Use std::unique_ptr<T> or std::make_unique<T>()
  Context: html += "  return new Date(iso).toLocaleString();\n";
- Line 604: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: html += "  return new Date(iso).toLocaleString();\n";
- Line 648: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Use std::unique_ptr<T> or std::make_unique<T>()
  Context: html += "  document.getElementById('refresh-indicator').textContent = 'Updated ' + new Date().toLoca
- Line 648: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: html += "  document.getElementById('refresh-indicator').textContent = 'Updated ' + new Date().toLoca
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    html += "<!DOCTYPE html>\\n<html lang=\\"en\\">\\n<head>\\n";', '    html += "<meta charset=\\"UTF-8\\">\\n";', '    html += "<meta name=\\"viewport\\" content=\\"width=device-width, initial-scale=1\\">\\n";', '    html += "<title>ThemisDB – Task Scheduler</title>\\n";', '    html += "<style>\\n";']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #3178 [WIP] Add web UI for task management in scheduler (2026-03-12T06:26:01Z)
- Line 58: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(std::string(field_name) + " must be a positive integer");
- Line 63: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(std::string(field_name) + " is too large");
- Line 807: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(*err);
- Line 826: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(*err);
- Line 831: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("aql_query must not be empty for aql_query tasks");
- Line 835: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("aql_query exceeds maximum allowed length");
- Line 838: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("aql_query contains potentially unsafe patterns");
- Line 865: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("interval_ms must be a positive integer");
- Line 877: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("timeout_ms must be a positive integer");
- Line 889: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(*err);
- Line 898: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(*err);
- Line 147: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(taskToJson(t));
  Confidence: band=high; score=0.74
- Line 148: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: items.push_back(taskToJson(t));
- Line 315: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(r.toJson());
  Confidence: band=high; score=0.74
- Line 316: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: items.push_back(r.toJson());
- Line 373: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { return def; }
- Line 405: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: else if (v.is_string()) { try { ms = std::stoll(v.get<std::string>()); } catch (...) {} }
- Line 412: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: else if (v.is_string()) { try { ms = std::stoll(v.get<std::string>()); } catch (...) {} }
- Line 416: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: params.sort_by = scheduler::AuditQueryParams::SortBy::TIMESTAMP_DESC;
  Confidence: band=high; score=0.74
- Line 430: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: items.push_back(ev.toJson(false));
- Line 439: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: html += "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n";
  Confidence: band=high; score=0.74
- Line 443: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<title>ThemisDB – Task Scheduler</title>\n";
- Line 488: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</style>\n</head>\n<body>\n";
- Line 491: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <h1>&#x23F2; Task Scheduler</h1>\n";
- Line 492: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <span class=\"badge\">ThemisDB</span>\n";
- Line 492: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <span class=\"badge\">ThemisDB</span>\n";
- Line 493: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</header>\n";
- Line 499: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-registered\">–</div><div class=\"lbl\"
- Line 499: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-registered\">–</div><div class=\"lbl\"
- Line 499: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-registered\">–</div><div class=\"lbl\"
- Line 499: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-registered\">–</div><div class=\"lbl\"
- Line 499: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-registered\">–</div><div class=\"lbl\"
- Line 500: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-active\">–</div><div class=\"lbl\">Act
- Line 500: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-active\">–</div><div class=\"lbl\">Act
- Line 500: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-active\">–</div><div class=\"lbl\">Act
- Line 500: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-active\">–</div><div class=\"lbl\">Act
- Line 500: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-active\">–</div><div class=\"lbl\">Act
- Line 501: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-running\">–</div><div class=\"lbl\">Ru
- Line 501: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-running\">–</div><div class=\"lbl\">Ru
- Line 501: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-running\">–</div><div class=\"lbl\">Ru
- Line 501: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-running\">–</div><div class=\"lbl\">Ru
- Line 501: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-running\">–</div><div class=\"lbl\">Ru
- Line 502: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-total\">–</div><div class=\"lbl\">Exec
- Line 502: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-total\">–</div><div class=\"lbl\">Exec
- Line 502: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-total\">–</div><div class=\"lbl\">Exec
- Line 502: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-total\">–</div><div class=\"lbl\">Exec
- Line 502: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-total\">–</div><div class=\"lbl\">Exec
- Line 503: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-failed\">–</div><div class=\"lbl\">Fai
- Line 503: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-failed\">–</div><div class=\"lbl\">Fai
- Line 503: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-failed\">–</div><div class=\"lbl\">Fai
- Line 503: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-failed\">–</div><div class=\"lbl\">Fai
- Line 503: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-failed\">–</div><div class=\"lbl\">Fai
- Line 504: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-status\">–</div><div class=\"lbl\">Sch
- Line 504: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-status\">–</div><div class=\"lbl\">Sch
- Line 504: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-status\">–</div><div class=\"lbl\">Sch
- Line 504: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-status\">–</div><div class=\"lbl\">Sch
- Line 504: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-status\">–</div><div class=\"lbl\">Sch
- Line 505: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";
- Line 509: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn-primary\" onclick=\"openCreateDialog()\">&#43; New Task</button>\n";
- Line 509: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn-primary\" onclick=\"openCreateDialog()\">&#43; New Task</button>\n";
- Line 509: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn-primary\" onclick=\"openCreateDialog()\">&#43; New Task</button>\n";
- Line 510: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn-secondary\" onclick=\"loadAll()\">&#8635; Refresh</button>\n";
- Line 510: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn-secondary\" onclick=\"loadAll()\">&#8635; Refresh</button>\n";
- Line 510: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn-secondary\" onclick=\"loadAll()\">&#8635; Refresh</button>\n";
- Line 511: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <span id=\"refresh-indicator\"></span>\n";
- Line 511: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <span id=\"refresh-indicator\"></span>\n";
- Line 512: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";
- Line 516: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <th>ID / Name</th><th>Type</th><th>Trigger</th><th>Status</th>\n";
- Line 517: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <th>Executions</th><th>Last Error</th><th>Next Run</th><th>Actions</th>\n";
- Line 518: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</tr></thead>\n";
- Line 519: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<tbody id=\"task-table-body\"><tr><td colspan=\"8\" style=\"text-align:center;color:#64748b
- Line 519: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<tbody id=\"task-table-body\"><tr><td colspan=\"8\" style=\"text-align:center;color:#64748b
- Line 519: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<tbody id=\"task-table-body\"><tr><td colspan=\"8\" style=\"text-align:center;color:#64748b
- Line 519: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<tbody id=\"task-table-body\"><tr><td colspan=\"8\" style=\"text-align:center;color:#64748b
- Line 520: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</table>\n";
- Line 522: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n"; // end .container
- Line 525: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div class=\"toast\" id=\"toast\"></div>\n";
- Line 525: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div class=\"toast\" id=\"toast\"></div>\n";
- Line 525: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div class=\"toast\" id=\"toast\"></div>\n";
- Line 529: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <h2 id=\"dialog-title\">New Task</h2>\n";
- Line 529: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <h2 id=\"dialog-title\">New Task</h2>\n";
- Line 531: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label>Name</label>\n";
- Line 533: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label>Description</label>\n";
- Line 535: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label>Type</label>\n";
- Line 537: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <option value=\"aql_query\">AQL Query</option>\n";
- Line 537: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <option value=\"aql_query\">AQL Query</option>\n";
- Line 538: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <option value=\"function\">Function</option>\n";
- Line 538: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <option value=\"function\">Function</option>\n";
- Line 539: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  </select>\n";
- Line 541: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <label>AQL Query</label>\n";
- Line 542: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <textarea id=\"f-aql\" placeholder=\"FOR d IN collection RETURN d\"></textarea>\n";
- Line 542: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <textarea id=\"f-aql\" placeholder=\"FOR d IN collection RETURN d\"></textarea>\n";
- Line 542: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <textarea id=\"f-aql\" placeholder=\"FOR d IN collection RETURN d\"></textarea>\n";
- Line 543: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  </div>\n";
- Line 545: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <label>Function Name</label>\n";
- Line 547: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  </div>\n";
- Line 548: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label>Trigger</label>\n";
- Line 550: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <option value=\"interval\">Interval</option>\n";
- Line 550: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <option value=\"interval\">Interval</option>\n";
- Line 551: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <option value=\"cron\">Cron</option>\n";
- Line 551: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <option value=\"cron\">Cron</option>\n";
- Line 552: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <option value=\"manual\">Manual</option>\n";
- Line 552: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <option value=\"manual\">Manual</option>\n";
- Line 553: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  </select>\n";
- Line 555: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <label>Interval (seconds)</label>\n";
- Line 557: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  </div>\n";
- Line 559: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <label>Cron Expression</label>\n";
- Line 560: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <input id=\"f-cron\" type=\"text\" placeholder=\"*/5 * * * *\">\n";
- Line 560: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <input id=\"f-cron\" type=\"text\" placeholder=\"*/5 * * * *\">\n";
- Line 560: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <input id=\"f-cron\" type=\"text\" placeholder=\"*/5 * * * *\">\n";
- Line 560: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <input id=\"f-cron\" type=\"text\" placeholder=\"*/5 * * * *\">\n";
- Line 561: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  </div>\n";
- Line 562: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label>Timeout (seconds)</label>\n";
- Line 564: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label>Max Retries</label>\n";
- Line 567: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <button class=\"btn-secondary\" onclick=\"closeDialog()\">Cancel</button>\n";
- Line 567: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <button class=\"btn-secondary\" onclick=\"closeDialog()\">Cancel</button>\n";
- Line 567: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <button class=\"btn-secondary\" onclick=\"closeDialog()\">Cancel</button>\n";
- Line 568: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <button class=\"btn-primary\" onclick=\"saveTask()\">Save</button>\n";
- Line 568: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <button class=\"btn-primary\" onclick=\"saveTask()\">Save</button>\n";
- Line 568: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <button class=\"btn-primary\" onclick=\"saveTask()\">Save</button>\n";
- Line 569: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  </div>\n";
- Line 570: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</dialog>\n";
- Line 574: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "const API = '/api/tasks';\n";
- Line 577: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  const opts = { method, headers: {'Content-Type':'application/json'} };\n";
- Line 591: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  const s = await api('GET', API + '/stats');\n";
- Line 611: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    tbody.innerHTML = '<tr><td colspan=\"8\" style=\"text-align:center;color:#64748b\">No t
- Line 611: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    tbody.innerHTML = '<tr><td colspan=\"8\" style=\"text-align:center;color:#64748b\">No t
- Line 611: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    tbody.innerHTML = '<tr><td colspan=\"8\" style=\"text-align:center;color:#64748b\">No t
- Line 618: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      ? '<span class=\"tag tag-aql\">AQL</span>'\n";
- Line 618: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      ? '<span class=\"tag tag-aql\">AQL</span>'\n";
- Line 619: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      : '<span class=\"tag tag-fn\">Fn</span>';\n";
- Line 619: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      : '<span class=\"tag tag-fn\">Fn</span>';\n";
- Line 621: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      ? `<button class=\"btn-warning\" onclick=\"disableTask('${t.id}')\">Pause</button>`\n
- Line 621: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      ? `<button class=\"btn-warning\" onclick=\"disableTask('${t.id}')\">Pause</button>`\n
- Line 621: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      ? `<button class=\"btn-warning\" onclick=\"disableTask('${t.id}')\">Pause</button>`\n
- Line 622: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      : `<button class=\"btn-success\" onclick=\"enableTask('${t.id}')\">Resume</button>`;\
- Line 622: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      : `<button class=\"btn-success\" onclick=\"enableTask('${t.id}')\">Resume</button>`;\
- Line 622: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      : `<button class=\"btn-success\" onclick=\"enableTask('${t.id}')\">Resume</button>`;\
- Line 624: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      <td><b>${escHtml(t.id)}</b><br><small style=\"color:#94a3b8\">${escHtml(t.name)}</sma
- Line 624: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      <td><b>${escHtml(t.id)}</b><br><small style=\"color:#94a3b8\">${escHtml(t.name)}</sma
- Line 625: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      <td>${typeTag}</td>\n";
- Line 626: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      <td>${escHtml(t.trigger_type || '–')}</td>\n";
- Line 627: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      <td class=\"${statusClass}\">${statusText}</td>\n";
- Line 627: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      <td class=\"${statusClass}\">${statusText}</td>\n";
- Line 628: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      <td>${t.successful_executions ?? 0} / ${t.total_executions ?? 0}</td>\n";
- Line 629: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      <td style=\"color:#f87171;font-size:.8rem\">${escHtml(t.last_error || '')}</td>\n";
- Line 629: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      <td style=\"color:#f87171;font-size:.8rem\">${escHtml(t.last_error || '')}</td>\n";
- Line 630: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      <td style=\"font-size:.8rem\">${formatDate(t.next_run)}</td>\n";
- Line 630: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      <td style=\"font-size:.8rem\">${formatDate(t.next_run)}</td>\n";
- Line 632: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        <button class=\"btn-primary\" onclick=\"runTask('${t.id}')\">&#9654; Run</button>\n
- Line 632: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        <button class=\"btn-primary\" onclick=\"runTask('${t.id}')\">&#9654; Run</button>\n
- Line 632: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        <button class=\"btn-primary\" onclick=\"runTask('${t.id}')\">&#9654; Run</button>\n
- Line 634: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        <button class=\"btn-secondary\" onclick=\"editTask('${t.id}')\">&#9998; Edit</butto
- Line 634: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        <button class=\"btn-secondary\" onclick=\"editTask('${t.id}')\">&#9998; Edit</butto
- Line 634: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        <button class=\"btn-secondary\" onclick=\"editTask('${t.id}')\">&#9998; Edit</butto
- Line 635: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        <button class=\"btn-danger\" onclick=\"deleteTask('${t.id}')\">&#x1F5D1;</button>\n
- Line 635: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        <button class=\"btn-danger\" onclick=\"deleteTask('${t.id}')\">&#x1F5D1;</button>\n
- Line 635: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        <button class=\"btn-danger\" onclick=\"deleteTask('${t.id}')\">&#x1F5D1;</button>\n
- Line 636: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      </td>\n";
- Line 637: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    </tr>`;\n";
- Line 642: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  return String(s).replace(/&/g,'&amp;').replace(/</g,'<').replace(/>/g,'>').replace(/\"/g,
- Line 642: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  return String(s).replace(/&/g,'&amp;').replace(/</g,'<').replace(/>/g,'>').replace(/\"/g,
- Line 642: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  return String(s).replace(/&/g,'&amp;').replace(/</g,'<').replace(/>/g,'>').replace(/\"/g,
- Line 642: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  return String(s).replace(/&/g,'&amp;').replace(/</g,'<').replace(/>/g,'>').replace(/\"/g,
- Line 642: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  return String(s).replace(/&/g,'&amp;').replace(/</g,'<').replace(/>/g,'>').replace(/\"/g,
- Line 652: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  const r = await api('POST', API + '/' + id + '/execute');\n";
- Line 652: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  const r = await api('POST', API + '/' + id + '/execute');\n";
- Line 658: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  const r = await api('POST', API + '/' + id + '/enable');\n";
- Line 658: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  const r = await api('POST', API + '/' + id + '/enable');\n";
- Line 664: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  const r = await api('POST', API + '/' + id + '/disable');\n";
- Line 664: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  const r = await api('POST', API + '/' + id + '/disable');\n";
- Line 671: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  const r = await api('DELETE', API + '/' + id);\n";
- Line 694: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  const t = await api('GET', API + '/' + id);\n";
- Line 704: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  document.getElementById('f-interval').value = Math.round((t.interval_ms || 300000) / 1000
- Line 706: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  document.getElementById('f-timeout').value = Math.round((t.timeout_ms || 600000) / 1000);
- Line 713: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: html += "  document.getElementById('task-dialog').close();\n";
- Line 746: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    ? await api('PUT', API + '/' + id, body)\n";
- Line 757: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "// Auto-refresh every 30 seconds\n";
- Line 760: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</script>\n";
- Line 761: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</body>\n</html>\n";
- Line 863: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ms = j["interval_ms"].get<int64_t>();
  Confidence: band=high; score=0.74
- Line 869: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto secs = j["interval_seconds"].get<int64_t>();
  Confidence: band=high; score=0.74
- Line 875: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ms = j["timeout_ms"].get<int64_t>();
  Confidence: band=high; score=0.74
- Line 881: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto secs = j["timeout_seconds"].get<int64_t>();
  Confidence: band=high; score=0.74
- Line 917: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto task_ids = request["task_ids"].get<std::vector<std::string>>();
  Confidence: band=high; score=0.74
- Line 983: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cfg.extra_labels.emplace_back(it.key(), it.value().get<std::string>());
  Confidence: band=high; score=0.74
- Line 983: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = req["extra_labels"].begin(); it != req["extra_labels"].end(); ++it) {
  Confidence: band=high; score=0.74
- Line 1011: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cfg.tags.push_back(tag.get<std::string>());
  Confidence: band=high; score=0.74
- Line 1012: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cfg.tags.push_back(tag.get<std::string>());
- Line 1083: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tasks.push_back(*task_ptr);
  Confidence: band=high; score=0.74
- Line 1084: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tasks.push_back(*task_ptr);
- Line 509: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: html += "  <button class=\"btn-primary\" onclick=\"openCreateDialog()\">&#43; New Task</button>\n";
  Confidence: band=medium; score=0.6
- Line 567: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: html += "    <button class=\"btn-secondary\" onclick=\"closeDialog()\">Cancel</button>\n";
  Confidence: band=medium; score=0.6
- Line 676: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: html += "function openCreateDialog() {\n";
  Confidence: band=medium; score=0.6
- Line 712: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: html += "function closeDialog() {\n";
  Confidence: band=medium; score=0.6
- Line 750: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: html += "    closeDialog();\n";
  Confidence: band=medium; score=0.6

### src/server/postgres_session.cpp
Total findings: 180

- Line 125: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (auto self = weak_self.lock()) {
- Line 150: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: readTimeoutTimer_.async_wait([this, self](const boost::beast::error_code& ec) {
- Line 172: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: writeTimeoutTimer_.async_wait([this, self](const boost::beast::error_code& ec) {
- Line 822: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = preparedStatements_.find(name);
- Line 832: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = portals_.find(name);
- Line 932: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator ltrim may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto ltrim = field.find_first_not_of(" \t");
- Line 933: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator rtrim may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto rtrim = field.find_last_not_of(" \t");
- Line 1569: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (auto self = weak_self.lock()) {
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 151: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (ec || stopped_.load(std::memory_order_acquire)) {
- Line 173: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (ec || stopped_.load(std::memory_order_acquire)) {
- Line 192: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: switch (transactionState_.load(std::memory_order_acquire)) {
- Line 303: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Handle schema queries (pg_catalog, information_schema) for BI tool compatibility
  Confidence: band=high; score=0.8
- Line 490: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < params.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 551: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t dotPos = colName.find('.');
- Line 665: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < rowsToSend; ++i) {
  Confidence: band=very_high; score=0.9
- Line 727: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t dotPos = colName.find('.');
- Line 789: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t dotPos = colName.find('.');
- Line 856: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!copyInProgress_.load(std::memory_order_acquire)) {
- Line 870: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(copyMutex_);
- Line 880: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!copyInProgress_.load(std::memory_order_acquire)) {
- Line 1292: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (ec || stopped_.load(std::memory_order_acquire)) {
- Line 1304: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (inStartup_.load(std::memory_order_acquire)) {
- Line 1314: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: int32_t protocolVersion = (buffer_[4] << 24) | (buffer_[5] << 16) |
- Line 1315: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (buffer_[6] << 8) | buffer_[7];
- Line 1320: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: while (offset < static_cast<size_t>(length) && buffer_[offset] != 0) {
- Line 1332: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: char messageType = buffer_[0];
- Line 1333: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: int32_t length = (buffer_[1] << 24) | (buffer_[2] << 16) |
- Line 1334: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (buffer_[3] << 8) | buffer_[4];
- Line 1346: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string query(buffer_.data() + offset);
- Line 1364: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: int32_t typeOid = (static_cast<uint8_t>(buffer_[offset]) << 24) |
- Line 1365: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (static_cast<uint8_t>(buffer_[offset + 1]) << 16) |
- Line 1366: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (static_cast<uint8_t>(buffer_[offset + 2]) << 8) |
- Line 1367: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<uint8_t>(buffer_[offset + 3]);
- Line 1390: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: int16_t format = (static_cast<uint8_t>(buffer_[offset]) << 8) |
- Line 1391: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<uint8_t>(buffer_[offset + 1]);
- Line 1405: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: int32_t paramLen = (static_cast<uint8_t>(buffer_[offset]) << 24) |
- Line 1406: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (static_cast<uint8_t>(buffer_[offset + 1]) << 16) |
- Line 1407: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (static_cast<uint8_t>(buffer_[offset + 2]) << 8) |
- Line 1408: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<uint8_t>(buffer_[offset + 3]);
- Line 1441: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: | (static_cast<uint8_t>(buffer_[offset+1]) << 16)
- Line 1442: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: | (static_cast<uint8_t>(buffer_[offset+2]) << 8)
- Line 1443: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: | static_cast<uint8_t>(buffer_[offset+3]);
- Line 1523: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (ec || stopped_.load(std::memory_order_acquire)) {
- Line 1580: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (stopped_.load(std::memory_order_acquire)) {
- Line 1628: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Standard PostgreSQL types required for BI tool compatibility
  Confidence: band=high; score=0.8
- Line 1775: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Invalid SELECT query");
- Line 2000: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Invalid INSERT statement: missing INTO");
- Line 2016: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Invalid INSERT statement: missing column list");
- Line 2037: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Invalid INSERT statement: missing VALUES");
- Line 2043: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Invalid INSERT statement: missing values list");
- Line 2101: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Invalid UPDATE statement: missing SET");
- Line 2169: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t eqPos = assignment.find('=');
- Line 2206: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Invalid DELETE statement: missing FROM");
- Line 34: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += "''";  // PostgreSQL escapes single quotes by doubling
  Confidence: band=high; score=0.74
- Line 35: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += "''";  // PostgreSQL escapes single quotes by doubling
- Line 37: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: result += "\\\\";  // Escape backslashes
- Line 37: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += "\\\\";  // Escape backslashes
- Line 88: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 144: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: socket_.close(ec);
- Line 157: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 179: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 204: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::string>& params) {
  Confidence: band=high; score=0.74
- Line 260: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void PostgresSession::handleQuery(const std::string& query) {
  Confidence: band=high; score=0.74
- Line 457: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void PostgresSession::handleExecute(const std::string& portal, int32_t maxRows) {
  Confidence: band=high; score=0.74
- Line 548: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back({"?column?", 0, 0, 25, -1, -1, 0});
  Confidence: band=high; score=0.74
- Line 549: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back({"?column?", 0, 0, 25, -1, -1, 0});
- Line 556: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back({colName, 0, 0, 25, -1, -1, 0});
- Line 601: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: row_vals.push_back(
  Confidence: band=high; score=0.74
- Line 601: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: row_vals.push_back(
  Confidence: band=high; score=0.74
- Line 602: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: row_vals.push_back(
- Line 606: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: row_vals.push_back("");
- Line 723: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back({"?column?", 0, 0, 25, -1, -1, 0}); // text type
  Confidence: band=high; score=0.74
- Line 724: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back({"?column?", 0, 0, 25, -1, -1, 0}); // text type
- Line 732: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back({colName, 0, 0, 25, -1, -1, 0}); // text type
- Line 737: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back({"?column?", 0, 0, 25, -1, -1, 0});
- Line 786: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back({"?column?", 0, 0, 25, -1, -1, 0});
  Confidence: band=high; score=0.74
- Line 787: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back({"?column?", 0, 0, 25, -1, -1, 0});
- Line 794: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back({colName, 0, 0, 25, -1, -1, 0});
- Line 798: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back({"?column?", 0, 0, 25, -1, -1, 0});
- Line 871: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: copyBuffer_.push_back(line);
- Line 913: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: field += '"';
  Confidence: band=high; score=0.74
- Line 914: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: field += '"';
- Line 914: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: field += '"';
- Line 914: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: field += '"';
- Line 941: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back(field);
- Line 1034: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void PostgresSession::sendReadyForQuery(char transactionStatus) {
  Confidence: band=high; score=0.74
- Line 1049: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: payload.push_back(0);
  Confidence: band=high; score=0.74
- Line 1050: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(0);
- Line 1053: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((field.tableOid >> 24) & 0xFF);
- Line 1054: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((field.tableOid >> 16) & 0xFF);
- Line 1055: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((field.tableOid >> 8) & 0xFF);
- Line 1056: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(field.tableOid & 0xFF);
- Line 1059: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((field.columnAttrNumber >> 8) & 0xFF);
- Line 1060: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(field.columnAttrNumber & 0xFF);
- Line 1063: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((field.dataTypeOid >> 24) & 0xFF);
- Line 1064: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((field.dataTypeOid >> 16) & 0xFF);
- Line 1065: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((field.dataTypeOid >> 8) & 0xFF);
- Line 1066: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(field.dataTypeOid & 0xFF);
- Line 1079: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((field.formatCode >> 8) & 0xFF);
- Line 1080: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(field.formatCode & 0xFF);
- Line 1091: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((colCount >> 8) & 0xFF);
- Line 1092: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(colCount & 0xFF);
- Line 1096: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: payload.push_back((len >> 24) & 0xFF);
  Confidence: band=high; score=0.74
- Line 1097: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((len >> 24) & 0xFF);
- Line 1098: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((len >> 16) & 0xFF);
- Line 1099: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((len >> 8) & 0xFF);
- Line 1100: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(len & 0xFF);
- Line 1116: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((colCount >> 8) & 0xFF);
- Line 1117: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(colCount & 0xFF);
- Line 1121: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: payload.push_back((len >> 24) & 0xFF);
  Confidence: band=high; score=0.74
- Line 1122: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((len >> 24) & 0xFF);
- Line 1123: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((len >> 16) & 0xFF);
- Line 1124: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((len >> 8) & 0xFF);
- Line 1125: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(len & 0xFF);
- Line 1164: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: payload.push_back((typeOid >> 24) & 0xFF);
  Confidence: band=high; score=0.74
- Line 1165: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((typeOid >> 24) & 0xFF);
- Line 1166: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((typeOid >> 16) & 0xFF);
- Line 1167: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((typeOid >> 8) & 0xFF);
- Line 1168: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(typeOid & 0xFF);
- Line 1190: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(overallFormat);
- Line 1194: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((numColumns >> 8) & 0xFF);
- Line 1195: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(numColumns & 0xFF);
- Line 1198: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: payload.push_back((format >> 8) & 0xFF);
  Confidence: band=high; score=0.74
- Line 1199: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((format >> 8) & 0xFF);
- Line 1200: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(format & 0xFF);
- Line 1213: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(overallFormat);
- Line 1216: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((numColumns >> 8) & 0xFF);
- Line 1217: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(numColumns & 0xFF);
- Line 1219: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: payload.push_back((format >> 8) & 0xFF);
  Confidence: band=high; score=0.74
- Line 1220: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((format >> 8) & 0xFF);
- Line 1221: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(format & 0xFF);
- Line 1234: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(overallFormat);
- Line 1237: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((numColumns >> 8) & 0xFF);
- Line 1238: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(numColumns & 0xFF);
- Line 1240: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: payload.push_back((format >> 8) & 0xFF);
  Confidence: band=high; score=0.74
- Line 1241: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((format >> 8) & 0xFF);
- Line 1242: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(format & 0xFF);
- Line 1367: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: paramTypes.push_back(typeOid);
  Confidence: band=high; score=0.74
- Line 1368: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: paramTypes.push_back(typeOid);
- Line 1391: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: paramFormats.push_back(format);
  Confidence: band=high; score=0.74
- Line 1392: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: paramFormats.push_back(format);
- Line 1412: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: params.push_back("NULL");
  Confidence: band=high; score=0.74
- Line 1413: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: params.push_back("NULL");
- Line 1417: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: params.push_back(param);
- Line 1496: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1545: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1599: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool PostgresSession::isSchemaQuery(const std::string& query) {
  Confidence: band=high; score=0.74
- Line 1612: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void PostgresSession::handleSchemaQuery(const std::string& query) {
  Confidence: band=high; score=0.74
- Line 1722: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "[PostgresSession] pg_attribute query: document parse error: " << e.what() << "\n";
- Line 1763: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: PostgresSession::QueryInfo PostgresSession::parseSelectQuery(const std::string& query) {
  Confidence: band=high; score=0.74
- Line 1798: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: info.aggregates.push_back(col);
- Line 1800: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: info.selectColumns.push_back(col);
- Line 1922: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) cypher += ", ";
  Confidence: band=high; score=0.74
- Line 1923: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) cypher += ", ";
- Line 1928: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: cypher += "count(n)";
- Line 1934: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: cypher += "count(n." + col + ")";
- Line 1939: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: cypher += "sum(n." + col + ")";
- Line 1944: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: cypher += "avg(n." + col + ")";
- Line 1949: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: cypher += "min(n." + col + ")";
- Line 1961: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) cypher += ", ";
  Confidence: band=high; score=0.74
- Line 1962: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) cypher += ", ";
- Line 1965: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: cypher += "n";
- Line 1967: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: cypher += "n." + col;
- Line 1993: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string PostgresSession::parseInsertQuery(const std::string& query) {
  Confidence: band=high; score=0.74
- Line 2029: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: columns.push_back(col);
- Line 2065: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.push_back(currentValue);
  Confidence: band=high; score=0.74
- Line 2066: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: values.push_back(currentValue);
- Line 2081: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) cypher += ", ";
  Confidence: band=high; score=0.74
- Line 2081: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) cypher += ", ";
  Confidence: band=high; score=0.74
- Line 2082: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) cypher += ", ";
- Line 2091: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string PostgresSession::parseUpdateQuery(const std::string& query) {
  Confidence: band=high; score=0.74
- Line 2154: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: assignments.push_back(cypherSetClause.substr(start, i - start));
  Confidence: band=high; score=0.74
- Line 2155: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: assignments.push_back(cypherSetClause.substr(start, i - start));
- Line 2160: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: assignments.push_back(cypherSetClause.substr(start));
- Line 2185: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) cypherSetClause += ", ";
  Confidence: band=high; score=0.74
- Line 2186: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) cypherSetClause += ", ";
- Line 2196: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string PostgresSession::parseDeleteQuery(const std::string& query) {
  Confidence: band=high; score=0.74
- Line 2242: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string PostgresSession::translateQuery(const std::string& postgresQuery) {
  Confidence: band=high; score=0.74

### src/server/llm_api_handler.cpp
Total findings: 136

- Line 150: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: return handleLoadModel(req);
  Confidence: band=very_high; score=0.99
- Line 152: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: return handleUnloadModel(req);
  Confidence: band=very_high; score=0.99
- Line 343: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: rag_mode = json_value_to<std::string>(body->at("rag_mode"));
- Line 389: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: llm_request.metadata["rag_tensor_slots"] = body->at("rag_tensor_slots");
- Line 392: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: llm_request.metadata["rag_tensor_slot_chars"] = body->at("rag_tensor_slot_chars");
- Line 690: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: http::response<http::string_body> LLMApiHandler::handleLoadModel(
  Confidence: band=very_high; score=0.99
- Line 692: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto span = Tracer::startSpan("handleLoadModel");
  Confidence: band=very_high; score=0.99
- Line 719: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: bool loaded = plugin_mgr.loadModel(model_id, path);
  Confidence: band=very_high; score=0.99
- Line 722: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: loaded = plugin_mgr.loadModel(model_id, path);
  Confidence: band=very_high; score=0.99
- Line 747: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: logCurrentException("LLMApiHandler::handleLoadModel");
  Confidence: band=very_high; score=0.99
- Line 752: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: http::response<http::string_body> LLMApiHandler::handleUnloadModel(
  Confidence: band=very_high; score=0.99
- Line 754: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto span = Tracer::startSpan("handleUnloadModel");
  Confidence: band=very_high; score=0.99
- Line 776: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: plugin_mgr.unloadModel(model_id);
  Confidence: band=very_high; score=0.99
- Line 792: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: logCurrentException("LLMApiHandler::handleUnloadModel");
  Confidence: band=very_high; score=0.99
- Line 1773: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto perm = policy_engine_->checkInferencePermission(header_map);
- Line 19: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: #include "llm/async_inference_engine.h"
  Confidence: band=very_high; score=0.9
- Line 116: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return lora_handler_->handleRequest(req);
- Line 139: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (target == "/api/v1/llm/inference" && method == http::verb::post) {
  Confidence: band=very_high; score=0.9
- Line 140: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: return handleInference(req);
  Confidence: band=very_high; score=0.9
- Line 146: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: return handleStreamInference(req);
  Confidence: band=very_high; score=0.9
- Line 204: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: http::response<http::string_body> LLMApiHandler::handleInference(
  Confidence: band=very_high; score=0.9
- Line 206: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto span = Tracer::startSpan("handleInference");
  Confidence: band=very_high; score=0.9
- Line 248: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm::InferenceRequest llm_request;
  Confidence: band=very_high; score=0.9
- Line 250: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: llm_request.model_id = model_id.empty() ? std::string("default") : model_id;
- Line 252: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: llm_request.temperature = static_cast<float>(temperature);
- Line 258: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto llm_response = plugin_mgr.generate(llm_request);
  Confidence: band=very_high; score=0.9
- Line 265: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const auto tokens_generated = llm_response.tokens_generated;
  Confidence: band=very_high; score=0.9
- Line 266: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const auto inference_time_ms = llm_response.inference_time_ms;
  Confidence: band=very_high; score=0.9
- Line 267: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const double safe_inference_time_ms = inference_time_ms > 0.0 ? inference_time_ms : 1.0;
  Confidence: band=very_high; score=0.9
- Line 268: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const int safe_tokens_generated = tokens_generated > 0 ? tokens_generated : 1;
  Confidence: band=very_high; score=0.9
- Line 270: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: static_cast<double>(tokens_generated) * 1000.0 / safe_inference_time_ms;
  Confidence: band=very_high; score=0.9
- Line 271: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const double ms_per_token = static_cast<double>(inference_time_ms) / static_cast<double>(safe_tokens_generated);
  Confidence: band=very_high; score=0.9
- Line 281: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: {"inference_time_ms", inference_time_ms},
  Confidence: band=very_high; score=0.9
- Line 290: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "LLMApiHandler::handleInference success: model='{}' prompt_len={} tokens_generated={} inference_time_ms={:.2f} lora='{}'",
  Confidence: band=very_high; score=0.9
- Line 294: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_time_ms,
  Confidence: band=very_high; score=0.9
- Line 302: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "Inference failed",
  Confidence: band=very_high; score=0.9
- Line 306: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: logCurrentException("LLMApiHandler::handleInference");
  Confidence: band=very_high; score=0.9
- Line 307: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: return createErrorResponse(http::status::internal_server_error, "Inference failed");
  Confidence: band=very_high; score=0.9
- Line 383: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Prepare inference request
  Confidence: band=very_high; score=0.9
- Line 384: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm::InferenceRequest llm_request;
  Confidence: band=very_high; score=0.9
- Line 387: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: llm_request.metadata["rag_mode"] = rag_mode;
- Line 395: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Call LLMPluginManager for RAG inference
  Confidence: band=very_high; score=0.9
- Line 396: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto llm_response = plugin_mgr.generateRAG(rag_context, llm_request);
  Confidence: band=very_high; score=0.9
- Line 404: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: {"inference_time_ms", llm_response.inference_time_ms},
  Confidence: band=very_high; score=0.9
- Line 409: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "LLMApiHandler::handleRAG success: query_len={} collection='{}' top_k={} docs_retrieved={} tokens_generated={} inference_time_ms={:.2f} cache_hit={} rag_mode='{}' lora='{}'",
  Confidence: band=very_high; score=0.9
- Line 415: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm_response.inference_time_ms,
  Confidence: band=very_high; score=0.9
- Line 425: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "RAG inference failed",
  Confidence: band=very_high; score=0.9
- Line 430: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: return createErrorResponse(http::status::internal_server_error, "RAG inference failed");
  Confidence: band=very_high; score=0.9
- Line 490: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: http::response<http::string_body> LLMApiHandler::handleStreamInference(
  Confidence: band=very_high; score=0.9
- Line 492: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto span = Tracer::startSpan("handleStreamInference");
  Confidence: band=very_high; score=0.9
- Line 570: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: logCurrentException("LLMApiHandler::handleStreamInference");
  Confidence: band=very_high; score=0.9
- Line 658: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: http::response<http::string_body> LLMApiHandler::handleListModels(
  Confidence: band=very_high; score=0.9
- Line 686: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: return createErrorResponse(http::status::internal_server_error, "Failed to list models");
  Confidence: band=very_high; score=0.9
- Line 690: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: http::response<http::string_body> LLMApiHandler::handleLoadModel(
  Confidence: band=very_high; score=0.9
- Line 706: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: return createErrorResponse(http::status::bad_request, "Missing 'model_id' field");
  Confidence: band=very_high; score=0.9
- Line 713: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: return createErrorResponse(http::status::bad_request, "Invalid load model parameters", e.what());
  Confidence: band=very_high; score=0.9
- Line 748: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: return createErrorResponse(http::status::internal_server_error, "Failed to load model");
  Confidence: band=very_high; score=0.9
- Line 752: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: http::response<http::string_body> LLMApiHandler::handleUnloadModel(
  Confidence: band=very_high; score=0.9
- Line 767: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: return createErrorResponse(http::status::bad_request, "Missing 'model_id' field");
  Confidence: band=very_high; score=0.9
- Line 770: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: return createErrorResponse(http::status::bad_request, "Invalid unload model parameters", e.what());
  Confidence: band=very_high; score=0.9
- Line 793: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: return createErrorResponse(http::status::internal_server_error, "Failed to unload model");
  Confidence: band=very_high; score=0.9
- Line 797: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: http::response<http::string_body> LLMApiHandler::handleModelInfo(
  Confidence: band=very_high; score=0.9
- Line 836: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: return createErrorResponse(http::status::internal_server_error, "Model info retrieval failed");
  Confidence: band=very_high; score=0.9
- Line 840: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: http::response<http::string_body> LLMApiHandler::handleIngestModel(
  Confidence: band=very_high; score=0.9
- Line 858: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: return createErrorResponse(http::status::bad_request, "Missing 'model_id' field");
  Confidence: band=very_high; score=0.9
- Line 886: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: return createErrorResponse(http::status::internal_server_error, "Model ingestion failed");
  Confidence: band=very_high; score=0.9
- Line 1044: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Get statistics from AsyncInferenceEngine and LLMPluginManager
  Confidence: band=very_high; score=0.9
- Line 1143: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Check health of LLMPluginManager and AsyncInferenceEngine
  Confidence: band=very_high; score=0.9
- Line 1293: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = assistant.query(query);
- Line 1315: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response_data["relevant_documents"] = docs_array;
- Line 1539: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response_data["message"] = "Feedback recorded successfully";
- Line 1598: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: json response_data = feedback->toJson();
- Line 1773: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto perm = policy_engine_->checkInferencePermission(header_map);
  Confidence: band=very_high; score=0.9
- Line 1791: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Parse the OpenAI request into an InferenceRequest
  Confidence: band=very_high; score=0.9
- Line 1798: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto& llm_request = std::get<llm::InferenceRequest>(parse_result);
  Confidence: band=very_high; score=0.9
- Line 1812: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // write chunks incrementally; here we buffer them for compatibility
  Confidence: band=high; score=0.8
- Line 1821: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: llm_request.stream_callback = [&](const std::string& token) {
- Line 1831: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string{"Inference failed: "} + e.what(),
  Confidence: band=very_high; score=0.9
- Line 1836: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto err = llm::OpenAICompatAdapter::buildError("Inference failed", "server_error");
  Confidence: band=very_high; score=0.9
- Line 1855: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm::InferenceResponse llm_response;
  Confidence: band=very_high; score=0.9
- Line 1858: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm_response = plugin_mgr.generate(llm_request);
  Confidence: band=very_high; score=0.9
- Line 1861: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string{"Inference failed: "} + e.what(),
  Confidence: band=very_high; score=0.9
- Line 1866: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto err = llm::OpenAICompatAdapter::buildError("Inference failed", "server_error");
  Confidence: band=very_high; score=0.9
- Line 1888: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: http::response<http::string_body> LLMApiHandler::handleOpenAIListModels(
  Confidence: band=very_high; score=0.9
- Line 72: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 144: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: return handleEmbed(req);
  Confidence: band=high; score=0.74
- Line 194: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 305: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 369: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: const std::vector<float> query_vec = plugin_mgr.embed(query);
  Confidence: band=high; score=0.74
- Line 376: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 428: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 434: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: http::response<http::string_body> LLMApiHandler::handleEmbed(
  Confidence: band=high; score=0.74
- Line 465: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: embedding_vector.push_back(val);
  Confidence: band=high; score=0.74
- Line 466: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: embedding_vector.push_back(val);
- Line 482: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 513: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (raw[i] == '+') { decoded += ' '; ++i; }
  Confidence: band=high; score=0.74
- Line 514: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (raw[i] == '+') { decoded += ' '; ++i; }
- Line 569: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 640: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 668: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: models.push_back(json{{"model_id", model_id}});
  Confidence: band=high; score=0.74
- Line 669: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: models.push_back(json{{"model_id", model_id}});
- Line 684: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 746: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 791: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 834: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 884: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 906: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: loras.push_back(lora_obj);
  Confidence: band=high; score=0.74
- Line 907: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: loras.push_back(lora_obj);
- Line 924: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 987: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1034: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1065: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1106: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1133: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1164: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1196: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1308: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: docs_array.push_back({
  Confidence: band=high; score=0.74
- Line 1309: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: docs_array.push_back({
- Line 1325: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1387: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1451: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1545: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1610: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::string status_filter;
- Line 1690: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: feedback_array.push_back(feedback.toJson());
  Confidence: band=high; score=0.74
- Line 1691: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: feedback_array.push_back(feedback.toJson());
- Line 1707: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1768: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> header_map;
  Confidence: band=medium; score=0.66
- Line 1781: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: : http::status::forbidden);
- Line 1828: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: plugin_mgr.generate(llm_request);
  Confidence: band=high; score=0.74
- Line 1834: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1858: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: llm_response = plugin_mgr.generate(llm_request);
  Confidence: band=high; score=0.74
- Line 1864: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1880: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1899: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: models_arr.push_back(json{
  Confidence: band=high; score=0.74
- Line 1900: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: models_arr.push_back(json{
- Line 1911: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/voice_api_handler.cpp
Total findings: 135

- Line 1060: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: metadata.meeting_id = body->value("meeting_id", "");
- Line 1435: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto macros = voice_assistant_->macroManager().listMacros("", tag_filter);
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 131: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 419: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: audio_data = decodeBase64((*body)["audio_base64"]);
- Line 689: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto audio_data = decodeBase64((*body)["audio_base64"]);
- Line 697: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto audio_response = voice_assistant_->processVoiceCommand(audio_data, session_id);
- Line 766: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto audio_data = decodeBase64((*body)["audio_base64"]);
- Line 825: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto detection = voice_assistant_->detectWakeWord(audio_data);
- Line 859: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: audio_data = decodeBase64((*body)["audio_base64"]);
- Line 918: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (body->contains("custom_fields") && !(*body)["custom_fields"].is_object()) {
- Line 936: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.call_id = body->value("call_id", "");
- Line 944: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.caller_number = body->value("caller", "");
- Line 945: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.callee_number = body->value("callee", "");
- Line 946: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.start_time = body->value("start_time", 0LL);
- Line 947: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.end_time = body->value("end_time", 0LL);
- Line 948: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.call_type = body->value("call_type", "inbound");
- Line 955: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto result = voice_assistant_->recordPhoneCall(audio_data, metadata);
- Line 983: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: audio_data = decodeBase64((*body)["audio_base64"]);
- Line 1042: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (body->contains("custom_fields") && !(*body)["custom_fields"].is_object()) {
- Line 1060: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.meeting_id = body->value("meeting_id", "");
- Line 1068: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.title = body->value("title", "");
- Line 1069: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.start_time = body->value("start_time", 0LL);
- Line 1070: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.end_time = body->value("end_time", 0LL);
- Line 1071: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.organizer = body->value("organizer", "");
- Line 1091: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto result = voice_assistant_->generateMeetingProtocol(audio_data, metadata);
- Line 1182: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["languages"] = json::array({
- Line 1332: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (body->contains("options") && !(*body)["options"].is_object()) {
- Line 1336: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (body->contains("options") && (*body)["options"].is_object()) {
- Line 1495: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (body->contains("options") && !(*body)["options"].is_object()) {
- Line 1499: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (body->contains("options") && (*body)["options"].is_object()) {
- Line 1582: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["macro"]   = info ? macroInfoToResponseJson(*info) : json(nullptr);
- Line 1654: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: r["metadata"]         = rec.metadata;
- Line 1702: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["metadata"]         = rec->metadata;
- Line 1753: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: r["metadata"]         = rec.metadata;
- Line 1866: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!body || !body->is_object()) {
- Line 1958: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("URL cannot be empty");
- Line 1962: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("HTTP client pool is not initialized");
- Line 1988: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Access to localhost is not allowed");
- Line 1998: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Access to metadata endpoints is not allowed");
- Line 2005: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Access to private or restricted IP addresses is not allowed");
- Line 2023: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: if (response_future.wait_for(std::chrono::seconds(70)) == std::future_status::timeout) {
- Line 2024: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Audio download timed out");
- Line 2031: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 2038: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Downloaded audio is empty");
- Line 76: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 277: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Missing macro ID");
- Line 281: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Invalid macro ID");
- Line 299: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Missing session ID");
- Line 310: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Invalid session ID");
- Line 318: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Invalid session path");
- Line 323: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Invalid session ID");
- Line 344: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Missing recording ID");
- Line 348: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Invalid recording ID");
- Line 372: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Missing profile ID");
- Line 376: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Invalid profile ID");
- Line 1081: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: metadata.participants.push_back(p);
  Confidence: band=high; score=0.74
- Line 1082: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: metadata.participants.push_back(p);
- Line 1211: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = step_json["parameters"].begin(); it != step_json["parameters"].end(); ++it) {
  Confidence: band=high; score=0.74
- Line 1235: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = j["parameters"].begin(); it != j["parameters"].end(); ++it) {
  Confidence: band=high; score=0.74
- Line 1274: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: steps.push_back(sj);
  Confidence: band=high; score=0.74
- Line 1274: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: steps.push_back(sj);
  Confidence: band=high; score=0.74
- Line 1275: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back(sj);
- Line 1297: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Invalid JSON body");
- Line 1308: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "trigger_phrase must be a string");
- Line 1314: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "trigger_phrase must not be empty");
- Line 1320: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "'steps' must be an array");
- Line 1327: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: steps.push_back(parseStep(sj));
  Confidence: band=high; score=0.74
- Line 1328: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back(parseStep(sj));
- Line 1334: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "options must be an object");
- Line 1365: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "name must be a string");
- Line 1369: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "description must be a string");
- Line 1373: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "tags must be an array");
- Line 1431: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!token.empty()) tag_filter.push_back(token);
- Line 1438: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(macroInfoToResponseJson(m));
  Confidence: band=high; score=0.74
- Line 1439: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(macroInfoToResponseJson(m));
- Line 1471: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Invalid JSON body");
- Line 1481: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "'steps' must be an array");
- Line 1490: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: steps.push_back(parseStep(sj));
  Confidence: band=high; score=0.74
- Line 1491: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back(parseStep(sj));
- Line 1497: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "options must be an object");
- Line 1528: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "name must be a string");
- Line 1532: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "description must be a string");
- Line 1536: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "tags must be an array");
- Line 1549: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "enabled must be a boolean");
- Line 1654: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 1655: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(r));
- Line 1753: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result["recordings"].push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 1754: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result["recordings"].push_back(std::move(r));
- Line 1786: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 1853: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1915: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint8_t>((val >> valb) & 0xFF));
- Line 1932: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64_table[(n >> 18) & 63]);
- Line 1933: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64_table[(n >> 12) & 63]);
- Line 1934: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64_table[(n >>  6) & 63]);
- Line 1935: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64_table[ n        & 63]);
- Line 1940: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64_table[(n >> 18) & 63]);
- Line 1941: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64_table[(n >> 12) & 63]);
- Line 1942: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back('=');
- Line 1943: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back('=');
- Line 1947: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64_table[(n >> 18) & 63]);
- Line 1970: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2082: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Invalid JSON body");
- Line 2093: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "user_id must be a string");
- Line 2099: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "user_id must not be empty");
- Line 2103: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Invalid user_id");
- Line 2109: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "audio_samples must be a non-empty array");
- Line 2131: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: audio_samples.push_back(std::move(decoded_sample));
  Confidence: band=high; score=0.74
- Line 2132: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: audio_samples.push_back(std::move(decoded_sample));
- Line 2197: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Invalid JSON body");
- Line 2208: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "profile_id must be a string");
- Line 2212: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "audio must be a base64 string");
- Line 2218: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "profile_id must not be empty");
- Line 2222: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Invalid profile_id");
- Line 2227: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "audio must not be empty");
- Line 2254: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Invalid JSON body");
- Line 2265: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "user_id must be a string");
- Line 2269: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "audio must be a base64 string");
- Line 2275: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "user_id must not be empty");
- Line 2279: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Invalid user_id");
- Line 2284: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "audio must not be empty");
- Line 2306: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: : http::status::unauthorized;
- Line 2317: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Invalid JSON body");
- Line 2329: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "candidate_profiles must be an array");
- Line 2333: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "candidate_profiles must not be empty");
- Line 2337: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "audio must be a base64 string");
- Line 2341: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "audio must not be empty");
- Line 2357: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(candidate);
  Confidence: band=high; score=0.74
- Line 2358: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: candidates.push_back(candidate);
- Line 2376: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matches_arr.push_back(mj);
  Confidence: band=high; score=0.74
- Line 2377: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: matches_arr.push_back(mj);
- Line 2394: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(pid);
  Confidence: band=high; score=0.74
- Line 2395: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(pid);
- Line 2410: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Missing profile ID");
- Line 2416: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::not_found, "Not Found", "Voice profile not found");

### src/server/mcp_server.cpp
Total findings: 112

- Line 2572: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = pending_approvals_.begin(); it != pending_approvals_.end(); ) {
- Line 2833: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: auto self = weak_self.lock();
- Line 3049: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = clients_.find(client_id);
- Line 3077: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: keepalive_timer_.async_wait([weak_self](const boost::system::error_code& ec) {
- Line 3078: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: auto self = weak_self.lock();
- Line 3165: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = sessions_.find(session_id);
- Line 3229: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: ping_timer_.async_wait([weak_self](const boost::system::error_code& ec) {
- Line 3230: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: auto self = weak_self.lock();
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 264: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: stdio_transport_->setMessageHandler([this](const json& req) { return handleRequest(req); });
- Line 275: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: sse_transport_->setMessageHandler([this](const json& req) { return handleRequest(req); });
- Line 286: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ws_transport_->setMessageHandler([this](const json& req) { return handleRequest(req); });
- Line 345: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Fall back to legacy path if new one doesn't exist
  Confidence: band=high; score=0.8
- Line 491: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.contains("jsonrpc") || request["jsonrpc"] != "2.0") {
- Line 495: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.contains("method")) {
- Line 500: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json params = request.contains("params") ? request["params"] : json::object();
- Line 584: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!it->second.handler) {
- Line 588: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: json result = it->second.handler(args);
- Line 623: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!it->second.handler) {
- Line 627: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: json content = it->second.handler(uri);
- Line 669: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!it->second.handler) {
- Line 673: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: json messages = it->second.handler(name, args);
- Line 725: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: without = nullptr;
  Context: {"description", "AI Safety: preview the delete without executing it (Phase 1 guard)"}}}
- Line 864: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: {"description", "Search query (e.g., 'gpu', 'lora', 'storage')"}
- Line 864: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function query without trace point
  Context: {"description", "Search query (e.g., 'gpu', 'lora', 'storage')"}
  Confidence: band=very_high; score=0.9
- Line 1236: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: entity = nullptr;
  Context: {"message", "Failed to delete entity"},
- Line 1519: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const bool database_connected = db_ && db_->isOpen();
- Line 1550: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: schema_json["database_connected"] = database_connected;
- Line 1570: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const bool database_connected = db_ && db_->isOpen();
- Line 1766: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: out["mode_id"]         = result.metadata.mode_id;
- Line 1767: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: out["model_id"]        = result.metadata.model_id;
- Line 1768: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: out["tokens_prompt"]   = result.metadata.tokens_prompt;
- Line 1769: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: out["tokens_generated"]= result.metadata.tokens_generated;
- Line 1770: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: out["retrieved_docs"]  = result.metadata.retrieved_docs;
- Line 1771: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: out["latency_ms"]      = result.metadata.latency.total_ms;
- Line 2083: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(results.size(), size_t(3)); ++i) {
- Line 2104: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(results.size(), size_t(3)); ++i) {
- Line 2136: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [this](const std::string& uri) { return resourceMetadata(uri); });
- Line 2189: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: {"last_refresh", metadata.toJSON()["last_refresh"]}
- Line 2232: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: {"database_open", db_ && db_->isOpen()}
- Line 2486: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: } else if (!tool_it->second.handler) {
- Line 2490: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: exec_result = tool_it->second.handler(mutable_args);
- Line 2824: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!is_running_.load(std::memory_order_acquire)) return;
- Line 2842: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (self->is_running_.load(std::memory_order_acquire)) {
- Line 2883: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (self->message_handler_) {
- Line 2884: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: json response = self->message_handler_(request);
- Line 2893: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: } else if (!self->is_running_.load(std::memory_order_acquire)) {
- Line 2907: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (self->message_handler_) {
- Line 2908: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: json response = self->message_handler_(request);
- Line 2924: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 2935: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (self->is_running_.load(std::memory_order_acquire)) {
- Line 2958: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (self->message_handler_) {
- Line 2959: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: json response = self->message_handler_(request);
- Line 2982: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << data << std::flush;
  Confidence: band=very_high; score=0.9
- Line 3021: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!is_running_.load(std::memory_order_acquire)) return;
- Line 3028: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [client_id, buffer] : clients_) {
  Confidence: band=very_high; score=0.9
- Line 3028: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (auto& [client_id, buffer] : clients_) {
- Line 3059: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!is_running_.load(std::memory_order_acquire)) return;
- Line 3065: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (auto& [client_id, buffer] : clients_) {
- Line 3073: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!is_running_.load(std::memory_order_acquire)) return;
- Line 3081: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!ec && self->is_running_.load(std::memory_order_acquire)) {
- Line 3124: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!is_running_.load(std::memory_order_acquire)) return;
- Line 3129: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [session_id, session_data] : sessions_) {
  Confidence: band=very_high; score=0.9
- Line 3129: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (auto& [session_id, session_data] : sessions_) {
- Line 3139: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!is_running_.load(std::memory_order_acquire)) return;
- Line 3178: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!is_running_.load(std::memory_order_acquire)) return;
- Line 3204: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!is_running_.load(std::memory_order_acquire)) return;
- Line 3215: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (const auto& [session_id, session_data] : sessions_) {
- Line 3225: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!is_running_.load(std::memory_order_acquire)) return;
- Line 3233: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!ec && self->is_running_.load(std::memory_order_acquire)) {
- Line 110: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: new_cfg.denied_collections.push_back(c.as<std::string>());
  Confidence: band=high; score=0.74
- Line 111: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: new_cfg.denied_collections.push_back(c.as<std::string>());
- Line 116: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: new_cfg.allowed_collections.push_back(c.as<std::string>());
  Confidence: band=high; score=0.74
- Line 117: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: new_cfg.allowed_collections.push_back(c.as<std::string>());
- Line 526: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: json McpServer::handleInitialize(const json& params) {
  Confidence: band=medium; score=0.66
- Line 555: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tools_list.push_back({
  Confidence: band=high; score=0.74
- Line 556: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tools_list.push_back({
- Line 598: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resources_list.push_back({
  Confidence: band=high; score=0.74
- Line 599: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: resources_list.push_back({
- Line 643: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: prompts_list.push_back({
  Confidence: band=high; score=0.74
- Line 644: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: prompts_list.push_back({
- Line 725: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: {"description", "AI Safety: preview the delete without executing it (Phase 1 guard)"}}}
- Line 864: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: {"description", "Search query (e.g., 'gpu', 'lora', 'storage')"}
  Confidence: band=high; score=0.74
- Line 889: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json McpServer::toolQuery(const json& args) {
  Confidence: band=high; score=0.74
- Line 1236: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: {"message", "Failed to delete entity"},
- Line 1288: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: SecondaryIndexManager::Status status;
- Line 1303: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ft_config = args["fulltext_config"];
  Confidence: band=high; score=0.74
- Line 1407: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: SecondaryIndexManager::Status status;
- Line 1496: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: indexes.push_back(index_info);
  Confidence: band=high; score=0.74
- Line 1496: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: indexes.push_back(index_info);
  Confidence: band=high; score=0.74
- Line 1497: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: indexes.push_back(index_info);
- Line 1683: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: messages.push_back({
  Confidence: band=high; score=0.74
- Line 1684: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: messages.push_back({
- Line 1804: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: modes_arr.push_back(std::move(entry));
  Confidence: band=high; score=0.74
- Line 1805: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: modes_arr.push_back(std::move(entry));
- Line 1839: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1851: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors_json.push_back(error.toJSON());
  Confidence: band=high; score=0.74
- Line 1852: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors_json.push_back(error.toJSON());
- Line 1879: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors_json.push_back(error.toJSON());
  Confidence: band=high; score=0.74
- Line 1880: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors_json.push_back(error.toJSON());
- Line 2022: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: answer += fmt::format("**{}** ({} error types)\n", category, errors.size());
  Confidence: band=high; score=0.74
- Line 2048: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) docs_str += ", ";
  Confidence: band=high; score=0.74
- Line 2049: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) docs_str += ", ";
- Line 2281: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json McpServer::promptSimpleQuery(const std::string& name, const json& args) {
  Confidence: band=high; score=0.74
- Line 2296: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json McpServer::promptComplexQuery(const std::string& name, const json& args) {
  Confidence: band=high; score=0.74
- Line 2548: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: list.push_back({
  Confidence: band=high; score=0.74
- Line 2549: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: list.push_back({
- Line 2780: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void StdioTransport::start() {
  Confidence: band=medium; score=0.66
- Line 2810: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { fn(); } catch (...) {}
- Line 2998: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void SseTransport::start() {
  Confidence: band=medium; score=0.66
- Line 3169: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: messages.push_back(std::move(it->second.pending_messages.front()));

### src/server/monitoring_api_handler.cpp
Total findings: 74

- Line 118: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: bool alive = (is_running_ == nullptr) || is_running_->load(std::memory_order_relaxed);
- Line 145: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: bool server_running = (is_running_ == nullptr) || is_running_->load(std::memory_order_relaxed);
- Line 0: severity=HIGH; category=uncategorized
  Context: ['        html += "<!DOCTYPE html>\\n<html lang=\\"en\\">\\n<head>\\n";', '        html += "<meta charset=\\"UTF-8\\">\\n";', '        html += "<meta name=\\"viewport\\" content=\\"width=device-width, initial-scale=1\\">\\n";', '        html += "<title>ThemisDB Metrics</title>\\n";', '        html += "<style>\\n";']
  Confidence: band=high; score=0.78
- Line 118: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: bool alive = (is_running_ == nullptr) || is_running_->load(std::memory_order_relaxed);
- Line 118: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: bool alive = (is_running_ == nullptr) || is_running_->load(std::memory_order_relaxed);
- Line 145: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: bool server_running = (is_running_ == nullptr) || is_running_->load(std::memory_order_relaxed);
- Line 145: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: bool server_running = (is_running_ == nullptr) || is_running_->load(std::memory_order_relaxed);
- Line 152: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: storage_ok = (storage_->getRawDB() != nullptr);
- Line 853: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Config path resolution metrics (hit rate, miss rate, legacy fallback rate)
  Confidence: band=high; score=0.8
- Line 1126: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: (alertmanager_ != nullptr) && alertmanager_->getConfig().enabled;
- Line 1375: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: body["license_key"]      = nullptr;
- Line 1376: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: body["organization"]     = nullptr;
- Line 1378: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: body["expiry_date"]      = nullptr;
- Line 1379: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: body["days_until_expiry"] = nullptr;
- Line 288: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: modules_compiled.push_back(module_info);
  Confidence: band=high; score=0.74
- Line 289: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: modules_compiled.push_back(module_info);
- Line 291: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: modules_disabled.push_back(module_info);
- Line 338: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: supported.push_back({
  Confidence: band=high; score=0.74
- Line 339: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: supported.push_back({
- Line 430: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 476: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 550: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 587: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 619: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "# HELP themis_build_info ThemisDB build information\n";
  Confidence: band=high; score=0.74
- Line 682: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = r["files_per_level"].begin(); it != r["files_per_level"].end(); ++it) {
  Confidence: band=high; score=0.74
- Line 685: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: level_rows.emplace_back(it.key(), val);
  Confidence: band=high; score=0.74
- Line 692: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 706: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "rocksdb_files_level{level=\"" + level + "\"} " + std::to_string(val) + "\n";
  Confidence: band=high; score=0.74
- Line 706: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "rocksdb_files_level{level=\"" + level + "\"} " + std::to_string(val) + "\n";
  Confidence: band=high; score=0.74
- Line 707: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "rocksdb_files_level{level=\"" + level + "\"} " + std::to_string(val) + "\n";
- Line 748: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sorted_plugin_names.push_back(plugin_name);
  Confidence: band=high; score=0.74
- Line 749: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sorted_plugin_names.push_back(plugin_name);
- Line 753: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "\n# HELP themis_plugin_loads_total Total number of plugin loads\n";
  Confidence: band=high; score=0.74
- Line 756: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "themis_plugin_loads_total{plugin=\"" + plugin_name + "\"} 1\n";
  Confidence: band=high; score=0.74
- Line 757: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "themis_plugin_loads_total{plugin=\"" + plugin_name + "\"} 1\n";
- Line 763: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "themis_plugin_reloads_total{plugin=\"" + plugin_name + "\"} "
  Confidence: band=high; score=0.74
- Line 764: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "themis_plugin_reloads_total{plugin=\"" + plugin_name + "\"} "
- Line 771: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "themis_plugin_errors_total{plugin=\"" + plugin_name + "\"} "
  Confidence: band=high; score=0.74
- Line 772: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "themis_plugin_errors_total{plugin=\"" + plugin_name + "\"} "
- Line 779: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "themis_plugin_function_calls_total{plugin=\"" + plugin_name + "\"} "
  Confidence: band=high; score=0.74
- Line 780: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "themis_plugin_function_calls_total{plugin=\"" + plugin_name + "\"} "
- Line 788: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "themis_plugin_load_duration_seconds_sum{plugin=\"" + plugin_name + "\"} "
  Confidence: band=high; score=0.74
- Line 789: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "themis_plugin_load_duration_seconds_sum{plugin=\"" + plugin_name + "\"} "
- Line 791: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "themis_plugin_load_duration_seconds_count{plugin=\"" + plugin_name + "\"} 1\n";
- Line 797: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "themis_plugin_memory_bytes{plugin=\"" + plugin_name + "\"} "
  Confidence: band=high; score=0.74
- Line 798: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "themis_plugin_memory_bytes{plugin=\"" + plugin_name + "\"} "
- Line 806: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "themis_plugin_call_latency_milliseconds{plugin=\"" + plugin_name
  Confidence: band=high; score=0.74
- Line 807: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "themis_plugin_call_latency_milliseconds{plugin=\"" + plugin_name
- Line 809: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "themis_plugin_call_latency_milliseconds{plugin=\"" + plugin_name
- Line 811: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "themis_plugin_call_latency_milliseconds_sum{plugin=\"" + plugin_name
- Line 813: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "themis_plugin_call_latency_milliseconds_count{plugin=\"" + plugin_name
- Line 821: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 835: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 849: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 864: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 879: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 907: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sorted_plugin_names.push_back(plugin_name);
  Confidence: band=high; score=0.74
- Line 908: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sorted_plugin_names.push_back(plugin_name);
- Line 1118: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(a);
  Confidence: band=high; score=0.74
- Line 1119: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(a);
- Line 1165: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1304: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<title>ThemisDB Metrics</title>\n";
- Line 1315: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<h1>ThemisDB Metrics Dashboard</h1>\n";
- Line 1316: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<p class=\"sub\">Version: <b>" + version + "</b> &nbsp;|&nbsp; ";
- Line 1316: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<p class=\"sub\">Version: <b>" + version + "</b> &nbsp;|&nbsp; ";
- Line 1318: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<a href=\"/metrics\">Raw Prometheus</a></p>\n";
- Line 1318: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<a href=\"/metrics\">Raw Prometheus</a></p>\n";
- Line 1319: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<table>\n<tr><th>Metric</th><th class=\"val\">Value</th></tr>\n";
- Line 1319: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<table>\n<tr><th>Metric</th><th class=\"val\">Value</th></tr>\n";
- Line 1320: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: html += "<tr><td>" + name + "</td><td class=\"val\">" + val + "</td></tr>\n";
  Confidence: band=high; score=0.74
- Line 1321: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<tr><td>" + name + "</td><td class=\"val\">" + val + "</td></tr>\n";
- Line 1321: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<tr><td>" + name + "</td><td class=\"val\">" + val + "</td></tr>\n";
- Line 1321: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: html += "<tr><td>" + name + "</td><td class=\"val\">" + val + "</td></tr>\n";
- Line 1323: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</table>\n</body>\n</html>\n";

### src/server/rpc/rpc_service_impl.cpp
Total findings: 73

- Line 3316: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto auth_result = auth.authorize(token, required_scope);
  Confidence: band=very_high; score=0.99
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
- Line 644: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: cascade = nullptr;
  Context: "Request deadline exceeded during delete cascade scan"
- Line 655: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: them = nullptr;
  Context: " reference this entity. Use cascade=true to delete them."
- Line 674: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: cascade = nullptr;
  Context: "Request deadline exceeded during delete cascade traversal"
- Line 681: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t first_colon  = curr_key.find(':');
- Line 698: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: cascade = nullptr;
  Context: "Request deadline exceeded during delete cascade scan"
- Line 716: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: cascade = nullptr;
  Context: "Request deadline exceeded during delete cascade write"
- Line 722: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: child = nullptr;
  Context: "Failed to delete child entity during cascade: " + *it
- Line 732: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: write = nullptr;
  Context: "Request deadline exceeded during delete write"
- Line 738: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: entity = nullptr;
  Context: "Failed to delete entity from database"
- Line 783: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const auto& keys_array = params["keys"];
- Line 883: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const auto& entities_array = params["entities"];
- Line 920: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Set version: Client provides version in entity, or 0 for new entities
- Line 980: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const auto& keys_array = params["keys"];
- Line 1220: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const bool has_vector = params.contains("vector") && params["vector"].is_array();
- Line 1221: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const bool has_query_vector = params.contains("query_vector") && params["query_vector"].is_array();
- Line 2099: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: {"database_path", storage->getConfig().db_path},
- Line 2273: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const auto& updates_array = params["updates"];
- Line 2506: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: storage->scanPrefix(prefix, [&](std::string_view /*key*/, std::string_view value) -> bool {
- Line 3038: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: storage->scanPrefix(
- Line 3267: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::min(backoff, remaining));
- Line 3285: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // for backward compatibility. In production, auth should always be configured.
  Confidence: band=high; score=0.8
- Line 125: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 628: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: children.push_back(iter_key);
- Line 644: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: "Request deadline exceeded during delete cascade scan"
- Line 655: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: " reference this entity. Use cascade=true to delete them."
- Line 664: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys_to_delete.push_back(child_key);
  Confidence: band=high; score=0.74
- Line 665: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: keys_to_delete.push_back(child_key);
- Line 674: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: "Request deadline exceeded during delete cascade traversal"
- Line 698: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: "Request deadline exceeded during delete cascade scan"
- Line 702: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys_to_delete.push_back(gc_key);
  Confidence: band=high; score=0.74
- Line 703: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: keys_to_delete.push_back(gc_key);
- Line 716: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: "Request deadline exceeded during delete cascade write"
- Line 722: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: "Failed to delete child entity during cascade: " + *it
- Line 732: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: "Request deadline exceeded during delete write"
- Line 738: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: "Failed to delete entity from database"
- Line 805: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(key);
  Confidence: band=high; score=0.74
- Line 806: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: keys.push_back(key);
- Line 1031: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json ThemisRPCService::handleQuery(const json& params) {
  Confidence: band=high; score=0.74
- Line 1083: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: prefix += model + ":";
  Confidence: band=high; score=0.74
- Line 1130: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(entity);
  Confidence: band=high; score=0.74
- Line 1131: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(entity);
- Line 1316: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json ThemisRPCService::handleGeoQuery(const json& params) {
  Confidence: band=high; score=0.74
- Line 1382: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto bbox_json = params["bbox"];
  Confidence: band=high; score=0.74
- Line 1422: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(result_obj);
  Confidence: band=high; score=0.74
- Line 1442: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto center = params["center"];
  Confidence: band=high; score=0.74
- Line 1545: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json ThemisRPCService::handleTimeSeriesQuery(const json& params) {
  Confidence: band=high; score=0.74
- Line 2039: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(entity);
  Confidence: band=high; score=0.74
- Line 2040: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(entity);
- Line 2358: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json ThemisRPCService::handlePaginatedQuery(const json& params) {
  Confidence: band=high; score=0.74
- Line 2514: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: indexes.push_back(idx_meta);
- Line 2617: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: documents.push_back(entity);
- Line 2655: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered.push_back(doc);
  Confidence: band=high; score=0.74
- Line 2655: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered.push_back(doc);
  Confidence: band=high; score=0.74
- Line 2656: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: filtered.push_back(doc);
- Line 2677: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: limited.push_back(results[i]);
  Confidence: band=high; score=0.74
- Line 2678: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: limited.push_back(results[i]);
- Line 2707: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: projected.push_back(proj_doc);
  Confidence: band=high; score=0.74
- Line 2708: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: projected.push_back(proj_doc);
- Line 2788: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: collections_array.push_back({
  Confidence: band=high; score=0.74
- Line 2789: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: collections_array.push_back({
- Line 3026: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: models_array.push_back({
  Confidence: band=high; score=0.74
- Line 3027: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: models_array.push_back({
- Line 3244: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " (attempt " << attempt << "/" << max_attempts << "): " << e.what() << "\n";
- Line 3244: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " (attempt " << attempt << "/" << max_attempts << "): " << e.what() << "\n";
- Line 3245: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 3255: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " (attempt " << attempt << "/" << max_attempts << "): " << error_message << "\n";
- Line 3255: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " (attempt " << attempt << "/" << max_attempts << "): " << error_message << "\n";

### src/server/http3_session.cpp
Total findings: 70

- Line 40: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: cid->data[i] = static_cast<uint8_t>(rd() & 0xFFu);
- Line 201: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator cid_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto cid_it = cid_to_session_key_.find(cid_hex);
- Line 203: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator sess_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto sess_it = sessions_.find(cid_it->second);
- Line 230: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: THEMIS_INFO("HTTP/3 rejecting new QUIC from {} (HTTP/2 fallback active)", client_ip);
- Line 235: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: THEMIS_INFO("HTTP/3 new QUIC connection from {}", session_key);
- Line 262: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: cleanup_timer_.async_wait([this](boost::system::error_code ec) {
- Line 280: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = sessions_.begin(); it != sessions_.end(); ) {
- Line 290: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = cid_to_session_key_.begin(); it != cid_to_session_key_.end(); ) {
- Line 317: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 6 > array 0
  Remediation: Fix loop condition or increase array size
  Context: const uint8_t first = data[0];
- Line 322: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 6 > array 5
  Remediation: Fix loop condition or increase array size
  Context: uint8_t dcid_len = data[5];
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
- Line 6: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #3291 [network] QUIC/HTTP3 transport layer integration (Issue #1994) (2026-03-12T06:49:48Z)
- Line 38: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cid->datalen = NGTCP2_MIN_CIDLEN;
- Line 39: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (size_t i = 0; i < cid->datalen; ++i) {
- Line 40: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cid->data[i] = static_cast<uint8_t>(rd() & 0xFFu);
- Line 156: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!running_.load(std::memory_order_acquire) || !socket_.is_open()) {
- Line 168: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (running_.load(std::memory_order_acquire) && socket_.is_open()) {
- Line 195: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: it->second->handlePacket(recv_buffer_.data(), bytes_transferred, remote_endpoint_);
- Line 210: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: THEMIS_ERROR("HTTP/3: null session ptr in CID migration path for key '{}'", cid_it->second);
- Line 220: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: session->handlePacket(recv_buffer_.data(), bytes_transferred, remote_endpoint_);
- Line 220: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: session->handlePacket(recv_buffer_.data(), bytes_transferred, remote_endpoint_);
- Line 245: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: session->handlePacket(recv_buffer_.data(), bytes_transferred, remote_endpoint_);
- Line 254: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!running_.load(std::memory_order_acquire)) {
- Line 263: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (ec || !running_.load(std::memory_order_acquire)) {
- Line 273: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (running_.load(std::memory_order_acquire)) {
- Line 334: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: hex += kHex[(data[i] >> 4) & 0xf];
- Line 335: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: hex += kHex[data[i] & 0xf];
- Line 830: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: dr.read_data = [](nghttp3_conn* /*conn*/, int64_t /*stream_id*/,
- Line 833: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: void* stream_user_data) -> nghttp3_ssize {
- Line 839: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: vec[0].base = (uint8_t*)body_ptr->data();
- Line 840: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: vec[0].len = body_ptr->size();
- Line 920: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: self->http3_conn_, stream_id, data, datalen, 0
- Line 1023: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: self->datagram_dispatcher_.dispatch(data, datalen);
- Line 59: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 91: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_CTX_free(ssl_ctx_);
- Line 109: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_CTX_free(ssl_ctx);
- Line 115: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_CTX_free(ssl_ctx);
- Line 136: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void Http3Handler::start() {
  Confidence: band=medium; score=0.66
- Line 149: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: socket_.close(ignored);
- Line 166: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 247: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 269: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 383: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: SSL_free(ssl_);
- Line 387: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void Http3Session::start() {
  Confidence: band=medium; score=0.66
- Line 491: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 527: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 791: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> response_headers;
  Confidence: band=medium; score=0.66
- Line 802: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, std::string>& headers) {
  Confidence: band=medium; score=0.66
- Line 824: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nva.push_back(header);
  Confidence: band=high; score=0.74
- Line 825: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nva.push_back(header);
- Line 897: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 929: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 948: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 965: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 981: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1001: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1025: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1120: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1157: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1177: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1194: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/mqtt_client_service.cpp
Total findings: 57

- Line 259: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: asio::write(*asio_->ssl_stream, asio::buffer(pkt), ec);
- Line 268: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: asio::write(asio_->socket, asio::buffer(pkt), ec);
- Line 423: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: asio_->connect_timer.async_wait([this, connected](boost::system::error_code ec3) {
- Line 436: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: asio::write(*asio_->ssl_stream, asio::buffer(pkt), we);
- Line 440: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: asio::write(asio_->socket, asio::buffer(pkt), we);
- Line 640: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: asio_->keepalive_timer.async_wait([this](boost::system::error_code ec) {
- Line 669: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: asio_->reconnect_timer.async_wait([this](boost::system::error_code ec) {
- Line 59: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(len, size_t{4}); ++i) {
- Line 60: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: uint8_t byte = data[i];
- Line 181: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: static std::vector<uint8_t> buildDisconnect() { return {0xE0, 0x00}; }
- Line 256: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto pkt = detail::buildDisconnect();
- Line 399: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (ec) { scheduleReconnect(); return; }
- Line 405: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: asio_->socket.async_connect(
- Line 426: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: asio_->socket.close(ce); // triggers the async_connect error handler
- Line 442: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (we) { scheduleReconnect(); return; }
- Line 455: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (ec) { handleDisconnect(ec.message()); return; }
- Line 469: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (ec) { handleDisconnect(ec.message()); return; }
- Line 523: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: handleDisconnect("broker sent DISCONNECT");
- Line 603: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (ec) { handleDisconnect(ec.message()); return; }
- Line 615: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (ec) { handleDisconnect(ec.message()); return; }
- Line 674: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: void MqttClientService::handleDisconnect(const std::string& reason) {
- Line 701: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (running_.load()) scheduleReconnect();
- Line 717: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: scheduleReconnect();
- Line 730: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (ec) { scheduleReconnect(); return; }
- Line 751: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (ec) { scheduleReconnect(); return; }
- Line 758: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (ec) { scheduleReconnect(); return; }
- Line 777: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: SSL_set_tlsext_host_name(asio_->ssl_stream->native_handle(),
- Line 790: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: scheduleReconnect();
- Line 12: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "server/mqtt_client_service.h"
  Confidence: band=high; score=0.74
- Line 70: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.push_back(static_cast<uint8_t>(val >> 8));
  Confidence: band=high; score=0.74
- Line 71: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>(val >> 8));
- Line 72: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>(val & 0xFF));
- Line 152: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vheader.push_back(qos & 0x03);
  Confidence: band=high; score=0.74
- Line 153: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vheader.push_back(qos & 0x03);
- Line 157: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pkt.push_back(0x82); // SUBSCRIBE
- Line 172: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pkt.push_back(0xA2); // UNSUBSCRIBE
  Confidence: band=high; score=0.74
- Line 173: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pkt.push_back(0xA2); // UNSUBSCRIBE
- Line 238: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void MqttClientService::start() {
  Confidence: band=medium; score=0.66
- Line 262: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: asio_->ssl_stream->lowest_layer().close(ec);
- Line 270: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: asio_->socket.close(ec);
- Line 276: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: asio_->ssl_stream->lowest_layer().close(ec);
- Line 282: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: asio_->socket.close(ec);
- Line 385: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: asio_->ssl_stream->lowest_layer().close(ec);
- Line 390: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: asio_->socket.close(ec);
- Line 426: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: asio_->socket.close(ce); // triggers the async_connect error handler
- Line 553: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { h->onConnected(cid); } catch (...) {}
- Line 569: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { h->onMessage(topic, payload, qos); } catch (...) {}
- Line 681: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: asio_->ssl_stream->lowest_layer().close(ec);
- Line 687: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: asio_->socket.close(ec);
- Line 697: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { h->onDisconnected(reason); } catch (...) {}
- Line 716: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 777: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: SSL_set_tlsext_host_name(asio_->ssl_stream->native_handle(),
  Confidence: band=high; score=0.74
- Line 788: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: asio_->ssl_stream->lowest_layer().close(ce);
- Line 803: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: , topic_prefix_(service.getConfig().cdc_topic_prefix)
  Confidence: band=high; score=0.74
- Line 804: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: , qos_(service.getConfig().cdc_qos) {}
  Confidence: band=high; score=0.74
- Line 806: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: bool MqttCDCTransport::start() {
  Confidence: band=medium; score=0.66
- Line 821: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/shard_repair_api_handler.cpp
Total findings: 56

- Line 130: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: << "input{padding:10px 12px;border-radius:999px;border:1px solid var(--line);min-width:220px;background:#fff}"
  Confidence: band=very_high; score=0.99
- Line 146: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: << "<input id=\"shardId\" placeholder=\"Shard ID for targeted repair\">"
  Confidence: band=very_high; score=0.99
- Line 159: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Use std::unique_ptr<T> or std::make_unique<T>()
  Context: << "function fmtTime(v){return v?new Date(v).toLocaleString():'-';}"
- Line 159: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: << "function fmtTime(v){return v?new Date(v).toLocaleString():'-';}"
- Line 163: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Use std::unique_ptr<T> or std::make_unique<T>()
  Context: << "async function post(url,body){const res=await fetch(url,{method:'POST',headers:{'Content-Type':'
- Line 163: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: << "async function post(url,body){const res=await fetch(url,{method:'POST',headers:{'Content-Type':'
- Line 208: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto ar = auth.authorize(*token, required_scope);
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Context: ['         << "<html lang=\\"en\\">\\n"', '         << "<head><meta charset=\\"utf-8\\">\\n"', '         << "<meta name=\\"viewport\\" content=\\"width=device-width,initial-scale=1\\">\\n"', '         << "<title>Themis Repair Dashboard</title>\\n"', '         << "<style>"']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 130: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: << "input{padding:10px 12px;border-radius:999px;border:1px solid var(--line);min-width:220px;background:#fff}"
  Confidence: band=very_high; score=0.9
- Line 146: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: << "<input id=\"shardId\" placeholder=\"Shard ID for targeted repair\">"
  Confidence: band=very_high; score=0.9
- Line 163: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: << "async function post(url,body){const res=await fetch(url,{method:'POST',headers:{'Content-Type':'
- Line 335: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: const std::string job_id = extractJobId(std::string(req.target()));
  Confidence: band=very_high; score=0.9
- Line 117: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<title>Themis Repair Dashboard</title>\n"
- Line 138: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "</style></head>\n"
- Line 141: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<p class=\"sub\">Shard health, repair jobs, and anti-entropy triggers.</p>"
- Line 141: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<p class=\"sub\">Shard health, repair jobs, and anti-entropy triggers.</p>"
- Line 142: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<div id=\"flash\"></div>"
- Line 142: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<div id=\"flash\"></div>"
- Line 143: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<section class=\"grid\" id=\"summary\"></section>"
- Line 143: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<section class=\"grid\" id=\"summary\"></section>"
- Line 143: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<section class=\"grid\" id=\"summary\"></section>"
- Line 145: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<button id=\"scanBtn\">Start Full Scan</button>"
- Line 145: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<button id=\"scanBtn\">Start Full Scan</button>"
- Line 147: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<button id=\"repairBtn\" class=\"secondary\">Queue Shard Repair</button>"
- Line 147: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<button id=\"repairBtn\" class=\"secondary\">Queue Shard Repair</button>"
- Line 147: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<button id=\"repairBtn\" class=\"secondary\">Queue Shard Repair</button>"
- Line 148: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<button id=\"refreshBtn\">Refresh</button>"
- Line 148: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<button id=\"refreshBtn\">Refresh</button>"
- Line 150: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<h2>Shard Health</h2><table><thead><tr><th>Shard</th><th>Status</th><th>Scanned</th><th>Healthy<
- Line 150: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<h2>Shard Health</h2><table><thead><tr><th>Shard</th><th>Status</th><th>Scanned</th><th>Healthy<
- Line 151: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<h2>Active Jobs</h2><table><thead><tr><th>Job</th><th>Shard</th><th>Document</th><th>Full Scan</
- Line 151: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<h2>Active Jobs</h2><table><thead><tr><th>Job</th><th>Shard</th><th>Document</th><th>Full Scan</
- Line 152: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<h2>Raw JSON</h2><pre id=\"raw\">loading...</pre>"
- Line 152: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<h2>Raw JSON</h2><pre id=\"raw\">loading...</pre>"
- Line 160: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "function badge(status){return `<span class=\"badge ${status}\">${status}</span>`;}"
- Line 160: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "function badge(status){return `<span class=\"badge ${status}\">${status}</span>`;}"
- Line 162: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "async function load(){const res=await fetch('/v1/admin/repair/health');const data=await res.json
- Line 162: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "async function load(){const res=await fetch('/v1/admin/repair/health');const data=await res.json
- Line 162: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "async function load(){const res=await fetch('/v1/admin/repair/health');const data=await res.json
- Line 162: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "async function load(){const res=await fetch('/v1/admin/repair/health');const data=await res.json
- Line 162: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "async function load(){const res=await fetch('/v1/admin/repair/health');const data=await res.json
- Line 162: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "async function load(){const res=await fetch('/v1/admin/repair/health');const data=await res.json
- Line 162: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "async function load(){const res=await fetch('/v1/admin/repair/health');const data=await res.json
- Line 162: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "async function load(){const res=await fetch('/v1/admin/repair/health');const data=await res.json
- Line 162: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "async function load(){const res=await fetch('/v1/admin/repair/health');const data=await res.json
- Line 162: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "async function load(){const res=await fetch('/v1/admin/repair/health');const data=await res.json
- Line 162: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "async function load(){const res=await fetch('/v1/admin/repair/health');const data=await res.json
- Line 162: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "async function load(){const res=await fetch('/v1/admin/repair/health');const data=await res.json
- Line 194: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 254: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: body["active_jobs"].push_back(repairJobToJson(job));
  Confidence: band=high; score=0.74
- Line 254: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: body["active_jobs"].push_back(repairJobToJson(job));
  Confidence: band=high; score=0.74
- Line 255: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: body["active_jobs"].push_back(repairJobToJson(job));
- Line 257: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: body["shards"].push_back(shardReportToJson(report));
  Confidence: band=high; score=0.74
- Line 258: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: body["shards"].push_back(shardReportToJson(report));
- Line 343: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: : http::status::ok;

### src/server/mqtt_session.cpp
Total findings: 55

- Line 209: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = incomingQos2_.find(packetId);
- Line 488: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 5 > array 0
  Remediation: Fix loop condition or increase array size
  Context: const uint8_t packetType = static_cast<uint8_t>(buffer_[0]);
- Line 679: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = subscriptions_.find(topic);
- Line 703: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = persistentSessions_.find(clientId);
- Line 788: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (auto session = sessionWeak.lock()) {
- Line 802: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (auto session = sessions[idx].lock()) {
- Line 822: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: auto session = weak_session.lock();
- Line 0: severity=HIGH; category=uncategorized
  Context: ['                            // Read packet ID (2 bytes)', '                            packetId = static_cast<uint16_t>(', '                                (static_cast<uint16_t>(static_cast<uint8_t>(buffer_[payloadOffset])) << 8) |', '                                static_cast<uint16_t>(static_cast<uint8_t>(buffer_[payloadOffset + 1]))', '                            );']
  Confidence: band=high; score=0.78
- Line 278: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: void MqttSession::handleDisconnect() {
- Line 488: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const uint8_t packetType = static_cast<uint8_t>(buffer_[0]);
- Line 498: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const uint8_t encodedByte = static_cast<uint8_t>(buffer_[i]);
- Line 519: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (static_cast<uint16_t>(static_cast<uint8_t>(buffer_[headerSize])) << 8) |
- Line 520: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<uint16_t>(static_cast<uint8_t>(buffer_[headerSize + 1]))
- Line 593: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (static_cast<uint16_t>(static_cast<uint8_t>(buffer_[headerSize + 2])) << 8) |
- Line 594: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<uint16_t>(static_cast<uint8_t>(buffer_[headerSize + 3]))
- Line 603: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const uint8_t qos = static_cast<uint8_t>(buffer_[topic_offset + topicLen]);
- Line 626: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: handleDisconnect();
- Line 692: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 735: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [topic, msg] : retainedMessages_) {
  Confidence: band=very_high; score=0.9
- Line 796: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [shareName, topics] : sharedSubscriptions_) {
  Confidence: band=very_high; score=0.9
- Line 797: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [filter, sessions] : topics) {
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['    // Build MQTT PUBLISH packet', '    std::vector<uint8_t> packet;', '    uint8_t flags = static_cast<uint8_t>(qos << 1);', '    if (retain) {', '        flags = static_cast<uint8_t>(flags | 0x01u);']
  Confidence: band=medium; score=0.65
- Line 43: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 53: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void MqttSession::start() {
  Confidence: band=medium; score=0.66
- Line 67: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: wsStream_->close(websocket::close_code::normal, ec);
- Line 70: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: socket_.close(ec);
- Line 314: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: packet.push_back(static_cast<uint8_t>(0x20u)); // CONNACK packet type
  Confidence: band=high; score=0.74
- Line 315: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packet.push_back(static_cast<uint8_t>(0x20u)); // CONNACK packet type
- Line 319: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packet.push_back(static_cast<uint8_t>(3u));    // Remaining length
- Line 320: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packet.push_back(static_cast<uint8_t>(sessionPresent ? 1u : 0u));
- Line 321: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packet.push_back(returnCode);
- Line 322: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packet.push_back(static_cast<uint8_t>(0u));    // Properties length
- Line 324: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packet.push_back(static_cast<uint8_t>(2u));    // Remaining length
- Line 325: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packet.push_back(static_cast<uint8_t>(sessionPresent ? 1u : 0u));
- Line 326: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packet.push_back(returnCode);
- Line 329: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: writeQueue_.push_back(std::move(packet));
- Line 374: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packet.push_back(encodedByte);
- Line 385: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packet.push_back(static_cast<uint8_t>((topicLen >> 8) & 0xFFu));
- Line 386: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packet.push_back(static_cast<uint8_t>(topicLen & 0xFFu));
- Line 393: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packet.push_back(static_cast<uint8_t>((packetId >> 8) & 0xFFu));
- Line 394: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packet.push_back(static_cast<uint8_t>(packetId & 0xFFu));
- Line 400: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: writeQueue_.push_back(std::move(packet));
- Line 407: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packet.push_back(static_cast<uint8_t>((packetId >> 8) & 0xFFu));
- Line 408: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packet.push_back(static_cast<uint8_t>(packetId & 0xFFu));
- Line 409: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: writeQueue_.push_back(std::move(packet));
- Line 425: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packet.push_back(static_cast<uint8_t>((packetId >> 8) & 0xFFu));
- Line 426: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packet.push_back(static_cast<uint8_t>(packetId & 0xFFu));
- Line 427: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: writeQueue_.push_back(std::move(packet));
- Line 434: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packet.push_back(static_cast<uint8_t>((packetId >> 8) & 0xFFu));
- Line 435: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packet.push_back(static_cast<uint8_t>(packetId & 0xFFu));
- Line 436: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: writeQueue_.push_back(std::move(packet));
- Line 736: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(msg);
  Confidence: band=high; score=0.74
- Line 737: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(msg);
- Line 802: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: if (auto session = sessions[idx].lock()) {
  Confidence: band=high; score=0.74
- Line 819: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<MqttSession*> seen;
  Confidence: band=medium; score=0.66

### src/server/websocket_session.cpp
Total findings: 43

- Line 780: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: cdc_poll_timer_->async_wait([this](beast::error_code ec) {
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 95: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ws_tls_->async_accept(
- Line 108: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ws_plain_->async_accept(
- Line 132: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(welcome.dump());
- Line 202: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto responses = cdc_stream_handler_->handleFrame(msg);
- Line 204: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(resp.dump());
- Line 231: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(response.dump());
- Line 270: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(response.dump());
- Line 277: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(response.dump());
- Line 294: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(response.dump());
- Line 306: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(response.dump());
- Line 329: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: is_aql ? server_->query_api_->handleQueryAql(http_req)
- Line 330: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: : server_->query_api_->handleQuery(http_req);
- Line 342: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(ws_resp.dump());
- Line 350: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(response.dump());
- Line 362: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(response.dump());
- Line 373: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(response.dump());
- Line 396: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(response.dump());
- Line 399: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: void WebSocketSession::send(const std::string& message) {
- Line 419: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: net::dispatch(ws_plain_->get_executor(), [self, data]() mutable {
- Line 420: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: self->sendBinaryOnExecutor(std::move(data));
- Line 696: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (auto* handler = session->getCdcStreamHandler()) {
- Line 697: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!handler->hasSubscriptions()) continue;
- Line 699: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto frames = handler->pollEvents(*changefeed_);
- Line 703: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto redeliveries = handler->checkRedelivery();
- Line 720: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Legacy /v2/changes polling path.
  Confidence: band=high; score=0.8
- Line 761: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: session->send(cdc_message.dump());
- Line 791: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, session] : sessions_) {
  Confidence: band=very_high; score=0.9
- Line 818: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [id, session] : sessions_) {
  Confidence: band=very_high; score=0.9
- Line 820: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: session->send(message);
- Line 840: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, session] : sessions_) {
  Confidence: band=very_high; score=0.9
- Line 5: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: * W1-S03: active_ data race fixed (bool→atomic<bool>); close() dispatched to executor
- Line 510: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ws_tls_->close(websocket::close_code::internal_error, close_ec);
- Line 512: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ws_plain_->close(websocket::close_code::internal_error, close_ec);
- Line 546: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ws_tls_->close(websocket::close_code::internal_error, close_ec);
- Line 548: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ws_plain_->close(websocket::close_code::internal_error, close_ec);
- Line 556: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void WebSocketSession::close() {
- Line 584: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ws_tls_->close(websocket::close_code::normal, ec);
- Line 586: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ws_plain_->close(websocket::close_code::normal, ec);
- Line 792: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cdc_sessions.push_back(session);
  Confidence: band=high; score=0.74
- Line 793: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cdc_sessions.push_back(session);
- Line 855: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: session->close();

### src/server/policy_engine.cpp
Total findings: 40

- Line 205: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: "replaced all policies, new count=" + std::to_string(count));
- Line 245: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: PolicyEngine::Decision PolicyEngine::authorize(const std::string& user_id,
  Confidence: band=very_high; score=0.99
- Line 150: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& p : list) out.push_back(toJson(p));
  Confidence: band=very_high; score=0.9
- Line 214: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::length_error(
- Line 374: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["subjects"] = json::array(); for (const auto& s : p.subjects) j["subjects"].push_back(s);
- Line 375: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["actions"] = json::array(); for (const auto& a : p.actions) j["actions"].push_back(a);
- Line 68: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& r : n["resources"]) p.resources.push_back(r.as<std::string>());
  Confidence: band=high; score=0.74
- Line 68: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& r : n["resources"]) p.resources.push_back(r.as<std::string>());
  Confidence: band=high; score=0.74
- Line 69: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& r : n["resources"]) p.resources.push_back(r.as<std::string>());
- Line 72: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto eff = n["effect"].as<std::string>("allow");
  Confidence: band=high; score=0.74
- Line 77: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& ip : n["allowed_ip_prefixes"]) p.allowed_ip_prefixes.push_back(ip.as<std::string>());
  Confidence: band=high; score=0.74
- Line 78: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& ip : n["allowed_ip_prefixes"]) p.allowed_ip_prefixes.push_back(ip.as<std::string>()
- Line 82: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& ua : n["allowed_user_agent_patterns"]) p.allowed_user_agent_patterns.push_back(ua.as<std::string>());
  Confidence: band=high; score=0.74
- Line 83: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& ua : n["allowed_user_agent_patterns"]) p.allowed_user_agent_patterns.push_back(ua.a
- Line 86: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 93: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (p) loaded.push_back(std::move(*p));
  Confidence: band=high; score=0.74
- Line 93: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (p) loaded.push_back(std::move(*p));
  Confidence: band=high; score=0.74
- Line 94: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (p) loaded.push_back(std::move(*p));
- Line 100: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (p) loaded.push_back(std::move(*p));
  Confidence: band=high; score=0.74
- Line 101: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (p) loaded.push_back(std::move(*p));
- Line 118: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (p) loaded.push_back(std::move(*p));
  Confidence: band=high; score=0.74
- Line 119: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (p) loaded.push_back(std::move(*p));
- Line 123: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (p) loaded.push_back(std::move(*p));
  Confidence: band=high; score=0.74
- Line 124: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (p) loaded.push_back(std::move(*p));
- Line 136: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 150: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& p : list) out.push_back(toJson(p));
- Line 362: severity=MEDIUM; category=expensive_copy
  Description: Unnecessary expensive copy
  Remediation: Use const reference (const T&) or std::move if transfer is needed
  Context: if (user_agent->find(pat) != std::string::npos) { ok = true; break; }
- Line 373: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["subjects"] = json::array(); for (const auto& s : p.subjects) j["subjects"].push_back(s);
  Confidence: band=high; score=0.74
- Line 374: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: j["subjects"] = json::array(); for (const auto& s : p.subjects) j["subjects"].push_back(s);
- Line 374: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["actions"] = json::array(); for (const auto& a : p.actions) j["actions"].push_back(a);
  Confidence: band=high; score=0.74
- Line 375: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: j["actions"] = json::array(); for (const auto& a : p.actions) j["actions"].push_back(a);
- Line 391: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (j.contains("resources")) for (const auto& r : j["resources"]) p.resources.push_back(r.get<std::string>());
  Confidence: band=high; score=0.74
- Line 391: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (j.contains("resources")) for (const auto& r : j["resources"]) p.resources.push_back(r.get<std::string>());
  Confidence: band=high; score=0.74
- Line 391: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (j.contains("resources")) for (const auto& r : j["resources"]) p.resources.push_back(r.get<std::string>());
  Confidence: band=high; score=0.74
- Line 392: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (j.contains("resources")) for (const auto& r : j["resources"]) p.resources.push_back(r.get<std::s
- Line 394: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (j.contains("allowed_ip_prefixes")) for (const auto& ip : j["allowed_ip_prefixes"]) p.allowed_ip_prefixes.push_back(ip.get<std::string>());
  Confidence: band=high; score=0.74
- Line 395: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (j.contains("allowed_ip_prefixes")) for (const auto& ip : j["allowed_ip_prefixes"]) p.allowed_ip_
- Line 397: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (j.contains("allowed_user_agent_patterns")) for (const auto& ua : j["allowed_user_agent_patterns"]) p.allowed_user_agent_patterns.push_back(ua.get<std::string>());
  Confidence: band=high; score=0.74
- Line 398: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (j.contains("allowed_user_agent_patterns")) for (const auto& ua : j["allowed_user_agent_patterns"
- Line 400: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/async_job_api_handler.cpp
Total findings: 38

- Line 144: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = jobs_.begin(); it != jobs_.end(); ) {
- Line 160: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = jobs_.find(id);
- Line 228: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = jobs_.begin(); it != jobs_.end(); ) {
- Line 520: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto jobs = registry_->allJsonSnapshots();
- Line 541: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto job = registry_->getJsonSnapshot(job_id);
- Line 567: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto status_result = registry_->requestCancel(job_id);
- Line 578: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return makeJsonResponse(http::status::conflict,
  Confidence: band=very_high; score=0.99
- Line 144: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = jobs_.begin(); it != jobs_.end(); ) {
  Confidence: band=very_high; score=0.9
- Line 146: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> rlock(rec.mu);
- Line 158: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: std::shared_ptr<AsyncJobRecord> AsyncJobRegistry::get(const std::string& id) const {
  Confidence: band=very_high; score=0.9
- Line 161: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return (it != jobs_.end()) ? it->second : nullptr;
- Line 161: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return (it != jobs_.end()) ? it->second : nullptr;
- Line 168: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& kv : jobs_) {
  Confidence: band=very_high; score=0.9
- Line 177: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = jobs_.find(id);
- Line 191: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& job : jobs) {
  Confidence: band=very_high; score=0.9
- Line 201: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = jobs_.find(id);
- Line 228: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = jobs_.begin(); it != jobs_.end(); ) {
  Confidence: band=very_high; score=0.9
- Line 230: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> rlock(rec.mu);
- Line 275: severity=HIGH; category=posix_only_api
  Description: POSIX-only API getpid( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: std::to_string(static_cast<long long>(::getpid())) +
- Line 295: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: f.wait_for(std::chrono::seconds(2));
- Line 360: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (job->cancel_requested.load(std::memory_order_acquire)) {
- Line 372: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (job->cancel_requested.load(std::memory_order_acquire)) {
- Line 430: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: f.wait_for(std::chrono::seconds(0)) ==
- Line 529: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: const std::string target(req.target());
  Confidence: band=very_high; score=0.9
- Line 555: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: const std::string target(req.target());
  Confidence: band=very_high; score=0.9
- Line 58: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool isValidAsyncQuery(std::string_view query) {
  Confidence: band=high; score=0.74
- Line 168: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(kv.second);
  Confidence: band=high; score=0.74
- Line 169: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(kv.second);
- Line 191: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(job->toJson());
  Confidence: band=high; score=0.74
- Line 192: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(job->toJson());
- Line 369: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::string final_status;
- Line 391: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::string final_status;
- Line 407: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 431: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::future_status::ready;
- Line 433: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures_.push_back(std::move(fut));
  Confidence: band=high; score=0.74
- Line 434: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures_.push_back(std::move(fut));
- Line 448: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_hdr = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 512: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: {{"job_id", job->id}, {"status", "pending"}}, req);

### src/server/entity_api_handler.cpp
Total findings: 37

- Line 157: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto authz = auth_->authorize(*token_opt, scope);
  Confidence: band=very_high; score=0.99
- Line 589: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: auto write_result = strategy->write(
- Line 882: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator pos may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto pos = key.find(':');
- Line 1138: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ct_it->value().find("application/x-ndjson") == std::string_view::npos) {
- Line 569: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const std::vector<uint8_t>& data) -> bool {
- Line 574: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: bool ok = storage_->put(prefixed_key, data);
- Line 574: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: bool ok = storage_->put(prefixed_key, data);
- Line 739: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: hook = nullptr;
  Context: THEMIS_WARN("Geo index delete hook failed for {}:{}: {}", table, pk, e.what());
- Line 756: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: failed = nullptr;
  Context: "Index/Storage delete failed: " + st.message, req);
- Line 989: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: hook = nullptr;
  Context: THEMIS_WARN("Geo index delete hook failed for {}:{}: {}", vop.table, vop.pk, e.what());
- Line 110: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 144: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 235: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { entity_json = json::parse(blob_str); } catch (...) {
- Line 249: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto coll = schema["collections"][table];
  Confidence: band=high; score=0.74
- Line 254: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto& f : coll["encryption"]["fields"]) if (f.is_string()) fields.push_back(f.get<std::string>(
- Line 263: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { encFlag = entity_json[f + "_enc"].get<bool>(); } catch (...) {
- Line 269: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto enc_meta_str = entity_json[f + "_encrypted"].get<std::string>();
  Confidence: band=high; score=0.74
- Line 276: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { group_name = entity_json[f + "_group"].get<std::string>(); } catch (...) {
- Line 305: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 396: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto coll = schema["collections"][table];
  Confidence: band=high; score=0.74
- Line 408: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (f.is_string()) fields.push_back(f.get<std::string>());
  Confidence: band=high; score=0.74
- Line 409: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (f.is_string()) fields.push_back(f.get<std::string>());
- Line 443: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (float val : vec) j_arr.push_back(val);
- Line 756: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: "Index/Storage delete failed: " + st.message, req);
- Line 852: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back({
  Confidence: band=high; score=0.74
- Line 853: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({
- Line 861: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({
- Line 884: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({
- Line 896: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({
- Line 958: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back({
  Confidence: band=high; score=0.74
- Line 959: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({
- Line 995: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({
- Line 1007: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({
- Line 1174: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: documents.push_back(json::parse(line));
- Line 1176: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({
- Line 1210: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back({
  Confidence: band=high; score=0.74
- Line 1211: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({

### src/server/lora_api_handler.cpp
Total findings: 37

- Line 344: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto& td = body->at("training_data");
- Line 460: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto& td = body->at("additional_training_data");
- Line 787: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: memory_mb = static_cast<double>(adapter_opt->memory_bytes) / (1024.0 * 1024.0);
- Line 937: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto handle   = inference_engine_->submit(eng_req);
- Line 304: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: model = nullptr;
  Context: "Failed to delete model",
- Line 419: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: json response_data = adapter_info->toJSON();
- Line 420: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response_data["status"] = adapter_info->is_loaded ? "ready" : "stored";
- Line 421: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response_data["created_at"] = std::chrono::system_clock::to_time_t(
- Line 422: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: adapter_info->metadata.created_at
- Line 532: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: adapter = nullptr;
  Context: "Failed to delete adapter: " + adapter_id
- Line 541: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: adapter = nullptr;
  Context: "Failed to delete adapter",
- Line 926: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: eng_req.base_request.model_id   = model_id.empty() ? "default" : model_id;
- Line 937: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto handle   = inference_engine_->submit(eng_req);
- Line 1323: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: bool stored = storage_service->saveAdapter(adapter_id, weights, metadata);
- Line 17: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "llm/lora_framework/lora_storage_service.h"
  Confidence: band=high; score=0.74
- Line 18: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "llm/lora_framework/lora_training_service.h"
  Confidence: band=high; score=0.74
- Line 250: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 261: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 532: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: "Failed to delete adapter: " + adapter_id
- Line 562: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::string status_filter;
- Line 577: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 588: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 618: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered_adapters.push_back(adapter);
  Confidence: band=high; score=0.74
- Line 619: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: filtered_adapters.push_back(adapter);
- Line 628: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adapters.push_back(filtered_adapters[i].toJSON());
  Confidence: band=high; score=0.74
- Line 629: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: adapters.push_back(filtered_adapters[i].toJSON());
- Line 857: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::string status_str;
- Line 1084: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1132: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1294: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto created_ns = metadata_json["created_at"].get<uint64_t>();
  Confidence: band=high; score=0.74
- Line 1302: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto updated_ns = metadata_json["updated_at"].get<uint64_t>();
  Confidence: band=high; score=0.74
- Line 1430: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::created);
- Line 1459: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(e.toJSON());
  Confidence: band=high; score=0.74
- Line 1460: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(e.toJSON());
- Line 1492: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(s.toJSON());
  Confidence: band=high; score=0.74
- Line 1493: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(s.toJSON());
- Line 1531: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: }, status);

### src/server/vector_api_handler.cpp
Total findings: 37

- Line 764: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: // Extract Bearer token and use auth_->authorize() to check the required
  Confidence: band=very_high; score=0.99
- Line 779: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto ar = auth_->authorize(*token, permission);
  Confidence: band=very_high; score=0.99
- Line 207: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Legacy Format
  Confidence: band=high; score=0.8
- Line 263: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::string object_name = vector_index_->getObjectName();
- Line 281: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: object_name = vector_index_->getObjectName();
- Line 420: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: {"objectName", vector_index_->getObjectName()},
- Line 482: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::string fullPrefix = vector_index_->getObjectName() + ":" + prefix;
- Line 483: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: storage_->scanPrefix(fullPrefix, [&](std::string_view key, std::string_view /*value*/){
- Line 588: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: {"objectName", vector_index_->getObjectName()},
- Line 654: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: {"objectName", vector_index_->getObjectName()},
- Line 124: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: queryVector.push_back(val.get<float>());
  Confidence: band=high; score=0.74
- Line 125: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: queryVector.push_back(val.get<float>());
- Line 172: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 185: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "Vector search failed: " + status.message, req);
- Line 193: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back({{"pk", results[i].pk}, {"distance", results[i].distance}});
  Confidence: band=high; score=0.74
- Line 194: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: items.push_back({{"pk", results[i].pk}, {"distance", results[i].distance}});
- Line 209: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resultJson.push_back({{"pk", result.pk}, {"distance", result.distance}});
  Confidence: band=high; score=0.74
- Line 210: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: resultJson.push_back({{"pk", result.pk}, {"distance", result.distance}});
- Line 304: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto coll = schema_json["collections"][object_name];
  Confidence: band=high; score=0.74
- Line 306: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ecfg = coll["encryption"];
  Confidence: band=high; score=0.74
- Line 309: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& f : ecfg["fields"]) if (f.is_string()) vector_enc_fields.push_back(f.get<std::strin
- Line 314: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto itf = coll["fields"].begin(); itf != coll["fields"].end(); ++itf) {
  Confidence: band=high; score=0.74
- Line 316: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vector_enc_fields.push_back(itf.key());
  Confidence: band=high; score=0.74
- Line 316: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vector_enc_fields.push_back(itf.key());
  Confidence: band=high; score=0.74
- Line 317: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vector_enc_fields.push_back(itf.key());
- Line 319: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { /* ignore */ }
- Line 326: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 349: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vec.push_back(v.get<float>());
  Confidence: band=high; score=0.74
- Line 350: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vec.push_back(v.get<float>());
- Line 358: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto fit = it["fields"].begin(); fit != it["fields"].end(); ++fit) {
  Confidence: band=high; score=0.74
- Line 407: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 488: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 520: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "Failed to save index: " + status.message, req);
- Line 556: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "Failed to load index: " + status.message, req);
- Line 621: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "Failed to set efSearch: " + status.message, req);
- Line 767: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 792: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/http2_session.cpp
Total findings: 32

- Line 254: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: read_timer_.async_wait([weak_self](const boost::system::error_code& ec) {
- Line 284: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: write_timer_.async_wait([weak_self](const boost::system::error_code& ec) {
- Line 665: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = headers.find("content-type");
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 204: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ssize_t readlen = nghttp2_session_mem_recv(ng2_session_, read_buffer_.data(), bytes_transferred);
- Line 216: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ssize_t datalen = nghttp2_session_mem_send(ng2_session_, &data);
- Line 250: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: const uint32_t timeout_ms = server_->hot_request_timeout_ms_.load(std::memory_order_acquire);
- Line 280: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: const uint32_t timeout_ms = server_->hot_request_timeout_ms_.load(std::memory_order_acquire);
- Line 357: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: self->response_buffers_.erase(stream_id);
- Line 374: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto it = self->response_buffers_.find(stream_id);
- Line 564: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [name, value] : headers) {
  Confidence: band=very_high; score=0.9
- Line 577: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response_buffers_[stream_id] = resp_buffer;
- Line 679: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [name, value] : headers) {
  Confidence: band=very_high; score=0.9
- Line 694: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response_buffers_[promised_stream_id] = resp_buffer;
- Line 31: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: static const unsigned char alpn_proto_list[] = "\x02h2\x08http/1.1";
- Line 115: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void Http2Session::start() {
  Confidence: band=medium; score=0.66
- Line 267: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: self->stream_.lowest_layer().close(close_ec);
- Line 297: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: self->stream_.lowest_layer().close(close_ec);
- Line 533: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> response_headers;
  Confidence: band=medium; score=0.66
- Line 550: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, std::string>& headers) {
  Confidence: band=medium; score=0.66
- Line 554: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nva.push_back({
  Confidence: band=high; score=0.74
- Line 555: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nva.push_back({
- Line 564: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nva.push_back({
  Confidence: band=high; score=0.74
- Line 565: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nva.push_back({
- Line 607: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nva.push_back({
- Line 615: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nva.push_back({
- Line 623: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nva.push_back({
- Line 655: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response_nva.push_back({
- Line 670: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response_nva.push_back({
- Line 680: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response_nva.push_back({
  Confidence: band=high; score=0.74
- Line 681: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response_nva.push_back({

### src/server/graph_api_handler.cpp
Total findings: 30

- Line 75: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto [status, visited] = graph_index_->bfs(start_vertex, static_cast<int>(max_depth));
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #450 [REFACTOR] Extract GraphApiHandler from http_server.cpp (2026-03-11T21:31:09Z)
- Line 237: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: edge = nullptr;
  Context: "Failed to delete edge: " + status.message, req);
- Line 267: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: error = nullptr;
  Context: THEMIS_ERROR("Edge delete error: {}", e.what());
- Line 513: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto handle = optimizer_->registerIncrementalBFS(
- Line 515: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [this, handle_ptr](
- Line 517: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: incremental_results_[*handle_ptr] = result;
- Line 595: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: optimizer_->unregisterIncrementalQuery(handle);
- Line 731: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Normalize legacy/empty exports to an object JSON so clients can round-trip
  Confidence: band=high; score=0.8
- Line 154: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "Failed to create edge: " + status.message, req);
- Line 237: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: "Failed to delete edge: " + status.message, req);
- Line 237: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "Failed to delete edge: " + status.message, req);
- Line 382: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: body += "themis_graph_latency_ms_bucket{le=\"";
  Confidence: band=high; score=0.74
- Line 383: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: body += "themis_graph_latency_ms_bucket{le=\"";
- Line 385: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: body += "\"} ";
- Line 387: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: body += '\n';
- Line 583: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 815: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (v.is_string()) qc.forbidden_vertices.push_back(v.get<std::string>());
  Confidence: band=high; score=0.74
- Line 816: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (v.is_string()) qc.forbidden_vertices.push_back(v.get<std::string>());
- Line 820: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (v.is_string()) qc.required_vertices.push_back(v.get<std::string>());
  Confidence: band=high; score=0.74
- Line 821: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (v.is_string()) qc.required_vertices.push_back(v.get<std::string>());
- Line 825: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (v.is_string()) qc.node_labels.push_back(v.get<std::string>());
  Confidence: band=high; score=0.74
- Line 826: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (v.is_string()) qc.node_labels.push_back(v.get<std::string>());
- Line 864: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: alt_arr.push_back({{"algorithm", algo_name(alt_algo)}, {"estimated_cost", alt_cost}});
  Confidence: band=high; score=0.74
- Line 865: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: alt_arr.push_back({{"algorithm", algo_name(alt_algo)}, {"estimated_cost", alt_cost}});
- Line 869: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_arr.push_back(sid);
  Confidence: band=high; score=0.74
- Line 870: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shard_arr.push_back(sid);
- Line 990: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (pv.is_string()) pverts.push_back(pv.get<std::string>());
  Confidence: band=high; score=0.74
- Line 991: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (pv.is_string()) pverts.push_back(pv.get<std::string>());
- Line 998: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pedges.emplace_back(pe[0].get<std::string>(), pe[1].get<std::string>());
  Confidence: band=high; score=0.74

### src/server/rope_api_handler.cpp
Total findings: 30

- Line 195: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto config_opt = vector_index_->getRotaryEmbeddingConfig();
- Line 812: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto config_opt = vector_index_->getRotaryEmbeddingConfig();
- Line 893: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: // auth_->authorize(); deny with HTTP 403 when the scope is not granted.
  Confidence: band=very_high; score=0.99
- Line 906: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto ar = auth_->authorize(*token, permission);
  Confidence: band=very_high; score=0.99
- Line 71: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: span.setAttribute("http.path", std::string(req.target()));
- Line 174: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: span.setAttribute("http.path", std::string(req.target()));
- Line 238: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: span.setAttribute("http.path", std::string(req.target()));
- Line 274: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: error = nullptr;
  Context: THEMIS_ERROR("RoPE config delete error: {}", e.what());
- Line 291: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: span.setAttribute("http.path", std::string(req.target()));
- Line 416: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: span.setAttribute("http.path", std::string(req.target()));
- Line 540: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: span.setAttribute("http.path", std::string(req.target()));
- Line 566: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: "Missing or invalid required field: query (must be array)", req);
- Line 654: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: span.setAttribute("http.path", std::string(req.target()));
- Line 792: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: span.setAttribute("http.path", std::string(req.target()));
- Line 132: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "Failed to apply RoPE configuration: " + status.message, req);
- Line 367: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vec.push_back(v.get<float>());
  Confidence: band=high; score=0.74
- Line 368: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vec.push_back(v.get<float>());
- Line 380: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "Failed to add entity with rotation: " + status.message, req);
- Line 491: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vec.push_back(v.get<float>());
  Confidence: band=high; score=0.74
- Line 492: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vec.push_back(v.get<float>());
- Line 504: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "Failed to add entity with relational rotation: " + status.message, req);
- Line 582: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: query_vector.push_back(val.get<float>());
  Confidence: band=high; score=0.74
- Line 583: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: query_vector.push_back(val.get<float>());
- Line 605: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "Search with rotation failed: " + status.message, req);
- Line 610: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results_array.push_back({
  Confidence: band=high; score=0.74
- Line 611: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results_array.push_back({
- Line 733: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vec.push_back(v.get<float>());
  Confidence: band=high; score=0.74
- Line 734: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vec.push_back(v.get<float>());
- Line 751: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 894: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/distributed_gateway.cpp
Total findings: 28

- Line 165: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: seen.insert(node.node_id);
  Confidence: band=very_high; score=0.99
- Line 275: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (node.has_value() && node->node_id != config_.node_id) {
- Line 475: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator q may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto q = key.find('?');
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 120: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (unsigned char c : salted) {
  Confidence: band=very_high; score=0.9
- Line 129: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (uint32_t i = 0; i < virtual_nodes_; ++i) {
  Confidence: band=very_high; score=0.9
- Line 183: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("DistributedGateway: gateway must be non-null");
- Line 286: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return gateway_->handleRequest(req, std::move(local_handler));
- Line 315: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: return future.get();
  Confidence: band=very_high; score=0.9
- Line 341: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: gateway_->registerHandler(pattern, std::move(handler));
- Line 473: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: std::string key = std::string(req.target());
  Confidence: band=very_high; score=0.9
- Line 59: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: routes_json.push_back(r.toJson());
  Confidence: band=high; score=0.74
- Line 60: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: routes_json.push_back(r.toJson());
- Line 92: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cfg.routes.push_back(GatewayRouteConfig::fromJson(r));
  Confidence: band=high; score=0.74
- Line 93: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cfg.routes.push_back(GatewayRouteConfig::fromJson(r));
- Line 98: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = j["rate_limits"].begin(); it != j["rate_limits"].end(); ++it) {
  Confidence: band=high; score=0.74
- Line 137: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ConsistentHashRing::removeNode(const std::string& node_id)
  Context: void ConsistentHashRing::removeNode(const std::string& node_id) {
  Confidence: band=medium; score=0.56
- Line 161: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ConsistentHashRing::nodeCount()
  Context: std::size_t ConsistentHashRing::nodeCount() const {
  Confidence: band=medium; score=0.56
- Line 163: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 215: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void DistributedGateway::start() {
  Confidence: band=medium; score=0.66
- Line 366: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Stale entry – ignore (idempotent apply)
  Confidence: band=high; score=0.74
- Line 408: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back({
  Confidence: band=high; score=0.74
- Line 409: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nodes.push_back({
- Line 448: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: raft_cfg.cluster_members.push_back(n.node_id);
  Confidence: band=high; score=0.74
- Line 449: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: raft_cfg.cluster_members.push_back(n.node_id);
- Line 487: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto upgrade = req[http::field::upgrade];
  Confidence: band=high; score=0.74
- Line 498: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto accept = req[http::field::accept];
  Confidence: band=high; score=0.74

### src/server/schema_api_handler.cpp
Total findings: 28

- Line 681: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& s : is.getStatistics(std::string_view(table_name))) {
- Line 138: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response["tables"].push_back(table_info);
  Confidence: band=high; score=0.74
- Line 139: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response["tables"].push_back(table_info);
- Line 668: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(col.toJSON());
  Confidence: band=high; score=0.74
- Line 669: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(col.toJSON());
- Line 681: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats_arr.push_back(s.toJSON());
  Confidence: band=high; score=0.74
- Line 682: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats_arr.push_back(s.toJSON());
- Line 685: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats_arr.push_back(s.toJSON());
  Confidence: band=high; score=0.74
- Line 686: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats_arr.push_back(s.toJSON());
- Line 829: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: c_arr.push_back(c.toJSON());
  Confidence: band=high; score=0.74
- Line 830: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: c_arr.push_back(c.toJSON());
- Line 912: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 973: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { return std::stoull(val); } catch (...) { return 0; }
- Line 1036: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rec_arr.push_back(r.toJSON());
  Confidence: band=high; score=0.74
- Line 1036: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rec_arr.push_back(r.toJSON());
  Confidence: band=high; score=0.74
- Line 1037: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rec_arr.push_back(r.toJSON());
- Line 1057: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rec_arr.push_back(r.toJSON());
  Confidence: band=high; score=0.74
- Line 1058: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rec_arr.push_back(r.toJSON());
- Line 1157: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back({{"error", "Table schema missing 'name' field"}});
  Confidence: band=high; score=0.74
- Line 1158: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({{"error", "Table schema missing 'name' field"}});
- Line 1164: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({{"table", schema.name},
- Line 1261: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: valid_rows.push_back({{"index", row_index}, {"row", row_json}});
  Confidence: band=high; score=0.74
- Line 1262: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: valid_rows.push_back({{"index", row_index}, {"row", row_json}});
- Line 1265: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& v : violations) viol_arr.push_back(v.toJSON());
- Line 1265: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: invalid_rows.push_back({{"index", row_index}, {"row", row_json},
  Confidence: band=high; score=0.74
- Line 1266: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: invalid_rows.push_back({{"index", row_index}, {"row", row_json},
- Line 1392: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entry.source_columns.push_back(
  Confidence: band=high; score=0.74
- Line 1393: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entry.source_columns.push_back(

### src/server/tenant_manager.cpp
Total findings: 28

- Line 59: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: usage_[config_.default_tenant_id]->tenant_id = config_.default_tenant_id;
- Line 210: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = tenants_.find(tid);
- Line 623: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto it = domain_to_tenant_.find(key);
- Line 632: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator tenantIt may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto tenantIt = tenants_.find(tid);
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
- Line 89: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [tid, cfg] : tenants_) {
  Confidence: band=very_high; score=0.9
- Line 90: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& domain : cfg.custom_domains) {
  Confidence: band=very_high; score=0.9
- Line 139: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& domain : newConfig.custom_domains) {
  Confidence: band=very_high; score=0.9
- Line 206: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: default = nullptr;
  Context: THEMIS_WARN("TenantManager: Cannot delete default tenant");
- Line 216: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& domain : it->second.custom_domains) {
  Confidence: band=very_high; score=0.9
- Line 264: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, config] : tenants_) {
  Confidence: band=very_high; score=0.9
- Line 461: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return it != usage_.end() ? it->second.get() : nullptr;
- Line 461: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return it != usage_.end() ? it->second.get() : nullptr;
- Line 467: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return it != usage_.end() ? it->second.get() : nullptr;
- Line 467: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return it != usage_.end() ? it->second.get() : nullptr;
- Line 664: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Escape label value safely using a new string
- Line 206: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: THEMIS_WARN("TenantManager: Cannot delete default tenant");
- Line 264: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(config);
  Confidence: band=high; score=0.74
- Line 265: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(config);
- Line 511: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void TenantManager::recordQuery(std::string_view tenant_id) {
  Confidence: band=high; score=0.74
- Line 668: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: tid += '\\';
  Confidence: band=high; score=0.74
- Line 668: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: tid += '\\';
  Confidence: band=high; score=0.74
- Line 669: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: tid += '\\';
- Line 669: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: tid += '\\';

### src/server/import_api_handler.cpp
Total findings: 27

- Line 312: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto job = registry_->getJsonSnapshot(job_id);
- Line 324: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto snapshot = registry_->getRunningAndJsonSnapshot(job_id);
- Line 340: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto updated = registry_->getJsonSnapshot(job_id);
- Line 347: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto jobs = registry_->allJsonSnapshots();
- Line 355: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto jobs = registry_->allJsonSnapshots();
- Line 491: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto source_path_opt = registry_->getSourcePathSnapshot(job_id);
- Line 194: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto handle = importer_->importDataAsync(source_path, opts);
- Line 195: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: registry_->add(handle);
- Line 197: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: jsonOk(res, handle->toJson());
- Line 247: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto handle = importer->importDataAsync(source_path, opts);
- Line 248: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: registry_->add(handle);
- Line 250: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: jsonOk(res, handle->toJson());
- Line 303: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: registry_->add(handle);
- Line 305: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: jsonOk(res, handle->toJson());
- Line 370: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (status == "completed" && stats_it != job.end() && stats_it->is_object()) {
- Line 653: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& entry : body) {
  Confidence: band=very_high; score=0.9
- Line 441: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (t.is_string()) opts.include_tables.push_back(t.get<std::string>());
- Line 445: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (t.is_string()) opts.exclude_tables.push_back(t.get<std::string>());
- Line 591: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dst.push_back(entry.get<std::string>());
  Confidence: band=high; score=0.74
- Line 592: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dst.push_back(entry.get<std::string>());
- Line 605: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: warn_arr.push_back(message);
  Confidence: band=high; score=0.74
- Line 606: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: warn_arr.push_back(message);
- Line 608: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: err_arr.push_back(message);
- Line 614: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: warn_arr.push_back("Circular reference detected: " + c.get<std::string>());
  Confidence: band=high; score=0.74
- Line 615: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: warn_arr.push_back("Circular reference detected: " + c.get<std::string>());
- Line 659: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: validated.push_back(entry);
  Confidence: band=high; score=0.74
- Line 660: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: validated.push_back(entry);

### src/server/changefeed_api_handler.cpp
Total findings: 26

- Line 68: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator start may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto start = token.find_first_not_of(" \t");
- Line 69: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator end may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto end = token.find_last_not_of(" \t");
- Line 1027: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto auth_result = auth_->authorize(*token, required_scope);
  Confidence: band=very_high; score=0.99
- Line 1122: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto auth_result = auth_->authorize(*token, required_scope);
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 205: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response["events"] = json::array();
- Line 453: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Activation (legacy): `THEMIS_ENABLE_SSE` + `keep_alive=true` + `sse_manager_ != nullptr`
  Confidence: band=high; score=0.8
- Line 462: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: uint64_t conn_id = sse_manager_->registerConnection(from_seq, key_prefix, event_types);
- Line 540: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 206: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response["events"].push_back(event.toJson());
  Confidence: band=high; score=0.74
- Line 207: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response["events"].push_back(event.toJson());
- Line 316: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 332: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 352: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 372: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 408: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 422: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 851: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto v = body["max_age_hours"].get<uint32_t>();
  Confidence: band=high; score=0.74
- Line 859: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto v = body["max_event_count"].get<uint64_t>();
  Confidence: band=high; score=0.74
- Line 867: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto v = body["max_size_bytes"].get<uint64_t>();
  Confidence: band=high; score=0.74
- Line 875: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto v = body["cleanup_interval_minutes"].get<uint32_t>();
  Confidence: band=high; score=0.74
- Line 998: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 1057: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> headers_map;
  Confidence: band=medium; score=0.66
- Line 1093: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 1142: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> headers_map;
  Confidence: band=medium; score=0.66

### src/server/content_api_handler.cpp
Total findings: 26

- Line 228: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator pos may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto pos = path.find("/chunks");
- Line 72: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 136: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(static_cast<char>((val >> valb) & 0xFF));
  Confidence: band=high; score=0.74
- Line 137: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: decoded.push_back(static_cast<char>((val >> valb) & 0xFF));
- Line 236: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(std::move(j));
  Confidence: band=high; score=0.74
- Line 237: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(std::move(j));
- Line 265: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resp.push_back({{"pk", result.first}, {"score", result.second}});
  Confidence: band=high; score=0.74
- Line 266: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: resp.push_back({{"pk", result.first}, {"score", result.second}});
- Line 275: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 333: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vectorQuery.push_back(val.get<float>());
  Confidence: band=high; score=0.74
- Line 334: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vectorQuery.push_back(val.get<float>());
- Line 361: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> scores;
  Confidence: band=medium; score=0.66
- Line 375: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fusedResults.emplace_back(pk, score);
  Confidence: band=high; score=0.74
- Line 397: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> scores;
  Confidence: band=medium; score=0.66
- Line 414: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fusedResults.emplace_back(pk, score);
  Confidence: band=high; score=0.74
- Line 432: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resp.push_back({
  Confidence: band=high; score=0.74
- Line 433: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: resp.push_back({
- Line 459: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 505: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resp.push_back({
  Confidence: band=high; score=0.74
- Line 506: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: resp.push_back({
- Line 526: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 670: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 690: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 710: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 724: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = body["weights"].begin(); it != body["weights"].end(); ++it) {
  Confidence: band=high; score=0.74
- Line 736: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/replication_topology_api_handler.cpp
Total findings: 26

- Line 295: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Use std::unique_ptr<T> or std::make_unique<T>()
  Context: << "if(!t.ok)throw new Error('topology '+t.status);\n"
- Line 295: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: << "if(!t.ok)throw new Error('topology '+t.status);\n"
- Line 296: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Use std::unique_ptr<T> or std::make_unique<T>()
  Context: << "if(!h.ok)throw new Error('health '+h.status);\n"
- Line 296: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: << "if(!h.ok)throw new Error('health '+h.status);\n"
- Line 0: severity=HIGH; category=uncategorized
  Context: ['         << "<html lang=\\"en\\">\\n"', '         << "<head><meta charset=\\"utf-8\\">\\n"', '         << "<meta name=\\"viewport\\" content=\\"width=device-width,initial-scale=1\\">\\n"', '         << "<title>Themis Replication Topology</title>\\n"', '         << "<style>body{font-family:system-ui,sans-serif;margin:16px}"']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 99: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: http::response<http::string_body> ReplicationTopologyApiHandler::handleTopologyGet(
  Confidence: band=very_high; score=0.9
- Line 174: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: http::response<http::string_body> ReplicationTopologyApiHandler::handleHealthGet(
  Confidence: band=very_high; score=0.9
- Line 227: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: http::response<http::string_body> ReplicationTopologyApiHandler::handleUiGet(
  Confidence: band=very_high; score=0.9
- Line 232: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: const std::string target{req.target()};
  Confidence: band=very_high; score=0.9
- Line 141: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(std::move(node));
  Confidence: band=high; score=0.74
- Line 142: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nodes.push_back(std::move(node));
- Line 148: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back({
  Confidence: band=high; score=0.74
- Line 149: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edges.push_back({
- Line 282: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<title>Themis Replication Topology</title>\n"
- Line 286: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "#error{color:#b91c1c;margin:8px 0;display:none}</style></head>\n"
- Line 287: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<body><h1>Replication Topology</h1><div id=\"error\"></div>\n"
- Line 287: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<body><h1>Replication Topology</h1><div id=\"error\"></div>\n"
- Line 288: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<h2>Topology</h2><pre id=\"topology\">loading...</pre>\n"
- Line 288: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<h2>Topology</h2><pre id=\"topology\">loading...</pre>\n"
- Line 289: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<h2>Health</h2><pre id=\"health\">loading...</pre>\n"
- Line 289: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<h2>Health</h2><pre id=\"health\">loading...</pre>\n"
- Line 293: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "const t=await fetch(API_BASE+'/api/v1/replication/topology');\n"
- Line 294: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "const h=await fetch(API_BASE+'/api/v1/replication/health');\n"
- Line 301: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "load();setInterval(load,5000);</script></body></html>\n";

### src/server/sse_connection_manager.cpp
Total findings: 25

- Line 148: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: budget = config_.max_events_per_second - conn->sent_in_window;
- Line 185: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: conn->sent_in_window += static_cast<uint32_t>(events.size());
- Line 217: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: budget = config_.max_events_per_second - conn->sent_in_window;
- Line 245: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: conn->sent_in_window += static_cast<uint32_t>(raw_events.size());
- Line 327: severity=CRITICAL; category=missing_dtor
  Description: Class PollTarget allocates resources but has no destructor
  Remediation: Add explicit destructor: ~PollTarget() { /* cleanup */ }
  Context: class/struct PollTarget
- Line 350: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (conn->buffered_events.size() >= config_.max_buffered_events
- Line 441: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: poll_timer_->async_wait([this](const boost::system::error_code& ec) {
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 66: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: conn->key_prefix = key_prefix;
- Line 75: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: conn->id, from_seq, key_prefix);
- Line 165: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: conn->buffered_events.begin(),
- Line 166: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: conn->buffered_events.begin() + static_cast<ptrdiff_t>(count)
- Line 170: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: conn->buffered_events.begin() + static_cast<ptrdiff_t>(count)
- Line 179: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: conn->raw_buffered_events.begin() + static_cast<std::ptrdiff_t>(raw_count));
- Line 233: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: conn->raw_buffered_events.begin(),
- Line 234: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: conn->raw_buffered_events.begin() + static_cast<std::ptrdiff_t>(count)
- Line 238: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: conn->raw_buffered_events.begin() + static_cast<std::ptrdiff_t>(count)
- Line 295: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [id, conn] : connections_) {
  Confidence: band=very_high; score=0.9
- Line 359: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: conn->key_prefix,
- Line 365: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& target : active_conns) {
  Confidence: band=very_high; score=0.9
- Line 366: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Query new events since last sequence — without holding connections_mutex_.
- Line 391: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // new events to preserve the hard max_buffered_events bound.
- Line 354: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_conns.push_back(PollTarget{
  Confidence: band=high; score=0.74
- Line 355: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: active_conns.push_back(PollTarget{
- Line 424: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: c.raw_buffered_events.push_back(event);

### src/server/bpmn_api_handler.cpp
Total findings: 22

- Line 115: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: // auth_->authorize() which checks that the token contains the required scope.
  Confidence: band=very_high; score=0.99
- Line 116: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto ar = auth_->authorize(*token, scope);
  Confidence: band=very_high; score=0.99
- Line 190: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string process_key = request.value("process_definition_key", "");
- Line 191: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json variables = request.value("variables", json::object());
- Line 192: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string business_key = request.value("business_key", "");
- Line 259: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response["active_task_ids"] = json::array();
- Line 309: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json variables = request.value("variables", json::object());
- Line 480: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: const auto tsIt = token.visit_timestamps.find(node);
  Confidence: band=very_high; score=0.9
- Line 487: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: event["data"] = json::object();
- Line 488: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: event["data"]["node_id"] = node;
- Line 494: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response["history"] = json::array();
- Line 61: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 96: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 209: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "Failed to start process: " + status.message, req);
- Line 253: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_tasks.push_back(instance_id + ":" + token.current_node);
  Confidence: band=high; score=0.74
- Line 254: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: active_tasks.push_back(instance_id + ":" + token.current_node);
- Line 419: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "Process instance not found: " + status.message, req);
- Line 458: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_tasks.push_back(task);
  Confidence: band=high; score=0.74
- Line 459: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: active_tasks.push_back(task);
- Line 488: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: history.push_back(event);
  Confidence: band=high; score=0.74
- Line 488: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: history.push_back(event);
  Confidence: band=high; score=0.74
- Line 489: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: history.push_back(event);

### src/server/rate_limiter_v2.cpp
Total findings: 22

- Line 302: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: redis_pool_.pool_cv.wait(lk, [this]() {
- Line 500: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: THEMIS_WARN("PerClientRateLimiter: Max clients ({}) reached, rejecting new client: {}",
- Line 563: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = client_buckets_.begin(); it != client_buckets_.end(); ) {
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    }', '', '    auto& slot = redis_pool_.slots[slot_idx];', '    std::string key = redisKey(config_.bucket_id, prio);', '    int result = redisExecEvalsha(slot, key, capacity, refill_rate, consume_count);']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 46: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: local tokens  = tonumber(data[1]) or capacity
- Line 47: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: local last_ms = tonumber(data[2]) or now_ms
- Line 122: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (config_.backend == Backend::REDIS && redis_healthy_.load(std::memory_order_acquire)) {
- Line 165: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: bucket->refill();
- Line 188: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: redis_healthy_.load(std::memory_order_acquire);
- Line 195: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [prio, bucket] : buckets_) {
  Confidence: band=very_high; score=0.9
- Line 196: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(bucket->mutex);
- Line 198: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: bucket->last_refill = std::chrono::steady_clock::now();
- Line 422: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (redis_healthy_.load(std::memory_order_acquire)) return;
- Line 460: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: std::memory_order_acquire)) {
- Line 524: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: lock.unlock(); // Unlock before trying to acquire tokens
- Line 272: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: redis_pool_.available.push_back(static_cast<size_t>(i));
  Confidence: band=high; score=0.74
- Line 273: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: redis_pool_.available.push_back(static_cast<size_t>(i));
- Line 274: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ++healthy;

### src/server/rpc/differential_update_engine.cpp
Total findings: 22

- Line 116: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = base_hashes.find(chunk.hash);
- Line 30: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: boundaries.push_back(0);  // Start
- Line 40: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: boundaries.push_back(i);
  Confidence: band=high; score=0.74
- Line 41: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: boundaries.push_back(i);
- Line 45: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: boundaries.push_back(data.size());  // End
- Line 74: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: file.close();
- Line 104: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, uint32_t> base_hashes;
  Confidence: band=high; score=0.74
- Line 118: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.unchanged_chunks.push_back(chunk.index);
  Confidence: band=high; score=0.74
- Line 118: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.unchanged_chunks.push_back(chunk.index);
  Confidence: band=high; score=0.74
- Line 119: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.unchanged_chunks.push_back(chunk.index);
- Line 123: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.changed_chunks.push_back(chunk.index);
- Line 154: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<uint32_t, std::string> ExtractChunks(
  Confidence: band=high; score=0.74
- Line 158: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<uint32_t, std::string> chunks;
  Confidence: band=high; score=0.74
- Line 170: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: file.close();
- Line 185: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: manifest.push_back(info);
  Confidence: band=high; score=0.74
- Line 186: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: manifest.push_back(info);
- Line 195: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: manifest.push_back(info);
- Line 199: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<uint32_t, const ChunkInfo*> by_index;
  Confidence: band=medium; score=0.66
- Line 230: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: manifest.push_back(info);
  Confidence: band=high; score=0.74
- Line 231: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: manifest.push_back(info);
- Line 253: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: manifest.push_back(info);
  Confidence: band=high; score=0.74
- Line 254: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: manifest.push_back(info);

### src/server/saga_api_handler.cpp
Total findings: 22

- Line 88: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["steps"] = nlohmann::json::array();
- Line 176: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["batches"] = nlohmann::json::array();
- Line 30: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64[(n >> 18) & 63]);
- Line 31: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64[(n >> 12) & 63]);
- Line 32: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64[(n >> 6) & 63]);
- Line 33: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64[n & 63]);
- Line 38: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64[(n >> 18) & 63]);
- Line 39: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64[(n >> 12) & 63]);
- Line 40: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back('=');
- Line 41: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back('=');
- Line 44: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64[(n >> 18) & 63]);
- Line 45: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64[(n >> 12) & 63]);
- Line 46: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64[(n >> 6) & 63]);
- Line 47: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back('=');
- Line 108: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["steps"].push_back(step_json);
  Confidence: band=high; score=0.74
- Line 149: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto sig = j["signature"];
  Confidence: band=high; score=0.74
- Line 158: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result["batches"].push_back(info.toJson());
  Confidence: band=high; score=0.74
- Line 182: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result["batches"].push_back(info.toJson());
- Line 227: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hash.push_back(byte.get<uint8_t>());
  Confidence: band=high; score=0.74
- Line 228: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: hash.push_back(byte.get<uint8_t>());
- Line 237: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/feedback_api_handler.cpp
Total findings: 20

- Line 57: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("FeedbackAPIHandler: storage_service is required");
- Line 542: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Invalid adapter_id filter");
- Line 548: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Invalid user_id filter");
- Line 555: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Invalid min_rating filter");
- Line 565: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Invalid flagged_for_training filter");
- Line 570: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Invalid training_category filter");
- Line 577: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Invalid limit filter");
- Line 94: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: auto stored = storage_service.createFeedback(feedback);
  Confidence: band=high; score=0.74
- Line 151: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: auto feedback_list = storage_service.listFeedback(filter);
  Confidence: band=high; score=0.74
- Line 158: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response["feedback"].push_back(fb.toJSON());
  Confidence: band=high; score=0.74
- Line 159: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response["feedback"].push_back(fb.toJSON());
- Line 210: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: auto feedback = storage_service.getFeedback(id);
  Confidence: band=high; score=0.74
- Line 273: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: bool success = storage_service.updateFeedback(id, feedback);
  Confidence: band=high; score=0.74
- Line 284: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: auto updated = storage_service.getFeedback(id);
  Confidence: band=high; score=0.74
- Line 326: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: bool success = storage_service.deleteFeedback(id);
  Confidence: band=high; score=0.74
- Line 396: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: auto feedback_list = storage_service.getFeedbackForAdapter(adapter_id, limit);
  Confidence: band=high; score=0.74
- Line 403: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response["feedback"].push_back(fb.toJSON());
  Confidence: band=high; score=0.74
- Line 404: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response["feedback"].push_back(fb.toJSON());
- Line 458: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: auto stats = storage_service.getStatistics(adapter_id);
  Confidence: band=high; score=0.74
- Line 516: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: llm::lora::FeedbackFilter FeedbackAPIHandler::parseFilterFromQuery(const std::string& query) const {
  Confidence: band=high; score=0.74

### src/server/auth_middleware.cpp
Total findings: 19

- Line 184: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: AuthMiddleware::AuthResult AuthMiddleware::authorize(std::string_view token, std::string_view required_scope) const {
  Confidence: band=very_high; score=0.99
- Line 614: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: auto claims = mtls_auth.authenticate(std::string(cert_pem));
  Confidence: band=very_high; score=0.99
- Line 171: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = role_scope_map_.find(role);
- Line 171: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& role : roles) {
  Confidence: band=very_high; score=0.9
- Line 174: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& granted : it->second) {
  Confidence: band=very_high; score=0.9
- Line 201: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // inputs (which would require zero-padding and may confuse static
  Confidence: band=very_high; score=0.9
- Line 308: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: std::find(it->second.begin(), it->second.end(), required_scope) != it->second.end()) {
- Line 318: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: std::find(it->second.begin(), it->second.end(), required_scope) != it->second.end()) {
- Line 542: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: std::find(it->second.begin(), it->second.end(), required_scope) != it->second.end()) {
- Line 160: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::string>> mapping)
  Confidence: band=medium; score=0.66
- Line 214: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!scopes_list.empty()) scopes_list += ",";
- Line 343: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> granted_scopes(claims.scopes.begin(),
  Confidence: band=medium; score=0.66
- Line 415: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 529: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) roles_str += ", ";
  Confidence: band=high; score=0.74
- Line 530: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) roles_str += ", ";
- Line 699: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::string>> mapping;
  Confidence: band=medium; score=0.66
- Line 704: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scopes.push_back(s.as<std::string>());
  Confidence: band=high; score=0.74
- Line 704: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scopes.push_back(s.as<std::string>());
  Confidence: band=high; score=0.74
- Line 705: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: scopes.push_back(s.as<std::string>());

### src/server/buffer_binary_protocol.cpp
Total findings: 19

- Line 57: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ts_buffer_->start();
- Line 66: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: vector_buffer_->start();
- Line 75: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: graph_buffer_->start();
- Line 154: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: point.metric = data["metric"].as<std::string>();
- Line 155: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: point.entity = data["entity"].as<std::string>();
- Line 156: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: point.timestamp_ms = data["timestamp_ms"].as<int64_t>();
- Line 157: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: point.value = data["value"].as<double>();
- Line 160: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto status = ts_buffer_->add(point);
- Line 194: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto status = ts_buffer_->add(point);
- Line 235: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto status = vector_buffer_->add(entity);
- Line 266: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::string pk = data["pk"].as<std::string>();
- Line 268: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto status = vector_buffer_->remove(pk);
- Line 293: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: node.setPrimaryKey(data["pk"].as<std::string>());
- Line 294: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::string graph_id = data["graph_id"].as<std::string>();
- Line 296: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto status = graph_buffer_->addNode(node, graph_id);
- Line 319: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto ts_stats = ts_buffer_->getStats();
- Line 320: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto vector_stats = vector_buffer_->getStats();
- Line 321: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto graph_stats = graph_buffer_->getStats();
- Line 384: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::string buffer_name = data["buffer"].as<std::string>();

### src/server/profiling_api_handler.cpp
Total findings: 19

- Line 295: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("storage_profiler.slow_op_threshold_ms must be >= 0");
- Line 312: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("analyzer.slow_query_threshold_ms must be >= 0");
- Line 320: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("analyzer.cache_hit_rate_threshold must be between 0.0 and 1.0");
- Line 128: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(profile->toJSON());
  Confidence: band=high; score=0.74
- Line 129: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(profile->toJSON());
- Line 151: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(profile->toJSON());
  Confidence: band=high; score=0.74
- Line 152: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(profile->toJSON());
- Line 193: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: query_json.push_back(profile->toJSON());
  Confidence: band=high; score=0.74
- Line 194: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: query_json.push_back(profile->toJSON());
- Line 198: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: storage_json.push_back(stats.toJSON());
  Confidence: band=high; score=0.74
- Line 199: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: storage_json.push_back(stats.toJSON());
- Line 269: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto qp = body["query_profiler"];
  Confidence: band=high; score=0.74
- Line 275: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto threshold_ms = qp["slow_query_threshold_ms"].get<int>();
  Confidence: band=high; score=0.74
- Line 289: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto sp = body["storage_profiler"];
  Confidence: band=high; score=0.74
- Line 293: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto threshold_ms = sp["slow_op_threshold_ms"].get<int>();
  Confidence: band=high; score=0.74
- Line 307: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto an = body["analyzer"];
  Confidence: band=high; score=0.74
- Line 310: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto threshold_ms = an["slow_query_threshold_ms"].get<int>();
  Confidence: band=high; score=0.74
- Line 318: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto cache_hit_rate = an["cache_hit_rate_threshold"].get<double>();
  Confidence: band=high; score=0.74
- Line 406: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/saml_auth_provider.cpp
Total findings: 18

- Line 126: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = pending_requests_.find(in_response_to);
- Line 293: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = pending_requests_.begin(); it != pending_requests_.end(); ) {
- Line 149: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: attrs[name].push_back(value);
  Confidence: band=high; score=0.74
- Line 150: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: attrs[name].push_back(value);
- Line 250: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " index=\"1\"/>\n";
- Line 250: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " index=\"1\"/>\n";
- Line 255: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " Location=\"" << config_.sp_slo_url << "\"/>\n";
- Line 255: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " Location=\"" << config_.sp_slo_url << "\"/>\n";
- Line 258: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "  </md:SPSSODescriptor>\n";
- Line 262: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "    <md:OrganizationName xml:lang=\"en\">" << config_.org_name << "</md:OrganizationName>\n"
- Line 262: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "    <md:OrganizationName xml:lang=\"en\">" << config_.org_name << "</md:OrganizationName>\n"
- Line 264: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << config_.org_display_name << "</md:OrganizationDisplayName>\n"
- Line 265: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "    <md:OrganizationURL xml:lang=\"en\">" << config_.org_url << "</md:OrganizationURL>\n"
- Line 265: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "    <md:OrganizationURL xml:lang=\"en\">" << config_.org_url << "</md:OrganizationURL>\n"
- Line 266: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "  </md:Organization>\n";
- Line 271: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "    <md:EmailAddress>" << config_.contact_email << "</md:EmailAddress>\n"
- Line 272: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "  </md:ContactPerson>\n";
- Line 275: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "</md:EntityDescriptor>\n";

### src/server/api_gateway.cpp
Total findings: 17

- Line 50: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator first may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto first = s.find_first_not_of(" \t");
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 257: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw Error(static_cast<int>(ErrorCode::FeatureDisabled),
- Line 262: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw Error(static_cast<int>(ErrorCode::ConfigurationError),
- Line 289: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: stats["datacenter"] = config_.datacenter;
- Line 703: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response_body["results"] = nlohmann::json::array();
- Line 825: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Priority 2: Accept-Version header (legacy)
  Confidence: band=high; score=0.8
- Line 900: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Check if endpoint is deprecated
  Confidence: band=high; score=0.8
- Line 911: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Add API-Deprecated header (issue-specified format: "v1.0 (remove YYYY-MM-DD)")
  Confidence: band=high; score=0.8
- Line 135: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 514: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 560: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 706: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response_body["results"].push_back(result.data);
  Confidence: band=high; score=0.74
- Line 707: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response_body["results"].push_back(result.data);
- Line 778: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> labels = {
  Confidence: band=high; score=0.74
- Line 948: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: R"(^/v(\d+(?:\.\d+){0,2})(?=/|$))"
- Line 966: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: R"(^/v\d+(?:\.\d+){0,2}(?=/|$))"

### src/server/llm_grpc_service.cpp
Total findings: 17

- Line 548: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto* stats = response->mutable_cache_stats();
- Line 97: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const auto& metadata = context->client_metadata();
- Line 115: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const llm::InferenceRequest& pb_req,
  Confidence: band=very_high; score=0.9
- Line 116: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: ::themis::llm::InferenceRequest& internal_req) {
  Confidence: band=very_high; score=0.9
- Line 146: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: ::themis::llm::InferenceRequest internal_req;
  Confidence: band=very_high; score=0.9
- Line 181: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: ::themis::llm::InferenceRequest internal_req;
  Confidence: band=very_high; score=0.9
- Line 182: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: internal_req.prompt = request->query();
- Line 237: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: ::themis::llm::InferenceRequest internal_req;
  Confidence: band=very_high; score=0.9
- Line 524: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto* inference_stats = response->mutable_inference_stats();
  Confidence: band=very_high; score=0.9
- Line 620: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto lora_data = plugin_mgr.exportLoRA(request->lora_id());
- Line 12: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "server/llm_grpc_service.h"
  Confidence: band=high; score=0.74
- Line 55: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: while (s.size() % 4) s += '=';
  Confidence: band=high; score=0.74
- Line 80: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto exp = claims["exp"].get<int64_t>();
  Confidence: band=high; score=0.74
- Line 150: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto internal_resp = plugin_mgr.generate(internal_req);
  Confidence: band=high; score=0.74
- Line 177: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rag_context.documents.push_back(internal_doc);
  Confidence: band=high; score=0.74
- Line 178: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rag_context.documents.push_back(internal_doc);
- Line 409: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: temp_file.close();

### src/server/pki_api_handler.cpp
Total findings: 17

- Line 300: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::string data_b64 = body["data_b64"].get<std::string>();
- Line 59: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(chars[(val>>valb)&0x3F]);
  Confidence: band=high; score=0.74
- Line 60: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(chars[(val>>valb)&0x3F]);
- Line 64: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (valb>-6) out.push_back(chars[((val<<8)>>(valb+8))&0x3F]);
- Line 65: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: while (out.size()%4) out.push_back('=');
- Line 79: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back((uint8_t)((val>>valb)&0xFF));
  Confidence: band=high; score=0.74
- Line 79: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back((uint8_t)((val>>valb)&0xFF));
  Confidence: band=high; score=0.74
- Line 80: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back((uint8_t)((val>>valb)&0xFF));
- Line 120: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: SigningResult res = signing_service.sign(data, key_id);
  Confidence: band=high; score=0.74
- Line 150: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto data_b64 = body["data_b64"].get<std::string>();
  Confidence: band=high; score=0.74
- Line 159: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: bool ok = signing_service.verify(data, sig, key_id);
  Confidence: band=high; score=0.74
- Line 232: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(std::move(jk));
  Confidence: band=high; score=0.74
- Line 233: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(std::move(jk));
- Line 392: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto qualified_sig = body["qualified_signature"];
  Confidence: band=high; score=0.74
- Line 464: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: certs_array.push_back(std::move(entry));
  Confidence: band=high; score=0.74
- Line 465: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: certs_array.push_back(std::move(entry));
- Line 537: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/ethics_api_handler.cpp
Total findings: 16

- Line 504: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("QueryEngine not available");
- Line 538: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("AQL parse error: " + parse_result.error().message());
- Line 544: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("AQL translation error: " + translation.error_message);
- Line 103: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto philosophies = body["philosophy_schools"].get<std::vector<std::string>>();
  Confidence: band=high; score=0.74
- Line 146: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto philosophies = body["philosophy_schools"].get<std::vector<std::string>>();
  Confidence: band=high; score=0.74
- Line 239: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: types.push_back(type);
- Line 373: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto philosophies = body["philosophy_schools"].get<std::vector<std::string>>();
  Confidence: band=high; score=0.74
- Line 430: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: prom += "# TYPE " + prefix + " gauge\n";
  Confidence: band=high; score=0.74
- Line 519: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '\'') { escaped += "''"; } else { escaped += c; }
  Confidence: band=high; score=0.74
- Line 519: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '\'') { escaped += "''"; } else { escaped += c; }
  Confidence: band=high; score=0.74
- Line 520: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '\'') { escaped += "''"; } else { escaped += c; }
- Line 520: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '\'') { escaped += "''"; } else { escaped += c; }
- Line 553: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rows.push_back(nlohmann::json::parse(entity.toJson()));
  Confidence: band=high; score=0.74
- Line 554: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rows.push_back(nlohmann::json::parse(entity.toJson()));
- Line 560: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rows.push_back(nlohmann::json::parse(entity.toJson()));
  Confidence: band=high; score=0.74
- Line 561: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rows.push_back(nlohmann::json::parse(entity.toJson()));

### src/server/ranger_adapter.cpp
Total findings: 16

- Line 120: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(backoff));
- Line 209: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: item["users"] = json::array();
- Line 144: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: resource_prefixes.push_back(path["value"].get<std::string>());
- Line 147: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& v : path["values"]) if (v.is_string()) resource_prefixes.push_back(v.get<std::strin
- Line 150: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (resource_prefixes.empty()) resource_prefixes.push_back("/");
  Confidence: band=high; score=0.74
- Line 151: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (resource_prefixes.empty()) resource_prefixes.push_back("/");
- Line 174: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(std::move(p));
  Confidence: band=high; score=0.74
- Line 174: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(std::move(p));
  Confidence: band=high; score=0.74
- Line 174: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(std::move(p));
  Confidence: band=high; score=0.74
- Line 175: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(std::move(p));
- Line 206: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& a : p.actions) accesses.push_back(json{{"type", a}, {"isAllowed", p.effect_allow}});
  Confidence: band=high; score=0.74
- Line 207: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& a : p.actions) accesses.push_back(json{{"type", a}, {"isAllowed", p.effect_allow}})
- Line 209: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& u : p.subjects) item["users"].push_back(u);
  Confidence: band=high; score=0.74
- Line 210: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& u : p.subjects) item["users"].push_back(u);
- Line 213: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(std::move(rp));
  Confidence: band=high; score=0.74
- Line 214: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(std::move(rp));

### src/server/smart_routing.cpp
Total findings: 16

- Line 75: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = backends_.find(backend_id);
- Line 177: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: return s->cached_p99_latency <= config_.tail_latency_threshold_ms ||
- Line 177: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: return s->cached_p99_latency <= config_.tail_latency_threshold_ms ||
- Line 204: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (sc < cc || (sc == cc && s->cached_avg_latency < chosen->cached_avg_latency)) {
- Line 204: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (sc < cc || (sc == cc && s->cached_avg_latency < chosen->cached_avg_latency)) {
- Line 247: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (sc < cc || (sc == cc && state.cached_avg_latency < chosen->cached_avg_latency)) {
- Line 247: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (sc < cc || (sc == cc && state.cached_avg_latency < chosen->cached_avg_latency)) {
- Line 61: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, state] : backends_) {
  Confidence: band=very_high; score=0.9
- Line 308: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::out_of_range("SmartRouter: unknown backend '" + backend_id + "'");
- Line 61: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(state.endpoint);
  Confidence: band=high; score=0.74
- Line 62: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(state.endpoint);
- Line 79: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: state.latency_window.push_back(latency_ms);
- Line 169: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(&state);
  Confidence: band=high; score=0.74
- Line 170: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: candidates.push_back(&state);
- Line 296: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(bs);
  Confidence: band=high; score=0.74
- Line 297: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(bs);

### src/server/api_key_mgmt_handler.cpp
Total findings: 15

- Line 253: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = keys_.find(key_id);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 41: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to generate secure random bytes for API token");
- Line 55: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to generate secure random bytes for key ID");
- Line 125: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& p : body["permissions"]) {
  Confidence: band=very_high; score=0.9
- Line 152: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& p : permissions) {
  Confidence: band=very_high; score=0.9
- Line 181: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, rec] : keys_) {
  Confidence: band=very_high; score=0.9
- Line 232: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& p : rec.permissions) {
  Confidence: band=very_high; score=0.9
- Line 125: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (p.is_string()) permissions.push_back(p.get<std::string>());
  Confidence: band=high; score=0.74
- Line 126: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (p.is_string()) permissions.push_back(p.get<std::string>());
- Line 156: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(recordToJson(rec));
  Confidence: band=high; score=0.74
- Line 182: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: items.push_back(recordToJson(rec));
- Line 223: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (p.is_string()) rec.permissions.push_back(p.get<std::string>());
  Confidence: band=high; score=0.74
- Line 224: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (p.is_string()) rec.permissions.push_back(p.get<std::string>());

### src/server/geo_topology_api_handler.cpp
Total findings: 15

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 133: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: std::find(zones_arr.begin(), zones_arr.end(), s.zone) == zones_arr.end()) {
  Confidence: band=very_high; score=0.9
- Line 133: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: std::find(zones_arr.begin(), zones_arr.end(), s.zone) == zones_arr.end()) {
- Line 191: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (ratio == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 91: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(entry));
  Confidence: band=high; score=0.74
- Line 92: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(entry));
- Line 133: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: zones_arr.push_back(s.zone);
  Confidence: band=high; score=0.74
- Line 133: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: zones_arr.push_back(s.zone);
  Confidence: band=high; score=0.74
- Line 134: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: zones_arr.push_back(s.zone);
- Line 145: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(entry));
- Line 191: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failed_regions.push_back(region);
  Confidence: band=high; score=0.74
- Line 192: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: failed_regions.push_back(region);
- Line 194: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: degraded_regions.push_back({
- Line 201: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: healthy_regions.push_back(region);
- Line 206: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::string overall_status;

### src/server/prompt_engineering_api_handler.cpp
Total findings: 15

- Line 27: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: #include "utils/input_validator.h"
  Confidence: band=very_high; score=0.99
- Line 43: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: themis::utils::InputValidator validator;
  Confidence: band=very_high; score=0.99
- Line 114: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: test.input = tc.value("input", "");
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 27: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: #include "utils/input_validator.h"
  Confidence: band=very_high; score=0.9
- Line 43: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: themis::utils::InputValidator validator;
  Confidence: band=very_high; score=0.9
- Line 114: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: test.input = tc.value("input", "");
  Confidence: band=very_high; score=0.9
- Line 118: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: test_cases.push_back(test);
  Confidence: band=high; score=0.74
- Line 119: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: test_cases.push_back(test);
- Line 170: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response.push_back(test.toJson());
  Confidence: band=high; score=0.74
- Line 171: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response.push_back(test.toJson());
- Line 380: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response.push_back(entry.toJson());
  Confidence: band=high; score=0.74
- Line 381: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response.push_back(entry.toJson());
- Line 429: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response.push_back(version.toJson());
  Confidence: band=high; score=0.74
- Line 430: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response.push_back(version.toJson());

### src/server/rpc/snapshot_transfer_handler.cpp
Total findings: 15

- Line 73: severity=CRITICAL; category=missing_dtor
  Description: Class SnapshotTransferHandler allocates resources but has no destructor
  Remediation: Add explicit destructor: ~SnapshotTransferHandler() { /* cleanup */ }
  Context: class/struct SnapshotTransferHandler
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 87: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: checkpoint_ = nullptr;
  Context: delete checkpoint_;
- Line 141: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::recursive_directory_iterator(snapshot_dir_)) {
- Line 421: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::recursive_directory_iterator(snapshot_dir_)) {
- Line 552: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: THEMIS_ERROR("LZ4: Failed to allocate memory: {}", e.what());
- Line 606: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: THEMIS_ERROR("Snappy: Failed to allocate memory: {}", e.what());
- Line 645: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: THEMIS_ERROR("LZ4: Failed to allocate memory: {}", e.what());
- Line 733: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::recursive_directory_iterator(snapshot_dir_)) {
- Line 762: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::recursive_directory_iterator(dir)) {
- Line 87: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete checkpoint_;
- Line 142: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: files.push_back(entry.path());
  Confidence: band=high; score=0.74
- Line 143: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: files.push_back(entry.path());
- Line 734: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: files.push_back(entry.path());
  Confidence: band=high; score=0.74
- Line 735: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: files.push_back(entry.path());

### src/server/wasm_handler_registry.cpp
Total findings: 15

- Line 131: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: const char* pos = std::find(kBase64Chars,
- Line 131: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: const char* pos = std::find(kBase64Chars,
- Line 149: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: const char* pos = std::find(kBase64Chars,
  Confidence: band=very_high; score=0.9
- Line 149: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: const char* pos = std::find(kBase64Chars,
- Line 139: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(char3[j]);
  Confidence: band=high; score=0.74
- Line 139: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(char3[j]);
  Confidence: band=high; score=0.74
- Line 140: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(char3[j]);
- Line 157: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(char3[j]);
  Confidence: band=high; score=0.74
- Line 157: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(char3[j]);
  Confidence: band=high; score=0.74
- Line 157: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(char3[j]);
  Confidence: band=high; score=0.74
- Line 158: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(char3[j]);
- Line 275: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry.toJson());
  Confidence: band=high; score=0.74
- Line 276: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(entry.toJson());
- Line 480: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: already_exists ? http::status::ok : http::status::created;
- Line 579: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/export_api_handler.cpp
Total findings: 14

- Line 115: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto* exporter = static_cast<exporters::IExporter*>(plugin->getInstance());
- Line 476: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 32 > array 0
  Remediation: Fix loop condition or increase array size
  Context: query = conditions[0];
- Line 115: severity=HIGH; category=unsafe_singleton
  Description: Singleton access without thread-safety mechanism
  Remediation: Protect with std::lock_guard or use Meyer singleton pattern
  Context: auto* exporter = static_cast<exporters::IExporter*>(plugin->getInstance());
- Line 405: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 411: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 417: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 210: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 246: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 250: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entities.push_back(std::move(entity));
- Line 264: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: exported_file.close();
- Line 386: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string ExportApiHandler::buildAqlQuery(const json& request_json) {
  Confidence: band=high; score=0.74
- Line 429: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: conditions.push_back("category='" + theme + "'");
- Line 478: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: query += " AND " + conditions[i];
- Line 507: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/health_error_service.cpp
Total findings: 14

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 161: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: http::read(socket, buffer, req, ec);
- Line 176: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: http::write(socket, error_res, ec);
- Line 184: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: http::write(socket, res, ec);
- Line 137: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
- Line 141: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 243: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: handler_req.query = parse_query(query_string);
- Line 246: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: error_handler_->handleGetErrors(handler_req, handler_res);
- Line 258: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: error_handler_->handleGetCategories(handler_req, handler_res);
- Line 268: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: handler_req.query = parse_query(query_string);
- Line 271: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: error_handler_->handleSearchErrors(handler_req, handler_res);
- Line 287: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: error_handler_->handleGetError(handler_req, handler_res);
- Line 12: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "server/health_error_service.h"
  Confidence: band=high; score=0.74
- Line 91: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: acceptor_->close(ec);

### src/server/index_api_handler.cpp
Total findings: 14

- Line 342: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: resp["indexes"] = stats_array;
- Line 76: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto configObj = body["config"];
  Confidence: band=high; score=0.74
- Line 81: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (s.is_string()) config.stopwords.push_back(s.get<std::string>());
  Confidence: band=high; score=0.74
- Line 82: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (s.is_string()) config.stopwords.push_back(s.get<std::string>());
- Line 121: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: columns.push_back(c.get<std::string>());
  Confidence: band=high; score=0.74
- Line 122: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: columns.push_back(c.get<std::string>());
- Line 256: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resp.push_back(stat_obj);
  Confidence: band=high; score=0.74
- Line 257: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: resp.push_back(stat_obj);
- Line 335: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats_array.push_back({
  Confidence: band=high; score=0.74
- Line 336: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats_array.push_back({
- Line 396: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response.push_back(suggestion.toJson());
  Confidence: band=high; score=0.74
- Line 397: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response.push_back(suggestion.toJson());
- Line 438: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response.push_back(pattern.toJson());
  Confidence: band=high; score=0.74
- Line 439: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response.push_back(pattern.toJson());

### src/server/audit_api_handler.cpp
Total findings: 13

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 208: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["entries"] = nlohmann::json::array();
- Line 39: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: AuditLogEntry::toJson()
  Context: nlohmann::json AuditLogEntry::toJson() const {
  Confidence: band=medium; score=0.56
- Line 78: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ciphertext_b64 = payload["ciphertext_b64"].get<std::string>();
  Confidence: band=high; score=0.74
- Line 119: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 185: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 191: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort by timestamp descending (newest first)
  Confidence: band=high; score=0.74
- Line 212: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result["entries"].push_back(all_entries[i].toJson());
  Confidence: band=high; score=0.74
- Line 213: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result["entries"].push_back(all_entries[i].toJson());
- Line 240: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"') escaped += "\"\"";
  Confidence: band=high; score=0.74
- Line 240: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"') escaped += "\"\"";
  Confidence: band=high; score=0.74
- Line 241: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '"') escaped += "\"\"";

### src/server/spatial_api_handler.cpp
Total findings: 13

- Line 148: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: storage_->scanPrefix(scan_prefix, [&](std::string_view key, std::string_view value) -> bool {
- Line 307: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response["exact_check_precision"] = nullptr;
- Line 308: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response["false_positive_rate"] = nullptr;
- Line 324: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response["gpu_backend"] = nullptr;
- Line 66: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto cfg = j["config"];
  Confidence: band=high; score=0.74
- Line 68: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto bounds = cfg["total_bounds"];
  Confidence: band=high; score=0.74
- Line 140: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "Failed to re-create spatial index: " + create_status.message, req);
- Line 165: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 175: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 181: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 323: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 356: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += ' ';
- Line 365: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::unordered_map<std::string, std::string> SpatialApiHandler::parseQuery(const std::string& target) {
  Confidence: band=high; score=0.74

### src/server/timeseries_api_handler.cpp
Total findings: 13

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 144: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = ts_store.query(query_opts);
- Line 167: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response["data"].push_back(point_json);
- Line 386: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response["note"] = "Configuration updated. Changes apply to new data points only.";
- Line 409: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: storage_->scanPrefix("wm:cagg:", [&materialized](std::string_view key, std::string_view value) {
- Line 528: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metrics->updateStorageStats(stats.total_data_points, stats.total_metrics, stats.total_size_bytes);
- Line 166: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response["data"].push_back(point_json);
  Confidence: band=high; score=0.74
- Line 167: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response["data"].push_back(point_json);
- Line 427: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: functions.push_back(name);
  Confidence: band=high; score=0.74
- Line 428: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: functions.push_back(name);
- Line 638: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch.push_back(std::move(dp));
  Confidence: band=high; score=0.74
- Line 638: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch.push_back(std::move(dp));
  Confidence: band=high; score=0.74
- Line 639: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batch.push_back(std::move(dp));

### src/server/policy_versioning_api_handler.cpp
Total findings: 12

- Line 376: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto auth_result = auth.authorize(*token, required_scope);
  Confidence: band=very_high; score=0.99
- Line 124: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return makeResponse(http::status::ok, version_data->toJson().dump(2), req);
- Line 348: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward compatibility: If no auth configured or disabled, allow access but log a warning
  Confidence: band=high; score=0.8
- Line 79: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: json_array.push_back(version.toJson());
  Confidence: band=high; score=0.74
- Line 80: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: json_array.push_back(version.toJson());
- Line 266: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 278: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 287: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: json_array.push_back(entry.toJson());
  Confidence: band=high; score=0.74
- Line 288: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: json_array.push_back(entry.toJson());
- Line 323: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conflicts_arr.push_back(c.toJson());
  Confidence: band=high; score=0.74
- Line 324: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: conflicts_arr.push_back(c.toJson());
- Line 357: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/oauth2_provider.cpp
Total findings: 11

- Line 135: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = pending_states_.find(state);
- Line 148: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = pending_states_.begin(); it != pending_states_.end(); ) {
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 35: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: static_cast<std::string*>(userdata)->append(ptr, total);
- Line 35: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<std::string*>(userdata)->append(ptr, total);
- Line 86: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw auth::AuthException(auth::AuthError(
- Line 93: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw auth::AuthException(auth::AuthError(
- Line 100: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw auth::AuthException(auth::AuthError(
- Line 207: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to initialize libcurl handle");
- Line 232: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(

### src/server/wal_grpc_service.cpp
Total findings: 11

- Line 175: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: for (const auto& item : request.entries()) {
- Line 193: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.entries_compressed().empty()) {
- Line 194: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::vector<uint8_t> compressed(request.entries_compressed().begin(), request.entries_compressed().e
- Line 241: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(error);
- Line 277: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return impl_ ? static_cast<void*>(impl_->get()) : nullptr;
- Line 12: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "server/wal_grpc_service.h"
  Confidence: band=high; score=0.74
- Line 162: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(std::move(e));
  Confidence: band=high; score=0.74
- Line 163: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(std::move(e));
- Line 187: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(std::move(e));
  Confidence: band=high; score=0.74
- Line 188: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(std::move(e));
- Line 260: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/chunked_response_writer.cpp
Total findings: 10

- Line 51: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: body += "0\r\n\r\n";
  Confidence: band=high; score=0.74
- Line 101: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: current_chunk += '\n';
  Confidence: band=high; score=0.74
- Line 105: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fragments.push_back(std::move(current_chunk));
  Confidence: band=high; score=0.74
- Line 106: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fragments.push_back(std::move(current_chunk));
- Line 113: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fragments.push_back(std::move(current_chunk));
- Line 154: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: chunk_data += '\n';
  Confidence: band=high; score=0.74
- Line 155: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: chunk_data += '\n';
- Line 157: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fragments.push_back(std::move(chunk_data));
  Confidence: band=high; score=0.74
- Line 158: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fragments.push_back(std::move(chunk_data));
- Line 184: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/graphql_api_handler.cpp
Total findings: 10

- Line 55: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!val || val->isNull())  return json(nullptr);
- Line 87: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: *   { "data": {...}, "errors": [...] }
- Line 185: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result_json["data"] = exec_result.data
- Line 197: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result_json["errors"] = errors_array;
- Line 63: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(serializeValue(item));
  Confidence: band=high; score=0.74
- Line 64: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(serializeValue(item));
- Line 119: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors_array.push_back({{"message", pe.toString()}});
  Confidence: band=high; score=0.74
- Line 120: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors_array.push_back({{"message", pe.toString()}});
- Line 191: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors_array.push_back({
  Confidence: band=high; score=0.74
- Line 192: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors_array.push_back({

### src/server/session_api_handler.cpp
Total findings: 10

- Line 121: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto auth_result = auth_->authorize(bearer_token, "auth:sessions");
  Confidence: band=very_high; score=0.99
- Line 183: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto auth_result = auth_->authorize(bearer_token, "auth:sessions");
  Confidence: band=very_high; score=0.99
- Line 224: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto auth_result = auth_->authorize(bearer_token, "auth:sessions");
  Confidence: band=very_high; score=0.99
- Line 233: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: bool is_admin = auth_->authorize(bearer_token, "admin:all").authorized;
  Confidence: band=very_high; score=0.99
- Line 275: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto auth_result = auth_->authorize(bearer_token, "auth:sessions");
  Confidence: band=very_high; score=0.99
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #2770 [auth] Implement session management and revocation endpoint (2026-03-12T05:58:05Z)
- Line 73: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (!auth_)    { throw std::invalid_argument("SessionApiHandler: auth must not be null"); }
- Line 74: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (!manager_) { throw std::invalid_argument("SessionApiHandler: manager must not be null"); }
- Line 195: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(std::move(j));
  Confidence: band=high; score=0.74
- Line 196: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(std::move(j));

### src/server/transaction_api_handler.cpp
Total findings: 10

- Line 156: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors_array.push_back({{"index", i}, {"error", "Missing 'type' field"}});
  Confidence: band=high; score=0.74
- Line 157: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors_array.push_back({{"index", i}, {"error", "Missing 'type' field"}});
- Line 161: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors_array.push_back({{"index", i}, {"error", "Missing 'table' field"}});
- Line 165: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors_array.push_back({{"index", i}, {"error", "Missing 'key' field"}});
- Line 186: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: TransactionManager::Status status;
- Line 558: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 573: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: locks_json.push_back({{"key", lock.key}, {"lock_type", lock.lock_type}});
  Confidence: band=high; score=0.74
- Line 574: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: locks_json.push_back({{"key", lock.key}, {"lock_type", lock.lock_type}});
- Line 578: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: write_set_json.push_back({{"key", entry.key}, {"operation", entry.operation}});
  Confidence: band=high; score=0.74
- Line 579: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: write_set_json.push_back({{"key", entry.key}, {"operation", entry.operation}});

### src/server/maintenance_api_handler.cpp
Total findings: 9

- Line 19: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: #include "utils/input_validator.h"
  Confidence: band=very_high; score=0.99
- Line 38: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: themis::utils::InputValidator validator;
  Confidence: band=very_high; score=0.99
- Line 19: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: #include "utils/input_validator.h"
  Confidence: band=very_high; score=0.9
- Line 38: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: themis::utils::InputValidator validator;
  Confidence: band=very_high; score=0.9
- Line 49: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: themis::utils::InputValidator validator;
  Confidence: band=very_high; score=0.9
- Line 116: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto& e : schedules) arr.push_back(scheduleToResponse(e));
- Line 188: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto& j : jobs) arr.push_back(jobToResponse(j));
- Line 254: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back({{"task_type", task_type}, {"handler", handler_name}});
  Confidence: band=high; score=0.74
- Line 255: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back({{"task_type", task_type}, {"handler", handler_name}});

### src/server/rate_limiter.cpp
Total findings: 9

- Line 187: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: refill_rate = static_cast<double>(custom_it->second) / 60.0;
- Line 193: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: THEMIS_DEBUG("Created new rate limit bucket: key={}, capacity={}, rate={}/min",
- Line 360: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = ip_last_access_.begin(); it != ip_last_access_.end(); ) {
- Line 375: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = user_last_access_.begin(); it != user_last_access_.end(); ) {
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 187: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: refill_rate = static_cast<double>(custom_it->second) / 60.0;
- Line 332: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [ip, entry] : adaptive_state_) {
  Confidence: band=very_high; score=0.9
- Line 30: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: TokenBucket::refill()
  Context: void TokenBucket::refill() {
  Confidence: band=medium; score=0.56
- Line 340: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RateLimiter::reset()
  Context: void RateLimiter::reset() {
  Confidence: band=medium; score=0.56

### src/server/response_transformer.cpp
Total findings: 9

- Line 63: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator rename_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto rename_it = field_renames_.find(version_key);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 116: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(k);
  Confidence: band=high; score=0.74
- Line 117: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: keys.push_back(k);
- Line 121: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(k);
  Confidence: band=high; score=0.74
- Line 122: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: keys.push_back(k);
- Line 126: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(k);
  Confidence: band=high; score=0.74
- Line 127: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: keys.push_back(k);

### src/server/retention_api_handler.cpp
Total findings: 9

- Line 77: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (policy.name.find(filter.name_filter) == std::string::npos) {
- Line 249: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("retention_period_days must be positive");
- Line 257: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("archive_after_days must be between 0 and retention_period_days");
- Line 90: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered.push_back(policy);
  Confidence: band=high; score=0.74
- Line 91: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: filtered.push_back(policy);
- Line 104: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(policyToJson(filtered[static_cast<size_t>(i)]));
  Confidence: band=high; score=0.74
- Line 105: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: items.push_back(policyToJson(filtered[static_cast<size_t>(i)]));
- Line 191: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(actionToJson(action));
  Confidence: band=high; score=0.74
- Line 192: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: items.push_back(actionToJson(action));

### src/server/compliance_reporting_api_handler.cpp
Total findings: 8

- Line 315: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto auth_result = auth.authorize(*token, required_scope);
  Confidence: band=very_high; score=0.99
- Line 286: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward compatibility: If no auth configured or disabled, allow access but log a warning
  Confidence: band=high; score=0.8
- Line 58: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 120: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: json_array.push_back(gap.toJson());
  Confidence: band=high; score=0.74
- Line 121: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: json_array.push_back(gap.toJson());
- Line 200: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back(
  Confidence: band=high; score=0.74
- Line 201: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entries.push_back(
- Line 295: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/http3_datagram.cpp
Total findings: 8

- Line 54: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = contexts_.find(context_id);
- Line 0: severity=HIGH; category=uncategorized
  Context: ['', '    const uint8_t* payload    = data + consumed;', '    const size_t   payload_len = len - consumed;', '', '    // Look up and invoke handler.']
  Confidence: band=high; score=0.81
- Line 109: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handler = it->second.handler;
- Line 206: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: value_out = static_cast<uint64_t>(data[0] & 0x3F);
- Line 229: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (static_cast<uint64_t>(data[4])          << 24) |
- Line 230: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (static_cast<uint64_t>(data[5])          << 16) |
- Line 231: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: (static_cast<uint64_t>(data[6])          <<  8) |
- Line 232: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<uint64_t>(data[7]);

### src/server/opa_adapter.cpp
Total findings: 8

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 25: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: buf->append(static_cast<char*>(ptr), size * nmemb);
- Line 25: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: buf->append(static_cast<char*>(ptr), size * nmemb);
- Line 41: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("OpaAdapter: endpoint_url must not be empty");
- Line 44: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("OpaAdapter: policy_path must not be empty");
- Line 47: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("OpaAdapter: timeout_ms must be positive");
- Line 30: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void ensure_curl_global_init() {
  Confidence: band=medium; score=0.66
- Line 93: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### include/server/examples/workload_fingerprint_example.cpp
Total findings: 6

- Line 54: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: window.push_back({ 3200, 0.95, 0.75, false });  // vector read
- Line 56: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: window.push_back({ 3200, 0.0, 0.0, true });     // write
- Line 58: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: window.push_back({ 3200, 0.95, 0.0, false });   // scalar read
- Line 66: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: window.push_back({ 8500, 0.0, 0.0, true });     // write
- Line 68: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: window.push_back({ 8500, 0.95, 0.0, false });   // scalar read
- Line 196: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "\nSee docs/issues/optimization_layers/IMPL-B8-workload-fingerprint.md\n";

### src/server/branch_api_handler.cpp
Total findings: 6

- Line 231: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: branch = nullptr;
  Context: sendError(res, 400, "Failed to delete branch. Branch may be active or not fully merged.");
- Line 148: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(branch.toJson());
  Confidence: band=high; score=0.74
- Line 149: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(branch.toJson());
- Line 231: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: sendError(res, 400, "Failed to delete branch. Branch may be active or not fully merged.");
- Line 325: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resolutions.push_back(std::move(res_item));
  Confidence: band=high; score=0.74
- Line 326: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: resolutions.push_back(std::move(res_item));

### src/server/buffer_api_handler.cpp
Total findings: 6

- Line 331: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response["buffers"]["ts_buffer"] = {{"enabled", false}};
- Line 346: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response["buffers"]["vector_buffer"] = {{"enabled", false}};
- Line 361: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response["buffers"]["graph_buffer"] = {{"enabled", false}};
- Line 207: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: status.message, req);
- Line 266: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: PropertyGraphManager::Status status;
- Line 279: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: status.message, req);

### src/server/distributed_txn_api_handler.cpp
Total findings: 6

- Line 310: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: std::string_view path  = req.target();
  Confidence: band=very_high; score=0.9
- Line 96: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_ids.push_back(std::move(shard_id));
  Confidence: band=high; score=0.74
- Line 97: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shard_ids.push_back(std::move(shard_id));
- Line 164: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto        op      = body["operation"];
  Confidence: band=high; score=0.74
- Line 282: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_ids.push_back(std::move(shard_id));
  Confidence: band=high; score=0.74
- Line 283: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shard_ids.push_back(std::move(shard_id));

### src/server/openapi_route_registry.cpp
Total findings: 6

- Line 199: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto tag_description = [](const std::string& t) -> std::string {
- Line 77: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: params.push_back(std::move(param));
  Confidence: band=high; score=0.74
- Line 77: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: params.push_back(std::move(param));
  Confidence: band=high; score=0.74
- Line 78: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: params.push_back(std::move(param));
- Line 87: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& t : op.tags) tags_arr.push_back(t);
- Line 166: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: {"example","<https://docs.themisdb.com/migration/v1-to-v2>; rel=\"deprecation\""}}}

### src/server/pii_api_handler.cpp
Total findings: 6

- Line 132: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {
- Line 99: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 147: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out_items.push_back(j);
  Confidence: band=high; score=0.74
- Line 148: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out_items.push_back(j);
- Line 152: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 164: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: csv += r.value("original_uuid", ""); csv += ",";
  Confidence: band=high; score=0.74

### src/server/policy_manager_api_handler.cpp
Total findings: 6

- Line 464: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto auth_result = auth.authorize(*token, required_scope);
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 435: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward compatibility: If no auth configured or disabled, allow access but log a warning
  Confidence: band=high; score=0.8
- Line 78: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: json_array.push_back(rule.toJson());
  Confidence: band=high; score=0.74
- Line 79: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: json_array.push_back(rule.toJson());
- Line 444: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/policy_template_api_handler.cpp
Total findings: 6

- Line 243: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto auth_result = auth.authorize(*token, required_scope);
  Confidence: band=very_high; score=0.99
- Line 57: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: json_array.push_back(tmpl->toJson());
- Line 215: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward compatibility: If no auth configured or disabled, allow access but log a warning
  Confidence: band=high; score=0.8
- Line 56: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: json_array.push_back(tmpl->toJson());
  Confidence: band=high; score=0.74
- Line 57: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: json_array.push_back(tmpl->toJson());
- Line 224: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/rpc/blob_transfer_handler.cpp
Total findings: 6

- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #970 [P1] Implement checkpoint/resume logic for BlobTransferHandler (2026-03-11T21:57:44Z)
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['                for (int j = 0; j < 8; ++j) {', '                    const uint32_t mask = (crc & 1u) ? 0xFFFFFFFFu : 0u;', '                    crc = (crc >> 1) ^ (0xEDB88320u & mask);', '                }', '            }']
  Confidence: band=medium; score=0.62
- Line 261: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: output_file_.close();
- Line 399: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 488: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: checkpoint_file.close();
- Line 514: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: checkpoint_file.close();

### src/server/serverless_function_api_handler.cpp
Total findings: 6

- Line 501: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = registry_.find(id);
- Line 595: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = version_history_.find(id);
- Line 369: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(fn.toJson());
  Confidence: band=high; score=0.74
- Line 370: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(fn.toJson());
- Line 602: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(snap.toJson());
  Confidence: band=high; score=0.74
- Line 603: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(snap.toJson());

### src/server/wal_api_handler.cpp
Total findings: 6

- Line 56: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (hdr == req.end() || hdr->value() != wal_shared_secret_) {
- Line 120: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back(std::move(e));
  Confidence: band=high; score=0.74
- Line 121: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entries.push_back(std::move(e));
- Line 205: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hex.push_back(hex_digits[(result[i] >> 4) & 0x0F]);
  Confidence: band=high; score=0.74
- Line 206: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: hex.push_back(hex_digits[(result[i] >> 4) & 0x0F]);
- Line 207: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: hex.push_back(hex_digits[result[i] & 0x0F]);

### src/server/cache_admin_api_handler.cpp
Total findings: 5

- Line 169: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto ar = auth_->authorize(*token, required_scope);
  Confidence: band=very_high; score=0.99
- Line 104: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: output.push_back(static_cast<char>((val >> valb) & 0xFF));
  Confidence: band=high; score=0.74
- Line 104: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: output.push_back(static_cast<char>((val >> valb) & 0xFF));
  Confidence: band=high; score=0.74
- Line 105: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: output.push_back(static_cast<char>((val >> valb) & 0xFF));
- Line 155: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/grpc_web_proxy_handler.cpp
Total findings: 5

- Line 174: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: stub_holder_    = std::make_shared<grpc::GenericStub>(channel);
- Line 300: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto* stub = static_cast<grpc::GenericStub*>(stub_holder_.get());
- Line 352: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto call = stub->PrepareUnaryCall(&ctx, method, request_buf, &cq);
- Line 326: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 348: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: grpc::Status status;

### src/server/policy_validation_api_handler.cpp
Total findings: 5

- Line 181: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto auth_result = auth.authorize(*token, required_scope);
  Confidence: band=very_high; score=0.99
- Line 152: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward compatibility: If no auth configured or disabled, allow access but log a warning
  Confidence: band=high; score=0.8
- Line 131: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: json_array.push_back(metric.toJson());
  Confidence: band=high; score=0.74
- Line 132: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: json_array.push_back(metric.toJson());
- Line 161: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/review_scheduling_api_handler.cpp
Total findings: 5

- Line 241: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto auth_result = auth.authorize(*token, required_scope);
  Confidence: band=very_high; score=0.99
- Line 213: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward compatibility: If no auth configured or disabled, allow access but log a warning
  Confidence: band=high; score=0.8
- Line 49: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: json_array.push_back(review.toJson());
  Confidence: band=high; score=0.74
- Line 50: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: json_array.push_back(review.toJson());
- Line 222: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/snapshot_api_handler.cpp
Total findings: 5

- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #384 [WIP] Add Named Snapshots feature for ThemisDB MVCC system (2026-03-11T21:29:03Z)
- Line 95: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 103: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: sort_by = "timestamp";
  Confidence: band=high; score=0.74
- Line 117: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(snapshot.toJson());
  Confidence: band=high; score=0.74
- Line 118: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(snapshot.toJson());

### src/server/cdn_cache_middleware.cpp
Total findings: 4

- Line 222: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: std::string_view inm  = inm_it->value();
- Line 223: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: std::string_view etag = etag_it->value();
- Line 83: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "W/\"" << std::hex << std::setfill('0') << std::setw(16) << h << "\"";
- Line 83: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "W/\"" << std::hex << std::setfill('0') << std::setw(16) << h << "\"";

### src/server/classification_api_handler.cpp
Total findings: 4

- Line 51: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back({
  Confidence: band=high; score=0.74
- Line 52: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: items.push_back({
- Line 113: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: detected_entities.push_back({
  Confidence: band=high; score=0.74
- Line 114: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: detected_entities.push_back({

### src/server/error_api_handler.cpp
Total findings: 4

- Line 38: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors_json.push_back(error.toJSON());
  Confidence: band=high; score=0.74
- Line 39: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors_json.push_back(error.toJSON());
- Line 138: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors_json.push_back(error.toJSON());
  Confidence: band=high; score=0.74
- Line 139: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors_json.push_back(error.toJSON());

### src/server/rate_limiting_middleware.cpp
Total findings: 4

- Line 58: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& ep : config_.endpoint_overrides) {
  Confidence: band=very_high; score=0.9
- Line 226: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& lim : override_limiters_) {
  Confidence: band=very_high; score=0.9
- Line 63: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: override_limiters_.push_back(
  Confidence: band=high; score=0.74
- Line 64: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: override_limiters_.push_back(

### src/server/reports_api_handler.cpp
Total findings: 4

- Line 48: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto lvl = j["level"].get<std::string>();
  Confidence: band=high; score=0.74
- Line 61: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto ts = j["ts"].get<std::string>();
  Confidence: band=high; score=0.74
- Line 65: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto ts = j["timestamp"].get<std::string>();
  Confidence: band=high; score=0.74
- Line 69: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/request_coalescing.cpp
Total findings: 4

- Line 66: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it->second.waiter_count >= config_.max_waiters_per_key) {
- Line 73: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: shared_future = it->second.future;
- Line 191: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator qpos may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto qpos = target.find('?');
- Line 107: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/themis_core_grpc_service.cpp
Total findings: 4

- Line 162: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return impl_ ? static_cast<void*>(impl_->get()) : nullptr;
- Line 12: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "server/themis_core_grpc_service.h"
  Confidence: band=high; score=0.74
- Line 26: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: // path.  This mirrors the pattern used by WalGrpcService / wal_grpc_service.cpp.
  Confidence: band=high; score=0.74
- Line 145: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/adaptive_rate_limiter.cpp
Total findings: 3

- Line 98: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = tenants_.find(tenant_id);
- Line 187: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: latencies.push_back(s.latency_ms.count());
  Confidence: band=high; score=0.74
- Line 188: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: latencies.push_back(s.latency_ms.count());

### src/server/cache_api_handler.cpp
Total findings: 3

- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #446 [REFACTOR] Extract Cache Operations into CacheApiHandler (2026-03-11T21:30:43Z)
- Line 55: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = semantic_cache.query(prompt, params);
- Line 63: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: {"metadata", result->metadata},

### src/server/continuous_query_api_handler.cpp
Total findings: 3

- Line 42: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 236: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(infoToJson(info));
  Confidence: band=high; score=0.74
- Line 237: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(infoToJson(info));

### src/server/cost_based_rate_limiter.cpp
Total findings: 3

- Line 116: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = clients_.begin(); it != clients_.end(); ) {
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 116: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = clients_.begin(); it != clients_.end(); ) {
  Confidence: band=very_high; score=0.9

### src/server/diff_api_handler.cpp
Total findings: 3

- Line 150: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 160: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 178: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/mvcc_api_handler.cpp
Total findings: 3

- Line 52: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("MvccApiHandler: store cannot be null");
- Line 244: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response["versions"] = std::move(versions_array);
- Line 125: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/pitr_api_handler.cpp
Total findings: 3

- Line 64: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: PITRManager::Status status;
- Line 210: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: options.tables.push_back(table.get<std::string>());
  Confidence: band=high; score=0.74
- Line 211: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: options.tables.push_back(table.get<std::string>());

### src/server/pitr_grpc_service.cpp
Total findings: 3

- Line 37: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("PITRServiceImpl: pitr_manager cannot be null");
- Line 40: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("PITRServiceImpl: snapshot_manager cannot be null");
- Line 12: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "server/pitr_grpc_service.h"
  Confidence: band=high; score=0.74

### src/server/prompt_api_handler.cpp
Total findings: 3

- Line 147: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: nlohmann::json out = updated_opt ? updated_opt->toJson() : nlohmann::json::object();
- Line 78: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(t.toJson());
  Confidence: band=high; score=0.74
- Line 79: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(t.toJson());

### src/server/prompt_engineering_grpc_service.cpp
Total findings: 3

- Line 13: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: * @file prompt_engineering_grpc_service.cpp
  Confidence: band=high; score=0.74
- Line 34: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "server/prompt_engineering_grpc_service.h"
  Confidence: band=high; score=0.74
- Line 77: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/update_api_handler.cpp
Total findings: 3

- Line 105: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: config_json["is_running"] = checker_->isRunning();
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73

### src/server/api_security_audit.cpp
Total findings: 2

- Line 55: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: findings.push_back({severity, endpoint_pattern, http_method, finding, recommendation});
  Confidence: band=high; score=0.74
- Line 56: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: findings.push_back({severity, endpoint_pattern, http_method, finding, recommendation});

### src/server/hot_reload_api_handler.cpp
Total findings: 2

- Line 250: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response_json.push_back(point);
  Confidence: band=high; score=0.74
- Line 251: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response_json.push_back(point);

### src/server/http3_production_config.cpp
Total findings: 2

- Line 89: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = clients_.begin(); it != clients_.end(); ) {
- Line 89: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = clients_.begin(); it != clients_.end(); ) {
  Confidence: band=very_high; score=0.9

### src/server/http_type_adapter.cpp
Total findings: 2

- Line 44: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += ' ';
  Confidence: band=high; score=0.74
- Line 45: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += ' ';

### src/server/merge_api_handler.cpp
Total findings: 2

- Line 217: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: options.manual_resolutions.push_back(resolution);
  Confidence: band=high; score=0.74
- Line 218: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: options.manual_resolutions.push_back(resolution);

### src/server/sharding_metrics_handler.cpp
Total findings: 2

- Line 80: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: oss << "# HELP themisdb_slo_error_budget Remaining error budget (0-1)\n";
  Confidence: band=very_high; score=0.9
- Line 104: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: double global_error_budget = slo_monitor.getGlobalErrorBudget();
  Confidence: band=very_high; score=0.9

### src/server/udf_api_handler.cpp
Total findings: 2

- Line 176: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(d.toJson());
  Confidence: band=high; score=0.74
- Line 177: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(d.toJson());

### src/server/workload_fingerprint_engine.cpp
Total findings: 2

- Line 0: severity=HIGH; category=uncategorized
  Context: ['        confidence = 0.0;', '    } else {', '        pattern = kPatternMap[domIdx];', '', '        // Confidence as dominance against the runner-up class.']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['        // This keeps confidence expressive even when the normalized 4-way', '        // distribution is softened by residual MIXED mass.', '        double first = vec[domIdx];', '        double second = 0.0;', '        for (std::size_t i = 0; i < vec.size(); ++i) {']
  Confidence: band=high; score=0.78

### src/server/api_version.cpp
Total findings: 1

- Line 107: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Support current major version and previous major version for backward compatibility
  Confidence: band=high; score=0.8

### src/server/policy_api_handler.cpp
Total findings: 1

- Line 49: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto jsonOpt = ranger_client.fetchPolicies(&err);

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
