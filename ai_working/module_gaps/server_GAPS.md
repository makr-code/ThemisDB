# server Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: server
- Generated: 2026-06-02 11:55:48
- Status: Critical Findings Present
- Total Findings: 2709
- Actionable Findings (Critical + High): 846
- Affected Files: 115

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 222 |
| High | 624 |
| Medium | 1858 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 521 |
| platform | 490 |
| reliability | 432 |
| container | 402 |
| security | 118 |
| concurrency | 114 |
| llm_ai_safety | 103 |
| raii | 101 |
| memory | 93 |
| observability | 75 |
| performance | 72 |
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
| src/server/http_server.cpp | 352 | 59 | 111 | 182 | 0 |
| src/server/import_wizard_builder.cpp | 220 | 0 | 2 | 218 | 0 |
| src/server/query_api_handler.cpp | 208 | 34 | 40 | 134 | 0 |
| src/server/task_scheduler_api_handler.cpp | 190 | 4 | 4 | 177 | 5 |
| src/server/llm_api_handler.cpp | 143 | 15 | 76 | 52 | 0 |
| src/server/postgres_session.cpp | 135 | 4 | 19 | 112 | 0 |
| src/server/monitoring_api_handler.cpp | 88 | 6 | 4 | 78 | 0 |
| src/server/voice_api_handler.cpp | 80 | 2 | 6 | 72 | 0 |
| src/server/mcp_server.cpp | 74 | 2 | 34 | 38 | 0 |
| src/server/shard_repair_api_handler.cpp | 55 | 7 | 5 | 43 | 0 |
| src/server/rpc/rpc_service_impl.cpp | 50 | 1 | 18 | 31 | 0 |
| src/server/mqtt_client_service.cpp | 49 | 4 | 19 | 26 | 0 |
| src/server/http3_session.cpp | 48 | 5 | 21 | 22 | 0 |
| src/server/entity_api_handler.cpp | 32 | 4 | 5 | 23 | 0 |
| src/server/policy_engine.cpp | 32 | 2 | 2 | 28 | 0 |
| src/server/websocket_session.cpp | 32 | 0 | 23 | 9 | 0 |
| src/server/rope_api_handler.cpp | 30 | 4 | 10 | 16 | 0 |
| src/server/async_job_api_handler.cpp | 29 | 2 | 17 | 10 | 0 |
| src/server/lora_api_handler.cpp | 29 | 4 | 4 | 21 | 0 |
| src/server/vector_api_handler.cpp | 28 | 2 | 0 | 26 | 0 |
| src/server/mqtt_session.cpp | 27 | 4 | 4 | 19 | 0 |
| src/server/rate_limiter_v2.cpp | 27 | 2 | 22 | 3 | 0 |
| src/server/schema_api_handler.cpp | 27 | 0 | 1 | 26 | 0 |
| src/server/http2_session.cpp | 25 | 0 | 8 | 17 | 0 |
| src/server/replication_topology_api_handler.cpp | 24 | 4 | 7 | 13 | 0 |
| src/server/changefeed_api_handler.cpp | 23 | 2 | 5 | 16 | 0 |
| src/server/distributed_gateway.cpp | 23 | 1 | 6 | 16 | 0 |
| src/server/graph_api_handler.cpp | 23 | 1 | 3 | 19 | 0 |
| src/server/content_api_handler.cpp | 21 | 0 | 0 | 21 | 0 |
| src/server/api_gateway.cpp | 18 | 1 | 9 | 8 | 0 |
| src/server/auth_middleware.cpp | 17 | 2 | 5 | 10 | 0 |
| src/server/bpmn_api_handler.cpp | 17 | 2 | 6 | 9 | 0 |
| src/server/import_api_handler.cpp | 17 | 6 | 3 | 8 | 0 |
| src/server/llm_grpc_service.cpp | 16 | 1 | 8 | 7 | 0 |
| src/server/profiling_api_handler.cpp | 16 | 0 | 0 | 16 | 0 |
| src/server/health_error_service.cpp | 15 | 4 | 8 | 3 | 0 |
| src/server/saml_auth_provider.cpp | 15 | 0 | 0 | 15 | 0 |
| src/server/tenant_manager.cpp | 15 | 1 | 8 | 6 | 0 |
| src/server/pki_api_handler.cpp | 14 | 0 | 0 | 14 | 0 |
| src/server/prompt_engineering_api_handler.cpp | 14 | 3 | 4 | 7 | 0 |
| src/server/rpc/snapshot_transfer_handler.cpp | 14 | 1 | 9 | 4 | 0 |
| src/server/wasm_handler_registry.cpp | 14 | 0 | 4 | 10 | 0 |
| src/server/ethics_api_handler.cpp | 13 | 0 | 1 | 12 | 0 |
| src/server/feedback_api_handler.cpp | 13 | 0 | 0 | 13 | 0 |
| src/server/index_api_handler.cpp | 13 | 0 | 1 | 12 | 0 |
| src/server/rpc/differential_update_engine.cpp | 13 | 0 | 0 | 13 | 0 |
| src/server/audit_api_handler.cpp | 12 | 0 | 2 | 10 | 0 |
| src/server/ranger_adapter.cpp | 12 | 0 | 1 | 11 | 0 |
| src/server/geo_topology_api_handler.cpp | 11 | 0 | 4 | 7 | 0 |
| src/server/policy_versioning_api_handler.cpp | 11 | 1 | 1 | 9 | 0 |
| src/server/saga_api_handler.cpp | 11 | 0 | 1 | 10 | 0 |
| src/server/export_api_handler.cpp | 10 | 2 | 1 | 7 | 0 |
| src/server/spatial_api_handler.cpp | 10 | 0 | 1 | 9 | 0 |
| src/server/transaction_api_handler.cpp | 10 | 0 | 0 | 10 | 0 |
| src/server/buffer_binary_protocol.cpp | 9 | 0 | 9 | 0 | 0 |
| src/server/compliance_reporting_api_handler.cpp | 9 | 1 | 2 | 6 | 0 |
| src/server/maintenance_api_handler.cpp | 9 | 2 | 3 | 4 | 0 |
| src/server/api_key_mgmt_handler.cpp | 8 | 0 | 1 | 7 | 0 |
| src/server/graphql_api_handler.cpp | 8 | 0 | 2 | 6 | 0 |
| src/server/wal_grpc_service.cpp | 8 | 0 | 3 | 5 | 0 |
| src/server/chunked_response_writer.cpp | 7 | 0 | 0 | 7 | 0 |
| src/server/session_api_handler.cpp | 7 | 5 | 1 | 1 | 0 |
| src/server/timeseries_api_handler.cpp | 7 | 0 | 3 | 4 | 0 |
| src/server/buffer_api_handler.cpp | 6 | 0 | 3 | 3 | 0 |
| src/server/cache_admin_api_handler.cpp | 6 | 1 | 1 | 4 | 0 |
| src/server/oauth2_provider.cpp | 6 | 0 | 6 | 0 | 0 |
| src/server/policy_manager_api_handler.cpp | 6 | 1 | 2 | 3 | 0 |
| src/server/retention_api_handler.cpp | 6 | 0 | 1 | 5 | 0 |
| src/server/sse_connection_manager.cpp | 6 | 1 | 4 | 1 | 0 |
| src/server/grpc_web_proxy_handler.cpp | 5 | 0 | 3 | 2 | 0 |
| src/server/opa_adapter.cpp | 5 | 1 | 2 | 2 | 0 |
| src/server/policy_template_api_handler.cpp | 5 | 1 | 1 | 3 | 0 |
| src/server/policy_validation_api_handler.cpp | 5 | 1 | 1 | 3 | 0 |
| src/server/response_transformer.cpp | 5 | 0 | 2 | 3 | 0 |
| src/server/review_scheduling_api_handler.cpp | 5 | 1 | 1 | 3 | 0 |
| src/server/smart_routing.cpp | 5 | 2 | 0 | 3 | 0 |
| src/server/snapshot_api_handler.cpp | 5 | 0 | 1 | 4 | 0 |
| src/server/branch_api_handler.cpp | 4 | 0 | 1 | 3 | 0 |
| src/server/cdn_cache_middleware.cpp | 4 | 2 | 0 | 2 | 0 |
| src/server/classification_api_handler.cpp | 4 | 0 | 0 | 4 | 0 |
| src/server/distributed_txn_api_handler.cpp | 4 | 0 | 1 | 3 | 0 |
| src/server/error_api_handler.cpp | 4 | 0 | 0 | 4 | 0 |
| src/server/openapi_route_registry.cpp | 4 | 1 | 0 | 3 | 0 |
| src/server/pii_api_handler.cpp | 4 | 0 | 0 | 4 | 0 |
| src/server/prompt_engineering_grpc_service.cpp | 4 | 0 | 0 | 4 | 0 |
| src/server/rate_limiter.cpp | 4 | 1 | 1 | 2 | 0 |
| src/server/reports_api_handler.cpp | 4 | 0 | 0 | 4 | 0 |
| src/server/rpc/blob_transfer_handler.cpp | 4 | 0 | 1 | 3 | 0 |
| src/server/serverless_function_api_handler.cpp | 4 | 0 | 0 | 4 | 0 |
| src/server/themis_core_grpc_service.cpp | 4 | 0 | 0 | 4 | 0 |
| src/server/diff_api_handler.cpp | 3 | 0 | 0 | 3 | 0 |
| src/server/pitr_api_handler.cpp | 3 | 0 | 0 | 3 | 0 |
| src/server/update_api_handler.cpp | 3 | 1 | 2 | 0 | 0 |
| src/server/wal_api_handler.cpp | 3 | 1 | 0 | 2 | 0 |
| src/server/api_security_audit.cpp | 2 | 0 | 0 | 2 | 0 |
| src/server/cache_api_handler.cpp | 2 | 0 | 2 | 0 | 0 |
| src/server/continuous_query_api_handler.cpp | 2 | 0 | 0 | 2 | 0 |
| src/server/http3_datagram.cpp | 2 | 0 | 2 | 0 | 0 |
| src/server/http_type_adapter.cpp | 2 | 0 | 0 | 2 | 0 |
| src/server/mvcc_api_handler.cpp | 2 | 0 | 1 | 1 | 0 |
| src/server/pitr_grpc_service.cpp | 2 | 0 | 0 | 2 | 0 |
| src/server/prompt_api_handler.cpp | 2 | 0 | 0 | 2 | 0 |
| src/server/sharding_metrics_handler.cpp | 2 | 0 | 2 | 0 | 0 |
| src/server/udf_api_handler.cpp | 2 | 0 | 0 | 2 | 0 |
| src/server/workload_fingerprint_engine.cpp | 2 | 0 | 2 | 0 | 0 |
| include/server/examples/workload_fingerprint_example.cpp | 1 | 0 | 0 | 1 | 0 |
| src/server/adaptive_rate_limiter.cpp | 1 | 0 | 0 | 1 | 0 |
| src/server/api_auth_config.cpp | 1 | 0 | 1 | 0 | 0 |
| src/server/api_version.cpp | 1 | 0 | 1 | 0 | 0 |
| src/server/cost_based_rate_limiter.cpp | 1 | 0 | 1 | 0 | 0 |
| src/server/hot_reload_api_handler.cpp | 1 | 0 | 0 | 1 | 0 |
| src/server/merge_api_handler.cpp | 1 | 0 | 0 | 1 | 0 |
| src/server/policy_api_handler.cpp | 1 | 0 | 1 | 0 | 0 |
| src/server/rate_limiting_middleware.cpp | 1 | 0 | 0 | 1 | 0 |
| src/server/request_coalescing.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### src/server/http_server.cpp
Total findings: 352

- Line 551: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cf_result = storage_->getOrCreateColumnFamily("pii_mappings");
- Line 637: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: adaptive_index_ = std::make_shared<AdaptiveIndexManager>(storage_->getRawDB());
- Line 1586: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (auto audit = weak_audit.lock()) {
- Line 2186: severity=CRITICAL; category=no_timeout
  Description: thread_join without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: thread.join();
- Line 2202: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: void HttpServer::wait() {
- Line 2205: severity=CRITICAL; category=no_timeout
  Description: thread_join without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: thread.join();
- Line 2427: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: THEMIS_WARN("Max connections ({}) reached - rejecting new connection",
- Line 2459: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: THEMIS_WARN("TLS enabled but SSL context unavailable; rejecting new connection");
- Line 3724: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: std::string_view incoming_corr = (corr_it != req.end()) ? std::string_view(corr_it->value()) : "";
- Line 3725: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: correlation_id = tracing_middleware_->processRequest(incoming_corr);
- Line 3800: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto checkSegment = [&](const std::string& prefix) -> std::optional<http::response<http::string_body
- Line 3981: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: http::response<http::string_body> response = ethics_api_->handle(req, target);
- Line 4706: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = monitoring_api_->handleHealthCheck(req);
- Line 4709: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = monitoring_api_->handleLiveness(req);
- Line 4712: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = monitoring_api_->handleReadiness(req);
- Line 4715: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = monitoring_api_->handleOpenApi(req);
- Line 4718: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = monitoring_api_->handleVersion(req);
- Line 4721: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = monitoring_api_->handleStats(req);
- Line 4724: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = monitoring_api_->handleCapabilities(req);
- Line 4750: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = monitoring_api_->handleMetrics(req);
- Line 4775: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = monitoring_api_->handleMetricsHtml(req);
- Line 4800: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = monitoring_api_->handlePluginMetrics(req);
- Line 4810: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = monitoring_api_->handleObservabilityAlerts(req);
- Line 4819: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = monitoring_api_->handleObservabilityAlertSilence(req);
- Line 4829: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = monitoring_api_->handleObservabilityHealth(req);
- Line 4839: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = monitoring_api_->handleLicenseStatus(req);
- Line 5055: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = cache_api_->handleQuery(req);
- Line 5090: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = cache_api_->handlePut(req);
- Line 5097: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = cache_api_->handleStats(req);
- Line 5104: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = cache_admin_api_->handleHealth(req);
- Line 5111: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = cache_admin_api_->handleStats(req);
- Line 5118: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = cache_admin_api_->handleEvictKey(req);
- Line 5125: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = cache_admin_api_->handleEvictTenant(req);
- Line 5132: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = cache_admin_api_->handleCircuitBreakerReset(req);
- Line 5139: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = cache_admin_api_->handleCircuitBreakerStatus(req);
- Line 5146: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = cache_admin_api_->handleWarmup(req);
- Line 5153: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = cache_admin_api_->handleSnapshot(req);
- Line 5160: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = cache_admin_api_->handleListTenants(req);
- Line 5167: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = cache_admin_api_->handleTenantStats(req);
- Line 5174: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = cache_admin_api_->handleUpdateTenantQuota(req);
- Line 5181: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: response = cache_admin_api_->handlePiiEvict(req);
- Line 6863: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto authz = auth_->authorize(*token, "task:register");
  Confidence: band=very_high; score=0.99
- Line 7056: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto authz = auth_->authorize(*token, "task:execute");
  Confidence: band=very_high; score=0.99
- Line 9475: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto ar = auth_->authorize(*token, scope);
  Confidence: band=very_high; score=0.99
- Line 9557: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: auth
  Context: std::cerr << "[AUTH-DBG] validateToken -> authorized=" << (vres.authorized?"true":"false")
  Confidence: band=very_high; score=0.92
- Line 9561: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto ar = auth_->authorize(*token, required_scope);
  Confidence: band=very_high; score=0.99
- Line 9563: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: auth
  Context: std::cerr << "[AUTH-DBG] authorize -> authorized=" << (ar.authorized?"true":"false")
  Confidence: band=very_high; score=0.92
- Line 9593: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: auth
  Context: std::cerr << "[AUTH-DBG] before_policy_check -> user_id='" << user_id << "' action='" << action << "' resource='" << resource << "'\n";
  Confidence: band=very_high; score=0.92
- Line 9620: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto decision = policy_engine_->authorize(user_id, std::string(action), resource, client_ip);
  Confidence: band=very_high; score=0.99
- Line 9727: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto ar = auth_->authorize(*token, "pii:reveal");
  Confidence: band=very_high; score=0.99
- Line 9729: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: ar = auth_->authorize(*token, "admin");
  Confidence: band=very_high; score=0.99
- Line 9766: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto decision = policy_engine_->authorize(user_id, "pii.write", path_only, client_ip);
  Confidence: band=very_high; score=0.99
- Line 9850: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto ar = auth_->authorize(*token, "pii:write");
  Confidence: band=very_high; score=0.99
- Line 9851: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: THEMIS_INFO("PII Delete: authorize('pii:write') -> authorized={}", ar.authorized);
  Confidence: band=very_high; score=0.99
- Line 9853: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: THEMIS_INFO("PII Delete: trying fallback authorize('admin')");
  Confidence: band=very_high; score=0.99
- Line 9854: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: ar = auth_->authorize(*token, "admin");
  Confidence: band=very_high; score=0.99
- Line 9855: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: THEMIS_INFO("PII Delete: authorize('admin') -> authorized={}", ar.authorized);
  Confidence: band=very_high; score=0.99
- Line 9888: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto decision = policy_engine_->authorize(user_id, "pii.write", path_only, client_ip);
  Confidence: band=very_high; score=0.99
- Line 12635: severity=CRITICAL; category=data_race
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
- Line 704: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: content_manager_->registerProcessor(std::unique_ptr<themis::content::IContentProcessor>(text_process
- Line 1123: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::shared_ptr<QueryEngine>(ethics_query_engine_.get(), [](QueryEngine*) {}),
- Line 1218: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: feedback_api_handler_->setLiveFeedbackCollector(live_feedback_collector_);
- Line 1219: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: feedback_api_handler_->setLearningOrchestrator(continuous_learning_orchestrator_);
- Line 1246: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: schema_api_handler_->setStatisticsCollector(stats_collector_.get());
- Line 1247: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: schema_api_handler_->setSchemaConstraints(schema_constraints_.get());
- Line 1248: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: schema_api_handler_->setSchemaVersionManager(schema_version_mgr_.get());
- Line 1249: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: schema_api_handler_->setIndexRecommender(index_recommender_.get());
- Line 1250: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: schema_api_handler_->setAuditLog(schema_audit_log_.get());
- Line 1254: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: schema_api_handler_->setColumnLineageTracker(column_lineage_tracker_.get());
- Line 1888: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ssl_ctx_->use_certificate_chain_file(config_.tls_cert_path);
- Line 1889: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ssl_ctx_->use_private_key_file(config_.tls_key_path, boost::asio::ssl::context::pem);
- Line 1895: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: SSL_CTX_set_cipher_list(ssl_ctx_->native_handle(), config_.tls_cipher_list.c_str());
- Line 1900: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: SSL_CTX_set_cipher_list(ssl_ctx_->native_handle(),
- Line 1906: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ssl_ctx_->set_options(
- Line 1921: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ssl_ctx_->load_verify_file(config_.tls_ca_cert_path);
- Line 1922: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ssl_ctx_->set_verify_mode(
- Line 1932: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ssl_ctx_->set_verify_mode(boost::asio::ssl::verify_none);
- Line 2044: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: http3_handler_->start();
- Line 2097: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: http3_handler_->stop();
- Line 2111: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (active_requests_.load(std::memory_order_acquire) > 0
- Line 2113: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
- Line 2115: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: auto remaining = active_requests_.load(std::memory_order_acquire);
- Line 2232: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: new_ctx->use_certificate_chain_file(config_.tls_cert_path);
- Line 2233: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: new_ctx->use_private_key_file(config_.tls_key_path, boost::asio::ssl::context::pem);
- Line 2236: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: SSL_CTX_set_cipher_list(new_ctx->native_handle(), config_.tls_cipher_list.c_str());
- Line 2238: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: SSL_CTX_set_cipher_list(new_ctx->native_handle(),
- Line 2242: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: new_ctx->set_options(
- Line 2252: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: new_ctx->load_verify_file(config_.tls_ca_cert_path);
- Line 2253: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: new_ctx->set_verify_mode(
- Line 2261: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: new_ctx->set_verify_mode(boost::asio::ssl::verify_none);
- Line 2363: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: workload_optimizer_->record_query(
- Line 2397: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: acceptor_.async_accept(
- Line 3005: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward compatibility alias
  Confidence: band=high; score=0.8
- Line 3688: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: span.setAttribute("http.method", std::string(http::to_string(req.method())));
- Line 3689: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: span.setAttribute("http.target", std::string(req.target()));
- Line 3942: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!tenant_guard->acquireQuerySlot()) {
- Line 4399: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: llm_request.model_id = payload.value("model", std::string{"default"});
- Line 4401: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: llm_request.temperature = static_cast<float>(payload.value("temperature", 0.7));
- Line 4494: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = assistant.query(query);
- Line 4513: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response_body["relevant_documents"] = docs_array;
- Line 5582: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: snapshot_api_handler_->handleCreateTag(httplib_req, httplib_res);
- Line 5595: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: snapshot_api_handler_->handleListTags(httplib_req, httplib_res);
- Line 5608: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: snapshot_api_handler_->handleGetTag(httplib_req, httplib_res);
- Line 5621: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: snapshot_api_handler_->handleDeleteTag(httplib_req, httplib_res);
- Line 5634: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: snapshot_api_handler_->handleGetStats(httplib_req, httplib_res);
- Line 5649: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: diff_api_handler_->handleGetDiff(httplib_req, httplib_res);
- Line 5662: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: diff_api_handler_->handleGetCacheStats(httplib_req, httplib_res);
- Line 5675: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: diff_api_handler_->handleClearCache(httplib_req, httplib_res);
- Line 5690: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: pitr_api_handler_->handleRestore(httplib_req, httplib_res);
- Line 5703: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: pitr_api_handler_->handlePreview(httplib_req, httplib_res);
- Line 5716: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: pitr_api_handler_->handleGetProgress(httplib_req, httplib_res);
- Line 5729: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: branch_api_handler_->handleCreateBranch(httplib_req, httplib_res);
- Line 5740: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: branch_api_handler_->handleListBranches(httplib_req, httplib_res);
- Line 5751: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: branch_api_handler_->handleGetActiveBranch(httplib_req, httplib_res);
- Line 5762: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: branch_api_handler_->handleGetStats(httplib_req, httplib_res);
- Line 5773: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: branch_api_handler_->handleGetBranch(httplib_req, httplib_res);
- Line 5784: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: branch_api_handler_->handleSwitchBranch(httplib_req, httplib_res);
- Line 5795: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: branch_api_handler_->handleDeleteBranch(httplib_req, httplib_res);
- Line 5806: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: branch_api_handler_->handleMergeBranches(httplib_req, httplib_res);
- Line 5819: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: merge_api_handler_->handleMerge(httplib_req, httplib_res);
- Line 5830: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: merge_api_handler_->handleMergePreview(httplib_req, httplib_res);
- Line 5841: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: merge_api_handler_->handleMergeByTag(httplib_req, httplib_res);
- Line 5852: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: merge_api_handler_->handleCanFastForward(httplib_req, httplib_res);
- Line 6146: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = feedback_api_handler_->handleCreateFeedback(req);
- Line 6155: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = feedback_api_handler_->handleListFeedback(req);
- Line 6170: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = feedback_api_handler_->handleGetFeedback(req, id);
- Line 6186: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = feedback_api_handler_->handleUpdateFeedback(req, id);
- Line 6202: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = feedback_api_handler_->handleDeleteFeedback(req, id);
- Line 6218: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = feedback_api_handler_->handleGetAdapterFeedback(req, adapter_id);
- Line 6228: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = feedback_api_handler_->handleGetStatistics(req);
- Line 6563: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: mvcc_api_handler_->handleGetKey(httplib_req, httplib_res);
- Line 6565: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: mvcc_api_handler_->handlePutKey(httplib_req, httplib_res);
- Line 6587: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: mvcc_api_handler_->handleListVersions(httplib_req, httplib_res);
- Line 6589: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: mvcc_api_handler_->handleGcVersions(httplib_req, httplib_res);
- Line 6602: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: mvcc_api_handler_->handleGetClock(httplib_req, httplib_res);
- Line 6614: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: mvcc_api_handler_->handleGetStats(httplib_req, httplib_res);
- Line 6624: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = graphql_api_handler_->handlePost(req);
- Line 6629: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = graphql_api_handler_->handleSchemaGet(req);
- Line 6662: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = serverless_fn_handler_->handleRegister(req);
- Line 6666: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = serverless_fn_handler_->handleList(req);
- Line 6701: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = serverless_fn_handler_->handleInvoke(req, id);
- Line 6703: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = serverless_fn_handler_->handleVersions(req, id);
- Line 6705: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = serverless_fn_handler_->handleGet(req, id);
- Line 6707: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = serverless_fn_handler_->handleUpdate(req, id);
- Line 6709: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = serverless_fn_handler_->handleDelete(req, id);
- Line 6793: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = udf_api_handler_->handleRegister(req);
- Line 6796: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = udf_api_handler_->handleList(req);
- Line 6804: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = udf_api_handler_->handleGet(req, udf_name);
- Line 6813: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: response = udf_api_handler_->handleDelete(req, udf_name);
- Line 7470: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto& seg : {std::string("page"), std::string("page_size"),
- Line 7849: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: span.setAttribute("http.status_code", static_cast<int64_t>(response.result_int()));
- Line 9343: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: feature_semantic_cache_live_.store(enabled, std::memory_order_relaxed);
- Line 9415: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: {"max_write_buffer_number", storage_->getConfig().max_write_buffer_number},
- Line 9416: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: {"min_write_buffer_number_to_merge", storage_->getConfig().min_write_buffer_number_to_merge},
- Line 10169: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response["interactions"] = json::array();
- Line 11596: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: const uint32_t timeout_ms = server_->request_timeout_ms_live_.load(std::memory_order_relaxed);
- Line 11889: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: const uint32_t timeout_ms = server_->request_timeout_ms_live_.load(std::memory_order_relaxed);
- Line 12370: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: resp["indexes"] = stats_array;
- Line 12619: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: user_id = ctx->user_id;
- Line 12729: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: endpoints.push_back({"POST", "/api/aql",                  "AQL query (compat)"});
- Line 12847: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: const bool cap_semantic_cache = feature_semantic_cache_live_.load(std::memory_order_relaxed);
- Line 12848: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: const bool cap_llm_store      = feature_llm_store_live_.load(std::memory_order_relaxed);
- Line 13036: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: endpoints.push_back({"POST", "/api/v1/graphql",            "GraphQL query (v1)"});
- Line 666: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 704: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: content_manager_->registerProcessor(std::unique_ptr<themis::content::IContentProcessor>(text_process
- Line 765: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { audit_rate_limit_per_minute_ = static_cast<uint32_t>(std::stoul(lim)); } catch (...) {}
- Line 912: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { slo_cfg.warning_threshold = std::stod(warn_thr); } catch (...) {}
- Line 915: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { slo_cfg.critical_threshold = std::stod(crit_thr); } catch (...) {}
- Line 951: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { rcfg.connect_timeout_ms = std::stol(*ct); } catch (...) {}
- Line 954: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { rcfg.request_timeout_ms = std::stol(*rt); } catch (...) {}
- Line 957: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { rcfg.max_retries = std::stoi(*mr); } catch (...) {}
- Line 960: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { rcfg.retry_backoff_ms = std::stol(*rb); } catch (...) {}
- Line 965: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1151: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 1513: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { opa_cfg.timeout_ms = std::stol(*tms); } catch (...) {}
- Line 1565: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1592: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { audit->logEvent(entry); } catch (...) {}
- Line 1855: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { THEMIS_WARN("Invalid THEMIS_MAX_BODY_BYTES value, using default 10MB"); }
- Line 1900: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: SSL_CTX_set_cipher_list(ssl_ctx_->native_handle(),
  Confidence: band=high; score=0.74
- Line 1963: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2010: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads_.emplace_back([this, i] {
  Confidence: band=high; score=0.74
- Line 2165: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: storage_->close(); // This flushes and closes cleanly
- Line 2238: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: SSL_CTX_set_cipher_list(new_ctx->native_handle(),
  Confidence: band=high; score=0.74
- Line 2431: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: socket.close(close_ec);
- Line 2500: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += ' ';
- Line 3535: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: static constexpr std::string_view kMaintStatus{"/api/v1/maintenance/status"};
- Line 3682: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> req_headers;
  Confidence: band=high; score=0.74
- Line 3755: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: header_bytes += field.name_string().size() + field.value().size() + 4; // 2 for ": " + 2 for "\r\n"
  Confidence: band=high; score=0.74
- Line 3756: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: header_bytes += field.name_string().size() + field.value().size() + 4; // 2 for ": " + 2 for "\r\n"
- Line 3887: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> headers;
  Confidence: band=medium; score=0.66
- Line 4506: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: docs_array.push_back({
  Confidence: band=high; score=0.74
- Line 4507: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: docs_array.push_back({
- Line 4735: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 5235: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shards_arr.push_back({
  Confidence: band=high; score=0.74
- Line 5368: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back({
  Confidence: band=high; score=0.74
- Line 6858: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 7051: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 7477: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: if (seg == "page") try { filter.page = std::stoi(val); } catch (...) {}
- Line 7478: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: else if (seg == "page_size") try { filter.page_size = std::stoi(val); } catch (...) {}
- Line 7557: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { limit = std::stoi(std::string(req.target()).substr(qpos + 6)); } catch (...) {}
- Line 8219: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 8244: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { if (!req.body().empty()) body_json = json::parse(req.body()); } catch (...) {}
- Line 8269: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { body = json::parse(req.body()); } catch (...) {
- Line 8343: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { if (!req.body().empty()) body = json::parse(req.body()); } catch (...) {
- Line 8396: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 8407: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { body = json::parse(req.body()); } catch (...) {
- Line 8442: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 8474: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 8506: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 8522: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 8656: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 8776: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "# HELP themis_content_blob_uncompressed_bytes_total Total uncompressed/original bytes observ
- Line 8780: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "# HELP themis_content_blob_compression_ratio Histogram of compression ratios (original_size
- Line 8940: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "themis_wal_apply_latency_seconds_sum " << (static_cast<double>(wal_latency_sum_us) / 1'000'0
- Line 8940: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "themis_wal_apply_latency_seconds_sum " << (static_cast<double>(wal_latency_sum_us) / 1'000'0
- Line 8965: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "themis_content_blob_compression_skipped_total{mime_prefix=\"image/\"} " << skipped_img << "\
- Line 8965: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "themis_content_blob_compression_skipped_total{mime_prefix=\"image/\"} " << skipped_img << "\
- Line 8966: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "themis_content_blob_compression_skipped_total{mime_prefix=\"video/\"} " << skipped_vid << "\
- Line 8966: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "themis_content_blob_compression_skipped_total{mime_prefix=\"video/\"} " << skipped_vid << "\
- Line 8967: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "themis_content_blob_compression_skipped_total{mime_prefix=\"application/zip\"} " << skipped_
- Line 8967: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "themis_content_blob_compression_skipped_total{mime_prefix=\"application/zip\"} " << skipped_
- Line 8996: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "themis_content_blob_compression_skipped_total{mime_prefix=\"image/\"} 0\n";
- Line 8996: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "themis_content_blob_compression_skipped_total{mime_prefix=\"image/\"} 0\n";
- Line 8997: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "themis_content_blob_compression_skipped_total{mime_prefix=\"video/\"} 0\n";
- Line 8997: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "themis_content_blob_compression_skipped_total{mime_prefix=\"video/\"} 0\n";
- Line 8998: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "themis_content_blob_compression_skipped_total{mime_prefix=\"application/zip\"} 0\n";
- Line 8998: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "themis_content_blob_compression_skipped_total{mime_prefix=\"application/zip\"} 0\n";
- Line 9032: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: static std::unordered_map<std::string, std::string> parseQuery(const std::string& target) {
  Confidence: band=high; score=0.74
- Line 9052: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { return std::stoll(s); } catch (...) { return 0; }
- Line 9076: severity=MEDIUM; category=expensive_copy
  Description: Unnecessary expensive copy
  Remediation: Use const reference (const T&) or std::move if transfer is needed
  Context: if (zpos != std::string::npos) tzpos = zpos;
- Line 9101: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { tz_h = tz_m = 0; tz_sign = 0; }
- Line 9139: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { f.page = std::max(1, std::stoi(it->second)); } catch (...) {}
- Line 9146: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 9179: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { f.page = std::max(1, std::stoi(it->second)); } catch (...) {}
- Line 9186: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 9216: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 9265: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 9293: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 9302: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto lvl = lg["level"].get<std::string>();
  Confidence: band=high; score=0.74
- Line 9309: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto fmt = lg["format"].get<std::string>();
  Confidence: band=high; score=0.74
- Line 9325: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto timeout = body["request_timeout_ms"].get<uint32_t>();
  Confidence: band=high; score=0.74
- Line 9368: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto hours = body["cdc_retention_hours"].get<uint32_t>();
  Confidence: band=high; score=0.74
- Line 9453: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 9512: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 9531: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 9541: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 9559: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 9560: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 9565: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 9594: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 9647: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 9705: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 9828: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 9961: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 10047: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 10170: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response["interactions"].push_back(interaction.toJson());
  Confidence: band=high; score=0.74
- Line 10171: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response["interactions"].push_back(interaction.toJson());
- Line 10408: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(std::move(j));
  Confidence: band=high; score=0.74
- Line 10438: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resp.push_back({{"pk", pk}, {"score", score}});
  Confidence: band=high; score=0.74
- Line 10448: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 10494: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resp.push_back({
  Confidence: band=high; score=0.74
- Line 10515: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 10572: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vectorQuery.push_back(val.get<float>());
  Confidence: band=high; score=0.74
- Line 10600: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> scores;
  Confidence: band=medium; score=0.66
- Line 10617: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fusedResults.emplace_back(pk, score);
  Confidence: band=high; score=0.74
- Line 10639: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> scores;
  Confidence: band=medium; score=0.66
- Line 10678: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resp.push_back({
  Confidence: band=high; score=0.74
- Line 10705: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 10729: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 10753: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 10908: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 10926: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = body["weights"].begin(); it != body["weights"].end(); ++it) {
  Confidence: band=high; score=0.74
- Line 10938: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 11127: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto configObj = body["config"];
  Confidence: band=high; score=0.74
- Line 11132: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (s.is_string()) config.stopwords.push_back(s.get<std::string>());
  Confidence: band=high; score=0.74
- Line 11133: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (s.is_string()) config.stopwords.push_back(s.get<std::string>());
- Line 11173: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: columns.push_back(c.get<std::string>());
  Confidence: band=high; score=0.74
- Line 11714: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = request_[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 11909: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void HttpServer::SslSession::start() {
  Confidence: band=medium; score=0.66
- Line 11937: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 12042: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = request_[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 12291: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resp.push_back(stat_obj);
  Confidence: band=high; score=0.74
- Line 12363: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats_array.push_back({
  Confidence: band=high; score=0.74
- Line 12364: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats_array.push_back({
- Line 12434: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response.push_back(suggestion.toJson());
  Confidence: band=high; score=0.74
- Line 12435: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response.push_back(suggestion.toJson());
- Line 12476: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response.push_back(pattern.toJson());
  Confidence: band=high; score=0.74
- Line 12477: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response.push_back(pattern.toJson());
- Line 12929: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/merge/can-fast-forward", "Check fast-forward merge"});
- Line 12932: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/mvcc/keys/{key}", "Get key versions"});
- Line 12933: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/api/v1/mvcc/keys/{key}", "Put versioned key"});
- Line 12934: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/mvcc/keys/{key}/versions", "Get version history"});
- Line 12935: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"DELETE", "/api/v1/mvcc/keys/{key}/versions", "Delete versions"});
- Line 12936: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/mvcc/clock",      "Get HLC timestamp"});
- Line 12937: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/mvcc/stats",      "MVCC statistics"});
- Line 12942: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/ts/put",                "Store time-series data"});
- Line 12943: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/ts/query",              "Query time-series (beta)"});
- Line 12944: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/ts/aggregate",          "Aggregate time-series (beta)"});
- Line 12945: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/ts/config",             "Get time-series config"});
- Line 12946: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"PUT",  "/ts/config",             "Update time-series config"});
- Line 12947: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/ts/aggregates",         "List aggregates"});
- Line 12948: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/ts/retention",          "Get retention policy"});
- Line 12966: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/api/updates/check",     "Check for updates"});
- Line 12967: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/updates/config",    "Get update config"});
- Line 12968: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"PUT",  "/api/updates/config",    "Update update config"});
- Line 12975: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/auth/saml/login",    "SAML login initiator"});
- Line 12976: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/api/v1/auth/saml/acs",      "SAML assertion consumer"});
- Line 12977: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/api/v1/auth/saml/slo",      "SAML logout"});
- Line 12978: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/auth/saml/metadata", "SAML metadata"});
- Line 12982: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/api/pki/{key_id}/sign",     "Sign with PKI key"});
- Line 12983: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/api/pki/{key_id}/verify",   "Verify PKI signature"});
- Line 12984: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/api/pki/hsm/sign",          "HSM sign"});
- Line 12985: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/pki/hsm/keys",          "List HSM keys"});
- Line 13012: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/api/v1/export/jsonl_llm",   "Export to JSONL for LLM"});
- Line 13013: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/export/{id}/status", "Export job status"});
- Line 13016: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/api/keys",                  "Create API key"});
- Line 13017: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/keys",                  "List API keys"});
- Line 13018: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/keys/{id}",             "Get API key"});
- Line 13019: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"PUT",  "/api/keys/{id}",             "Update API key"});
- Line 13020: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"DELETE", "/api/keys/{id}",           "Delete API key"});
- Line 13023: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/auth/sessions",             "Create session"});
- Line 13024: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/auth/sessions",             "List sessions"});
- Line 13025: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"DELETE", "/auth/sessions/{id}",      "Delete session"});
- Line 13026: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"DELETE", "/auth/sessions",           "Revoke all other sessions"});
- Line 13029: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/api/v1/query/udfs",         "Register UDF"});
- Line 13030: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/query/udfs",         "List UDFs"});
- Line 13031: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/query/udfs/{name}",  "Get UDF"});
- Line 13097: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/information_schema", "INFORMATION_SCHEMA"});
- Line 13098: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/metadata/stats/{table}", "Table statistics"});
- Line 13099: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/api/v1/metadata/stats/{table}", "Update statistics"});
- Line 13100: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/metadata/constraints/{table}", "Table constraints"});
- Line 13101: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/metadata/index_recommendations", "Index recommendations"});
- Line 13102: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/metadata/audit",     "Metadata audit log"});
- Line 13103: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/metadata/lineage/{table}", "Column lineage"});
- Line 13104: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/api/v1/metadata/lineage",   "Track column lineage"});
- Line 13105: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"PUT",  "/api/v1/metadata/schema_import", "Import schema"});
- Line 13106: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/api/v1/metadata/constraints/validate/{table}", "Validate constraints"
- Line 13109: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/errors",             "List error codes"});
- Line 13110: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/errors/{code}",      "Get error documentation"});
- Line 13111: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/errors/categories",  "Error categories"});
- Line 13112: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"GET",  "/api/v1/errors/search",      "Search errors"});
- Line 13115: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/api/v1/bpmn/process/start", "Start BPMN process"});
- Line 13116: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: endpoints.push_back({"POST", "/api/v1/bpmn/task/{id}/complete", "Complete BPMN task"});
- Line 13619: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { offset = std::stoull(rv.substr(0, dash)); } catch (...) {}
- Line 13624: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}

### src/server/import_wizard_builder.cpp
Total findings: 220

- Line 0: severity=HIGH; category=uncategorized
  Context: ['    html += "<!DOCTYPE html>\\n<html lang=\\"en\\">\\n<head>\\n";', '    html += "<meta charset=\\"UTF-8\\">\\n";', '    html += "<meta name=\\"viewport\\" content=\\"width=device-width,initial-scale=1\\">\\n";', '    html += "<title>ThemisDB Import Wizard</title>\\n";', '    html += "<style>\\n";']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 34: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<title>ThemisDB Import Wizard</title>\n";
- Line 116: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</style>\n</head>\n<body>\n";
- Line 120: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<h1>&#128190; ThemisDB Import Wizard</h1>\n";
- Line 121: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<p class=\"sub\">Import data from PostgreSQL or S3 into ThemisDB in a few guided steps.</p>
- Line 121: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<p class=\"sub\">Import data from PostgreSQL or S3 into ThemisDB in a few guided steps.</p>
- Line 125: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"step active\" id=\"step-tab-1\">1&nbsp;Source</div>\n";
- Line 125: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"step active\" id=\"step-tab-1\">1&nbsp;Source</div>\n";
- Line 125: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"step active\" id=\"step-tab-1\">1&nbsp;Source</div>\n";
- Line 126: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"step\" id=\"step-tab-2\">2&nbsp;Connection</div>\n";
- Line 126: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"step\" id=\"step-tab-2\">2&nbsp;Connection</div>\n";
- Line 126: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"step\" id=\"step-tab-2\">2&nbsp;Connection</div>\n";
- Line 127: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"step\" id=\"step-tab-3\">3&nbsp;Options</div>\n";
- Line 127: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"step\" id=\"step-tab-3\">3&nbsp;Options</div>\n";
- Line 127: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"step\" id=\"step-tab-3\">3&nbsp;Options</div>\n";
- Line 128: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"step\" id=\"step-tab-4\">4&nbsp;Review</div>\n";
- Line 128: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"step\" id=\"step-tab-4\">4&nbsp;Review</div>\n";
- Line 128: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"step\" id=\"step-tab-4\">4&nbsp;Review</div>\n";
- Line 129: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"step\" id=\"step-tab-5\">5&nbsp;Progress</div>\n";
- Line 129: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"step\" id=\"step-tab-5\">5&nbsp;Progress</div>\n";
- Line 129: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"step\" id=\"step-tab-5\">5&nbsp;Progress</div>\n";
- Line 130: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";
- Line 134: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<h2>Choose a data source</h2>\n";
- Line 137: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <div class=\"icon\">&#128036;</div>\n";
- Line 137: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <div class=\"icon\">&#128036;</div>\n";
- Line 138: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <div class=\"name\">PostgreSQL</div>\n";
- Line 138: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <div class=\"name\">PostgreSQL</div>\n";
- Line 139: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <div class=\"desc\">pg_dump file or connection string</div>\n";
- Line 139: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <div class=\"desc\">pg_dump file or connection string</div>\n";
- Line 140: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  </div>\n";
- Line 142: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <div class=\"icon\">&#9729;</div>\n";
- Line 142: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <div class=\"icon\">&#9729;</div>\n";
- Line 143: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <div class=\"name\">S3 / Object Storage</div>\n";
- Line 143: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <div class=\"name\">S3 / Object Storage</div>\n";
- Line 144: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <div class=\"desc\">s3://bucket/key (CSV, TSV, JSONL)</div>\n";
- Line 144: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <div class=\"desc\">s3://bucket/key (CSV, TSV, JSONL)</div>\n";
- Line 145: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  </div>\n";
- Line 146: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";
- Line 147: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div class=\"btn-row\"><button class=\"btn btn-primary\" onclick=\"goStep(2)\">Next &rarr;<
- Line 147: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div class=\"btn-row\"><button class=\"btn btn-primary\" onclick=\"goStep(2)\">Next &rarr;<
- Line 147: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div class=\"btn-row\"><button class=\"btn btn-primary\" onclick=\"goStep(2)\">Next &rarr;<
- Line 147: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div class=\"btn-row\"><button class=\"btn btn-primary\" onclick=\"goStep(2)\">Next &rarr;<
- Line 148: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";  // panel-1
- Line 152: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<h2>Configure source</h2>\n";
- Line 154: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label for=\"pg-path\">pg_dump file path or PostgreSQL connection string</label>\n";
- Line 154: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label for=\"pg-path\">pg_dump file path or PostgreSQL connection string</label>\n";
- Line 155: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <input id=\"pg-path\" type=\"text\" placeholder=\"/path/to/dump.sql  or  postgresql://use
- Line 155: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <input id=\"pg-path\" type=\"text\" placeholder=\"/path/to/dump.sql  or  postgresql://use
- Line 155: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <input id=\"pg-path\" type=\"text\" placeholder=\"/path/to/dump.sql  or  postgresql://use
- Line 155: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <input id=\"pg-path\" type=\"text\" placeholder=\"/path/to/dump.sql  or  postgresql://use
- Line 156: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <p style=\"font-size:0.78rem;color:#7a7aaa;margin-top:4px\">The path is resolved on the T
- Line 156: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <p style=\"font-size:0.78rem;color:#7a7aaa;margin-top:4px\">The path is resolved on the T
- Line 157: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";
- Line 159: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label for=\"s3-path\">S3 URL</label>\n";
- Line 159: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label for=\"s3-path\">S3 URL</label>\n";
- Line 160: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <input id=\"s3-path\" type=\"text\" placeholder=\"s3://my-bucket/export/dump.csv\">\n";
- Line 160: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <input id=\"s3-path\" type=\"text\" placeholder=\"s3://my-bucket/export/dump.csv\">\n";
- Line 160: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <input id=\"s3-path\" type=\"text\" placeholder=\"s3://my-bucket/export/dump.csv\">\n";
- Line 160: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <input id=\"s3-path\" type=\"text\" placeholder=\"s3://my-bucket/export/dump.csv\">\n";
- Line 161: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <p style=\"font-size:0.78rem;color:#7a7aaa;margin-top:4px\">Use a trailing / to import an
- Line 161: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <p style=\"font-size:0.78rem;color:#7a7aaa;margin-top:4px\">Use a trailing / to import an
- Line 162: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";
- Line 163: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div id=\"step2-error\" class=\"alert alert-error\" style=\"display:none\"></div>\n";
- Line 163: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div id=\"step2-error\" class=\"alert alert-error\" style=\"display:none\"></div>\n";
- Line 163: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div id=\"step2-error\" class=\"alert alert-error\" style=\"display:none\"></div>\n";
- Line 163: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div id=\"step2-error\" class=\"alert alert-error\" style=\"display:none\"></div>\n";
- Line 165: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-secondary\" onclick=\"goStep(1)\">&larr; Back</button>\n";
- Line 165: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-secondary\" onclick=\"goStep(1)\">&larr; Back</button>\n";
- Line 165: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-secondary\" onclick=\"goStep(1)\">&larr; Back</button>\n";
- Line 166: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-primary\" onclick=\"goStep(3)\">Next &rarr;</button>\n";
- Line 166: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-primary\" onclick=\"goStep(3)\">Next &rarr;</button>\n";
- Line 166: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-primary\" onclick=\"goStep(3)\">Next &rarr;</button>\n";
- Line 167: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";
- Line 168: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";  // panel-2
- Line 172: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<h2>Import options</h2>\n";
- Line 173: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<label for=\"opt-namespace\">Target namespace</label>\n";
- Line 173: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<label for=\"opt-namespace\">Target namespace</label>\n";
- Line 175: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<label for=\"opt-batch\">Batch size (rows per transaction)</label>\n";
- Line 175: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<label for=\"opt-batch\">Batch size (rows per transaction)</label>\n";
- Line 179: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label for=\"opt-dryrun\">Dry-run (validate without writing data)</label>\n";
- Line 179: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label for=\"opt-dryrun\">Dry-run (validate without writing data)</label>\n";
- Line 180: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";
- Line 183: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label for=\"opt-skip-dup\">Skip duplicate records</label>\n";
- Line 183: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label for=\"opt-skip-dup\">Skip duplicate records</label>\n";
- Line 184: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";
- Line 187: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label for=\"opt-continue-err\">Continue on row errors</label>\n";
- Line 187: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label for=\"opt-continue-err\">Continue on row errors</label>\n";
- Line 188: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";
- Line 189: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<details><summary>&#9881; Advanced options</summary>\n";
- Line 190: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<label for=\"opt-include\">Include tables (comma-separated, empty = all)</label>\n";
- Line 190: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<label for=\"opt-include\">Include tables (comma-separated, empty = all)</label>\n";
- Line 192: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<label for=\"opt-exclude\">Exclude tables (comma-separated)</label>\n";
- Line 192: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<label for=\"opt-exclude\">Exclude tables (comma-separated)</label>\n";
- Line 194: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<label for=\"opt-conflict\">Conflict strategy</label>\n";
- Line 194: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<label for=\"opt-conflict\">Conflict strategy</label>\n";
- Line 196: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <option value=\"0\" selected>Overwrite (replace existing)</option>\n";
- Line 196: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <option value=\"0\" selected>Overwrite (replace existing)</option>\n";
- Line 197: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <option value=\"1\">Skip (keep existing)</option>\n";
- Line 197: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <option value=\"1\">Skip (keep existing)</option>\n";
- Line 198: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <option value=\"2\">Merge (field-level, incoming wins)</option>\n";
- Line 198: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <option value=\"2\">Merge (field-level, incoming wins)</option>\n";
- Line 199: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <option value=\"3\">Error (abort on first conflict)</option>\n";
- Line 199: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <option value=\"3\">Error (abort on first conflict)</option>\n";
- Line 200: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</select>\n";
- Line 201: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</details>\n";
- Line 203: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-secondary\" onclick=\"goStep(2)\">&larr; Back</button>\n";
- Line 203: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-secondary\" onclick=\"goStep(2)\">&larr; Back</button>\n";
- Line 203: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-secondary\" onclick=\"goStep(2)\">&larr; Back</button>\n";
- Line 204: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-primary\" onclick=\"goStep(4)\">Review &rarr;</button>\n";
- Line 204: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-primary\" onclick=\"goStep(4)\">Review &rarr;</button>\n";
- Line 204: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-primary\" onclick=\"goStep(4)\">Review &rarr;</button>\n";
- Line 205: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";
- Line 206: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";  // panel-3
- Line 210: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<h2>Review &amp; start import</h2>\n";
- Line 212: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "background:#0f1829;border-radius:4px;padding:14px\"></div>\n";
- Line 214: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-secondary\" onclick=\"goStep(3)\">&larr; Back</button>\n";
- Line 214: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-secondary\" onclick=\"goStep(3)\">&larr; Back</button>\n";
- Line 214: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-secondary\" onclick=\"goStep(3)\">&larr; Back</button>\n";
- Line 215: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-primary\" id=\"btn-start\" onclick=\"startImport()\">&#9654; Star
- Line 215: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-primary\" id=\"btn-start\" onclick=\"startImport()\">&#9654; Star
- Line 215: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-primary\" id=\"btn-start\" onclick=\"startImport()\">&#9654; Star
- Line 215: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-primary\" id=\"btn-start\" onclick=\"startImport()\">&#9654; Star
- Line 216: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";
- Line 217: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";  // panel-4
- Line 221: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<h2>Import progress</h2>\n";
- Line 224: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <span id=\"prog-stage\" style=\"font-size:0.85rem;color:#aaaacc;font-style:italic\">Ini
- Line 224: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <span id=\"prog-stage\" style=\"font-size:0.85rem;color:#aaaacc;font-style:italic\">Ini
- Line 224: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <span id=\"prog-stage\" style=\"font-size:0.85rem;color:#aaaacc;font-style:italic\">Ini
- Line 225: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <span id=\"prog-badge\" class=\"status-badge s-running\">running</span>\n";
- Line 225: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <span id=\"prog-badge\" class=\"status-badge s-running\">running</span>\n";
- Line 225: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <span id=\"prog-badge\" class=\"status-badge s-running\">running</span>\n";
- Line 226: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  </div>\n";
- Line 227: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"progress-bar-wrap\"><div class=\"progress-bar\" id=\"prog-bar\" style=\"widt
- Line 227: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"progress-bar-wrap\"><div class=\"progress-bar\" id=\"prog-bar\" style=\"widt
- Line 227: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"progress-bar-wrap\"><div class=\"progress-bar\" id=\"prog-bar\" style=\"widt
- Line 227: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"progress-bar-wrap\"><div class=\"progress-bar\" id=\"prog-bar\" style=\"widt
- Line 227: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"progress-bar-wrap\"><div class=\"progress-bar\" id=\"prog-bar\" style=\"widt
- Line 229: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "<span id=\"prog-current\">0</span> / <span id=\"prog-total\">?</span> records</div>\n";
- Line 229: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "<span id=\"prog-current\">0</span> / <span id=\"prog-total\">?</span> records</div>\n";
- Line 229: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "<span id=\"prog-current\">0</span> / <span id=\"prog-total\">?</span> records</div>\n";
- Line 230: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";
- Line 231: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div id=\"result-box\" style=\"display:none;margin-top:18px\"></div>\n";
- Line 231: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div id=\"result-box\" style=\"display:none;margin-top:18px\"></div>\n";
- Line 231: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div id=\"result-box\" style=\"display:none;margin-top:18px\"></div>\n";
- Line 233: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-secondary\" id=\"btn-new\" onclick=\"resetWizard()\">&#8635; New
- Line 233: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-secondary\" id=\"btn-new\" onclick=\"resetWizard()\">&#8635; New
- Line 233: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-secondary\" id=\"btn-new\" onclick=\"resetWizard()\">&#8635; New
- Line 233: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-secondary\" id=\"btn-new\" onclick=\"resetWizard()\">&#8635; New
- Line 234: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-danger\" id=\"btn-cancel\" onclick=\"cancelImport()\">&#9632; Can
- Line 234: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-danger\" id=\"btn-cancel\" onclick=\"cancelImport()\">&#9632; Can
- Line 234: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-danger\" id=\"btn-cancel\" onclick=\"cancelImport()\">&#9632; Can
- Line 234: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn btn-danger\" id=\"btn-cancel\" onclick=\"cancelImport()\">&#9632; Can
- Line 235: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";
- Line 236: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";  // panel-5
- Line 238: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";  // card
- Line 242: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<h2>Recent import jobs</h2>\n";
- Line 243: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div id=\"jobs-list\"><em style=\"color:#555\">Loading&#8230;</em></div>\n";
- Line 243: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div id=\"jobs-list\"><em style=\"color:#555\">Loading&#8230;</em></div>\n";
- Line 243: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div id=\"jobs-list\"><em style=\"color:#555\">Loading&#8230;</em></div>\n";
- Line 245: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "<button class=\"btn btn-secondary\" style=\"font-size:0.8rem\" onclick=\"loadJobs()\">&#8635; Refre
- Line 245: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "<button class=\"btn btn-secondary\" style=\"font-size:0.8rem\" onclick=\"loadJobs()\">&#8635; Refre
- Line 245: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "<button class=\"btn btn-secondary\" style=\"font-size:0.8rem\" onclick=\"loadJobs()\">&#8635; Refre
- Line 245: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "<button class=\"btn btn-secondary\" style=\"font-size:0.8rem\" onclick=\"loadJobs()\">&#8635; Refre
- Line 246: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "</div>\n";
- Line 247: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";  // jobs-panel
- Line 268: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: html += "    var p=document.getElementById('panel-'+i);\n";
  Confidence: band=high; score=0.74
- Line 293: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  function row(k,v){return '<tr><td style=\"color:#7a7aaa;padding:3px 8px 3px 0;white-space
- Line 293: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  function row(k,v){return '<tr><td style=\"color:#7a7aaa;padding:3px 8px 3px 0;white-space
- Line 294: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "<td style=\"color:#d0d0e8;padding:3px 0\">'+v+'</td></tr>';}\n";
- Line 294: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "<td style=\"color:#d0d0e8;padding:3px 0\">'+v+'</td></tr>';}\n";
- Line 295: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  html+=row('Source type',currentSource==='postgresql'?'PostgreSQL':'S3 / Object Storage');
- Line 296: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  html+=row('Source path','<code>'+escHtml(path)+'</code>');\n";
- Line 297: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  html+=row('Target namespace','<code>'+escHtml(ns)+'</code>');\n";
- Line 299: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  html+=row('Dry-run',dr?'<span style=\"color:#ffcc00\">Yes (no data written)</span>':'No')
- Line 299: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  html+=row('Dry-run',dr?'<span style=\"color:#ffcc00\">Yes (no data written)</span>':'No')
- Line 299: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  html+=row('Dry-run',dr?'<span style=\"color:#ffcc00\">Yes (no data written)</span>':'No')
- Line 302: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  if(inc) html+=row('Include tables','<code>'+escHtml(inc)+'</code>');\n";
- Line 303: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  if(exc) html+=row('Exclude tables','<code>'+escHtml(exc)+'</code>');\n";
- Line 305: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  html+='</table>';\n";
- Line 311: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  return String(s).replace(/&/g,'&amp;').replace(/</g,'<').replace(/>/g,'>');\n";
- Line 311: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  return String(s).replace(/&/g,'&amp;').replace(/</g,'<').replace(/>/g,'>');\n";
- Line 311: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  return String(s).replace(/&/g,'&amp;').replace(/</g,'<').replace(/>/g,'>');\n";
- Line 311: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  return String(s).replace(/&/g,'&amp;').replace(/</g,'<').replace(/>/g,'>');\n";
- Line 337: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  var url=(currentSource==='postgresql')?'/api/v1/import/postgresql':'/api/v1/import/s3';\n
- Line 337: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  var url=(currentSource==='postgresql')?'/api/v1/import/postgresql':'/api/v1/import/s3';\n
- Line 340: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  fetch(url,{method:'POST',headers:{'Content-Type':'application/json'},body:body})\n";
- Line 354: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  fetch('/api/v1/import/'+currentJobId+'/status')\n";
- Line 354: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  fetch('/api/v1/import/'+currentJobId+'/status')\n";
- Line 375: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  var pct=(tot>0)?Math.min(100,Math.round(cur/tot*100)):0;\n";
- Line 392: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    box.innerHTML+='<strong>Import complete</strong><br>';\n";
- Line 393: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    box.innerHTML+='Imported: <b>'+s.imported_records+'</b> &nbsp; ';\n";
- Line 394: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    box.innerHTML+='Skipped: <b>'+s.skipped_records+'</b> &nbsp; ';\n";
- Line 395: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    box.innerHTML+='Failed: <b>'+s.failed_records+'</b> &nbsp; ';\n";
- Line 396: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    box.innerHTML+='Time: <b>'+(s.elapsed_seconds||0).toFixed(2)+'s</b>';\n";
- Line 397: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    box.innerHTML+='</div>';\n";
- Line 399: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    box.innerHTML='<div class=\"alert alert-error\">Import '+data.status+'.</div>';\n";
- Line 399: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    box.innerHTML='<div class=\"alert alert-error\">Import '+data.status+'.</div>';\n";
- Line 407: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  fetch('/api/v1/import/'+currentJobId+'/cancel',{method:'POST'})\n";
- Line 407: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  fetch('/api/v1/import/'+currentJobId+'/cancel',{method:'POST'})\n";
- Line 407: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  fetch('/api/v1/import/'+currentJobId+'/cancel',{method:'POST'})\n";
- Line 428: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  fetch('/api/v1/import/jobs')\n";
- Line 432: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      if(!jobs.length){el.innerHTML='<em style=\"color:#555\">No jobs yet.</em>';return;}\n
- Line 432: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      if(!jobs.length){el.innerHTML='<em style=\"color:#555\">No jobs yet.</em>';return;}\n
- Line 437: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        html+='<span class=\"job-id\" title=\"'+escHtml(j.id)+'\">'+escHtml(j.id)+'</span>'
- Line 437: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        html+='<span class=\"job-id\" title=\"'+escHtml(j.id)+'\">'+escHtml(j.id)+'</span>'
- Line 437: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        html+='<span class=\"job-id\" title=\"'+escHtml(j.id)+'\">'+escHtml(j.id)+'</span>'
- Line 437: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        html+='<span class=\"job-id\" title=\"'+escHtml(j.id)+'\">'+escHtml(j.id)+'</span>'
- Line 438: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        html+='<span class=\"status-badge s-'+(j.status||'pending')+'\">'+escHtml(j.status)
- Line 438: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        html+='<span class=\"status-badge s-'+(j.status||'pending')+'\">'+escHtml(j.status)
- Line 438: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        html+='<span class=\"status-badge s-'+(j.status||'pending')+'\">'+escHtml(j.status)
- Line 440: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "          html+='<span class=\"job-stats\">'+stats.imported_records+' imported, '+stats.fai
- Line 440: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "          html+='<span class=\"job-stats\">'+stats.imported_records+' imported, '+stats.fai
- Line 442: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "          html+='<span class=\"job-stage\">'+escHtml(j.stage||'')+'</span>';\n";
- Line 442: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "          html+='<span class=\"job-stage\">'+escHtml(j.stage||'')+'</span>';\n";
- Line 444: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        html+='</div>';\n";
- Line 447: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    }).catch(function(e){document.getElementById('jobs-list').innerHTML='<em style=\"color:
- Line 447: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    }).catch(function(e){document.getElementById('jobs-list').innerHTML='<em style=\"color:
- Line 452: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</script>\n";
- Line 453: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</body>\n</html>\n";

### src/server/query_api_handler.cpp
Total findings: 208

- Line 712: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: std::function<std::string(const std::shared_ptr<Expression>&, std::string&)> fieldFromFA = [&](const
- Line 749: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (!colL.empty() && rvL == var1 && bin->right->getType() == ASTNodeType::Literal) { auto lit = std:
- Line 750: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (!colL.empty() && rvL == var2 && bin->right->getType() == ASTNodeType::Literal) { auto lit = std:
- Line 751: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (bin->left->getType() == ASTNodeType::Literal) { std::string rv; std::string col = fieldFromFA(bi
- Line 792: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if ((*parse_result) && (*parse_result)->limit) { auto off = static_cast<size_t>(std::max<int64_t>(0,
- Line 817: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: resolveToLoopField = [&](const std::shared_ptr<Expression>& e)->std::optional<std::string> {
- Line 861: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lit = std::static_pointer_cast<LiteralExpr>(be->right);
- Line 866: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lit = std::static_pointer_cast<LiteralExpr>(be->left);
- Line 983: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: evalExprToLiteral = [&](std::shared_ptr<themis::query::Expression> expr, nlohmann::json& out)->bool
- Line 1118: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto parseSide = [&](std::shared_ptr<Expression> e, char& var, std::string& field) -> bool {
- Line 1129: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto parseSimpleFromExpr = [&](std::shared_ptr<Expression> expr, SimplePred& out)->bool {
- Line 1179: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto parseSide = [&](std::shared_ptr<Expression> e, char& var, std::string& field) -> bool {
- Line 1348: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator itp may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto itp = parent.find(cur);
- Line 1561: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: std::function<std::string(const std::shared_ptr<Expression>&, std::string&)> fieldFromFA = [&](const
- Line 1621: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lit = std::static_pointer_cast<LiteralExpr>(bin->right);
- Line 1634: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lit = std::static_pointer_cast<LiteralExpr>(bin->right);
- Line 1649: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lit = std::static_pointer_cast<LiteralExpr>(bin->left);
- Line 1752: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto off = static_cast<size_t>(std::max<int64_t>(0, (*parse_result)->limit->offset));
- Line 1753: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cnt = static_cast<size_t>(std::max<int64_t>(0, (*parse_result)->limit->count));
- Line 1945: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = parent.find(node);
- Line 2073: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = parent.find(cur);
- Line 2233: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto bin = std::static_pointer_cast<BinaryOpExpr>(filter->condition);
- Line 2238: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto fa = std::static_pointer_cast<FieldAccessExpr>(bin->left);
- Line 2239: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lit = std::static_pointer_cast<LiteralExpr>(bin->right);
- Line 2340: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: evalExpr = [&](const std::shared_ptr<themis::query::Expression>& e) -> nlohmann::json {
- Line 2430: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: exprContainsFn = [&](const std::shared_ptr<themis::query::Expression>& expr, const std::string& name
- Line 2495: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: requested_count_for_cursor = static_cast<size_t>(std::max<int64_t>(1, (*parse_result)->limit->count)
- Line 2694: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto off = static_cast<size_t>(std::max<int64_t>(0, (*parse_result)->limit->offset));
- Line 2695: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cnt = static_cast<size_t>(std::max<int64_t>(0, (*parse_result)->limit->count));
- Line 2727: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: cursor_meta["anchor_set"] = q.orderBy->cursor_pk.has_value();
- Line 2745: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto extractColumn = [&](const std::shared_ptr<themis::query::Expression>& expr)->std::string {
- Line 2879: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: extractColFromFA = [&](const std::shared_ptr<Expression>& expr, bool& rootedAtLoop)->std::optional<s
- Line 2892: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: containsFunction = [&](const std::shared_ptr<Expression>& expr, const std::string& name)->bool{
- Line 3210: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: requested_count = static_cast<size_t>(std::max<int64_t>(1, (*parse_result)->limit->count));
- Line 0: severity=HIGH; category=uncategorized
  Context: Variable initialized conditionally
  Confidence: band=high; score=0.81
- Line 137: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: auto masking_policy = std::atomic_load_explicit(&masking_policy_, std::memory_order_acquire);
- Line 259: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: auto* stats_collector = stats_collector_.load(std::memory_order_acquire);
- Line 762: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: auto* stats_collector = stats_collector_.load(std::memory_order_acquire);
- Line 787: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::string retVar; if ((*parse_result)->return_node && (*parse_result)->return_node->expression) {
- Line 829: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = letMap.find(v->name);
- Line 901: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: auto* index_recommender = index_recommender_.load(std::memory_order_acquire);
- Line 964: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if ((*parse_result)->return_node && (*parse_result)->return_node->expression) {
- Line 966: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto* var = dynamic_cast<VariableExpr*>((*parse_result)->return_node->expression.get());
- Line 983: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: evalExprToLiteral = [&](std::shared_ptr<themis::query::Expression> expr, nlohmann::json& out)->bool
- Line 1049: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: char buf[32]; std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", tm.tm_year+1900, tm.tm_mon+1, tm.tm_
- Line 1118: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto parseSide = [&](std::shared_ptr<Expression> e, char& var, std::string& field) -> bool {
- Line 1254: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: case SimplePred::Op::Eq:  return aval == lit;
  Confidence: band=very_high; score=0.9
- Line 1255: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: case SimplePred::Op::Neq: return aval != lit;
  Confidence: band=very_high; score=0.9
- Line 1265: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: case SimplePred::Op::Eq:  return av == lit;
  Confidence: band=very_high; score=0.9
- Line 1266: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: case SimplePred::Op::Neq: return av != lit;
  Confidence: band=very_high; score=0.9
- Line 1490: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (objVar->name == "v" || objVar->name == "e") return true;
- Line 1672: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: auto* stats_collector = stats_collector_.load(std::memory_order_acquire);
- Line 1717: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if ((*parse_result)->return_node && (*parse_result)->return_node->expression) {
- Line 1718: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (auto* v = dynamic_cast<VariableExpr*>((*parse_result)->return_node->expression.get())) {
- Line 2033: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (!blob && pk.find(':') == std::string::npos) {
- Line 2072: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = parent.find(cur);
- Line 2089: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (!blob && pk.find(':') == std::string::npos) {
- Line 2161: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: auto* stats_collector = stats_collector_.load(std::memory_order_acquire);
- Line 2200: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Provide "result" alias for compatibility with older clients/tests
  Confidence: band=high; score=0.8
- Line 2292: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: auto* stats_collector = stats_collector_.load(std::memory_order_acquire);
- Line 2340: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: evalExpr = [&](const std::shared_ptr<themis::query::Expression>& e) -> nlohmann::json {
- Line 2365: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (const auto& [k, ce] : obj->fields) out[k] = evalExpr(ce);
- Line 2382: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto projected = evalExpr(jq.return_node->expression);
- Line 2557: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: auto* stats_collector = stats_collector_.load(std::memory_order_acquire);
- Line 2839: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: const auto it = mp.find(a.var);
  Confidence: band=very_high; score=0.9
- Line 2928: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if ((*parse_result)->return_node && (*parse_result)->return_node->expression) {
- Line 2929: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: usesFulltextScore = containsFunction((*parse_result)->return_node->expression, "fulltext_score");
- Line 2942: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if ((*parse_result)->return_node && (*parse_result)->return_node->expression) {
- Line 2943: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: usesScoreFn = containsFunction((*parse_result)->return_node->expression, "bm25");
- Line 3170: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if ((*parse_result)->return_node && (*parse_result)->return_node->expression) {
- Line 3171: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (auto* v = dynamic_cast<VariableExpr*>((*parse_result)->return_node->expression.get())) {
- Line 3189: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if ((*parse_result)->return_node && (*parse_result)->return_node->expression) {
- Line 3190: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto out = evalExpr((*parse_result)->return_node->expression, e, env);
- Line 3269: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Provide "result" alias for compatibility
  Confidence: band=high; score=0.8
- Line 210: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: preds.push_back({p["column"].get<std::string>(), p["value"].get<std::string>()});
  Confidence: band=high; score=0.74
- Line 211: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: preds.push_back({p["column"].get<std::string>(), p["value"].get<std::string>()});
- Line 230: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rpreds.push_back(std::move(pr));
  Confidence: band=high; score=0.74
- Line 267: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::pair<themis::QueryEngine::Status, size_t> res;
- Line 290: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
  Confidence: band=high; score=0.74
- Line 291: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
- Line 294: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan_json["estimates"].push_back({
  Confidence: band=high; score=0.74
- Line 295: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan_json["estimates"].push_back({
- Line 314: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
  Confidence: band=high; score=0.74
- Line 315: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
- Line 333: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::pair<themis::QueryEngine::Status, std::vector<std::string>> res;
- Line 356: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
  Confidence: band=high; score=0.74
- Line 360: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan_json["estimates"].push_back({
  Confidence: band=high; score=0.74
- Line 403: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: key_items.push_back(k);
  Confidence: band=high; score=0.74
- Line 411: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::pair<themis::QueryEngine::Status, std::vector<themis::BaseEntity>> res;
- Line 434: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
  Confidence: band=high; score=0.74
- Line 438: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan_json["estimates"].push_back({
  Confidence: band=high; score=0.74
- Line 458: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
  Confidence: band=high; score=0.74
- Line 480: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(e.toJson());
  Confidence: band=high; score=0.74
- Line 481: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entities.push_back(e.toJson());
- Line 491: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 496: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto coll = schema["collections"][table];
  Confidence: band=high; score=0.74
- Line 499: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto& f : coll["encryption"]["fields"]) if (f.is_string()) fields.push_back(f.get<std::string>(
- Line 511: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: try { obj = nlohmann::json::parse(e.toJson()); } catch (...) { entities.push_back(e.toJson()); continue; }
  Confidence: band=high; score=0.74
- Line 511: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: try { obj = nlohmann::json::parse(e.toJson()); } catch (...) { entities.push_back(e.toJson()); continue; }
  Confidence: band=high; score=0.74
- Line 512: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: try { obj = nlohmann::json::parse(e.toJson()); } catch (...) { entities.push_back(e.toJson()); conti
- Line 512: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { obj = nlohmann::json::parse(e.toJson()); } catch (...) { entities.push_back(e.toJson()); conti
- Line 519: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: bool encFlag = false; try { encFlag = obj[f + "_enc"].get<bool>(); } catch (...) { encFlag = false;
- Line 522: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto enc_meta_str = obj[f + "_encrypted"].get<std::string>();
  Confidence: band=high; score=0.74
- Line 527: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: std::string group_name; try { group_name = obj[f + "_group"].get<std::string>(); } catch (...) { gro
- Line 552: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 715: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: std::vector<std::string> parts; parts.push_back(fa->field);
- Line 717: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: while (auto* fa2 = dynamic_cast<FieldAccessExpr*>(cur)) { parts.push_back(fa2->field); cur = fa2->ob
- Line 766: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::pair<themis::QueryEngine::Status, std::vector<themis::BaseEntity>> res1;
- Line 775: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::pair<themis::QueryEngine::Status, std::vector<themis::BaseEntity>> res2;
- Line 811: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::shared_ptr<themis::query::Expression>> letMap;
  Confidence: band=medium; score=0.66
- Line 820: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: std::vector<std::string> parts; parts.push_back(fa->field);
  Confidence: band=high; score=0.74
- Line 821: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: std::vector<std::string> parts; parts.push_back(fa->field);
- Line 823: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: while (auto* fa2 = dynamic_cast<FieldAccessExpr*>(cur)) { parts.push_back(fa2->field); cur = fa2->ob
- Line 825: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: std::string col; for (auto it = parts.rbegin(); it != parts.rend(); ++it) { if (!col.empty()) col += "."; col += *it; }
  Confidence: band=high; score=0.74
- Line 976: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: char var = '\0'; // 'v' or 'e'
- Line 1305: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { return std::nullopt; }
- Line 1314: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { return std::nullopt; }
- Line 1350: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pathEdges.push_back(itp->second.edgeId);
- Line 1351: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pathNodes.push_back(itp->second.parent);
- Line 1566: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parts.push_back(fa->field);
- Line 1569: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parts.push_back(fa2->field);
- Line 1676: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::pair<themis::QueryEngine::Status, std::vector<themis::BaseEntity>> res1;
- Line 1689: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::pair<themis::QueryEngine::Status, std::vector<themis::BaseEntity>> res2;
- Line 1735: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (retVar == var1) out.push_back(l); else out.push_back(e);
  Confidence: band=high; score=0.74
- Line 1735: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (retVar == var1) out.push_back(l); else out.push_back(e);
  Confidence: band=high; score=0.74
- Line 1803: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { return false; }
- Line 1821: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { return false; }
- Line 1846: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { return false; }
- Line 1862: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { return false; }
- Line 1942: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resultVertices.push_back(node);
  Confidence: band=high; score=0.74
- Line 2039: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: res["entities"].push_back(entity.toJson());
  Confidence: band=high; score=0.74
- Line 2040: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: res["entities"].push_back(entity.toJson());
- Line 2041: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2042: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: res["entities"].push_back(nlohmann::json({{"_key", pk}}));
- Line 2045: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: res["entities"].push_back(nlohmann::json({{"_key", pk}}));
- Line 2054: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: res["entities"].push_back(edgeEnt.toJson());
  Confidence: band=high; score=0.74
- Line 2055: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: res["entities"].push_back(edgeEnt.toJson());
- Line 2056: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2057: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: res["entities"].push_back(nlohmann::json({{"_edge", eid}}));
- Line 2060: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: res["entities"].push_back(nlohmann::json({{"_edge", eid}}));
- Line 2070: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vertices.push_back(cur);
  Confidence: band=high; score=0.74
- Line 2075: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edges.push_back(it->second.edgeId);
- Line 2095: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: jpath["vertices"].push_back(ent.toJson());
  Confidence: band=high; score=0.74
- Line 2096: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: jpath["vertices"].push_back(ent.toJson());
- Line 2097: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2098: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: jpath["vertices"].push_back(nlohmann::json({{"_key", pk}}));
- Line 2101: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: jpath["vertices"].push_back(nlohmann::json({{"_key", pk}}));
- Line 2110: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: jpath["edges"].push_back(eent.toJson());
  Confidence: band=high; score=0.74
- Line 2111: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: jpath["edges"].push_back(eent.toJson());
- Line 2112: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2113: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: jpath["edges"].push_back(nlohmann::json({{"_edge", eid}}));
- Line 2116: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: jpath["edges"].push_back(nlohmann::json({{"_edge", eid}}));
- Line 2165: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::pair<themis::QueryEngine::Status, std::vector<std::string>> statusKeys;
- Line 2187: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(nlohmann::json::parse(entity.toJson()));
  Confidence: band=high; score=0.74
- Line 2189: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2201: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { response_body["result"] = response_body["entities"]; } catch (...) { /* ignore */ }
- Line 2242: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: parts.push_back(fa->field);
  Confidence: band=high; score=0.74
- Line 2243: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parts.push_back(fa->field);
- Line 2246: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parts.push_back(fa2->field);
- Line 2254: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!col.empty()) col += ".";
  Confidence: band=high; score=0.74
- Line 2254: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!col.empty()) col += ".";
  Confidence: band=high; score=0.74
- Line 2255: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!col.empty()) col += ".";
- Line 2255: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!col.empty()) col += ".";
- Line 2271: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cq.predicates.push_back({col, litToString(lit->value)});
  Confidence: band=high; score=0.74
- Line 2272: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cq.predicates.push_back({col, litToString(lit->value)});
- Line 2311: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(result);
  Confidence: band=high; score=0.74
- Line 2338: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, nlohmann::json> letValues;
  Confidence: band=high; score=0.74
- Line 2370: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& ce : arr->elements) a.push_back(evalExpr(ce));
  Confidence: band=high; score=0.74
- Line 2371: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& ce : arr->elements) a.push_back(evalExpr(ce));
- Line 2382: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(projected);
  Confidence: band=high; score=0.74
- Line 2382: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(projected);
  Confidence: band=high; score=0.74
- Line 2384: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2545: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2563: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::pair<themis::QueryEngine::Status, std::vector<themis::BaseEntity>> res;
- Line 2612: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
  Confidence: band=high; score=0.74
- Line 2616: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan_json["estimates"].push_back({
  Confidence: band=high; score=0.74
- Line 2752: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parts.push_back(fa2->field);
- Line 2757: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!col.empty()) col += ".";
  Confidence: band=high; score=0.74
- Line 2758: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!col.empty()) col += ".";
- Line 2781: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: aggs.push_back({a.varName, func, col});
  Confidence: band=high; score=0.74
- Line 2786: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::unordered_map<std::string, AggState>> acc;
  Confidence: band=medium; score=0.66
- Line 2799: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { out = std::stod(*sv); return true; } catch (...) { /* ignore */ }
- Line 2848: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: groups.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 2848: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: groups.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 2881: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: std::vector<std::string> parts; parts.push_back(fa->field);
- Line 2883: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: while (auto* fa2 = dynamic_cast<FieldAccessExpr*>(cur)) { parts.push_back(fa2->field); cur = fa2->ob
- Line 3087: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: else if (a.is_boolean()) out += (a.get<bool>()?"true":"false");
  Confidence: band=high; score=0.74
- Line 3160: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& el : ar->elements) arr.push_back(evalExpr(el, ent, env));
  Confidence: band=high; score=0.74
- Line 3160: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& el : ar->elements) arr.push_back(evalExpr(el, ent, env));
  Confidence: band=high; score=0.74
- Line 3161: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& el : ar->elements) arr.push_back(evalExpr(el, ent, env));
- Line 3177: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& e : sliced) entities.push_back(e.toJson());
  Confidence: band=high; score=0.74
- Line 3178: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& e : sliced) entities.push_back(e.toJson());
- Line 3182: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, nlohmann::json> env;
  Confidence: band=medium; score=0.66
- Line 3190: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(std::move(out));
  Confidence: band=high; score=0.74
- Line 3190: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(std::move(out));
  Confidence: band=high; score=0.74
- Line 3190: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(std::move(out));
  Confidence: band=high; score=0.74
- Line 3224: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& e : sliced) page_items.push_back(e.toJson());
- Line 3250: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 3270: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { response_body["result"] = response_body["entities"]; } catch (...) { /* ignore */ }
- Line 3279: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { plan_json["let_pre_extracted"] = true; } catch (...) { /* noop */ }
- Line 3436: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: enhanced_response["llm_context"].push_back(llm_entry);
  Confidence: band=high; score=0.74
- Line 3474: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 3510: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 3567: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: decoded += ' ';
  Confidence: band=high; score=0.74
- Line 3568: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: decoded += ' ';
- Line 3592: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { return def; }
- Line 3612: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_fwd = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 3626: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/task_scheduler_api_handler.cpp
Total findings: 190

- Line 602: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Use std::unique_ptr<T> or std::make_unique<T>()
  Context: html += "  return new Date(iso).toLocaleString();\n";
- Line 602: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: html += "  return new Date(iso).toLocaleString();\n";
- Line 646: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Use std::unique_ptr<T> or std::make_unique<T>()
  Context: html += "  document.getElementById('refresh-indicator').textContent = 'Updated ' + new Date().toLoca
- Line 646: severity=CRITICAL; category=smart_ptr_misuse
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
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3365 feat(scheduler): expose mul... (2026-03-12) | #3362 feat(scheduler): ex
- Line 145: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(taskToJson(t));
  Confidence: band=high; score=0.74
- Line 146: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: items.push_back(taskToJson(t));
- Line 313: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(r.toJson());
  Confidence: band=high; score=0.74
- Line 314: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: items.push_back(r.toJson());
- Line 371: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { return def; }
- Line 403: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: else if (v.is_string()) { try { ms = std::stoll(v.get<std::string>()); } catch (...) {} }
- Line 410: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: else if (v.is_string()) { try { ms = std::stoll(v.get<std::string>()); } catch (...) {} }
- Line 414: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: params.sort_by = scheduler::AuditQueryParams::SortBy::TIMESTAMP_DESC;
  Confidence: band=high; score=0.74
- Line 437: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: html += "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n";
  Confidence: band=high; score=0.74
- Line 441: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<title>ThemisDB – Task Scheduler</title>\n";
- Line 486: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</style>\n</head>\n<body>\n";
- Line 489: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <h1>&#x23F2; Task Scheduler</h1>\n";
- Line 490: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <span class=\"badge\">ThemisDB</span>\n";
- Line 490: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <span class=\"badge\">ThemisDB</span>\n";
- Line 491: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</header>\n";
- Line 497: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-registered\">–</div><div class=\"lbl\"
- Line 497: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-registered\">–</div><div class=\"lbl\"
- Line 497: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-registered\">–</div><div class=\"lbl\"
- Line 497: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-registered\">–</div><div class=\"lbl\"
- Line 497: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-registered\">–</div><div class=\"lbl\"
- Line 498: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-active\">–</div><div class=\"lbl\">Act
- Line 498: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-active\">–</div><div class=\"lbl\">Act
- Line 498: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-active\">–</div><div class=\"lbl\">Act
- Line 498: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-active\">–</div><div class=\"lbl\">Act
- Line 498: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-active\">–</div><div class=\"lbl\">Act
- Line 499: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-running\">–</div><div class=\"lbl\">Ru
- Line 499: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-running\">–</div><div class=\"lbl\">Ru
- Line 499: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-running\">–</div><div class=\"lbl\">Ru
- Line 499: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-running\">–</div><div class=\"lbl\">Ru
- Line 499: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-running\">–</div><div class=\"lbl\">Ru
- Line 500: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-total\">–</div><div class=\"lbl\">Exec
- Line 500: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-total\">–</div><div class=\"lbl\">Exec
- Line 500: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-total\">–</div><div class=\"lbl\">Exec
- Line 500: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-total\">–</div><div class=\"lbl\">Exec
- Line 500: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-total\">–</div><div class=\"lbl\">Exec
- Line 501: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-failed\">–</div><div class=\"lbl\">Fai
- Line 501: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-failed\">–</div><div class=\"lbl\">Fai
- Line 501: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-failed\">–</div><div class=\"lbl\">Fai
- Line 501: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-failed\">–</div><div class=\"lbl\">Fai
- Line 501: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-failed\">–</div><div class=\"lbl\">Fai
- Line 502: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-status\">–</div><div class=\"lbl\">Sch
- Line 502: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-status\">–</div><div class=\"lbl\">Sch
- Line 502: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-status\">–</div><div class=\"lbl\">Sch
- Line 502: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-status\">–</div><div class=\"lbl\">Sch
- Line 502: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-status\">–</div><div class=\"lbl\">Sch
- Line 503: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";
- Line 507: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn-primary\" onclick=\"openCreateDialog()\">&#43; New Task</button>\n";
- Line 507: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn-primary\" onclick=\"openCreateDialog()\">&#43; New Task</button>\n";
- Line 507: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn-primary\" onclick=\"openCreateDialog()\">&#43; New Task</button>\n";
- Line 508: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn-secondary\" onclick=\"loadAll()\">&#8635; Refresh</button>\n";
- Line 508: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn-secondary\" onclick=\"loadAll()\">&#8635; Refresh</button>\n";
- Line 508: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <button class=\"btn-secondary\" onclick=\"loadAll()\">&#8635; Refresh</button>\n";
- Line 509: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <span id=\"refresh-indicator\"></span>\n";
- Line 509: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <span id=\"refresh-indicator\"></span>\n";
- Line 510: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n";
- Line 514: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <th>ID / Name</th><th>Type</th><th>Trigger</th><th>Status</th>\n";
- Line 515: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <th>Executions</th><th>Last Error</th><th>Next Run</th><th>Actions</th>\n";
- Line 516: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</tr></thead>\n";
- Line 517: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<tbody id=\"task-table-body\"><tr><td colspan=\"8\" style=\"text-align:center;color:#64748b
- Line 517: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<tbody id=\"task-table-body\"><tr><td colspan=\"8\" style=\"text-align:center;color:#64748b
- Line 517: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<tbody id=\"task-table-body\"><tr><td colspan=\"8\" style=\"text-align:center;color:#64748b
- Line 517: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<tbody id=\"task-table-body\"><tr><td colspan=\"8\" style=\"text-align:center;color:#64748b
- Line 518: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</table>\n";
- Line 520: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</div>\n"; // end .container
- Line 523: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div class=\"toast\" id=\"toast\"></div>\n";
- Line 523: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div class=\"toast\" id=\"toast\"></div>\n";
- Line 523: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<div class=\"toast\" id=\"toast\"></div>\n";
- Line 527: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <h2 id=\"dialog-title\">New Task</h2>\n";
- Line 527: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <h2 id=\"dialog-title\">New Task</h2>\n";
- Line 529: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label>Name</label>\n";
- Line 531: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label>Description</label>\n";
- Line 533: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label>Type</label>\n";
- Line 535: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <option value=\"aql_query\">AQL Query</option>\n";
- Line 535: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <option value=\"aql_query\">AQL Query</option>\n";
- Line 536: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <option value=\"function\">Function</option>\n";
- Line 536: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <option value=\"function\">Function</option>\n";
- Line 537: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  </select>\n";
- Line 539: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <label>AQL Query</label>\n";
- Line 540: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <textarea id=\"f-aql\" placeholder=\"FOR d IN collection RETURN d\"></textarea>\n";
- Line 540: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <textarea id=\"f-aql\" placeholder=\"FOR d IN collection RETURN d\"></textarea>\n";
- Line 540: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <textarea id=\"f-aql\" placeholder=\"FOR d IN collection RETURN d\"></textarea>\n";
- Line 541: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  </div>\n";
- Line 543: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <label>Function Name</label>\n";
- Line 545: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  </div>\n";
- Line 546: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label>Trigger</label>\n";
- Line 548: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <option value=\"interval\">Interval</option>\n";
- Line 548: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <option value=\"interval\">Interval</option>\n";
- Line 549: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <option value=\"cron\">Cron</option>\n";
- Line 549: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <option value=\"cron\">Cron</option>\n";
- Line 550: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <option value=\"manual\">Manual</option>\n";
- Line 550: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <option value=\"manual\">Manual</option>\n";
- Line 551: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  </select>\n";
- Line 553: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <label>Interval (seconds)</label>\n";
- Line 555: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  </div>\n";
- Line 557: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <label>Cron Expression</label>\n";
- Line 558: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <input id=\"f-cron\" type=\"text\" placeholder=\"*/5 * * * *\">\n";
- Line 558: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <input id=\"f-cron\" type=\"text\" placeholder=\"*/5 * * * *\">\n";
- Line 558: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <input id=\"f-cron\" type=\"text\" placeholder=\"*/5 * * * *\">\n";
- Line 558: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <input id=\"f-cron\" type=\"text\" placeholder=\"*/5 * * * *\">\n";
- Line 559: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  </div>\n";
- Line 560: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label>Timeout (seconds)</label>\n";
- Line 562: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  <label>Max Retries</label>\n";
- Line 565: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <button class=\"btn-secondary\" onclick=\"closeDialog()\">Cancel</button>\n";
- Line 565: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <button class=\"btn-secondary\" onclick=\"closeDialog()\">Cancel</button>\n";
- Line 565: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <button class=\"btn-secondary\" onclick=\"closeDialog()\">Cancel</button>\n";
- Line 566: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <button class=\"btn-primary\" onclick=\"saveTask()\">Save</button>\n";
- Line 566: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <button class=\"btn-primary\" onclick=\"saveTask()\">Save</button>\n";
- Line 566: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    <button class=\"btn-primary\" onclick=\"saveTask()\">Save</button>\n";
- Line 567: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  </div>\n";
- Line 568: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</dialog>\n";
- Line 572: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "const API = '/api/tasks';\n";
- Line 575: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  const opts = { method, headers: {'Content-Type':'application/json'} };\n";
- Line 589: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  const s = await api('GET', API + '/stats');\n";
- Line 609: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    tbody.innerHTML = '<tr><td colspan=\"8\" style=\"text-align:center;color:#64748b\">No t
- Line 609: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    tbody.innerHTML = '<tr><td colspan=\"8\" style=\"text-align:center;color:#64748b\">No t
- Line 609: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    tbody.innerHTML = '<tr><td colspan=\"8\" style=\"text-align:center;color:#64748b\">No t
- Line 616: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      ? '<span class=\"tag tag-aql\">AQL</span>'\n";
- Line 616: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      ? '<span class=\"tag tag-aql\">AQL</span>'\n";
- Line 617: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      : '<span class=\"tag tag-fn\">Fn</span>';\n";
- Line 617: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      : '<span class=\"tag tag-fn\">Fn</span>';\n";
- Line 619: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      ? `<button class=\"btn-warning\" onclick=\"disableTask('${t.id}')\">Pause</button>`\n
- Line 619: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      ? `<button class=\"btn-warning\" onclick=\"disableTask('${t.id}')\">Pause</button>`\n
- Line 619: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      ? `<button class=\"btn-warning\" onclick=\"disableTask('${t.id}')\">Pause</button>`\n
- Line 620: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      : `<button class=\"btn-success\" onclick=\"enableTask('${t.id}')\">Resume</button>`;\
- Line 620: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      : `<button class=\"btn-success\" onclick=\"enableTask('${t.id}')\">Resume</button>`;\
- Line 620: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      : `<button class=\"btn-success\" onclick=\"enableTask('${t.id}')\">Resume</button>`;\
- Line 622: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      <td><b>${escHtml(t.id)}</b><br><small style=\"color:#94a3b8\">${escHtml(t.name)}</sma
- Line 622: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      <td><b>${escHtml(t.id)}</b><br><small style=\"color:#94a3b8\">${escHtml(t.name)}</sma
- Line 623: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      <td>${typeTag}</td>\n";
- Line 624: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      <td>${escHtml(t.trigger_type || '–')}</td>\n";
- Line 625: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      <td class=\"${statusClass}\">${statusText}</td>\n";
- Line 625: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      <td class=\"${statusClass}\">${statusText}</td>\n";
- Line 626: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      <td>${t.successful_executions ?? 0} / ${t.total_executions ?? 0}</td>\n";
- Line 627: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      <td style=\"color:#f87171;font-size:.8rem\">${escHtml(t.last_error || '')}</td>\n";
- Line 627: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      <td style=\"color:#f87171;font-size:.8rem\">${escHtml(t.last_error || '')}</td>\n";
- Line 628: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      <td style=\"font-size:.8rem\">${formatDate(t.next_run)}</td>\n";
- Line 628: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      <td style=\"font-size:.8rem\">${formatDate(t.next_run)}</td>\n";
- Line 630: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        <button class=\"btn-primary\" onclick=\"runTask('${t.id}')\">&#9654; Run</button>\n
- Line 630: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        <button class=\"btn-primary\" onclick=\"runTask('${t.id}')\">&#9654; Run</button>\n
- Line 630: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        <button class=\"btn-primary\" onclick=\"runTask('${t.id}')\">&#9654; Run</button>\n
- Line 632: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        <button class=\"btn-secondary\" onclick=\"editTask('${t.id}')\">&#9998; Edit</butto
- Line 632: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        <button class=\"btn-secondary\" onclick=\"editTask('${t.id}')\">&#9998; Edit</butto
- Line 632: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        <button class=\"btn-secondary\" onclick=\"editTask('${t.id}')\">&#9998; Edit</butto
- Line 633: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        <button class=\"btn-danger\" onclick=\"deleteTask('${t.id}')\">&#x1F5D1;</button>\n
- Line 633: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        <button class=\"btn-danger\" onclick=\"deleteTask('${t.id}')\">&#x1F5D1;</button>\n
- Line 633: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "        <button class=\"btn-danger\" onclick=\"deleteTask('${t.id}')\">&#x1F5D1;</button>\n
- Line 634: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "      </td>\n";
- Line 635: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    </tr>`;\n";
- Line 640: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  return String(s).replace(/&/g,'&amp;').replace(/</g,'<').replace(/>/g,'>').replace(/\"/g,
- Line 640: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  return String(s).replace(/&/g,'&amp;').replace(/</g,'<').replace(/>/g,'>').replace(/\"/g,
- Line 640: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  return String(s).replace(/&/g,'&amp;').replace(/</g,'<').replace(/>/g,'>').replace(/\"/g,
- Line 640: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  return String(s).replace(/&/g,'&amp;').replace(/</g,'<').replace(/>/g,'>').replace(/\"/g,
- Line 640: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  return String(s).replace(/&/g,'&amp;').replace(/</g,'<').replace(/>/g,'>').replace(/\"/g,
- Line 650: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  const r = await api('POST', API + '/' + id + '/execute');\n";
- Line 650: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  const r = await api('POST', API + '/' + id + '/execute');\n";
- Line 656: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  const r = await api('POST', API + '/' + id + '/enable');\n";
- Line 656: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  const r = await api('POST', API + '/' + id + '/enable');\n";
- Line 662: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  const r = await api('POST', API + '/' + id + '/disable');\n";
- Line 662: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  const r = await api('POST', API + '/' + id + '/disable');\n";
- Line 669: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  const r = await api('DELETE', API + '/' + id);\n";
- Line 692: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  const t = await api('GET', API + '/' + id);\n";
- Line 702: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  document.getElementById('f-interval').value = Math.round((t.interval_ms || 300000) / 1000
- Line 704: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "  document.getElementById('f-timeout').value = Math.round((t.timeout_ms || 600000) / 1000);
- Line 711: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: html += "  document.getElementById('task-dialog').close();\n";
- Line 744: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "    ? await api('PUT', API + '/' + id, body)\n";
- Line 755: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "// Auto-refresh every 30 seconds\n";
- Line 758: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</script>\n";
- Line 759: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</body>\n</html>\n";
- Line 861: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ms = j["interval_ms"].get<int64_t>();
  Confidence: band=high; score=0.74
- Line 867: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto secs = j["interval_seconds"].get<int64_t>();
  Confidence: band=high; score=0.74
- Line 873: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ms = j["timeout_ms"].get<int64_t>();
  Confidence: band=high; score=0.74
- Line 879: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto secs = j["timeout_seconds"].get<int64_t>();
  Confidence: band=high; score=0.74
- Line 915: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto task_ids = request["task_ids"].get<std::vector<std::string>>();
  Confidence: band=high; score=0.74
- Line 981: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cfg.extra_labels.emplace_back(it.key(), it.value().get<std::string>());
  Confidence: band=high; score=0.74
- Line 981: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = req["extra_labels"].begin(); it != req["extra_labels"].end(); ++it) {
  Confidence: band=high; score=0.74
- Line 1009: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cfg.tags.push_back(tag.get<std::string>());
  Confidence: band=high; score=0.74
- Line 1081: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tasks.push_back(*task_ptr);
  Confidence: band=high; score=0.74
- Line 507: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: html += "  <button class=\"btn-primary\" onclick=\"openCreateDialog()\">&#43; New Task</button>\n";
  Confidence: band=medium; score=0.6
- Line 565: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: html += "    <button class=\"btn-secondary\" onclick=\"closeDialog()\">Cancel</button>\n";
  Confidence: band=medium; score=0.6
- Line 674: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: html += "function openCreateDialog() {\n";
  Confidence: band=medium; score=0.6
- Line 710: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: html += "function closeDialog() {\n";
  Confidence: band=medium; score=0.6
- Line 748: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: html += "    closeDialog();\n";
  Confidence: band=medium; score=0.6

### src/server/llm_api_handler.cpp
Total findings: 143

- Line 192: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: return handleLoadModel(req);
  Confidence: band=very_high; score=0.99
- Line 194: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: return handleUnloadModel(req);
  Confidence: band=very_high; score=0.99
- Line 410: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: rag_mode = json_value_to<std::string>(body->at("rag_mode"));
- Line 578: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: llm_request.metadata["rag_tensor_slots"] = body->at("rag_tensor_slots");
- Line 581: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: llm_request.metadata["rag_tensor_slot_chars"] = body->at("rag_tensor_slot_chars");
- Line 925: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: http::response<http::string_body> LLMApiHandler::handleLoadModel(
  Confidence: band=very_high; score=0.99
- Line 927: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto span = Tracer::startSpan("handleLoadModel");
  Confidence: band=very_high; score=0.99
- Line 954: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: bool loaded = plugin_mgr.loadModel(model_id, path);
  Confidence: band=very_high; score=0.99
- Line 957: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: loaded = plugin_mgr.loadModel(model_id, path);
  Confidence: band=very_high; score=0.99
- Line 982: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: logCurrentException("LLMApiHandler::handleLoadModel");
  Confidence: band=very_high; score=0.99
- Line 987: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: http::response<http::string_body> LLMApiHandler::handleUnloadModel(
  Confidence: band=very_high; score=0.99
- Line 989: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto span = Tracer::startSpan("handleUnloadModel");
  Confidence: band=very_high; score=0.99
- Line 1011: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: plugin_mgr.unloadModel(model_id);
  Confidence: band=very_high; score=0.99
- Line 1027: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: logCurrentException("LLMApiHandler::handleUnloadModel");
  Confidence: band=very_high; score=0.99
- Line 2008: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto perm = policy_engine_->checkInferencePermission(header_map);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #5405 W1-S06: Close remaining unc... (2026-05-28) | #4187 feat(llm): OpenAI-c
- Line 17: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: #include "llm/async_inference_engine.h"
  Confidence: band=very_high; score=0.9
- Line 158: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return lora_handler_->handleRequest(req);
- Line 181: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (target == "/api/v1/llm/inference" && method == http::verb::post) {
  Confidence: band=very_high; score=0.9
- Line 182: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: return handleInference(req);
  Confidence: band=very_high; score=0.9
- Line 188: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: return handleStreamInference(req);
  Confidence: band=very_high; score=0.9
- Line 246: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: http::response<http::string_body> LLMApiHandler::handleInference(
  Confidence: band=very_high; score=0.9
- Line 248: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto span = Tracer::startSpan("handleInference");
  Confidence: band=very_high; score=0.9
- Line 290: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm::InferenceRequest llm_request;
  Confidence: band=very_high; score=0.9
- Line 292: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: llm_request.model_id = model_id.empty() ? std::string("default") : model_id;
- Line 294: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: llm_request.temperature = static_cast<float>(temperature);
- Line 300: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto llm_response = plugin_mgr.generate(llm_request);
  Confidence: band=very_high; score=0.9
- Line 307: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const auto tokens_generated = llm_response.tokens_generated;
  Confidence: band=very_high; score=0.9
- Line 308: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const auto inference_time_ms = llm_response.inference_time_ms;
  Confidence: band=very_high; score=0.9
- Line 309: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const double safe_inference_time_ms = inference_time_ms > 0.0 ? inference_time_ms : 1.0;
  Confidence: band=very_high; score=0.9
- Line 310: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const int safe_tokens_generated = tokens_generated > 0 ? tokens_generated : 1;
  Confidence: band=very_high; score=0.9
- Line 312: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: static_cast<double>(tokens_generated) * 1000.0 / safe_inference_time_ms;
  Confidence: band=very_high; score=0.9
- Line 313: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const double ms_per_token = static_cast<double>(inference_time_ms) / static_cast<double>(safe_tokens_generated);
  Confidence: band=very_high; score=0.9
- Line 323: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: {"inference_time_ms", inference_time_ms},
  Confidence: band=very_high; score=0.9
- Line 332: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "LLMApiHandler::handleInference success: model='{}' prompt_len={} tokens_generated={} inference_time_ms={:.2f} lora='{}'",
  Confidence: band=very_high; score=0.9
- Line 336: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_time_ms,
  Confidence: band=very_high; score=0.9
- Line 344: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "Inference failed",
  Confidence: band=very_high; score=0.9
- Line 348: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: logCurrentException("LLMApiHandler::handleInference");
  Confidence: band=very_high; score=0.9
- Line 349: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: return createErrorResponse(http::status::internal_server_error, "Inference failed");
  Confidence: band=very_high; score=0.9
- Line 569: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Prepare inference request
  Confidence: band=very_high; score=0.9
- Line 570: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm::InferenceRequest llm_request;
  Confidence: band=very_high; score=0.9
- Line 572: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: llm_request.model_id = model_id.empty() ? std::string("default") : model_id;
- Line 584: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Call LLMPluginManager for RAG inference
  Confidence: band=very_high; score=0.9
- Line 585: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto llm_response = plugin_mgr.generateRAG(rag_context, llm_request);
  Confidence: band=very_high; score=0.9
- Line 600: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: {"inference_time_ms", llm_response.inference_time_ms},
  Confidence: band=very_high; score=0.9
- Line 611: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm_response.inference_time_ms,
  Confidence: band=very_high; score=0.9
- Line 621: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "RAG inference failed",
  Confidence: band=very_high; score=0.9
- Line 626: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: return createErrorResponse(http::status::internal_server_error, "RAG inference failed");
  Confidence: band=very_high; score=0.9
- Line 686: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: http::response<http::string_body> LLMApiHandler::handleStreamInference(
  Confidence: band=very_high; score=0.9
- Line 688: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto span = Tracer::startSpan("handleStreamInference");
  Confidence: band=very_high; score=0.9
- Line 747: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "LLMApiHandler::handleStreamInference start: request_id='{}' prompt_len={} max_tokens={}",
  Confidence: band=very_high; score=0.9
- Line 769: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "LLMApiHandler::handleStreamInference complete: request_id='{}' sse_bytes={}",
  Confidence: band=very_high; score=0.9
- Line 774: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "LLMApiHandler::handleStreamInference failed: request_id='{}' error='{}'",
  Confidence: band=very_high; score=0.9
- Line 781: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "LLMApiHandler::handleStreamInference failed with unknown error: request_id='{}'",
  Confidence: band=very_high; score=0.9
- Line 783: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: logCurrentException("LLMApiHandler::handleStreamInference");
  Confidence: band=very_high; score=0.9
- Line 893: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: http::response<http::string_body> LLMApiHandler::handleListModels(
  Confidence: band=very_high; score=0.9
- Line 921: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: return createErrorResponse(http::status::internal_server_error, "Failed to list models");
  Confidence: band=very_high; score=0.9
- Line 925: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: http::response<http::string_body> LLMApiHandler::handleLoadModel(
  Confidence: band=very_high; score=0.9
- Line 941: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: return createErrorResponse(http::status::bad_request, "Missing 'model_id' field");
  Confidence: band=very_high; score=0.9
- Line 948: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: return createErrorResponse(http::status::bad_request, "Invalid load model parameters", e.what());
  Confidence: band=very_high; score=0.9
- Line 983: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: return createErrorResponse(http::status::internal_server_error, "Failed to load model");
  Confidence: band=very_high; score=0.9
- Line 987: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: http::response<http::string_body> LLMApiHandler::handleUnloadModel(
  Confidence: band=very_high; score=0.9
- Line 1002: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: return createErrorResponse(http::status::bad_request, "Missing 'model_id' field");
  Confidence: band=very_high; score=0.9
- Line 1005: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: return createErrorResponse(http::status::bad_request, "Invalid unload model parameters", e.what());
  Confidence: band=very_high; score=0.9
- Line 1028: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: return createErrorResponse(http::status::internal_server_error, "Failed to unload model");
  Confidence: band=very_high; score=0.9
- Line 1032: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: http::response<http::string_body> LLMApiHandler::handleModelInfo(
  Confidence: band=very_high; score=0.9
- Line 1071: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: return createErrorResponse(http::status::internal_server_error, "Model info retrieval failed");
  Confidence: band=very_high; score=0.9
- Line 1075: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: http::response<http::string_body> LLMApiHandler::handleIngestModel(
  Confidence: band=very_high; score=0.9
- Line 1093: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: return createErrorResponse(http::status::bad_request, "Missing 'model_id' field");
  Confidence: band=very_high; score=0.9
- Line 1121: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: return createErrorResponse(http::status::internal_server_error, "Model ingestion failed");
  Confidence: band=very_high; score=0.9
- Line 1279: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Get statistics from AsyncInferenceEngine and LLMPluginManager
  Confidence: band=very_high; score=0.9
- Line 1378: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Check health of LLMPluginManager and AsyncInferenceEngine
  Confidence: band=very_high; score=0.9
- Line 1528: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = assistant.query(query);
- Line 1550: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response_data["relevant_documents"] = docs_array;
- Line 1774: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response_data["message"] = "Feedback recorded successfully";
- Line 2008: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto perm = policy_engine_->checkInferencePermission(header_map);
  Confidence: band=very_high; score=0.9
- Line 2026: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Parse the OpenAI request into an InferenceRequest
  Confidence: band=very_high; score=0.9
- Line 2033: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto& llm_request = std::get<llm::InferenceRequest>(parse_result);
  Confidence: band=very_high; score=0.9
- Line 2047: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // write chunks incrementally; here we buffer them for compatibility
  Confidence: band=high; score=0.8
- Line 2059: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: llm_request.prompt.size(),
- Line 2062: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: llm_request.stream_callback = [&](const std::string& token) {
- Line 2076: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string{"Inference failed: "} + e.what(),
  Confidence: band=very_high; score=0.9
- Line 2084: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto err = llm::OpenAICompatAdapter::buildError("Inference failed", "server_error");
  Confidence: band=very_high; score=0.9
- Line 2111: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: llm_request.prompt.size(),
- Line 2114: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm::InferenceResponse llm_response;
  Confidence: band=very_high; score=0.9
- Line 2117: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm_response = plugin_mgr.generate(llm_request);
  Confidence: band=very_high; score=0.9
- Line 2124: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string{"Inference failed: "} + e.what(),
  Confidence: band=very_high; score=0.9
- Line 2132: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto err = llm::OpenAICompatAdapter::buildError("Inference failed", "server_error");
  Confidence: band=very_high; score=0.9
- Line 2140: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "LLMApiHandler::handleOpenAIChatCompletions non-stream complete: model='{}' tokens_generated={} inference_time_ms={:.2f}",
  Confidence: band=very_high; score=0.9
- Line 2143: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm_response.inference_time_ms);
  Confidence: band=very_high; score=0.9
- Line 2160: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: http::response<http::string_body> LLMApiHandler::handleOpenAIListModels(
  Confidence: band=very_high; score=0.9
- Line 73: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 80: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto content = entity["content"].get<std::string>();
  Confidence: band=high; score=0.74
- Line 86: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto content = entity["text"].get<std::string>();
  Confidence: band=high; score=0.74
- Line 92: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto content = entity["body"].get<std::string>();
  Confidence: band=high; score=0.74
- Line 105: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto source = entity["source"].get<std::string>();
  Confidence: band=high; score=0.74
- Line 111: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto source = entity["id"].get<std::string>();
  Confidence: band=high; score=0.74
- Line 236: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 347: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 531: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rag_context.documents.push_back(std::move(document));
  Confidence: band=high; score=0.74
- Line 552: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 624: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 661: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: embedding_vector.push_back(val);
  Confidence: band=high; score=0.74
- Line 678: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 709: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (raw[i] == '+') { decoded += ' '; ++i; }
  Confidence: band=high; score=0.74
- Line 710: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (raw[i] == '+') { decoded += ' '; ++i; }
- Line 779: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 872: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 903: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: models.push_back(json{{"model_id", model_id}});
  Confidence: band=high; score=0.74
- Line 919: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 981: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1026: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1069: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1119: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1141: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: loras.push_back(lora_obj);
  Confidence: band=high; score=0.74
- Line 1159: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1222: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1269: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1300: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1341: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1368: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1399: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1431: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1543: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: docs_array.push_back({
  Confidence: band=high; score=0.74
- Line 1544: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: docs_array.push_back({
- Line 1560: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1622: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1686: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1780: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1845: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::string status_filter;
- Line 1925: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: feedback_array.push_back(feedback.toJson());
  Confidence: band=high; score=0.74
- Line 1926: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: feedback_array.push_back(feedback.toJson());
- Line 1942: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2003: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> header_map;
  Confidence: band=medium; score=0.66
- Line 2016: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: : http::status::forbidden);
- Line 2069: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: plugin_mgr.generate(llm_request);
  Confidence: band=high; score=0.74
- Line 2079: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2117: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: llm_response = plugin_mgr.generate(llm_request);
  Confidence: band=high; score=0.74
- Line 2127: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2152: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2171: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: models_arr.push_back(json{
  Confidence: band=high; score=0.74
- Line 2172: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: models_arr.push_back(json{
- Line 2183: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/postgres_session.cpp
Total findings: 135

- Line 123: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (auto self = weak_self.lock()) {
- Line 930: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator ltrim may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto ltrim = field.find_first_not_of(" \t");
- Line 931: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator rtrim may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto rtrim = field.find_last_not_of(" \t");
- Line 1567: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (auto self = weak_self.lock()) {
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 149: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (ec || stopped_.load(std::memory_order_acquire)) {
- Line 171: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (ec || stopped_.load(std::memory_order_acquire)) {
- Line 190: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: switch (transactionState_.load(std::memory_order_acquire)) {
- Line 301: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Handle schema queries (pg_catalog, information_schema) for BI tool compatibility
  Confidence: band=high; score=0.8
- Line 549: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t dotPos = colName.find('.');
- Line 725: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t dotPos = colName.find('.');
- Line 787: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t dotPos = colName.find('.');
- Line 854: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!copyInProgress_.load(std::memory_order_acquire)) {
- Line 868: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(copyMutex_);
- Line 878: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!copyInProgress_.load(std::memory_order_acquire)) {
- Line 1290: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (ec || stopped_.load(std::memory_order_acquire)) {
- Line 1302: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (inStartup_.load(std::memory_order_acquire)) {
- Line 1344: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string query(buffer_.data() + offset);
- Line 1521: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (ec || stopped_.load(std::memory_order_acquire)) {
- Line 1578: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (stopped_.load(std::memory_order_acquire)) {
- Line 1626: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Standard PostgreSQL types required for BI tool compatibility
  Confidence: band=high; score=0.8
- Line 2167: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t eqPos = assignment.find('=');
- Line 32: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += "''";  // PostgreSQL escapes single quotes by doubling
  Confidence: band=high; score=0.74
- Line 33: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += "''";  // PostgreSQL escapes single quotes by doubling
- Line 35: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: result += "\\\\";  // Escape backslashes
- Line 35: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += "\\\\";  // Escape backslashes
- Line 86: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 142: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: socket_.close(ec);
- Line 155: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 177: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 202: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::string>& params) {
  Confidence: band=high; score=0.74
- Line 258: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void PostgresSession::handleQuery(const std::string& query) {
  Confidence: band=high; score=0.74
- Line 455: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void PostgresSession::handleExecute(const std::string& portal, int32_t maxRows) {
  Confidence: band=high; score=0.74
- Line 546: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back({"?column?", 0, 0, 25, -1, -1, 0});
  Confidence: band=high; score=0.74
- Line 547: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back({"?column?", 0, 0, 25, -1, -1, 0});
- Line 554: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back({colName, 0, 0, 25, -1, -1, 0});
- Line 599: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: row_vals.push_back(
  Confidence: band=high; score=0.74
- Line 599: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: row_vals.push_back(
  Confidence: band=high; score=0.74
- Line 600: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: row_vals.push_back(
- Line 604: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: row_vals.push_back("");
- Line 721: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back({"?column?", 0, 0, 25, -1, -1, 0}); // text type
  Confidence: band=high; score=0.74
- Line 722: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back({"?column?", 0, 0, 25, -1, -1, 0}); // text type
- Line 730: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back({colName, 0, 0, 25, -1, -1, 0}); // text type
- Line 735: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back({"?column?", 0, 0, 25, -1, -1, 0});
- Line 784: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back({"?column?", 0, 0, 25, -1, -1, 0});
  Confidence: band=high; score=0.74
- Line 785: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back({"?column?", 0, 0, 25, -1, -1, 0});
- Line 792: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back({colName, 0, 0, 25, -1, -1, 0});
- Line 796: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back({"?column?", 0, 0, 25, -1, -1, 0});
- Line 911: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: field += '"';
  Confidence: band=high; score=0.74
- Line 912: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: field += '"';
- Line 912: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: field += '"';
- Line 912: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: field += '"';
- Line 1032: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void PostgresSession::sendReadyForQuery(char transactionStatus) {
  Confidence: band=high; score=0.74
- Line 1047: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: payload.push_back(0);
  Confidence: band=high; score=0.74
- Line 1051: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((field.tableOid >> 24) & 0xFF);
- Line 1052: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((field.tableOid >> 16) & 0xFF);
- Line 1053: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((field.tableOid >> 8) & 0xFF);
- Line 1054: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(field.tableOid & 0xFF);
- Line 1057: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((field.columnAttrNumber >> 8) & 0xFF);
- Line 1058: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(field.columnAttrNumber & 0xFF);
- Line 1061: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((field.dataTypeOid >> 24) & 0xFF);
- Line 1062: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((field.dataTypeOid >> 16) & 0xFF);
- Line 1063: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((field.dataTypeOid >> 8) & 0xFF);
- Line 1064: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(field.dataTypeOid & 0xFF);
- Line 1077: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((field.formatCode >> 8) & 0xFF);
- Line 1078: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(field.formatCode & 0xFF);
- Line 1089: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((colCount >> 8) & 0xFF);
- Line 1090: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(colCount & 0xFF);
- Line 1094: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: payload.push_back((len >> 24) & 0xFF);
  Confidence: band=high; score=0.74
- Line 1095: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((len >> 24) & 0xFF);
- Line 1096: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((len >> 16) & 0xFF);
- Line 1097: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((len >> 8) & 0xFF);
- Line 1098: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(len & 0xFF);
- Line 1114: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((colCount >> 8) & 0xFF);
- Line 1115: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(colCount & 0xFF);
- Line 1119: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: payload.push_back((len >> 24) & 0xFF);
  Confidence: band=high; score=0.74
- Line 1120: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((len >> 24) & 0xFF);
- Line 1121: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((len >> 16) & 0xFF);
- Line 1122: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((len >> 8) & 0xFF);
- Line 1123: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(len & 0xFF);
- Line 1162: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: payload.push_back((typeOid >> 24) & 0xFF);
  Confidence: band=high; score=0.74
- Line 1163: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((typeOid >> 24) & 0xFF);
- Line 1164: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((typeOid >> 16) & 0xFF);
- Line 1165: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((typeOid >> 8) & 0xFF);
- Line 1166: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(typeOid & 0xFF);
- Line 1192: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((numColumns >> 8) & 0xFF);
- Line 1193: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(numColumns & 0xFF);
- Line 1196: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: payload.push_back((format >> 8) & 0xFF);
  Confidence: band=high; score=0.74
- Line 1197: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((format >> 8) & 0xFF);
- Line 1198: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(format & 0xFF);
- Line 1214: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((numColumns >> 8) & 0xFF);
- Line 1215: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(numColumns & 0xFF);
- Line 1217: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: payload.push_back((format >> 8) & 0xFF);
  Confidence: band=high; score=0.74
- Line 1218: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((format >> 8) & 0xFF);
- Line 1219: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(format & 0xFF);
- Line 1235: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((numColumns >> 8) & 0xFF);
- Line 1236: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(numColumns & 0xFF);
- Line 1238: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: payload.push_back((format >> 8) & 0xFF);
  Confidence: band=high; score=0.74
- Line 1239: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back((format >> 8) & 0xFF);
- Line 1240: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(format & 0xFF);
- Line 1365: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: paramTypes.push_back(typeOid);
  Confidence: band=high; score=0.74
- Line 1389: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: paramFormats.push_back(format);
  Confidence: band=high; score=0.74
- Line 1410: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: params.push_back("NULL");
  Confidence: band=high; score=0.74
- Line 1411: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: params.push_back("NULL");
- Line 1494: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1543: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1597: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool PostgresSession::isSchemaQuery(const std::string& query) {
  Confidence: band=high; score=0.74
- Line 1610: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void PostgresSession::handleSchemaQuery(const std::string& query) {
  Confidence: band=high; score=0.74
- Line 1720: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "[PostgresSession] pg_attribute query: document parse error: " << e.what() << "\n";
- Line 1761: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: PostgresSession::QueryInfo PostgresSession::parseSelectQuery(const std::string& query) {
  Confidence: band=high; score=0.74
- Line 1920: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) cypher += ", ";
  Confidence: band=high; score=0.74
- Line 1921: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) cypher += ", ";
- Line 1926: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: cypher += "count(n)";
- Line 1932: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: cypher += "count(n." + col + ")";
- Line 1937: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: cypher += "sum(n." + col + ")";
- Line 1942: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: cypher += "avg(n." + col + ")";
- Line 1947: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: cypher += "min(n." + col + ")";
- Line 1959: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) cypher += ", ";
  Confidence: band=high; score=0.74
- Line 1960: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) cypher += ", ";
- Line 1963: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: cypher += "n";
- Line 1965: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: cypher += "n." + col;
- Line 1991: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string PostgresSession::parseInsertQuery(const std::string& query) {
  Confidence: band=high; score=0.74
- Line 2063: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.push_back(currentValue);
  Confidence: band=high; score=0.74
- Line 2079: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) cypher += ", ";
  Confidence: band=high; score=0.74
- Line 2079: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) cypher += ", ";
  Confidence: band=high; score=0.74
- Line 2080: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) cypher += ", ";
- Line 2089: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string PostgresSession::parseUpdateQuery(const std::string& query) {
  Confidence: band=high; score=0.74
- Line 2152: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: assignments.push_back(cypherSetClause.substr(start, i - start));
  Confidence: band=high; score=0.74
- Line 2153: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: assignments.push_back(cypherSetClause.substr(start, i - start));
- Line 2158: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: assignments.push_back(cypherSetClause.substr(start));
- Line 2183: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) cypherSetClause += ", ";
  Confidence: band=high; score=0.74
- Line 2184: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) cypherSetClause += ", ";
- Line 2194: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string PostgresSession::parseDeleteQuery(const std::string& query) {
  Confidence: band=high; score=0.74
- Line 2240: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string PostgresSession::translateQuery(const std::string& postgresQuery) {
  Confidence: band=high; score=0.74

### src/server/monitoring_api_handler.cpp
Total findings: 88

- Line 117: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: bool alive = (is_running_ == nullptr) || is_running_->load(std::memory_order_relaxed);
- Line 144: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: bool server_running = (is_running_ == nullptr) || is_running_->load(std::memory_order_relaxed);
- Line 446: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto loop_context = continuous_learning_orchestrator_->serializeLoopContext();
- Line 660: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const std::string loop_context = continuous_learning_orchestrator_->serializeLoopContext();
- Line 1421: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: loop_context = continuous_learning_orchestrator_->serializeLoopContext();
- Line 1472: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator next may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto next = arr.find("},{", cur);
- Line 0: severity=HIGH; category=uncategorized
  Context: ['        html += "<!DOCTYPE html>\\n<html lang=\\"en\\">\\n<head>\\n";', '        html += "<meta charset=\\"UTF-8\\">\\n";', '        html += "<meta name=\\"viewport\\" content=\\"width=device-width, initial-scale=1\\">\\n";', '        html += "<title>ThemisDB Metrics</title>\\n";', '        html += "<style>\\n";']
  Confidence: band=high; score=0.78
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3149 feat(api): Complete OpenAPI... (2026-03-12) | #2831 [config] Wire Prome
- Line 117: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: bool alive = (is_running_ == nullptr) || is_running_->load(std::memory_order_relaxed);
- Line 144: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: bool server_running = (is_running_ == nullptr) || is_running_->load(std::memory_order_relaxed);
- Line 287: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: modules_compiled.push_back(module_info);
  Confidence: band=high; score=0.74
- Line 337: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: supported.push_back({
  Confidence: band=high; score=0.74
- Line 338: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: supported.push_back({
- Line 429: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 488: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 562: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 599: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 631: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "# HELP themis_build_info ThemisDB build information\n";
  Confidence: band=high; score=0.74
- Line 674: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "# HELP themis_continuous_learning_loop_signal_value Latest loop signal value\n";
  Confidence: band=high; score=0.74
- Line 683: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out += "# HELP themis_continuous_learning_loop_live_signal Loop uses live provider signal (1=yes,0=f
- Line 704: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "themis_continuous_learning_loop_signal_value" + labels +
  Confidence: band=high; score=0.74
- Line 720: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 761: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = r["files_per_level"].begin(); it != r["files_per_level"].end(); ++it) {
  Confidence: band=high; score=0.74
- Line 764: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: level_rows.emplace_back(it.key(), val);
  Confidence: band=high; score=0.74
- Line 771: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 785: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "rocksdb_files_level{level=\"" + level + "\"} " + std::to_string(val) + "\n";
  Confidence: band=high; score=0.74
- Line 785: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "rocksdb_files_level{level=\"" + level + "\"} " + std::to_string(val) + "\n";
  Confidence: band=high; score=0.74
- Line 786: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "rocksdb_files_level{level=\"" + level + "\"} " + std::to_string(val) + "\n";
- Line 827: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sorted_plugin_names.push_back(plugin_name);
  Confidence: band=high; score=0.74
- Line 832: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "\n# HELP themis_plugin_loads_total Total number of plugin loads\n";
  Confidence: band=high; score=0.74
- Line 835: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "themis_plugin_loads_total{plugin=\"" + plugin_name + "\"} 1\n";
  Confidence: band=high; score=0.74
- Line 836: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "themis_plugin_loads_total{plugin=\"" + plugin_name + "\"} 1\n";
- Line 842: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "themis_plugin_reloads_total{plugin=\"" + plugin_name + "\"} "
  Confidence: band=high; score=0.74
- Line 843: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "themis_plugin_reloads_total{plugin=\"" + plugin_name + "\"} "
- Line 850: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "themis_plugin_errors_total{plugin=\"" + plugin_name + "\"} "
  Confidence: band=high; score=0.74
- Line 851: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "themis_plugin_errors_total{plugin=\"" + plugin_name + "\"} "
- Line 858: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "themis_plugin_function_calls_total{plugin=\"" + plugin_name + "\"} "
  Confidence: band=high; score=0.74
- Line 859: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "themis_plugin_function_calls_total{plugin=\"" + plugin_name + "\"} "
- Line 867: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "themis_plugin_load_duration_seconds_sum{plugin=\"" + plugin_name + "\"} "
  Confidence: band=high; score=0.74
- Line 868: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "themis_plugin_load_duration_seconds_sum{plugin=\"" + plugin_name + "\"} "
- Line 870: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "themis_plugin_load_duration_seconds_count{plugin=\"" + plugin_name + "\"} 1\n";
- Line 876: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "themis_plugin_memory_bytes{plugin=\"" + plugin_name + "\"} "
  Confidence: band=high; score=0.74
- Line 877: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "themis_plugin_memory_bytes{plugin=\"" + plugin_name + "\"} "
- Line 885: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "themis_plugin_call_latency_milliseconds{plugin=\"" + plugin_name
  Confidence: band=high; score=0.74
- Line 886: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "themis_plugin_call_latency_milliseconds{plugin=\"" + plugin_name
- Line 888: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "themis_plugin_call_latency_milliseconds{plugin=\"" + plugin_name
- Line 890: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "themis_plugin_call_latency_milliseconds_sum{plugin=\"" + plugin_name
- Line 892: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "themis_plugin_call_latency_milliseconds_count{plugin=\"" + plugin_name
- Line 900: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 914: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 928: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 943: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 958: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 986: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sorted_plugin_names.push_back(plugin_name);
  Confidence: band=high; score=0.74
- Line 1197: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(a);
  Confidence: band=high; score=0.74
- Line 1244: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1384: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '&': escaped += "&amp;"; break;
  Confidence: band=high; score=0.74
- Line 1385: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '&': escaped += "&amp;"; break;
- Line 1386: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '<': escaped += "&lt;"; break;
- Line 1387: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '>': escaped += "&gt;"; break;
- Line 1388: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: default: escaped.push_back(ch); break;
  Confidence: band=high; score=0.74
- Line 1388: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"': escaped += "&quot;"; break;
- Line 1397: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<title>ThemisDB Metrics</title>\n";
- Line 1408: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<h1>ThemisDB Metrics Dashboard</h1>\n";
- Line 1409: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<p class=\"sub\">Version: <b>" + version + "</b> &nbsp;|&nbsp; ";
- Line 1409: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<p class=\"sub\">Version: <b>" + version + "</b> &nbsp;|&nbsp; ";
- Line 1411: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<a href=\"/metrics\">Raw Prometheus</a></p>\n";
- Line 1411: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<a href=\"/metrics\">Raw Prometheus</a></p>\n";
- Line 1412: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<table>\n<tr><th>Metric</th><th class=\"val\">Value</th></tr>\n";
- Line 1412: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<table>\n<tr><th>Metric</th><th class=\"val\">Value</th></tr>\n";
- Line 1413: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: html += "<tr><td>" + name + "</td><td class=\"val\">" + val + "</td></tr>\n";
  Confidence: band=high; score=0.74
- Line 1414: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<tr><td>" + name + "</td><td class=\"val\">" + val + "</td></tr>\n";
- Line 1414: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<tr><td>" + name + "</td><td class=\"val\">" + val + "</td></tr>\n";
- Line 1414: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: html += "<tr><td>" + name + "</td><td class=\"val\">" + val + "</td></tr>\n";
- Line 1416: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</table>\n";
- Line 1428: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<h2>Continuous Learning Loops</h2>\n";
- Line 1474: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: loop_items.push_back(arr.substr(cur));
- Line 1477: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: loop_items.push_back(arr.substr(cur, next - cur + 1));
- Line 1484: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<p><em>No loop results yet.</em></p>\n";
- Line 1496: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "</tr>\n";
- Line 1510: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: html += "<tr>";
  Confidence: band=high; score=0.74
- Line 1513: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<td class=\"val\">" + escape_html(sig_val) + "</td>";
- Line 1513: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<td class=\"val\">" + escape_html(sig_val) + "</td>";
- Line 1520: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<td class=\"val\">" + escape_html(mdelta) + "</td>";
- Line 1520: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "<td class=\"val\">" + escape_html(mdelta) + "</td>";
- Line 1523: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</tr>\n";
- Line 1525: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</table>\n";
- Line 1529: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: html += "</body>\n</html>\n";

### src/server/voice_api_handler.cpp
Total findings: 80

- Line 1058: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: metadata.meeting_id = body->value("meeting_id", "");
- Line 1433: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto macros = voice_assistant_->macroManager().listMacros("", tag_filter);
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 1180: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["languages"] = json::array({
- Line 1580: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["macro"]   = info ? macroInfoToResponseJson(*info) : json(nullptr);
- Line 1652: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: r["metadata"]         = rec.metadata;
- Line 1700: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["metadata"]         = rec->metadata;
- Line 2021: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: if (response_future.wait_for(std::chrono::seconds(70)) == std::future_status::timeout) {
- Line 74: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 275: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Missing macro ID");
- Line 279: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Invalid macro ID");
- Line 297: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Missing session ID");
- Line 308: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Invalid session ID");
- Line 316: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Invalid session path");
- Line 321: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Invalid session ID");
- Line 342: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Missing recording ID");
- Line 346: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Invalid recording ID");
- Line 370: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Missing profile ID");
- Line 374: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Invalid profile ID");
- Line 1079: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: metadata.participants.push_back(p);
  Confidence: band=high; score=0.74
- Line 1209: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = step_json["parameters"].begin(); it != step_json["parameters"].end(); ++it) {
  Confidence: band=high; score=0.74
- Line 1233: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = j["parameters"].begin(); it != j["parameters"].end(); ++it) {
  Confidence: band=high; score=0.74
- Line 1272: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: steps.push_back(sj);
  Confidence: band=high; score=0.74
- Line 1272: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: steps.push_back(sj);
  Confidence: band=high; score=0.74
- Line 1295: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Invalid JSON body");
- Line 1306: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "trigger_phrase must be a string");
- Line 1312: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "trigger_phrase must not be empty");
- Line 1318: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "'steps' must be an array");
- Line 1325: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: steps.push_back(parseStep(sj));
  Confidence: band=high; score=0.74
- Line 1326: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back(parseStep(sj));
- Line 1332: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "options must be an object");
- Line 1363: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "name must be a string");
- Line 1367: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "description must be a string");
- Line 1371: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "tags must be an array");
- Line 1436: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(macroInfoToResponseJson(m));
  Confidence: band=high; score=0.74
- Line 1437: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(macroInfoToResponseJson(m));
- Line 1469: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Invalid JSON body");
- Line 1479: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "'steps' must be an array");
- Line 1488: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: steps.push_back(parseStep(sj));
  Confidence: band=high; score=0.74
- Line 1489: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back(parseStep(sj));
- Line 1495: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "options must be an object");
- Line 1526: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "name must be a string");
- Line 1530: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "description must be a string");
- Line 1534: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "tags must be an array");
- Line 1547: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "enabled must be a boolean");
- Line 1652: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 1751: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result["recordings"].push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 1784: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 1851: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1945: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64_table[(n >> 18) & 63]);
- Line 1968: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2080: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Invalid JSON body");
- Line 2091: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "user_id must be a string");
- Line 2097: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "user_id must not be empty");
- Line 2101: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Invalid user_id");
- Line 2107: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "audio_samples must be a non-empty array");
- Line 2129: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: audio_samples.push_back(std::move(decoded_sample));
  Confidence: band=high; score=0.74
- Line 2195: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Invalid JSON body");
- Line 2206: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "profile_id must be a string");
- Line 2210: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "audio must be a base64 string");
- Line 2216: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "profile_id must not be empty");
- Line 2220: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Invalid profile_id");
- Line 2225: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "audio must not be empty");
- Line 2252: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Invalid JSON body");
- Line 2263: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "user_id must be a string");
- Line 2267: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "audio must be a base64 string");
- Line 2273: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "user_id must not be empty");
- Line 2277: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Invalid user_id");
- Line 2282: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "audio must not be empty");
- Line 2304: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: : http::status::unauthorized;
- Line 2315: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Invalid JSON body");
- Line 2327: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "candidate_profiles must be an array");
- Line 2331: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "candidate_profiles must not be empty");
- Line 2335: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "audio must be a base64 string");
- Line 2339: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "audio must not be empty");
- Line 2355: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(candidate);
  Confidence: band=high; score=0.74
- Line 2374: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matches_arr.push_back(mj);
  Confidence: band=high; score=0.74
- Line 2392: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(pid);
  Confidence: band=high; score=0.74
- Line 2408: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::bad_request, "Bad Request", "Missing profile ID");
- Line 2414: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::not_found, "Not Found", "Voice profile not found");

### src/server/mcp_server.cpp
Total findings: 74

- Line 2831: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: auto self = weak_self.lock();
- Line 3163: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = sessions_.find(session_id);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 489: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.contains("jsonrpc") || request["jsonrpc"] != "2.0") {
- Line 493: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.contains("method")) {
- Line 498: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json params = request.contains("params") ? request["params"] : json::object();
- Line 723: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: without = nullptr;
  Context: {"description", "AI Safety: preview the delete without executing it (Phase 1 guard)"}}}
- Line 862: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: {"description", "Search query (e.g., 'gpu', 'lora', 'storage')"}
- Line 862: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function query without trace point
  Context: {"description", "Search query (e.g., 'gpu', 'lora', 'storage')"}
  Confidence: band=very_high; score=0.9
- Line 1234: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: entity = nullptr;
  Context: {"message", "Failed to delete entity"},
- Line 1517: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const bool database_connected = db_ && db_->isOpen();
- Line 1548: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: schema_json["database_connected"] = database_connected;
- Line 1568: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const bool database_connected = db_ && db_->isOpen();
- Line 1767: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: out["tokens_generated"]= result.metadata.tokens_generated;
- Line 1768: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: out["retrieved_docs"]  = result.metadata.retrieved_docs;
- Line 1769: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: out["latency_ms"]      = result.metadata.latency.total_ms;
- Line 2081: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(results.size(), size_t(3)); ++i) {
- Line 2102: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(results.size(), size_t(3)); ++i) {
- Line 2134: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [this](const std::string& uri) { return resourceMetadata(uri); });
- Line 2230: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: {"database_open", db_ && db_->isOpen()}
- Line 2822: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!is_running_.load(std::memory_order_acquire)) return;
- Line 2840: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (self->is_running_.load(std::memory_order_acquire)) {
- Line 2891: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: } else if (!self->is_running_.load(std::memory_order_acquire)) {
- Line 2922: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 2933: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (self->is_running_.load(std::memory_order_acquire)) {
- Line 3019: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!is_running_.load(std::memory_order_acquire)) return;
- Line 3057: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!is_running_.load(std::memory_order_acquire)) return;
- Line 3071: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!is_running_.load(std::memory_order_acquire)) return;
- Line 3079: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!ec && self->is_running_.load(std::memory_order_acquire)) {
- Line 3122: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!is_running_.load(std::memory_order_acquire)) return;
- Line 3137: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!is_running_.load(std::memory_order_acquire)) return;
- Line 3176: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!is_running_.load(std::memory_order_acquire)) return;
- Line 3202: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!is_running_.load(std::memory_order_acquire)) return;
- Line 3223: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!is_running_.load(std::memory_order_acquire)) return;
- Line 3231: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!ec && self->is_running_.load(std::memory_order_acquire)) {
- Line 108: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: new_cfg.denied_collections.push_back(c.as<std::string>());
  Confidence: band=high; score=0.74
- Line 109: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: new_cfg.denied_collections.push_back(c.as<std::string>());
- Line 114: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: new_cfg.allowed_collections.push_back(c.as<std::string>());
  Confidence: band=high; score=0.74
- Line 115: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: new_cfg.allowed_collections.push_back(c.as<std::string>());
- Line 524: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: json McpServer::handleInitialize(const json& params) {
  Confidence: band=medium; score=0.66
- Line 553: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tools_list.push_back({
  Confidence: band=high; score=0.74
- Line 554: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tools_list.push_back({
- Line 596: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resources_list.push_back({
  Confidence: band=high; score=0.74
- Line 597: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: resources_list.push_back({
- Line 641: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: prompts_list.push_back({
  Confidence: band=high; score=0.74
- Line 642: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: prompts_list.push_back({
- Line 723: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: {"description", "AI Safety: preview the delete without executing it (Phase 1 guard)"}}}
- Line 862: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: {"description", "Search query (e.g., 'gpu', 'lora', 'storage')"}
  Confidence: band=high; score=0.74
- Line 887: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json McpServer::toolQuery(const json& args) {
  Confidence: band=high; score=0.74
- Line 1234: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: {"message", "Failed to delete entity"},
- Line 1286: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: SecondaryIndexManager::Status status;
- Line 1301: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ft_config = args["fulltext_config"];
  Confidence: band=high; score=0.74
- Line 1405: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: SecondaryIndexManager::Status status;
- Line 1494: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: indexes.push_back(index_info);
  Confidence: band=high; score=0.74
- Line 1494: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: indexes.push_back(index_info);
  Confidence: band=high; score=0.74
- Line 1681: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: messages.push_back({
  Confidence: band=high; score=0.74
- Line 1682: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: messages.push_back({
- Line 1802: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: modes_arr.push_back(std::move(entry));
  Confidence: band=high; score=0.74
- Line 1837: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1849: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors_json.push_back(error.toJSON());
  Confidence: band=high; score=0.74
- Line 1850: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors_json.push_back(error.toJSON());
- Line 1877: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors_json.push_back(error.toJSON());
  Confidence: band=high; score=0.74
- Line 1878: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors_json.push_back(error.toJSON());
- Line 2020: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: answer += fmt::format("**{}** ({} error types)\n", category, errors.size());
  Confidence: band=high; score=0.74
- Line 2046: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) docs_str += ", ";
  Confidence: band=high; score=0.74
- Line 2047: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) docs_str += ", ";
- Line 2279: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json McpServer::promptSimpleQuery(const std::string& name, const json& args) {
  Confidence: band=high; score=0.74
- Line 2294: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json McpServer::promptComplexQuery(const std::string& name, const json& args) {
  Confidence: band=high; score=0.74
- Line 2546: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: list.push_back({
  Confidence: band=high; score=0.74
- Line 2547: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: list.push_back({
- Line 2778: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void StdioTransport::start() {
  Confidence: band=medium; score=0.66
- Line 2808: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { fn(); } catch (...) {}
- Line 2996: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void SseTransport::start() {
  Confidence: band=medium; score=0.66

### src/server/shard_repair_api_handler.cpp
Total findings: 55

- Line 128: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: << "input{padding:10px 12px;border-radius:999px;border:1px solid var(--line);min-width:220px;background:#fff}"
  Confidence: band=very_high; score=0.99
- Line 144: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: << "<input id=\"shardId\" placeholder=\"Shard ID for targeted repair\">"
  Confidence: band=very_high; score=0.99
- Line 157: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Use std::unique_ptr<T> or std::make_unique<T>()
  Context: << "function fmtTime(v){return v?new Date(v).toLocaleString():'-';}"
- Line 157: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: << "function fmtTime(v){return v?new Date(v).toLocaleString():'-';}"
- Line 161: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Use std::unique_ptr<T> or std::make_unique<T>()
  Context: << "async function post(url,body){const res=await fetch(url,{method:'POST',headers:{'Content-Type':'
- Line 161: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: << "async function post(url,body){const res=await fetch(url,{method:'POST',headers:{'Content-Type':'
- Line 206: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto ar = auth.authorize(*token, required_scope);
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Context: ['         << "<html lang=\\"en\\">\\n"', '         << "<head><meta charset=\\"utf-8\\">\\n"', '         << "<meta name=\\"viewport\\" content=\\"width=device-width,initial-scale=1\\">\\n"', '         << "<title>Themis Repair Dashboard</title>\\n"', '         << "<style>"']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 128: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: << "input{padding:10px 12px;border-radius:999px;border:1px solid var(--line);min-width:220px;background:#fff}"
  Confidence: band=very_high; score=0.9
- Line 144: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: << "<input id=\"shardId\" placeholder=\"Shard ID for targeted repair\">"
  Confidence: band=very_high; score=0.9
- Line 333: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: const std::string job_id = extractJobId(std::string(req.target()));
  Confidence: band=very_high; score=0.9
- Line 115: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<title>Themis Repair Dashboard</title>\n"
- Line 136: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "</style></head>\n"
- Line 139: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<p class=\"sub\">Shard health, repair jobs, and anti-entropy triggers.</p>"
- Line 139: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<p class=\"sub\">Shard health, repair jobs, and anti-entropy triggers.</p>"
- Line 140: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<div id=\"flash\"></div>"
- Line 140: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<div id=\"flash\"></div>"
- Line 141: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<section class=\"grid\" id=\"summary\"></section>"
- Line 141: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<section class=\"grid\" id=\"summary\"></section>"
- Line 141: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<section class=\"grid\" id=\"summary\"></section>"
- Line 143: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<button id=\"scanBtn\">Start Full Scan</button>"
- Line 143: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<button id=\"scanBtn\">Start Full Scan</button>"
- Line 145: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<button id=\"repairBtn\" class=\"secondary\">Queue Shard Repair</button>"
- Line 145: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<button id=\"repairBtn\" class=\"secondary\">Queue Shard Repair</button>"
- Line 145: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<button id=\"repairBtn\" class=\"secondary\">Queue Shard Repair</button>"
- Line 146: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<button id=\"refreshBtn\">Refresh</button>"
- Line 146: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<button id=\"refreshBtn\">Refresh</button>"
- Line 148: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<h2>Shard Health</h2><table><thead><tr><th>Shard</th><th>Status</th><th>Scanned</th><th>Healthy<
- Line 148: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<h2>Shard Health</h2><table><thead><tr><th>Shard</th><th>Status</th><th>Scanned</th><th>Healthy<
- Line 149: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<h2>Active Jobs</h2><table><thead><tr><th>Job</th><th>Shard</th><th>Document</th><th>Full Scan</
- Line 149: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<h2>Active Jobs</h2><table><thead><tr><th>Job</th><th>Shard</th><th>Document</th><th>Full Scan</
- Line 150: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<h2>Raw JSON</h2><pre id=\"raw\">loading...</pre>"
- Line 150: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<h2>Raw JSON</h2><pre id=\"raw\">loading...</pre>"
- Line 158: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "function badge(status){return `<span class=\"badge ${status}\">${status}</span>`;}"
- Line 158: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "function badge(status){return `<span class=\"badge ${status}\">${status}</span>`;}"
- Line 160: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "async function load(){const res=await fetch('/v1/admin/repair/health');const data=await res.json
- Line 160: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "async function load(){const res=await fetch('/v1/admin/repair/health');const data=await res.json
- Line 160: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "async function load(){const res=await fetch('/v1/admin/repair/health');const data=await res.json
- Line 160: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "async function load(){const res=await fetch('/v1/admin/repair/health');const data=await res.json
- Line 160: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "async function load(){const res=await fetch('/v1/admin/repair/health');const data=await res.json
- Line 160: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "async function load(){const res=await fetch('/v1/admin/repair/health');const data=await res.json
- Line 160: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "async function load(){const res=await fetch('/v1/admin/repair/health');const data=await res.json
- Line 160: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "async function load(){const res=await fetch('/v1/admin/repair/health');const data=await res.json
- Line 160: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "async function load(){const res=await fetch('/v1/admin/repair/health');const data=await res.json
- Line 160: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "async function load(){const res=await fetch('/v1/admin/repair/health');const data=await res.json
- Line 160: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "async function load(){const res=await fetch('/v1/admin/repair/health');const data=await res.json
- Line 160: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "async function load(){const res=await fetch('/v1/admin/repair/health');const data=await res.json
- Line 192: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 252: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: body["active_jobs"].push_back(repairJobToJson(job));
  Confidence: band=high; score=0.74
- Line 252: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: body["active_jobs"].push_back(repairJobToJson(job));
  Confidence: band=high; score=0.74
- Line 253: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: body["active_jobs"].push_back(repairJobToJson(job));
- Line 255: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: body["shards"].push_back(shardReportToJson(report));
  Confidence: band=high; score=0.74
- Line 256: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: body["shards"].push_back(shardReportToJson(report));
- Line 341: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: : http::status::ok;

### src/server/rpc/rpc_service_impl.cpp
Total findings: 50

- Line 3314: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
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
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #5399 fix(rpc): complete deadline... (2026-05-27) | #3449 [RPC] Implement Bat
- Line 642: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: cascade = nullptr;
  Context: "Request deadline exceeded during delete cascade scan"
- Line 653: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: them = nullptr;
  Context: " reference this entity. Use cascade=true to delete them."
- Line 672: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: cascade = nullptr;
  Context: "Request deadline exceeded during delete cascade traversal"
- Line 679: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t first_colon  = curr_key.find(':');
- Line 696: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: cascade = nullptr;
  Context: "Request deadline exceeded during delete cascade scan"
- Line 714: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: cascade = nullptr;
  Context: "Request deadline exceeded during delete cascade write"
- Line 720: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: child = nullptr;
  Context: "Failed to delete child entity during cascade: " + *it
- Line 730: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: write = nullptr;
  Context: "Request deadline exceeded during delete write"
- Line 736: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: entity = nullptr;
  Context: "Failed to delete entity from database"
- Line 918: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Set version: Client provides version in entity, or 0 for new entities
- Line 3265: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::min(backoff, remaining));
- Line 3283: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // for backward compatibility. In production, auth should always be configured.
  Confidence: band=high; score=0.8
- Line 123: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 642: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: "Request deadline exceeded during delete cascade scan"
- Line 662: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys_to_delete.push_back(child_key);
  Confidence: band=high; score=0.74
- Line 672: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: "Request deadline exceeded during delete cascade traversal"
- Line 696: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: "Request deadline exceeded during delete cascade scan"
- Line 700: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys_to_delete.push_back(gc_key);
  Confidence: band=high; score=0.74
- Line 803: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(key);
  Confidence: band=high; score=0.74
- Line 1029: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json ThemisRPCService::handleQuery(const json& params) {
  Confidence: band=high; score=0.74
- Line 1081: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: prefix += model + ":";
  Confidence: band=high; score=0.74
- Line 1128: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(entity);
  Confidence: band=high; score=0.74
- Line 1314: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json ThemisRPCService::handleGeoQuery(const json& params) {
  Confidence: band=high; score=0.74
- Line 1380: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto bbox_json = params["bbox"];
  Confidence: band=high; score=0.74
- Line 1420: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(result_obj);
  Confidence: band=high; score=0.74
- Line 1440: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto center = params["center"];
  Confidence: band=high; score=0.74
- Line 1543: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json ThemisRPCService::handleTimeSeriesQuery(const json& params) {
  Confidence: band=high; score=0.74
- Line 2037: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(entity);
  Confidence: band=high; score=0.74
- Line 2356: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json ThemisRPCService::handlePaginatedQuery(const json& params) {
  Confidence: band=high; score=0.74
- Line 2653: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered.push_back(doc);
  Confidence: band=high; score=0.74
- Line 2653: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered.push_back(doc);
  Confidence: band=high; score=0.74
- Line 2675: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: limited.push_back(results[i]);
  Confidence: band=high; score=0.74
- Line 2676: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: limited.push_back(results[i]);
- Line 2705: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: projected.push_back(proj_doc);
  Confidence: band=high; score=0.74
- Line 2786: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: collections_array.push_back({
  Confidence: band=high; score=0.74
- Line 2787: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: collections_array.push_back({
- Line 3024: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: models_array.push_back({
  Confidence: band=high; score=0.74
- Line 3025: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: models_array.push_back({
- Line 3242: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " (attempt " << attempt << "/" << max_attempts << "): " << e.what() << "\n";
- Line 3242: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " (attempt " << attempt << "/" << max_attempts << "): " << e.what() << "\n";
- Line 3243: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 3253: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " (attempt " << attempt << "/" << max_attempts << "): " << error_message << "\n";
- Line 3253: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " (attempt " << attempt << "/" << max_attempts << "): " << error_message << "\n";

### src/server/mqtt_client_service.cpp
Total findings: 49

- Line 257: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: asio::write(*asio_->ssl_stream, asio::buffer(pkt), ec);
- Line 266: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: asio::write(asio_->socket, asio::buffer(pkt), ec);
- Line 434: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: asio::write(*asio_->ssl_stream, asio::buffer(pkt), we);
- Line 438: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: asio::write(asio_->socket, asio::buffer(pkt), we);
- Line 57: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(len, size_t{4}); ++i) {
- Line 58: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: uint8_t byte = data[i];
- Line 179: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: static std::vector<uint8_t> buildDisconnect() { return {0xE0, 0x00}; }
- Line 254: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto pkt = detail::buildDisconnect();
- Line 397: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (ec) { scheduleReconnect(); return; }
- Line 403: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: asio_->socket.async_connect(
- Line 440: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (we) { scheduleReconnect(); return; }
- Line 453: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (ec) { handleDisconnect(ec.message()); return; }
- Line 467: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (ec) { handleDisconnect(ec.message()); return; }
- Line 521: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: handleDisconnect("broker sent DISCONNECT");
- Line 601: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (ec) { handleDisconnect(ec.message()); return; }
- Line 613: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (ec) { handleDisconnect(ec.message()); return; }
- Line 672: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: void MqttClientService::handleDisconnect(const std::string& reason) {
- Line 699: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (running_.load()) scheduleReconnect();
- Line 715: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: scheduleReconnect();
- Line 728: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (ec) { scheduleReconnect(); return; }
- Line 749: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (ec) { scheduleReconnect(); return; }
- Line 756: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (ec) { scheduleReconnect(); return; }
- Line 788: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: scheduleReconnect();
- Line 2: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: * ThemisDB | File: mqtt_client_service.cpp | Version: 0.0.12 | Last Modified: 2026-05-31 12:17:24
  Confidence: band=high; score=0.74
- Line 10: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "server/mqtt_client_service.h"
  Confidence: band=high; score=0.74
- Line 68: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.push_back(static_cast<uint8_t>(val >> 8));
  Confidence: band=high; score=0.74
- Line 69: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>(val >> 8));
- Line 70: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>(val & 0xFF));
- Line 150: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vheader.push_back(qos & 0x03);
  Confidence: band=high; score=0.74
- Line 151: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vheader.push_back(qos & 0x03);
- Line 170: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pkt.push_back(0xA2); // UNSUBSCRIBE
  Confidence: band=high; score=0.74
- Line 236: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void MqttClientService::start() {
  Confidence: band=medium; score=0.66
- Line 260: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: asio_->ssl_stream->lowest_layer().close(ec);
- Line 268: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: asio_->socket.close(ec);
- Line 274: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: asio_->ssl_stream->lowest_layer().close(ec);
- Line 280: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: asio_->socket.close(ec);
- Line 388: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: asio_->socket.close(ec);
- Line 551: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { h->onConnected(cid); } catch (...) {}
- Line 567: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { h->onMessage(topic, payload, qos); } catch (...) {}
- Line 679: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: asio_->ssl_stream->lowest_layer().close(ec);
- Line 685: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: asio_->socket.close(ec);
- Line 695: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { h->onDisconnected(reason); } catch (...) {}
- Line 714: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 775: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: SSL_set_tlsext_host_name(asio_->ssl_stream->native_handle(),
  Confidence: band=high; score=0.74
- Line 786: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: asio_->ssl_stream->lowest_layer().close(ce);
- Line 801: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: , topic_prefix_(service.getConfig().cdc_topic_prefix)
  Confidence: band=high; score=0.74
- Line 802: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: , qos_(service.getConfig().cdc_qos) {}
  Confidence: band=high; score=0.74
- Line 804: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: bool MqttCDCTransport::start() {
  Confidence: band=medium; score=0.66
- Line 819: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/http3_session.cpp
Total findings: 48

- Line 39: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: cid->data[i] = static_cast<uint8_t>(rd() & 0xFFu);
- Line 229: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: THEMIS_INFO("HTTP/3 rejecting new QUIC from {} (HTTP/2 fallback active)", client_ip);
- Line 234: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: THEMIS_INFO("HTTP/3 new QUIC connection from {}", session_key);
- Line 316: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 6 > array 0
  Remediation: Fix loop condition or increase array size
  Context: const uint8_t first = data[0];
- Line 321: severity=CRITICAL; category=array_bounds
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
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #5367 Resolve PR merge conflicts ... (2026-05-27) | #3291 [network] QUIC/HTTP
- Line 37: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cid->datalen = NGTCP2_MIN_CIDLEN;
- Line 38: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (size_t i = 0; i < cid->datalen; ++i) {
- Line 39: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cid->data[i] = static_cast<uint8_t>(rd() & 0xFFu);
- Line 155: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!running_.load(std::memory_order_acquire) || !socket_.is_open()) {
- Line 167: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (running_.load(std::memory_order_acquire) && socket_.is_open()) {
- Line 253: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!running_.load(std::memory_order_acquire)) {
- Line 262: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (ec || !running_.load(std::memory_order_acquire)) {
- Line 272: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (running_.load(std::memory_order_acquire)) {
- Line 838: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: vec[0].base = (uint8_t*)body_ptr->data();
- Line 839: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: vec[0].len = body_ptr->size();
- Line 58: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 135: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void Http3Handler::start() {
  Confidence: band=medium; score=0.66
- Line 165: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 246: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 268: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 386: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void Http3Session::start() {
  Confidence: band=medium; score=0.66
- Line 490: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 526: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 790: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> response_headers;
  Confidence: band=medium; score=0.66
- Line 801: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, std::string>& headers) {
  Confidence: band=medium; score=0.66
- Line 823: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nva.push_back(header);
  Confidence: band=high; score=0.74
- Line 896: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 928: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 947: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 964: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 980: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1000: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1024: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1119: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1156: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1176: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1193: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/entity_api_handler.cpp
Total findings: 32

- Line 155: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto authz = auth_->authorize(*token_opt, scope);
  Confidence: band=very_high; score=0.99
- Line 587: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: auto write_result = strategy->write(
- Line 880: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator pos may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto pos = key.find(':');
- Line 1136: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ct_it->value().find("application/x-ndjson") == std::string_view::npos) {
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #2800 [cdc] Change event enrichme... (2026-03-12) | #2726 [api] Batch operati
- Line 572: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: bool ok = storage_->put(prefixed_key, data);
- Line 737: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: hook = nullptr;
  Context: THEMIS_WARN("Geo index delete hook failed for {}:{}: {}", table, pk, e.what());
- Line 754: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: failed = nullptr;
  Context: "Index/Storage delete failed: " + st.message, req);
- Line 987: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: hook = nullptr;
  Context: THEMIS_WARN("Geo index delete hook failed for {}:{}: {}", vop.table, vop.pk, e.what());
- Line 108: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 142: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 233: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { entity_json = json::parse(blob_str); } catch (...) {
- Line 247: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto coll = schema["collections"][table];
  Confidence: band=high; score=0.74
- Line 252: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto& f : coll["encryption"]["fields"]) if (f.is_string()) fields.push_back(f.get<std::string>(
- Line 261: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { encFlag = entity_json[f + "_enc"].get<bool>(); } catch (...) {
- Line 267: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto enc_meta_str = entity_json[f + "_encrypted"].get<std::string>();
  Confidence: band=high; score=0.74
- Line 274: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { group_name = entity_json[f + "_group"].get<std::string>(); } catch (...) {
- Line 303: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 394: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto coll = schema["collections"][table];
  Confidence: band=high; score=0.74
- Line 406: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (f.is_string()) fields.push_back(f.get<std::string>());
  Confidence: band=high; score=0.74
- Line 407: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (f.is_string()) fields.push_back(f.get<std::string>());
- Line 850: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back({
  Confidence: band=high; score=0.74
- Line 882: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({
- Line 894: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({
- Line 956: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back({
  Confidence: band=high; score=0.74
- Line 957: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({
- Line 993: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({
- Line 1005: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({
- Line 1172: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: documents.push_back(json::parse(line));
- Line 1174: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({
- Line 1208: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back({
  Confidence: band=high; score=0.74
- Line 1209: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({

### src/server/policy_engine.cpp
Total findings: 32

- Line 203: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: "replaced all policies, new count=" + std::to_string(count));
- Line 243: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: PolicyEngine::Decision PolicyEngine::authorize(const std::string& user_id,
  Confidence: band=very_high; score=0.99
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #5123 docs(server): update VCCDB ... (2026-05-14) | #3154 [governance] Implem
- Line 212: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::length_error(
- Line 66: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& r : n["resources"]) p.resources.push_back(r.as<std::string>());
  Confidence: band=high; score=0.74
- Line 66: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& r : n["resources"]) p.resources.push_back(r.as<std::string>());
  Confidence: band=high; score=0.74
- Line 67: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& r : n["resources"]) p.resources.push_back(r.as<std::string>());
- Line 70: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto eff = n["effect"].as<std::string>("allow");
  Confidence: band=high; score=0.74
- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& ip : n["allowed_ip_prefixes"]) p.allowed_ip_prefixes.push_back(ip.as<std::string>());
  Confidence: band=high; score=0.74
- Line 76: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& ip : n["allowed_ip_prefixes"]) p.allowed_ip_prefixes.push_back(ip.as<std::string>()
- Line 80: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& ua : n["allowed_user_agent_patterns"]) p.allowed_user_agent_patterns.push_back(ua.as<std::string>());
  Confidence: band=high; score=0.74
- Line 81: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& ua : n["allowed_user_agent_patterns"]) p.allowed_user_agent_patterns.push_back(ua.a
- Line 84: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 91: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (p) loaded.push_back(std::move(*p));
  Confidence: band=high; score=0.74
- Line 91: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (p) loaded.push_back(std::move(*p));
  Confidence: band=high; score=0.74
- Line 98: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (p) loaded.push_back(std::move(*p));
  Confidence: band=high; score=0.74
- Line 116: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (p) loaded.push_back(std::move(*p));
  Confidence: band=high; score=0.74
- Line 121: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (p) loaded.push_back(std::move(*p));
  Confidence: band=high; score=0.74
- Line 134: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 148: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& p : list) out.push_back(toJson(p));
- Line 360: severity=MEDIUM; category=expensive_copy
  Description: Unnecessary expensive copy
  Remediation: Use const reference (const T&) or std::move if transfer is needed
  Context: if (user_agent->find(pat) != std::string::npos) { ok = true; break; }
- Line 371: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["subjects"] = json::array(); for (const auto& s : p.subjects) j["subjects"].push_back(s);
  Confidence: band=high; score=0.74
- Line 372: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["actions"] = json::array(); for (const auto& a : p.actions) j["actions"].push_back(a);
  Confidence: band=high; score=0.74
- Line 389: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (j.contains("resources")) for (const auto& r : j["resources"]) p.resources.push_back(r.get<std::string>());
  Confidence: band=high; score=0.74
- Line 389: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (j.contains("resources")) for (const auto& r : j["resources"]) p.resources.push_back(r.get<std::string>());
  Confidence: band=high; score=0.74
- Line 389: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (j.contains("resources")) for (const auto& r : j["resources"]) p.resources.push_back(r.get<std::string>());
  Confidence: band=high; score=0.74
- Line 390: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (j.contains("resources")) for (const auto& r : j["resources"]) p.resources.push_back(r.get<std::s
- Line 392: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (j.contains("allowed_ip_prefixes")) for (const auto& ip : j["allowed_ip_prefixes"]) p.allowed_ip_prefixes.push_back(ip.get<std::string>());
  Confidence: band=high; score=0.74
- Line 393: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (j.contains("allowed_ip_prefixes")) for (const auto& ip : j["allowed_ip_prefixes"]) p.allowed_ip_
- Line 395: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (j.contains("allowed_user_agent_patterns")) for (const auto& ua : j["allowed_user_agent_patterns"]) p.allowed_user_agent_patterns.push_back(ua.get<std::string>());
  Confidence: band=high; score=0.74
- Line 396: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (j.contains("allowed_user_agent_patterns")) for (const auto& ua : j["allowed_user_agent_patterns"
- Line 398: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/websocket_session.cpp
Total findings: 32

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4184 feat(cdc): WebSocket Change... (2026-03-13) | #3316 [WIP] Add WebSocket
- Line 94: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ws_tls_->async_accept(
- Line 107: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ws_plain_->async_accept(
- Line 131: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(welcome.dump());
- Line 201: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto responses = cdc_stream_handler_->handleFrame(msg);
- Line 203: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(resp.dump());
- Line 230: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(response.dump());
- Line 269: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(response.dump());
- Line 276: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(response.dump());
- Line 293: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(response.dump());
- Line 305: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(response.dump());
- Line 341: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(ws_resp.dump());
- Line 349: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(response.dump());
- Line 361: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(response.dump());
- Line 372: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(response.dump());
- Line 395: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(response.dump());
- Line 398: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: void WebSocketSession::send(const std::string& message) {
- Line 696: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!handler->hasSubscriptions()) continue;
- Line 698: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto frames = handler->pollEvents(*changefeed_);
- Line 702: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto redeliveries = handler->checkRedelivery();
- Line 760: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: session->send(cdc_message.dump());
- Line 819: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: session->send(message);
- Line 509: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ws_tls_->close(websocket::close_code::internal_error, close_ec);
- Line 511: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ws_plain_->close(websocket::close_code::internal_error, close_ec);
- Line 545: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ws_tls_->close(websocket::close_code::internal_error, close_ec);
- Line 547: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ws_plain_->close(websocket::close_code::internal_error, close_ec);
- Line 555: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void WebSocketSession::close() {
- Line 583: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ws_tls_->close(websocket::close_code::normal, ec);
- Line 585: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ws_plain_->close(websocket::close_code::normal, ec);
- Line 791: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cdc_sessions.push_back(session);
  Confidence: band=high; score=0.74
- Line 854: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: session->close();

### src/server/rope_api_handler.cpp
Total findings: 30

- Line 193: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto config_opt = vector_index_->getRotaryEmbeddingConfig();
- Line 810: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto config_opt = vector_index_->getRotaryEmbeddingConfig();
- Line 891: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: // auth_->authorize(); deny with HTTP 403 when the scope is not granted.
  Confidence: band=very_high; score=0.99
- Line 904: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto ar = auth_->authorize(*token, permission);
  Confidence: band=very_high; score=0.99
- Line 69: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: span.setAttribute("http.path", std::string(req.target()));
- Line 172: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: span.setAttribute("http.path", std::string(req.target()));
- Line 236: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: span.setAttribute("http.path", std::string(req.target()));
- Line 272: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: error = nullptr;
  Context: THEMIS_ERROR("RoPE config delete error: {}", e.what());
- Line 289: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: span.setAttribute("http.path", std::string(req.target()));
- Line 414: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: span.setAttribute("http.path", std::string(req.target()));
- Line 538: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: span.setAttribute("http.path", std::string(req.target()));
- Line 564: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: "Missing or invalid required field: query (must be array)", req);
- Line 652: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: span.setAttribute("http.path", std::string(req.target()));
- Line 790: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: span.setAttribute("http.path", std::string(req.target()));
- Line 130: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "Failed to apply RoPE configuration: " + status.message, req);
- Line 365: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vec.push_back(v.get<float>());
  Confidence: band=high; score=0.74
- Line 366: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vec.push_back(v.get<float>());
- Line 378: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "Failed to add entity with rotation: " + status.message, req);
- Line 489: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vec.push_back(v.get<float>());
  Confidence: band=high; score=0.74
- Line 490: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vec.push_back(v.get<float>());
- Line 502: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "Failed to add entity with relational rotation: " + status.message, req);
- Line 580: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: query_vector.push_back(val.get<float>());
  Confidence: band=high; score=0.74
- Line 581: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: query_vector.push_back(val.get<float>());
- Line 603: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "Search with rotation failed: " + status.message, req);
- Line 608: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results_array.push_back({
  Confidence: band=high; score=0.74
- Line 609: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results_array.push_back({
- Line 731: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vec.push_back(v.get<float>());
  Confidence: band=high; score=0.74
- Line 732: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vec.push_back(v.get<float>());
- Line 749: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 892: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/async_job_api_handler.cpp
Total findings: 29

- Line 565: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto status_result = registry_->requestCancel(job_id);
- Line 576: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return makeJsonResponse(http::status::conflict,
  Confidence: band=very_high; score=0.99
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4285 feat(server): Versioned API... (2026-03-17) | #2763 [api] Async job API
- Line 138: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 142: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = jobs_.begin(); it != jobs_.end(); ) {
  Confidence: band=very_high; score=0.9
- Line 144: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> rlock(rec.mu);
- Line 156: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: std::shared_ptr<AsyncJobRecord> AsyncJobRegistry::get(const std::string& id) const {
  Confidence: band=very_high; score=0.9
- Line 175: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = jobs_.find(id);
- Line 199: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = jobs_.find(id);
- Line 224: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 226: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = jobs_.begin(); it != jobs_.end(); ) {
  Confidence: band=very_high; score=0.9
- Line 228: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> rlock(rec.mu);
- Line 273: severity=HIGH; category=posix_only_api
  Description: POSIX-only API getpid( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: std::to_string(static_cast<long long>(::getpid())) +
- Line 293: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: f.wait_for(std::chrono::seconds(2));
- Line 358: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (job->cancel_requested.load(std::memory_order_acquire)) {
- Line 370: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (job->cancel_requested.load(std::memory_order_acquire)) {
- Line 428: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: f.wait_for(std::chrono::seconds(0)) ==
- Line 527: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: const std::string target(req.target());
  Confidence: band=very_high; score=0.9
- Line 553: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: const std::string target(req.target());
  Confidence: band=very_high; score=0.9
- Line 56: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool isValidAsyncQuery(std::string_view query) {
  Confidence: band=high; score=0.74
- Line 166: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(kv.second);
  Confidence: band=high; score=0.74
- Line 189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(job->toJson());
  Confidence: band=high; score=0.74
- Line 367: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::string final_status;
- Line 389: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::string final_status;
- Line 405: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 429: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::future_status::ready;
- Line 431: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures_.push_back(std::move(fut));
  Confidence: band=high; score=0.74
- Line 446: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_hdr = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 510: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: {{"job_id", job->id}, {"status", "pending"}}, req);

### src/server/lora_api_handler.cpp
Total findings: 29

- Line 342: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto& td = body->at("training_data");
- Line 458: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto& td = body->at("additional_training_data");
- Line 785: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: memory_mb = static_cast<double>(adapter_opt->memory_bytes) / (1024.0 * 1024.0);
- Line 935: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto handle   = inference_engine_->submit(eng_req);
- Line 302: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: model = nullptr;
  Context: "Failed to delete model",
- Line 530: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: adapter = nullptr;
  Context: "Failed to delete adapter: " + adapter_id
- Line 539: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: adapter = nullptr;
  Context: "Failed to delete adapter",
- Line 924: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: eng_req.base_request.model_id   = model_id.empty() ? "default" : model_id;
- Line 15: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "llm/lora_framework/lora_storage_service.h"
  Confidence: band=high; score=0.74
- Line 16: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "llm/lora_framework/lora_training_service.h"
  Confidence: band=high; score=0.74
- Line 248: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 259: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 560: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::string status_filter;
- Line 575: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 586: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 616: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered_adapters.push_back(adapter);
  Confidence: band=high; score=0.74
- Line 626: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adapters.push_back(filtered_adapters[i].toJSON());
  Confidence: band=high; score=0.74
- Line 627: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: adapters.push_back(filtered_adapters[i].toJSON());
- Line 855: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::string status_str;
- Line 1082: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1130: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1292: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto created_ns = metadata_json["created_at"].get<uint64_t>();
  Confidence: band=high; score=0.74
- Line 1300: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto updated_ns = metadata_json["updated_at"].get<uint64_t>();
  Confidence: band=high; score=0.74
- Line 1428: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: http::status::created);
- Line 1457: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(e.toJSON());
  Confidence: band=high; score=0.74
- Line 1458: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(e.toJSON());
- Line 1490: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(s.toJSON());
  Confidence: band=high; score=0.74
- Line 1491: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(s.toJSON());
- Line 1529: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: }, status);

### src/server/vector_api_handler.cpp
Total findings: 28

- Line 762: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: // Extract Bearer token and use auth_->authorize() to check the required
  Confidence: band=very_high; score=0.99
- Line 777: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto ar = auth_->authorize(*token, permission);
  Confidence: band=very_high; score=0.99
- Line 122: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: queryVector.push_back(val.get<float>());
  Confidence: band=high; score=0.74
- Line 123: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: queryVector.push_back(val.get<float>());
- Line 170: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 183: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "Vector search failed: " + status.message, req);
- Line 191: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back({{"pk", results[i].pk}, {"distance", results[i].distance}});
  Confidence: band=high; score=0.74
- Line 192: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: items.push_back({{"pk", results[i].pk}, {"distance", results[i].distance}});
- Line 207: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resultJson.push_back({{"pk", result.pk}, {"distance", result.distance}});
  Confidence: band=high; score=0.74
- Line 208: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: resultJson.push_back({{"pk", result.pk}, {"distance", result.distance}});
- Line 302: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto coll = schema_json["collections"][object_name];
  Confidence: band=high; score=0.74
- Line 304: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ecfg = coll["encryption"];
  Confidence: band=high; score=0.74
- Line 307: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& f : ecfg["fields"]) if (f.is_string()) vector_enc_fields.push_back(f.get<std::strin
- Line 312: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto itf = coll["fields"].begin(); itf != coll["fields"].end(); ++itf) {
  Confidence: band=high; score=0.74
- Line 314: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vector_enc_fields.push_back(itf.key());
  Confidence: band=high; score=0.74
- Line 314: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vector_enc_fields.push_back(itf.key());
  Confidence: band=high; score=0.74
- Line 315: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vector_enc_fields.push_back(itf.key());
- Line 317: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { /* ignore */ }
- Line 324: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 347: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vec.push_back(v.get<float>());
  Confidence: band=high; score=0.74
- Line 356: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto fit = it["fields"].begin(); fit != it["fields"].end(); ++fit) {
  Confidence: band=high; score=0.74
- Line 405: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 486: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 518: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "Failed to save index: " + status.message, req);
- Line 554: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "Failed to load index: " + status.message, req);
- Line 619: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "Failed to set efSearch: " + status.message, req);
- Line 765: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 790: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/mqtt_session.cpp
Total findings: 27

- Line 486: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 5 > array 0
  Remediation: Fix loop condition or increase array size
  Context: const uint8_t packetType = static_cast<uint8_t>(buffer_[0]);
- Line 786: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (auto session = sessionWeak.lock()) {
- Line 800: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (auto session = sessions[idx].lock()) {
- Line 820: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: auto session = weak_session.lock();
- Line 0: severity=HIGH; category=uncategorized
  Context: ['                            // Read packet ID (2 bytes)', '                            packetId = static_cast<uint16_t>(', '                                (static_cast<uint16_t>(static_cast<uint8_t>(buffer_[payloadOffset])) << 8) |', '                                static_cast<uint16_t>(static_cast<uint8_t>(buffer_[payloadOffset + 1]))', '                            );']
  Confidence: band=high; score=0.78
- Line 276: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: void MqttSession::handleDisconnect() {
- Line 496: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const uint8_t encodedByte = static_cast<uint8_t>(buffer_[i]);
- Line 624: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: handleDisconnect();
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['    // Build MQTT PUBLISH packet', '    std::vector<uint8_t> packet;', '    uint8_t flags = static_cast<uint8_t>(qos << 1);', '    if (retain) {', '        flags = static_cast<uint8_t>(flags | 0x01u);']
  Confidence: band=medium; score=0.65
- Line 41: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 51: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void MqttSession::start() {
  Confidence: band=medium; score=0.66
- Line 65: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: wsStream_->close(websocket::close_code::normal, ec);
- Line 68: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: socket_.close(ec);
- Line 312: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: packet.push_back(static_cast<uint8_t>(0x20u)); // CONNACK packet type
  Confidence: band=high; score=0.74
- Line 313: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packet.push_back(static_cast<uint8_t>(0x20u)); // CONNACK packet type
- Line 317: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packet.push_back(static_cast<uint8_t>(3u));    // Remaining length
- Line 318: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packet.push_back(static_cast<uint8_t>(sessionPresent ? 1u : 0u));
- Line 320: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packet.push_back(static_cast<uint8_t>(0u));    // Properties length
- Line 322: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packet.push_back(static_cast<uint8_t>(2u));    // Remaining length
- Line 323: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packet.push_back(static_cast<uint8_t>(sessionPresent ? 1u : 0u));
- Line 383: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packet.push_back(static_cast<uint8_t>((topicLen >> 8) & 0xFFu));
- Line 384: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packet.push_back(static_cast<uint8_t>(topicLen & 0xFFu));
- Line 391: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packet.push_back(static_cast<uint8_t>((packetId >> 8) & 0xFFu));
- Line 392: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packet.push_back(static_cast<uint8_t>(packetId & 0xFFu));
- Line 734: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(msg);
  Confidence: band=high; score=0.74
- Line 800: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: if (auto session = sessions[idx].lock()) {
  Confidence: band=high; score=0.74
- Line 817: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<MqttSession*> seen;
  Confidence: band=medium; score=0.66

### src/server/rate_limiter_v2.cpp
Total findings: 27

- Line 300: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: redis_pool_.pool_cv.wait(lk, [this]() {
- Line 498: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: THEMIS_WARN("PerClientRateLimiter: Max clients ({}) reached, rejecting new client: {}",
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
- Line 44: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: local tokens  = tonumber(data[1]) or capacity
- Line 45: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: local last_ms = tonumber(data[2]) or now_ms
- Line 120: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (config_.backend == Backend::REDIS && redis_healthy_.load(std::memory_order_acquire)) {
- Line 186: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: redis_healthy_.load(std::memory_order_acquire);
- Line 193: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [prio, bucket] : buckets_) {
  Confidence: band=very_high; score=0.9
- Line 194: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(bucket->mutex);
- Line 223: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (slot.ctx && !slot.ctx->err) return true;  // Already healthy.
- Line 228: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!slot.ctx || slot.ctx->err) {
- Line 231: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: slot.ctx ? slot.ctx->errstr : "null context");
- Line 321: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (slot.ctx && !slot.ctx->err && !config_.redis.auth.empty()) {
- Line 331: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (slot.ctx && !slot.ctx->err) {
- Line 372: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!slot.ctx || slot.ctx->err || !slot.script_loaded) return -1;
- Line 389: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!reply || slot.ctx->err) {
- Line 391: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: slot.ctx ? slot.ctx->errstr : "null context");
- Line 420: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (redis_healthy_.load(std::memory_order_acquire)) return;
- Line 458: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: std::memory_order_acquire)) {
- Line 522: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: lock.unlock(); // Unlock before trying to acquire tokens
- Line 270: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: redis_pool_.available.push_back(static_cast<size_t>(i));
  Confidence: band=high; score=0.74
- Line 271: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: redis_pool_.available.push_back(static_cast<size_t>(i));
- Line 272: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ++healthy;

### src/server/schema_api_handler.cpp
Total findings: 27

- Line 679: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& s : is.getStatistics(std::string_view(table_name))) {
- Line 136: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response["tables"].push_back(table_info);
  Confidence: band=high; score=0.74
- Line 666: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(col.toJSON());
  Confidence: band=high; score=0.74
- Line 667: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(col.toJSON());
- Line 679: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats_arr.push_back(s.toJSON());
  Confidence: band=high; score=0.74
- Line 680: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats_arr.push_back(s.toJSON());
- Line 683: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats_arr.push_back(s.toJSON());
  Confidence: band=high; score=0.74
- Line 684: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats_arr.push_back(s.toJSON());
- Line 827: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: c_arr.push_back(c.toJSON());
  Confidence: band=high; score=0.74
- Line 828: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: c_arr.push_back(c.toJSON());
- Line 910: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 971: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { return std::stoull(val); } catch (...) { return 0; }
- Line 1034: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rec_arr.push_back(r.toJSON());
  Confidence: band=high; score=0.74
- Line 1034: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rec_arr.push_back(r.toJSON());
  Confidence: band=high; score=0.74
- Line 1035: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rec_arr.push_back(r.toJSON());
- Line 1055: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rec_arr.push_back(r.toJSON());
  Confidence: band=high; score=0.74
- Line 1056: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rec_arr.push_back(r.toJSON());
- Line 1155: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back({{"error", "Table schema missing 'name' field"}});
  Confidence: band=high; score=0.74
- Line 1156: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({{"error", "Table schema missing 'name' field"}});
- Line 1162: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({{"table", schema.name},
- Line 1259: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: valid_rows.push_back({{"index", row_index}, {"row", row_json}});
  Confidence: band=high; score=0.74
- Line 1260: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: valid_rows.push_back({{"index", row_index}, {"row", row_json}});
- Line 1263: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& v : violations) viol_arr.push_back(v.toJSON());
- Line 1263: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: invalid_rows.push_back({{"index", row_index}, {"row", row_json},
  Confidence: band=high; score=0.74
- Line 1264: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: invalid_rows.push_back({{"index", row_index}, {"row", row_json},
- Line 1390: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entry.source_columns.push_back(
  Confidence: band=high; score=0.74
- Line 1391: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entry.source_columns.push_back(

### src/server/http2_session.cpp
Total findings: 25

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 202: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ssize_t readlen = nghttp2_session_mem_recv(ng2_session_, read_buffer_.data(), bytes_transferred);
- Line 214: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ssize_t datalen = nghttp2_session_mem_send(ng2_session_, &data);
- Line 248: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: const uint32_t timeout_ms = server_->hot_request_timeout_ms_.load(std::memory_order_acquire);
- Line 278: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: const uint32_t timeout_ms = server_->hot_request_timeout_ms_.load(std::memory_order_acquire);
- Line 372: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto it = self->response_buffers_.find(stream_id);
- Line 599: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(push_mutex_);
- Line 29: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: static const unsigned char alpn_proto_list[] = "\x02h2\x08http/1.1";
- Line 113: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void Http2Session::start() {
  Confidence: band=medium; score=0.66
- Line 265: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: self->stream_.lowest_layer().close(close_ec);
- Line 295: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: self->stream_.lowest_layer().close(close_ec);
- Line 531: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> response_headers;
  Confidence: band=medium; score=0.66
- Line 548: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, std::string>& headers) {
  Confidence: band=medium; score=0.66
- Line 552: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nva.push_back({
  Confidence: band=high; score=0.74
- Line 553: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nva.push_back({
- Line 562: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nva.push_back({
  Confidence: band=high; score=0.74
- Line 563: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nva.push_back({
- Line 605: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nva.push_back({
- Line 613: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nva.push_back({
- Line 621: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nva.push_back({
- Line 653: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response_nva.push_back({
- Line 668: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response_nva.push_back({
- Line 678: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response_nva.push_back({
  Confidence: band=high; score=0.74
- Line 679: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response_nva.push_back({

### src/server/replication_topology_api_handler.cpp
Total findings: 24

- Line 293: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Use std::unique_ptr<T> or std::make_unique<T>()
  Context: << "if(!t.ok)throw new Error('topology '+t.status);\n"
- Line 293: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: << "if(!t.ok)throw new Error('topology '+t.status);\n"
- Line 294: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Use std::unique_ptr<T> or std::make_unique<T>()
  Context: << "if(!h.ok)throw new Error('health '+h.status);\n"
- Line 294: severity=CRITICAL; category=smart_ptr_misuse
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
- Line 97: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: http::response<http::string_body> ReplicationTopologyApiHandler::handleTopologyGet(
  Confidence: band=very_high; score=0.9
- Line 172: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: http::response<http::string_body> ReplicationTopologyApiHandler::handleHealthGet(
  Confidence: band=very_high; score=0.9
- Line 225: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: http::response<http::string_body> ReplicationTopologyApiHandler::handleUiGet(
  Confidence: band=very_high; score=0.9
- Line 230: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: const std::string target{req.target()};
  Confidence: band=very_high; score=0.9
- Line 139: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(std::move(node));
  Confidence: band=high; score=0.74
- Line 146: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back({
  Confidence: band=high; score=0.74
- Line 280: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<title>Themis Replication Topology</title>\n"
- Line 284: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "#error{color:#b91c1c;margin:8px 0;display:none}</style></head>\n"
- Line 285: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<body><h1>Replication Topology</h1><div id=\"error\"></div>\n"
- Line 285: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<body><h1>Replication Topology</h1><div id=\"error\"></div>\n"
- Line 286: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<h2>Topology</h2><pre id=\"topology\">loading...</pre>\n"
- Line 286: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<h2>Topology</h2><pre id=\"topology\">loading...</pre>\n"
- Line 287: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<h2>Health</h2><pre id=\"health\">loading...</pre>\n"
- Line 287: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<h2>Health</h2><pre id=\"health\">loading...</pre>\n"
- Line 291: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "const t=await fetch(API_BASE+'/api/v1/replication/topology');\n"
- Line 292: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "const h=await fetch(API_BASE+'/api/v1/replication/health');\n"
- Line 299: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "load();setInterval(load,5000);</script></body></html>\n";

### src/server/changefeed_api_handler.cpp
Total findings: 23

- Line 1025: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto auth_result = auth_->authorize(*token, required_scope);
  Confidence: band=very_high; score=0.99
- Line 1120: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto auth_result = auth_->authorize(*token, required_scope);
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #2846 [cdc] GDPR-aware PII field ... (2026-03-12) | #2791 feat(cache): Adapti
- Line 203: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response["events"] = json::array();
- Line 538: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 204: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response["events"].push_back(event.toJson());
  Confidence: band=high; score=0.74
- Line 205: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response["events"].push_back(event.toJson());
- Line 314: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 330: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 350: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 370: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 406: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 420: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 849: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto v = body["max_age_hours"].get<uint32_t>();
  Confidence: band=high; score=0.74
- Line 857: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto v = body["max_event_count"].get<uint64_t>();
  Confidence: band=high; score=0.74
- Line 865: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto v = body["max_size_bytes"].get<uint64_t>();
  Confidence: band=high; score=0.74
- Line 873: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto v = body["cleanup_interval_minutes"].get<uint32_t>();
  Confidence: band=high; score=0.74
- Line 996: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 1055: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> headers_map;
  Confidence: band=medium; score=0.66
- Line 1091: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 1140: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> headers_map;
  Confidence: band=medium; score=0.66

### src/server/distributed_gateway.cpp
Total findings: 23

- Line 163: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: seen.insert(node.node_id);
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 273: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (node.has_value() && node->node_id != config_.node_id) {
- Line 279: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: key, node->node_id);
- Line 313: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: return future.get();
  Confidence: band=very_high; score=0.9
- Line 471: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: std::string key = std::string(req.target());
  Confidence: band=very_high; score=0.9
- Line 57: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: routes_json.push_back(r.toJson());
  Confidence: band=high; score=0.74
- Line 58: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: routes_json.push_back(r.toJson());
- Line 90: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cfg.routes.push_back(GatewayRouteConfig::fromJson(r));
  Confidence: band=high; score=0.74
- Line 91: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cfg.routes.push_back(GatewayRouteConfig::fromJson(r));
- Line 96: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = j["rate_limits"].begin(); it != j["rate_limits"].end(); ++it) {
  Confidence: band=high; score=0.74
- Line 135: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ConsistentHashRing::removeNode(const std::string& node_id)
  Context: void ConsistentHashRing::removeNode(const std::string& node_id) {
  Confidence: band=medium; score=0.56
- Line 159: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ConsistentHashRing::nodeCount()
  Context: std::size_t ConsistentHashRing::nodeCount() const {
  Confidence: band=medium; score=0.56
- Line 161: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 213: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void DistributedGateway::start() {
  Confidence: band=medium; score=0.66
- Line 364: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Stale entry – ignore (idempotent apply)
  Confidence: band=high; score=0.74
- Line 406: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back({
  Confidence: band=high; score=0.74
- Line 407: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nodes.push_back({
- Line 446: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: raft_cfg.cluster_members.push_back(n.node_id);
  Confidence: band=high; score=0.74
- Line 447: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: raft_cfg.cluster_members.push_back(n.node_id);
- Line 485: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto upgrade = req[http::field::upgrade];
  Confidence: band=high; score=0.74
- Line 496: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto accept = req[http::field::accept];
  Confidence: band=high; score=0.74

### src/server/graph_api_handler.cpp
Total findings: 23

- Line 73: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto [status, visited] = graph_index_->bfs(start_vertex, static_cast<int>(max_depth));
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #450 [REFACTOR] Extract GraphApi... (2026-03-11)
- Line 235: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: edge = nullptr;
  Context: "Failed to delete edge: " + status.message, req);
- Line 265: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: error = nullptr;
  Context: THEMIS_ERROR("Edge delete error: {}", e.what());
- Line 152: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "Failed to create edge: " + status.message, req);
- Line 235: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "Failed to delete edge: " + status.message, req);
- Line 380: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: body += "themis_graph_latency_ms_bucket{le=\"";
  Confidence: band=high; score=0.74
- Line 381: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: body += "themis_graph_latency_ms_bucket{le=\"";
- Line 383: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: body += "\"} ";
- Line 385: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: body += '\n';
- Line 581: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 813: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (v.is_string()) qc.forbidden_vertices.push_back(v.get<std::string>());
  Confidence: band=high; score=0.74
- Line 814: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (v.is_string()) qc.forbidden_vertices.push_back(v.get<std::string>());
- Line 818: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (v.is_string()) qc.required_vertices.push_back(v.get<std::string>());
  Confidence: band=high; score=0.74
- Line 819: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (v.is_string()) qc.required_vertices.push_back(v.get<std::string>());
- Line 823: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (v.is_string()) qc.node_labels.push_back(v.get<std::string>());
  Confidence: band=high; score=0.74
- Line 824: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (v.is_string()) qc.node_labels.push_back(v.get<std::string>());
- Line 862: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: alt_arr.push_back({{"algorithm", algo_name(alt_algo)}, {"estimated_cost", alt_cost}});
  Confidence: band=high; score=0.74
- Line 863: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: alt_arr.push_back({{"algorithm", algo_name(alt_algo)}, {"estimated_cost", alt_cost}});
- Line 867: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_arr.push_back(sid);
  Confidence: band=high; score=0.74
- Line 988: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (pv.is_string()) pverts.push_back(pv.get<std::string>());
  Confidence: band=high; score=0.74
- Line 989: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (pv.is_string()) pverts.push_back(pv.get<std::string>());
- Line 996: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pedges.emplace_back(pe[0].get<std::string>(), pe[1].get<std::string>());
  Confidence: band=high; score=0.74

### src/server/content_api_handler.cpp
Total findings: 21

- Line 70: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 134: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(static_cast<char>((val >> valb) & 0xFF));
  Confidence: band=high; score=0.74
- Line 234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(std::move(j));
  Confidence: band=high; score=0.74
- Line 263: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resp.push_back({{"pk", result.first}, {"score", result.second}});
  Confidence: band=high; score=0.74
- Line 264: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: resp.push_back({{"pk", result.first}, {"score", result.second}});
- Line 273: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 331: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vectorQuery.push_back(val.get<float>());
  Confidence: band=high; score=0.74
- Line 359: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> scores;
  Confidence: band=medium; score=0.66
- Line 373: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fusedResults.emplace_back(pk, score);
  Confidence: band=high; score=0.74
- Line 395: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> scores;
  Confidence: band=medium; score=0.66
- Line 412: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fusedResults.emplace_back(pk, score);
  Confidence: band=high; score=0.74
- Line 430: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resp.push_back({
  Confidence: band=high; score=0.74
- Line 457: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 503: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resp.push_back({
  Confidence: band=high; score=0.74
- Line 504: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: resp.push_back({
- Line 524: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 668: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 688: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 708: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 722: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = body["weights"].begin(); it != body["weights"].end(); ++it) {
  Confidence: band=high; score=0.74
- Line 734: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/api_gateway.cpp
Total findings: 18

- Line 48: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator first may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto first = s.find_first_not_of(" \t");
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4146 feat(server): API Versionin... (2026-03-13) | #2991 feat(api): Integrat
- Line 255: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw Error(static_cast<int>(ErrorCode::FeatureDisabled),
- Line 260: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw Error(static_cast<int>(ErrorCode::ConfigurationError),
- Line 287: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: stats["datacenter"] = config_.datacenter;
- Line 521: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (ctx && !ctx->user_id.empty()) {
- Line 522: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: client_id = ctx->user_id;  // Use JWT subject as client ID
- Line 898: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Check if endpoint is deprecated
  Confidence: band=high; score=0.8
- Line 909: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Add API-Deprecated header (issue-specified format: "v1.0 (remove YYYY-MM-DD)")
  Confidence: band=high; score=0.8
- Line 133: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 512: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 558: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 704: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response_body["results"].push_back(result.data);
  Confidence: band=high; score=0.74
- Line 705: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response_body["results"].push_back(result.data);
- Line 776: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> labels = {
  Confidence: band=high; score=0.74
- Line 946: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: R"(^/v(\d+(?:\.\d+){0,2})(?=/|$))"
- Line 964: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: R"(^/v\d+(?:\.\d+){0,2}(?=/|$))"

### src/server/auth_middleware.cpp
Total findings: 17

- Line 182: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: AuthMiddleware::AuthResult AuthMiddleware::authorize(std::string_view token, std::string_view required_scope) const {
  Confidence: band=very_high; score=0.99
- Line 612: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: auto claims = mtls_auth.authenticate(std::string(cert_pem));
  Confidence: band=very_high; score=0.99
- Line 169: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = role_scope_map_.find(role);
- Line 199: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // inputs (which would require zero-padding and may confuse static
  Confidence: band=very_high; score=0.9
- Line 306: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: std::find(it->second.begin(), it->second.end(), required_scope) != it->second.end()) {
- Line 316: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: std::find(it->second.begin(), it->second.end(), required_scope) != it->second.end()) {
- Line 540: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: std::find(it->second.begin(), it->second.end(), required_scope) != it->second.end()) {
- Line 158: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::string>> mapping)
  Confidence: band=medium; score=0.66
- Line 212: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!scopes_list.empty()) scopes_list += ",";
- Line 341: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> granted_scopes(claims.scopes.begin(),
  Confidence: band=medium; score=0.66
- Line 413: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 527: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) roles_str += ", ";
  Confidence: band=high; score=0.74
- Line 528: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) roles_str += ", ";
- Line 697: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::string>> mapping;
  Confidence: band=medium; score=0.66
- Line 702: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scopes.push_back(s.as<std::string>());
  Confidence: band=high; score=0.74
- Line 702: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scopes.push_back(s.as<std::string>());
  Confidence: band=high; score=0.74
- Line 703: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: scopes.push_back(s.as<std::string>());

### src/server/bpmn_api_handler.cpp
Total findings: 17

- Line 113: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: // auth_->authorize() which checks that the token contains the required scope.
  Confidence: band=very_high; score=0.99
- Line 114: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto ar = auth_->authorize(*token, scope);
  Confidence: band=very_high; score=0.99
- Line 188: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string process_key = request.value("process_definition_key", "");
- Line 189: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json variables = request.value("variables", json::object());
- Line 190: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string business_key = request.value("business_key", "");
- Line 307: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: json variables = request.value("variables", json::object());
- Line 478: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: const auto tsIt = token.visit_timestamps.find(node);
  Confidence: band=very_high; score=0.9
- Line 492: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response["history"] = json::array();
- Line 59: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 94: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 207: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "Failed to start process: " + status.message, req);
- Line 251: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_tasks.push_back(instance_id + ":" + token.current_node);
  Confidence: band=high; score=0.74
- Line 252: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: active_tasks.push_back(instance_id + ":" + token.current_node);
- Line 417: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "Process instance not found: " + status.message, req);
- Line 456: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_tasks.push_back(task);
  Confidence: band=high; score=0.74
- Line 486: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: history.push_back(event);
  Confidence: band=high; score=0.74
- Line 486: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: history.push_back(event);
  Confidence: band=high; score=0.74

### src/server/import_api_handler.cpp
Total findings: 17

- Line 310: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto job = registry_->getJsonSnapshot(job_id);
- Line 322: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto snapshot = registry_->getRunningAndJsonSnapshot(job_id);
- Line 338: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto updated = registry_->getJsonSnapshot(job_id);
- Line 345: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto jobs = registry_->allJsonSnapshots();
- Line 353: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto jobs = registry_->allJsonSnapshots();
- Line 489: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto source_path_opt = registry_->getSourcePathSnapshot(job_id);
- Line 195: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: jsonOk(res, handle->toJson());
- Line 248: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: jsonOk(res, handle->toJson());
- Line 303: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: jsonOk(res, handle->toJson());
- Line 439: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (t.is_string()) opts.include_tables.push_back(t.get<std::string>());
- Line 443: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (t.is_string()) opts.exclude_tables.push_back(t.get<std::string>());
- Line 589: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dst.push_back(entry.get<std::string>());
  Confidence: band=high; score=0.74
- Line 590: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dst.push_back(entry.get<std::string>());
- Line 603: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: warn_arr.push_back(message);
  Confidence: band=high; score=0.74
- Line 612: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: warn_arr.push_back("Circular reference detected: " + c.get<std::string>());
  Confidence: band=high; score=0.74
- Line 613: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: warn_arr.push_back("Circular reference detected: " + c.get<std::string>());
- Line 657: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: validated.push_back(entry);
  Confidence: band=high; score=0.74

### src/server/llm_grpc_service.cpp
Total findings: 16

- Line 546: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto* stats = response->mutable_cache_stats();
- Line 95: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const auto& metadata = context->client_metadata();
- Line 113: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const llm::InferenceRequest& pb_req,
  Confidence: band=very_high; score=0.9
- Line 114: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: ::themis::llm::InferenceRequest& internal_req) {
  Confidence: band=very_high; score=0.9
- Line 144: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: ::themis::llm::InferenceRequest internal_req;
  Confidence: band=very_high; score=0.9
- Line 179: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: ::themis::llm::InferenceRequest internal_req;
  Confidence: band=very_high; score=0.9
- Line 180: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: internal_req.prompt = request->query();
- Line 235: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: ::themis::llm::InferenceRequest internal_req;
  Confidence: band=very_high; score=0.9
- Line 522: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto* inference_stats = response->mutable_inference_stats();
  Confidence: band=very_high; score=0.9
- Line 2: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: * ThemisDB | File: llm_grpc_service.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
  Confidence: band=high; score=0.74
- Line 10: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "server/llm_grpc_service.h"
  Confidence: band=high; score=0.74
- Line 53: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: while (s.size() % 4) s += '=';
  Confidence: band=high; score=0.74
- Line 78: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto exp = claims["exp"].get<int64_t>();
  Confidence: band=high; score=0.74
- Line 148: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto internal_resp = plugin_mgr.generate(internal_req);
  Confidence: band=high; score=0.74
- Line 175: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rag_context.documents.push_back(internal_doc);
  Confidence: band=high; score=0.74
- Line 407: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: temp_file.close();

### src/server/profiling_api_handler.cpp
Total findings: 16

- Line 126: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(profile->toJSON());
  Confidence: band=high; score=0.74
- Line 127: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(profile->toJSON());
- Line 149: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(profile->toJSON());
  Confidence: band=high; score=0.74
- Line 150: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(profile->toJSON());
- Line 191: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: query_json.push_back(profile->toJSON());
  Confidence: band=high; score=0.74
- Line 192: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: query_json.push_back(profile->toJSON());
- Line 196: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: storage_json.push_back(stats.toJSON());
  Confidence: band=high; score=0.74
- Line 197: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: storage_json.push_back(stats.toJSON());
- Line 267: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto qp = body["query_profiler"];
  Confidence: band=high; score=0.74
- Line 273: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto threshold_ms = qp["slow_query_threshold_ms"].get<int>();
  Confidence: band=high; score=0.74
- Line 287: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto sp = body["storage_profiler"];
  Confidence: band=high; score=0.74
- Line 291: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto threshold_ms = sp["slow_op_threshold_ms"].get<int>();
  Confidence: band=high; score=0.74
- Line 305: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto an = body["analyzer"];
  Confidence: band=high; score=0.74
- Line 308: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto threshold_ms = an["slow_query_threshold_ms"].get<int>();
  Confidence: band=high; score=0.74
- Line 316: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto cache_hit_rate = an["cache_hit_rate_threshold"].get<double>();
  Confidence: band=high; score=0.74
- Line 404: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/health_error_service.cpp
Total findings: 15

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 159: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: http::read(socket, buffer, req, ec);
- Line 174: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: http::write(socket, error_res, ec);
- Line 182: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: http::write(socket, res, ec);
- Line 135: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
- Line 139: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 241: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: handler_req.query = parse_query(query_string);
- Line 244: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: error_handler_->handleGetErrors(handler_req, handler_res);
- Line 256: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: error_handler_->handleGetCategories(handler_req, handler_res);
- Line 266: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: handler_req.query = parse_query(query_string);
- Line 269: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: error_handler_->handleSearchErrors(handler_req, handler_res);
- Line 285: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: error_handler_->handleGetError(handler_req, handler_res);
- Line 2: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: * ThemisDB | File: health_error_service.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
  Confidence: band=high; score=0.74
- Line 10: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "server/health_error_service.h"
  Confidence: band=high; score=0.74
- Line 89: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: acceptor_->close(ec);

### src/server/saml_auth_provider.cpp
Total findings: 15

- Line 147: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: attrs[name].push_back(value);
  Confidence: band=high; score=0.74
- Line 248: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " index=\"1\"/>\n";
- Line 248: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " index=\"1\"/>\n";
- Line 253: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " Location=\"" << config_.sp_slo_url << "\"/>\n";
- Line 253: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " Location=\"" << config_.sp_slo_url << "\"/>\n";
- Line 256: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "  </md:SPSSODescriptor>\n";
- Line 260: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "    <md:OrganizationName xml:lang=\"en\">" << config_.org_name << "</md:OrganizationName>\n"
- Line 260: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "    <md:OrganizationName xml:lang=\"en\">" << config_.org_name << "</md:OrganizationName>\n"
- Line 262: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << config_.org_display_name << "</md:OrganizationDisplayName>\n"
- Line 263: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "    <md:OrganizationURL xml:lang=\"en\">" << config_.org_url << "</md:OrganizationURL>\n"
- Line 263: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "    <md:OrganizationURL xml:lang=\"en\">" << config_.org_url << "</md:OrganizationURL>\n"
- Line 264: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "  </md:Organization>\n";
- Line 269: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "    <md:EmailAddress>" << config_.contact_email << "</md:EmailAddress>\n"
- Line 270: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "  </md:ContactPerson>\n";
- Line 273: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "</md:EntityDescriptor>\n";

### src/server/tenant_manager.cpp
Total findings: 15

- Line 630: severity=CRITICAL; category=iterator_invalidation
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
- Line 204: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: default = nullptr;
  Context: THEMIS_WARN("TenantManager: Cannot delete default tenant");
- Line 465: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return it != usage_.end() ? it->second.get() : nullptr;
- Line 662: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Escape label value safely using a new string
- Line 262: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(config);
  Confidence: band=high; score=0.74
- Line 509: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void TenantManager::recordQuery(std::string_view tenant_id) {
  Confidence: band=high; score=0.74
- Line 666: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: tid += '\\';
  Confidence: band=high; score=0.74
- Line 666: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: tid += '\\';
  Confidence: band=high; score=0.74
- Line 667: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: tid += '\\';
- Line 667: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: tid += '\\';

### src/server/pki_api_handler.cpp
Total findings: 14

- Line 57: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(chars[(val>>valb)&0x3F]);
  Confidence: band=high; score=0.74
- Line 58: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(chars[(val>>valb)&0x3F]);
- Line 62: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (valb>-6) out.push_back(chars[((val<<8)>>(valb+8))&0x3F]);
- Line 63: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: while (out.size()%4) out.push_back('=');
- Line 77: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back((uint8_t)((val>>valb)&0xFF));
  Confidence: band=high; score=0.74
- Line 77: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back((uint8_t)((val>>valb)&0xFF));
  Confidence: band=high; score=0.74
- Line 78: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back((uint8_t)((val>>valb)&0xFF));
- Line 118: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: SigningResult res = signing_service.sign(data, key_id);
  Confidence: band=high; score=0.74
- Line 148: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto data_b64 = body["data_b64"].get<std::string>();
  Confidence: band=high; score=0.74
- Line 157: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: bool ok = signing_service.verify(data, sig, key_id);
  Confidence: band=high; score=0.74
- Line 230: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(std::move(jk));
  Confidence: band=high; score=0.74
- Line 390: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto qualified_sig = body["qualified_signature"];
  Confidence: band=high; score=0.74
- Line 462: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: certs_array.push_back(std::move(entry));
  Confidence: band=high; score=0.74
- Line 535: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/prompt_engineering_api_handler.cpp
Total findings: 14

- Line 25: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: #include "utils/input_validator.h"
  Confidence: band=very_high; score=0.99
- Line 41: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: themis::utils::InputValidator validator;
  Confidence: band=very_high; score=0.99
- Line 112: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: test.input = tc.value("input", "");
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 25: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: #include "utils/input_validator.h"
  Confidence: band=very_high; score=0.9
- Line 41: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: themis::utils::InputValidator validator;
  Confidence: band=very_high; score=0.9
- Line 112: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: test.input = tc.value("input", "");
  Confidence: band=very_high; score=0.9
- Line 116: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: test_cases.push_back(test);
  Confidence: band=high; score=0.74
- Line 168: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response.push_back(test.toJson());
  Confidence: band=high; score=0.74
- Line 169: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response.push_back(test.toJson());
- Line 378: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response.push_back(entry.toJson());
  Confidence: band=high; score=0.74
- Line 379: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response.push_back(entry.toJson());
- Line 427: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response.push_back(version.toJson());
  Confidence: band=high; score=0.74
- Line 428: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response.push_back(version.toJson());

### src/server/rpc/snapshot_transfer_handler.cpp
Total findings: 14

- Line 71: severity=CRITICAL; category=missing_dtor
  Description: Class SnapshotTransferHandler allocates resources but has no destructor
  Remediation: Add explicit destructor: ~SnapshotTransferHandler() { /* cleanup */ }
  Context: class/struct SnapshotTransferHandler
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 85: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: checkpoint_ = nullptr;
  Context: delete checkpoint_;
- Line 139: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::recursive_directory_iterator(snapshot_dir_)) {
- Line 419: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::recursive_directory_iterator(snapshot_dir_)) {
- Line 550: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: THEMIS_ERROR("LZ4: Failed to allocate memory: {}", e.what());
- Line 604: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: THEMIS_ERROR("Snappy: Failed to allocate memory: {}", e.what());
- Line 643: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: THEMIS_ERROR("LZ4: Failed to allocate memory: {}", e.what());
- Line 731: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::recursive_directory_iterator(snapshot_dir_)) {
- Line 760: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::recursive_directory_iterator(dir)) {
- Line 140: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: files.push_back(entry.path());
  Confidence: band=high; score=0.74
- Line 141: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: files.push_back(entry.path());
- Line 732: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: files.push_back(entry.path());
  Confidence: band=high; score=0.74
- Line 733: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: files.push_back(entry.path());

### src/server/wasm_handler_registry.cpp
Total findings: 14

- Line 129: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: const char* pos = std::find(kBase64Chars,
- Line 129: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: const char* pos = std::find(kBase64Chars,
- Line 147: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: const char* pos = std::find(kBase64Chars,
  Confidence: band=very_high; score=0.9
- Line 147: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: const char* pos = std::find(kBase64Chars,
- Line 137: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(char3[j]);
  Confidence: band=high; score=0.74
- Line 137: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(char3[j]);
  Confidence: band=high; score=0.74
- Line 138: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(char3[j]);
- Line 155: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(char3[j]);
  Confidence: band=high; score=0.74
- Line 155: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(char3[j]);
  Confidence: band=high; score=0.74
- Line 155: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(char3[j]);
  Confidence: band=high; score=0.74
- Line 156: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(char3[j]);
- Line 273: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry.toJson());
  Confidence: band=high; score=0.74
- Line 478: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: already_exists ? http::status::ok : http::status::created;
- Line 577: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/ethics_api_handler.cpp
Total findings: 13

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3632 fix(build): register 40+ mi... (2026-03-12) | #946 [FEATURE] Ethics AI
- Line 101: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto philosophies = body["philosophy_schools"].get<std::vector<std::string>>();
  Confidence: band=high; score=0.74
- Line 144: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto philosophies = body["philosophy_schools"].get<std::vector<std::string>>();
  Confidence: band=high; score=0.74
- Line 371: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto philosophies = body["philosophy_schools"].get<std::vector<std::string>>();
  Confidence: band=high; score=0.74
- Line 428: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: prom += "# TYPE " + prefix + " gauge\n";
  Confidence: band=high; score=0.74
- Line 517: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '\'') { escaped += "''"; } else { escaped += c; }
  Confidence: band=high; score=0.74
- Line 517: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '\'') { escaped += "''"; } else { escaped += c; }
  Confidence: band=high; score=0.74
- Line 518: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '\'') { escaped += "''"; } else { escaped += c; }
- Line 518: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '\'') { escaped += "''"; } else { escaped += c; }
- Line 551: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rows.push_back(nlohmann::json::parse(entity.toJson()));
  Confidence: band=high; score=0.74
- Line 552: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rows.push_back(nlohmann::json::parse(entity.toJson()));
- Line 558: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rows.push_back(nlohmann::json::parse(entity.toJson()));
  Confidence: band=high; score=0.74
- Line 559: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rows.push_back(nlohmann::json::parse(entity.toJson()));

### src/server/feedback_api_handler.cpp
Total findings: 13

- Line 121: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: auto stored = storage_service.createFeedback(feedback);
  Confidence: band=high; score=0.74
- Line 214: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: auto feedback_list = storage_service.listFeedback(filter);
  Confidence: band=high; score=0.74
- Line 221: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response["feedback"].push_back(fb.toJSON());
  Confidence: band=high; score=0.74
- Line 222: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response["feedback"].push_back(fb.toJSON());
- Line 273: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: auto feedback = storage_service.getFeedback(id);
  Confidence: band=high; score=0.74
- Line 336: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: bool success = storage_service.updateFeedback(id, feedback);
  Confidence: band=high; score=0.74
- Line 347: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: auto updated = storage_service.getFeedback(id);
  Confidence: band=high; score=0.74
- Line 389: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: bool success = storage_service.deleteFeedback(id);
  Confidence: band=high; score=0.74
- Line 459: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: auto feedback_list = storage_service.getFeedbackForAdapter(adapter_id, limit);
  Confidence: band=high; score=0.74
- Line 466: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response["feedback"].push_back(fb.toJSON());
  Confidence: band=high; score=0.74
- Line 467: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response["feedback"].push_back(fb.toJSON());
- Line 521: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: auto stats = storage_service.getStatistics(adapter_id);
  Confidence: band=high; score=0.74
- Line 579: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: llm::lora::FeedbackFilter FeedbackAPIHandler::parseFilterFromQuery(const std::string& query) const {
  Confidence: band=high; score=0.74

### src/server/index_api_handler.cpp
Total findings: 13

- Line 340: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: resp["indexes"] = stats_array;
- Line 74: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto configObj = body["config"];
  Confidence: band=high; score=0.74
- Line 79: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (s.is_string()) config.stopwords.push_back(s.get<std::string>());
  Confidence: band=high; score=0.74
- Line 80: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (s.is_string()) config.stopwords.push_back(s.get<std::string>());
- Line 119: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: columns.push_back(c.get<std::string>());
  Confidence: band=high; score=0.74
- Line 120: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: columns.push_back(c.get<std::string>());
- Line 254: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resp.push_back(stat_obj);
  Confidence: band=high; score=0.74
- Line 333: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats_array.push_back({
  Confidence: band=high; score=0.74
- Line 334: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats_array.push_back({
- Line 394: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response.push_back(suggestion.toJson());
  Confidence: band=high; score=0.74
- Line 395: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response.push_back(suggestion.toJson());
- Line 436: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response.push_back(pattern.toJson());
  Confidence: band=high; score=0.74
- Line 437: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: response.push_back(pattern.toJson());

### src/server/rpc/differential_update_engine.cpp
Total findings: 13

- Line 38: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: boundaries.push_back(i);
  Confidence: band=high; score=0.74
- Line 43: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: boundaries.push_back(data.size());  // End
- Line 102: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, uint32_t> base_hashes;
  Confidence: band=high; score=0.74
- Line 116: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.unchanged_chunks.push_back(chunk.index);
  Confidence: band=high; score=0.74
- Line 116: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.unchanged_chunks.push_back(chunk.index);
  Confidence: band=high; score=0.74
- Line 117: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.unchanged_chunks.push_back(chunk.index);
- Line 121: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.changed_chunks.push_back(chunk.index);
- Line 152: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<uint32_t, std::string> ExtractChunks(
  Confidence: band=high; score=0.74
- Line 156: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<uint32_t, std::string> chunks;
  Confidence: band=high; score=0.74
- Line 183: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: manifest.push_back(info);
  Confidence: band=high; score=0.74
- Line 197: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<uint32_t, const ChunkInfo*> by_index;
  Confidence: band=medium; score=0.66
- Line 228: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: manifest.push_back(info);
  Confidence: band=high; score=0.74
- Line 251: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: manifest.push_back(info);
  Confidence: band=high; score=0.74

### src/server/audit_api_handler.cpp
Total findings: 12

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 37: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: AuditLogEntry::toJson()
  Context: nlohmann::json AuditLogEntry::toJson() const {
  Confidence: band=medium; score=0.56
- Line 76: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ciphertext_b64 = payload["ciphertext_b64"].get<std::string>();
  Confidence: band=high; score=0.74
- Line 117: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 183: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 189: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort by timestamp descending (newest first)
  Confidence: band=high; score=0.74
- Line 210: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result["entries"].push_back(all_entries[i].toJson());
  Confidence: band=high; score=0.74
- Line 211: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result["entries"].push_back(all_entries[i].toJson());
- Line 238: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"') escaped += "\"\"";
  Confidence: band=high; score=0.74
- Line 238: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"') escaped += "\"\"";
  Confidence: band=high; score=0.74
- Line 239: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '"') escaped += "\"\"";

### src/server/ranger_adapter.cpp
Total findings: 12

- Line 118: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(backoff));
- Line 142: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: resource_prefixes.push_back(path["value"].get<std::string>());
- Line 145: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& v : path["values"]) if (v.is_string()) resource_prefixes.push_back(v.get<std::strin
- Line 148: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (resource_prefixes.empty()) resource_prefixes.push_back("/");
  Confidence: band=high; score=0.74
- Line 149: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (resource_prefixes.empty()) resource_prefixes.push_back("/");
- Line 172: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(std::move(p));
  Confidence: band=high; score=0.74
- Line 172: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(std::move(p));
  Confidence: band=high; score=0.74
- Line 172: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(std::move(p));
  Confidence: band=high; score=0.74
- Line 204: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& a : p.actions) accesses.push_back(json{{"type", a}, {"isAllowed", p.effect_allow}});
  Confidence: band=high; score=0.74
- Line 205: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& a : p.actions) accesses.push_back(json{{"type", a}, {"isAllowed", p.effect_allow}})
- Line 207: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& u : p.subjects) item["users"].push_back(u);
  Confidence: band=high; score=0.74
- Line 211: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(std::move(rp));
  Confidence: band=high; score=0.74

### src/server/geo_topology_api_handler.cpp
Total findings: 11

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 131: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: std::find(zones_arr.begin(), zones_arr.end(), s.zone) == zones_arr.end()) {
  Confidence: band=very_high; score=0.9
- Line 131: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: std::find(zones_arr.begin(), zones_arr.end(), s.zone) == zones_arr.end()) {
- Line 189: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (ratio == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 89: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(entry));
  Confidence: band=high; score=0.74
- Line 131: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: zones_arr.push_back(s.zone);
  Confidence: band=high; score=0.74
- Line 131: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: zones_arr.push_back(s.zone);
  Confidence: band=high; score=0.74
- Line 132: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: zones_arr.push_back(s.zone);
- Line 189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failed_regions.push_back(region);
  Confidence: band=high; score=0.74
- Line 192: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: degraded_regions.push_back({
- Line 204: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::string overall_status;

### src/server/policy_versioning_api_handler.cpp
Total findings: 11

- Line 374: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto auth_result = auth.authorize(*token, required_scope);
  Confidence: band=very_high; score=0.99
- Line 346: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward compatibility: If no auth configured or disabled, allow access but log a warning
  Confidence: band=high; score=0.8
- Line 77: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: json_array.push_back(version.toJson());
  Confidence: band=high; score=0.74
- Line 78: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: json_array.push_back(version.toJson());
- Line 264: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 276: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 285: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: json_array.push_back(entry.toJson());
  Confidence: band=high; score=0.74
- Line 286: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: json_array.push_back(entry.toJson());
- Line 321: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conflicts_arr.push_back(c.toJson());
  Confidence: band=high; score=0.74
- Line 322: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: conflicts_arr.push_back(c.toJson());
- Line 355: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/saga_api_handler.cpp
Total findings: 11

- Line 86: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["steps"] = nlohmann::json::array();
- Line 44: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64[(n >> 6) & 63]);
- Line 45: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back('=');
- Line 106: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["steps"].push_back(step_json);
  Confidence: band=high; score=0.74
- Line 147: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto sig = j["signature"];
  Confidence: band=high; score=0.74
- Line 156: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result["batches"].push_back(info.toJson());
  Confidence: band=high; score=0.74
- Line 180: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result["batches"].push_back(info.toJson());
- Line 225: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hash.push_back(byte.get<uint8_t>());
  Confidence: band=high; score=0.74
- Line 226: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: hash.push_back(byte.get<uint8_t>());
- Line 235: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/export_api_handler.cpp
Total findings: 10

- Line 113: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto* exporter = static_cast<exporters::IExporter*>(plugin->getInstance());
- Line 474: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 32 > array 0
  Remediation: Fix loop condition or increase array size
  Context: query = conditions[0];
- Line 113: severity=HIGH; category=unsafe_singleton
  Description: Singleton access without thread-safety mechanism
  Remediation: Protect with std::lock_guard or use Meyer singleton pattern
  Context: auto* exporter = static_cast<exporters::IExporter*>(plugin->getInstance());
- Line 208: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 244: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 262: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: exported_file.close();
- Line 384: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string ExportApiHandler::buildAqlQuery(const json& request_json) {
  Confidence: band=high; score=0.74
- Line 427: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: conditions.push_back("category='" + theme + "'");
- Line 476: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: query += " AND " + conditions[i];
- Line 505: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/spatial_api_handler.cpp
Total findings: 10

- Line 322: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response["gpu_backend"] = nullptr;
- Line 64: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto cfg = j["config"];
  Confidence: band=high; score=0.74
- Line 66: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto bounds = cfg["total_bounds"];
  Confidence: band=high; score=0.74
- Line 138: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "Failed to re-create spatial index: " + create_status.message, req);
- Line 163: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 173: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 179: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 321: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 354: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += ' ';
- Line 363: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::unordered_map<std::string, std::string> SpatialApiHandler::parseQuery(const std::string& target) {
  Confidence: band=high; score=0.74

### src/server/transaction_api_handler.cpp
Total findings: 10

- Line 154: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors_array.push_back({{"index", i}, {"error", "Missing 'type' field"}});
  Confidence: band=high; score=0.74
- Line 155: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors_array.push_back({{"index", i}, {"error", "Missing 'type' field"}});
- Line 159: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors_array.push_back({{"index", i}, {"error", "Missing 'table' field"}});
- Line 163: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors_array.push_back({{"index", i}, {"error", "Missing 'key' field"}});
- Line 184: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: TransactionManager::Status status;
- Line 556: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 571: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: locks_json.push_back({{"key", lock.key}, {"lock_type", lock.lock_type}});
  Confidence: band=high; score=0.74
- Line 572: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: locks_json.push_back({{"key", lock.key}, {"lock_type", lock.lock_type}});
- Line 576: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: write_set_json.push_back({{"key", entry.key}, {"operation", entry.operation}});
  Confidence: band=high; score=0.74
- Line 577: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: write_set_json.push_back({{"key", entry.key}, {"operation", entry.operation}});

### src/server/buffer_binary_protocol.cpp
Total findings: 9

- Line 55: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ts_buffer_->start();
- Line 64: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: vector_buffer_->start();
- Line 73: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: graph_buffer_->start();
- Line 155: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: point.value = data["value"].as<double>();
- Line 158: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto status = ts_buffer_->add(point);
- Line 192: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto status = ts_buffer_->add(point);
- Line 317: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto ts_stats = ts_buffer_->getStats();
- Line 318: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto vector_stats = vector_buffer_->getStats();
- Line 319: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto graph_stats = graph_buffer_->getStats();

### src/server/compliance_reporting_api_handler.cpp
Total findings: 9

- Line 313: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto auth_result = auth.authorize(*token, required_scope);
  Confidence: band=very_high; score=0.99
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3154 [governance] Implement comp... (2026-03-12) | #1075 Implement GAP-004 P
- Line 284: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward compatibility: If no auth configured or disabled, allow access but log a warning
  Confidence: band=high; score=0.8
- Line 56: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 118: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: json_array.push_back(gap.toJson());
  Confidence: band=high; score=0.74
- Line 119: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: json_array.push_back(gap.toJson());
- Line 198: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back(
  Confidence: band=high; score=0.74
- Line 199: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entries.push_back(
- Line 293: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/maintenance_api_handler.cpp
Total findings: 9

- Line 17: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: #include "utils/input_validator.h"
  Confidence: band=very_high; score=0.99
- Line 36: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: themis::utils::InputValidator validator;
  Confidence: band=very_high; score=0.99
- Line 17: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: #include "utils/input_validator.h"
  Confidence: band=very_high; score=0.9
- Line 36: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: themis::utils::InputValidator validator;
  Confidence: band=very_high; score=0.9
- Line 47: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: themis::utils::InputValidator validator;
  Confidence: band=very_high; score=0.9
- Line 114: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto& e : schedules) arr.push_back(scheduleToResponse(e));
- Line 186: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto& j : jobs) arr.push_back(jobToResponse(j));
- Line 252: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back({{"task_type", task_type}, {"handler", handler_name}});
  Confidence: band=high; score=0.74
- Line 253: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back({{"task_type", task_type}, {"handler", handler_name}});

### src/server/api_key_mgmt_handler.cpp
Total findings: 8

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 123: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (p.is_string()) permissions.push_back(p.get<std::string>());
  Confidence: band=high; score=0.74
- Line 124: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (p.is_string()) permissions.push_back(p.get<std::string>());
- Line 154: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(recordToJson(rec));
  Confidence: band=high; score=0.74
- Line 180: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: items.push_back(recordToJson(rec));
- Line 221: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (p.is_string()) rec.permissions.push_back(p.get<std::string>());
  Confidence: band=high; score=0.74
- Line 222: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (p.is_string()) rec.permissions.push_back(p.get<std::string>());

### src/server/graphql_api_handler.cpp
Total findings: 8

- Line 85: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: *   { "data": {...}, "errors": [...] }
- Line 183: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result_json["data"] = exec_result.data
- Line 61: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(serializeValue(item));
  Confidence: band=high; score=0.74
- Line 62: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(serializeValue(item));
- Line 117: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors_array.push_back({{"message", pe.toString()}});
  Confidence: band=high; score=0.74
- Line 118: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors_array.push_back({{"message", pe.toString()}});
- Line 189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors_array.push_back({
  Confidence: band=high; score=0.74
- Line 190: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors_array.push_back({

### src/server/wal_grpc_service.cpp
Total findings: 8

- Line 173: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: for (const auto& item : request.entries()) {
- Line 191: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!request.entries_compressed().empty()) {
- Line 192: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::vector<uint8_t> compressed(request.entries_compressed().begin(), request.entries_compressed().e
- Line 2: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: * ThemisDB | File: wal_grpc_service.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
  Confidence: band=high; score=0.74
- Line 10: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "server/wal_grpc_service.h"
  Confidence: band=high; score=0.74
- Line 160: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(std::move(e));
  Confidence: band=high; score=0.74
- Line 185: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(std::move(e));
  Confidence: band=high; score=0.74
- Line 258: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/chunked_response_writer.cpp
Total findings: 7

- Line 49: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: body += "0\r\n\r\n";
  Confidence: band=high; score=0.74
- Line 99: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: current_chunk += '\n';
  Confidence: band=high; score=0.74
- Line 103: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fragments.push_back(std::move(current_chunk));
  Confidence: band=high; score=0.74
- Line 152: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: chunk_data += '\n';
  Confidence: band=high; score=0.74
- Line 153: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: chunk_data += '\n';
- Line 155: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fragments.push_back(std::move(chunk_data));
  Confidence: band=high; score=0.74
- Line 182: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/session_api_handler.cpp
Total findings: 7

- Line 119: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto auth_result = auth_->authorize(bearer_token, "auth:sessions");
  Confidence: band=very_high; score=0.99
- Line 181: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto auth_result = auth_->authorize(bearer_token, "auth:sessions");
  Confidence: band=very_high; score=0.99
- Line 222: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto auth_result = auth_->authorize(bearer_token, "auth:sessions");
  Confidence: band=very_high; score=0.99
- Line 231: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: bool is_admin = auth_->authorize(bearer_token, "admin:all").authorized;
  Confidence: band=very_high; score=0.99
- Line 273: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto auth_result = auth_->authorize(bearer_token, "auth:sessions");
  Confidence: band=very_high; score=0.99
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #2811 [auth] Wire session revocat... (2026-03-12) | #2770 [auth] Implement se
- Line 193: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(std::move(j));
  Confidence: band=high; score=0.74

### src/server/timeseries_api_handler.cpp
Total findings: 7

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 142: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = ts_store.query(query_opts);
- Line 526: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metrics->updateStorageStats(stats.total_data_points, stats.total_metrics, stats.total_size_bytes);
- Line 164: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response["data"].push_back(point_json);
  Confidence: band=high; score=0.74
- Line 425: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: functions.push_back(name);
  Confidence: band=high; score=0.74
- Line 636: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch.push_back(std::move(dp));
  Confidence: band=high; score=0.74
- Line 636: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch.push_back(std::move(dp));
  Confidence: band=high; score=0.74

### src/server/buffer_api_handler.cpp
Total findings: 6

- Line 329: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response["buffers"]["ts_buffer"] = {{"enabled", false}};
- Line 344: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response["buffers"]["vector_buffer"] = {{"enabled", false}};
- Line 359: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response["buffers"]["graph_buffer"] = {{"enabled", false}};
- Line 205: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: status.message, req);
- Line 264: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: PropertyGraphManager::Status status;
- Line 277: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: status.message, req);

### src/server/cache_admin_api_handler.cpp
Total findings: 6

- Line 167: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto ar = auth_->authorize(*token, required_scope);
  Confidence: band=very_high; score=0.99
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4329 Implement SLO monitor laten... (2026-03-18) | #2789 [cache] Admin HTTP
- Line 102: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: output.push_back(static_cast<char>((val >> valb) & 0xFF));
  Confidence: band=high; score=0.74
- Line 102: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: output.push_back(static_cast<char>((val >> valb) & 0xFF));
  Confidence: band=high; score=0.74
- Line 103: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: output.push_back(static_cast<char>((val >> valb) & 0xFF));
- Line 153: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/oauth2_provider.cpp
Total findings: 6

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 33: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<std::string*>(userdata)->append(ptr, total);
- Line 84: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw auth::AuthException(auth::AuthError(
- Line 91: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw auth::AuthException(auth::AuthError(
- Line 98: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw auth::AuthException(auth::AuthError(

### src/server/policy_manager_api_handler.cpp
Total findings: 6

- Line 462: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto auth_result = auth.authorize(*token, required_scope);
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 433: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward compatibility: If no auth configured or disabled, allow access but log a warning
  Confidence: band=high; score=0.8
- Line 76: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: json_array.push_back(rule.toJson());
  Confidence: band=high; score=0.74
- Line 77: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: json_array.push_back(rule.toJson());
- Line 442: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/retention_api_handler.cpp
Total findings: 6

- Line 75: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (policy.name.find(filter.name_filter) == std::string::npos) {
- Line 88: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered.push_back(policy);
  Confidence: band=high; score=0.74
- Line 102: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(policyToJson(filtered[static_cast<size_t>(i)]));
  Confidence: band=high; score=0.74
- Line 103: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: items.push_back(policyToJson(filtered[static_cast<size_t>(i)]));
- Line 189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(actionToJson(action));
  Confidence: band=high; score=0.74
- Line 190: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: items.push_back(actionToJson(action));

### src/server/sse_connection_manager.cpp
Total findings: 6

- Line 325: severity=CRITICAL; category=missing_dtor
  Description: Class PollTarget allocates resources but has no destructor
  Remediation: Add explicit destructor: ~PollTarget() { /* cleanup */ }
  Context: class/struct PollTarget
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 93: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::unique_lock<std::shared_mutex> lock(connections_mutex_);
- Line 364: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Query new events since last sequence — without holding connections_mutex_.
- Line 389: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // new events to preserve the hard max_buffered_events bound.
- Line 352: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_conns.push_back(PollTarget{
  Confidence: band=high; score=0.74

### src/server/grpc_web_proxy_handler.cpp
Total findings: 5

- Line 172: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: stub_holder_    = std::make_shared<grpc::GenericStub>(channel);
- Line 298: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto* stub = static_cast<grpc::GenericStub*>(stub_holder_.get());
- Line 350: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto call = stub->PrepareUnaryCall(&ctx, method, request_buf, &cq);
- Line 324: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 346: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: grpc::Status status;

### src/server/opa_adapter.cpp
Total findings: 5

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #5123 docs(server): update VCCDB ... (2026-05-14) | #3076 feat(governance): I
- Line 23: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: buf->append(static_cast<char*>(ptr), size * nmemb);
- Line 28: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void ensure_curl_global_init() {
  Confidence: band=medium; score=0.66
- Line 91: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/policy_template_api_handler.cpp
Total findings: 5

- Line 241: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto auth_result = auth.authorize(*token, required_scope);
  Confidence: band=very_high; score=0.99
- Line 213: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward compatibility: If no auth configured or disabled, allow access but log a warning
  Confidence: band=high; score=0.8
- Line 54: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: json_array.push_back(tmpl->toJson());
  Confidence: band=high; score=0.74
- Line 55: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: json_array.push_back(tmpl->toJson());
- Line 222: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/policy_validation_api_handler.cpp
Total findings: 5

- Line 179: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto auth_result = auth.authorize(*token, required_scope);
  Confidence: band=very_high; score=0.99
- Line 150: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward compatibility: If no auth configured or disabled, allow access but log a warning
  Confidence: band=high; score=0.8
- Line 129: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: json_array.push_back(metric.toJson());
  Confidence: band=high; score=0.74
- Line 130: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: json_array.push_back(metric.toJson());
- Line 159: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/response_transformer.cpp
Total findings: 5

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 114: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(k);
  Confidence: band=high; score=0.74
- Line 119: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(k);
  Confidence: band=high; score=0.74
- Line 124: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(k);
  Confidence: band=high; score=0.74

### src/server/review_scheduling_api_handler.cpp
Total findings: 5

- Line 239: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto auth_result = auth.authorize(*token, required_scope);
  Confidence: band=very_high; score=0.99
- Line 211: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward compatibility: If no auth configured or disabled, allow access but log a warning
  Confidence: band=high; score=0.8
- Line 47: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: json_array.push_back(review.toJson());
  Confidence: band=high; score=0.74
- Line 48: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: json_array.push_back(review.toJson());
- Line 220: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/smart_routing.cpp
Total findings: 5

- Line 202: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (sc < cc || (sc == cc && s->cached_avg_latency < chosen->cached_avg_latency)) {
- Line 202: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (sc < cc || (sc == cc && s->cached_avg_latency < chosen->cached_avg_latency)) {
- Line 59: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(state.endpoint);
  Confidence: band=high; score=0.74
- Line 167: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(&state);
  Confidence: band=high; score=0.74
- Line 294: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(bs);
  Confidence: band=high; score=0.74

### src/server/snapshot_api_handler.cpp
Total findings: 5

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #385 Phase 1 & 2: Implement Name... (2026-03-11) | #384 [WIP] Add Named Snaps
- Line 93: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 101: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: sort_by = "timestamp";
  Confidence: band=high; score=0.74
- Line 115: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(snapshot.toJson());
  Confidence: band=high; score=0.74
- Line 116: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(snapshot.toJson());

### src/server/branch_api_handler.cpp
Total findings: 4

- Line 229: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: branch = nullptr;
  Context: sendError(res, 400, "Failed to delete branch. Branch may be active or not fully merged.");
- Line 146: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(branch.toJson());
  Confidence: band=high; score=0.74
- Line 147: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(branch.toJson());
- Line 323: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resolutions.push_back(std::move(res_item));
  Confidence: band=high; score=0.74

### src/server/cdn_cache_middleware.cpp
Total findings: 4

- Line 220: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: std::string_view inm  = inm_it->value();
- Line 221: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: std::string_view etag = etag_it->value();
- Line 81: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "W/\"" << std::hex << std::setfill('0') << std::setw(16) << h << "\"";
- Line 81: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "W/\"" << std::hex << std::setfill('0') << std::setw(16) << h << "\"";

### src/server/classification_api_handler.cpp
Total findings: 4

- Line 49: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back({
  Confidence: band=high; score=0.74
- Line 50: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: items.push_back({
- Line 111: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: detected_entities.push_back({
  Confidence: band=high; score=0.74
- Line 112: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: detected_entities.push_back({

### src/server/distributed_txn_api_handler.cpp
Total findings: 4

- Line 308: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: std::string_view path  = req.target();
  Confidence: band=very_high; score=0.9
- Line 94: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_ids.push_back(std::move(shard_id));
  Confidence: band=high; score=0.74
- Line 162: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto        op      = body["operation"];
  Confidence: band=high; score=0.74
- Line 280: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_ids.push_back(std::move(shard_id));
  Confidence: band=high; score=0.74

### src/server/error_api_handler.cpp
Total findings: 4

- Line 36: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors_json.push_back(error.toJSON());
  Confidence: band=high; score=0.74
- Line 37: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors_json.push_back(error.toJSON());
- Line 136: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors_json.push_back(error.toJSON());
  Confidence: band=high; score=0.74
- Line 137: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors_json.push_back(error.toJSON());

### src/server/openapi_route_registry.cpp
Total findings: 4

- Line 197: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto tag_description = [](const std::string& t) -> std::string {
- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: params.push_back(std::move(param));
  Confidence: band=high; score=0.74
- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: params.push_back(std::move(param));
  Confidence: band=high; score=0.74
- Line 164: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: {"example","<https://docs.themisdb.com/migration/v1-to-v2>; rel=\"deprecation\""}}}

### src/server/pii_api_handler.cpp
Total findings: 4

- Line 97: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 145: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out_items.push_back(j);
  Confidence: band=high; score=0.74
- Line 150: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 162: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: csv += r.value("original_uuid", ""); csv += ",";
  Confidence: band=high; score=0.74

### src/server/prompt_engineering_grpc_service.cpp
Total findings: 4

- Line 2: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: * ThemisDB | File: prompt_engineering_grpc_service.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
  Confidence: band=high; score=0.74
- Line 11: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: * @file prompt_engineering_grpc_service.cpp
  Confidence: band=high; score=0.74
- Line 32: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "server/prompt_engineering_grpc_service.h"
  Confidence: band=high; score=0.74
- Line 75: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/rate_limiter.cpp
Total findings: 4

- Line 191: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: THEMIS_DEBUG("Created new rate limit bucket: key={}, capacity={}, rate={}/min",
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 28: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: TokenBucket::refill()
  Context: void TokenBucket::refill() {
  Confidence: band=medium; score=0.56
- Line 338: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RateLimiter::reset()
  Context: void RateLimiter::reset() {
  Confidence: band=medium; score=0.56

### src/server/reports_api_handler.cpp
Total findings: 4

- Line 46: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto lvl = j["level"].get<std::string>();
  Confidence: band=high; score=0.74
- Line 59: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto ts = j["ts"].get<std::string>();
  Confidence: band=high; score=0.74
- Line 63: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto ts = j["timestamp"].get<std::string>();
  Confidence: band=high; score=0.74
- Line 67: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/rpc/blob_transfer_handler.cpp
Total findings: 4

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #970 [P1] Implement checkpoint/r... (2026-03-11) | #104 RPC Framework with gR
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['                for (int j = 0; j < 8; ++j) {', '                    const uint32_t mask = (crc & 1u) ? 0xFFFFFFFFu : 0u;', '                    crc = (crc >> 1) ^ (0xEDB88320u & mask);', '                }', '            }']
  Confidence: band=medium; score=0.62
- Line 259: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: output_file_.close();
- Line 397: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/serverless_function_api_handler.cpp
Total findings: 4

- Line 367: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(fn.toJson());
  Confidence: band=high; score=0.74
- Line 368: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(fn.toJson());
- Line 600: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(snap.toJson());
  Confidence: band=high; score=0.74
- Line 601: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(snap.toJson());

### src/server/themis_core_grpc_service.cpp
Total findings: 4

- Line 2: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: * ThemisDB | File: themis_core_grpc_service.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
  Confidence: band=high; score=0.74
- Line 10: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "server/themis_core_grpc_service.h"
  Confidence: band=high; score=0.74
- Line 24: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: // path.  This mirrors the pattern used by WalGrpcService / wal_grpc_service.cpp.
  Confidence: band=high; score=0.74
- Line 143: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/diff_api_handler.cpp
Total findings: 3

- Line 148: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 158: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 176: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/pitr_api_handler.cpp
Total findings: 3

- Line 62: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: PITRManager::Status status;
- Line 208: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: options.tables.push_back(table.get<std::string>());
  Confidence: band=high; score=0.74
- Line 209: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: options.tables.push_back(table.get<std::string>());

### src/server/update_api_handler.cpp
Total findings: 3

- Line 103: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: config_json["is_running"] = checker_->isRunning();
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73

### src/server/wal_api_handler.cpp
Total findings: 3

- Line 54: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (hdr == req.end() || hdr->value() != wal_shared_secret_) {
- Line 118: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back(std::move(e));
  Confidence: band=high; score=0.74
- Line 203: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hex.push_back(hex_digits[(result[i] >> 4) & 0x0F]);
  Confidence: band=high; score=0.74

### src/server/api_security_audit.cpp
Total findings: 2

- Line 53: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: findings.push_back({severity, endpoint_pattern, http_method, finding, recommendation});
  Confidence: band=high; score=0.74
- Line 54: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: findings.push_back({severity, endpoint_pattern, http_method, finding, recommendation});

### src/server/cache_api_handler.cpp
Total findings: 2

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #446 [REFACTOR] Extract Cache Op... (2026-03-11)
- Line 53: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = semantic_cache.query(prompt, params);

### src/server/continuous_query_api_handler.cpp
Total findings: 2

- Line 234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(infoToJson(info));
  Confidence: band=high; score=0.74
- Line 235: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(infoToJson(info));

### src/server/http3_datagram.cpp
Total findings: 2

- Line 0: severity=HIGH; category=uncategorized
  Context: ['', '    const uint8_t* payload    = data + consumed;', '    const size_t   payload_len = len - consumed;', '', '    // Look up and invoke handler.']
  Confidence: band=high; score=0.81
- Line 99: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(contexts_mutex_);

### src/server/http_type_adapter.cpp
Total findings: 2

- Line 42: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += ' ';
  Confidence: band=high; score=0.74
- Line 43: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += ' ';

### src/server/mvcc_api_handler.cpp
Total findings: 2

- Line 242: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: response["versions"] = std::move(versions_array);
- Line 123: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/server/pitr_grpc_service.cpp
Total findings: 2

- Line 2: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: * ThemisDB | File: pitr_grpc_service.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
  Confidence: band=high; score=0.74
- Line 10: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "server/pitr_grpc_service.h"
  Confidence: band=high; score=0.74

### src/server/prompt_api_handler.cpp
Total findings: 2

- Line 76: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(t.toJson());
  Confidence: band=high; score=0.74
- Line 77: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(t.toJson());

### src/server/sharding_metrics_handler.cpp
Total findings: 2

- Line 78: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: oss << "# HELP themisdb_slo_error_budget Remaining error budget (0-1)\n";
  Confidence: band=very_high; score=0.9
- Line 102: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: double global_error_budget = slo_monitor.getGlobalErrorBudget();
  Confidence: band=very_high; score=0.9

### src/server/udf_api_handler.cpp
Total findings: 2

- Line 174: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(d.toJson());
  Confidence: band=high; score=0.74
- Line 175: severity=MEDIUM; category=copy_overhead
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

### include/server/examples/workload_fingerprint_example.cpp
Total findings: 1

- Line 196: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "\nSee docs/issues/optimization_layers/IMPL-B8-workload-fingerprint.md\n";

### src/server/adaptive_rate_limiter.cpp
Total findings: 1

- Line 188: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: latencies.push_back(s.latency_ms.count());
  Confidence: band=high; score=0.74

### src/server/api_auth_config.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #5123 docs(server): update VCCDB ... (2026-05-14) | #3104 feat(api): Implemen

### src/server/api_version.cpp
Total findings: 1

- Line 105: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Support current major version and previous major version for backward compatibility
  Confidence: band=high; score=0.8

### src/server/cost_based_rate_limiter.cpp
Total findings: 1

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73

### src/server/hot_reload_api_handler.cpp
Total findings: 1

- Line 248: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response_json.push_back(point);
  Confidence: band=high; score=0.74

### src/server/merge_api_handler.cpp
Total findings: 1

- Line 215: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: options.manual_resolutions.push_back(resolution);
  Confidence: band=high; score=0.74

### src/server/policy_api_handler.cpp
Total findings: 1

- Line 47: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto jsonOpt = ranger_client.fetchPolicies(&err);

### src/server/rate_limiting_middleware.cpp
Total findings: 1

- Line 61: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: override_limiters_.push_back(
  Confidence: band=high; score=0.74

### src/server/request_coalescing.cpp
Total findings: 1

- Line 105: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
