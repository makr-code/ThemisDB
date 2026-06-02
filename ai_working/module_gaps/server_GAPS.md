# server Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: server
- Generated: 2026-06-02 11:09:13
- Status: Critical Findings Present
- Total Findings: 805
- Actionable Findings (Critical + High): 184
- Affected Files: 115

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 63 |
| High | 121 |
| Medium | 616 |
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
| src/server/llm_api_handler.cpp | 92 | 11 | 65 | 16 | 0 |
| src/server/http_server.cpp | 65 | 17 | 1 | 47 | 0 |
| src/server/query_api_handler.cpp | 64 | 0 | 7 | 57 | 0 |
| src/server/postgres_session.cpp | 37 | 0 | 2 | 35 | 0 |
| src/server/monitoring_api_handler.cpp | 24 | 0 | 0 | 24 | 0 |
| src/server/mcp_server.cpp | 23 | 0 | 1 | 22 | 0 |
| src/server/rpc/rpc_service_impl.cpp | 21 | 1 | 1 | 19 | 0 |
| src/server/policy_engine.cpp | 18 | 1 | 0 | 17 | 0 |
| src/server/task_scheduler_api_handler.cpp | 18 | 0 | 0 | 13 | 5 |
| src/server/distributed_gateway.cpp | 15 | 1 | 2 | 12 | 0 |
| src/server/voice_api_handler.cpp | 15 | 0 | 0 | 15 | 0 |
| src/server/vector_api_handler.cpp | 14 | 2 | 0 | 12 | 0 |
| src/server/content_api_handler.cpp | 12 | 0 | 0 | 12 | 0 |
| src/server/llm_grpc_service.cpp | 12 | 0 | 6 | 6 | 0 |
| src/server/schema_api_handler.cpp | 12 | 0 | 0 | 12 | 0 |
| src/server/async_job_api_handler.cpp | 11 | 1 | 5 | 5 | 0 |
| src/server/changefeed_api_handler.cpp | 11 | 2 | 0 | 9 | 0 |
| src/server/feedback_api_handler.cpp | 11 | 0 | 0 | 11 | 0 |
| src/server/profiling_api_handler.cpp | 11 | 0 | 0 | 11 | 0 |
| src/server/entity_api_handler.cpp | 10 | 1 | 0 | 9 | 0 |
| src/server/mqtt_client_service.cpp | 10 | 0 | 0 | 10 | 0 |
| src/server/prompt_engineering_api_handler.cpp | 10 | 3 | 3 | 4 | 0 |
| src/server/rpc/differential_update_engine.cpp | 10 | 0 | 0 | 10 | 0 |
| src/server/shard_repair_api_handler.cpp | 10 | 3 | 3 | 4 | 0 |
| src/server/auth_middleware.cpp | 9 | 2 | 1 | 6 | 0 |
| src/server/bpmn_api_handler.cpp | 9 | 2 | 1 | 6 | 0 |
| src/server/pki_api_handler.cpp | 9 | 0 | 0 | 9 | 0 |
| src/server/ethics_api_handler.cpp | 8 | 0 | 0 | 8 | 0 |
| src/server/graph_api_handler.cpp | 8 | 0 | 0 | 8 | 0 |
| src/server/lora_api_handler.cpp | 8 | 0 | 0 | 8 | 0 |
| src/server/rope_api_handler.cpp | 8 | 2 | 0 | 6 | 0 |
| src/server/api_gateway.cpp | 7 | 0 | 2 | 5 | 0 |
| src/server/index_api_handler.cpp | 7 | 0 | 0 | 7 | 0 |
| src/server/ranger_adapter.cpp | 7 | 0 | 0 | 7 | 0 |
| src/server/wasm_handler_registry.cpp | 7 | 0 | 1 | 6 | 0 |
| src/server/audit_api_handler.cpp | 6 | 0 | 0 | 6 | 0 |
| src/server/geo_topology_api_handler.cpp | 6 | 0 | 2 | 4 | 0 |
| src/server/http2_session.cpp | 6 | 0 | 0 | 6 | 0 |
| src/server/maintenance_api_handler.cpp | 6 | 2 | 3 | 1 | 0 |
| src/server/policy_versioning_api_handler.cpp | 6 | 1 | 1 | 4 | 0 |
| src/server/replication_topology_api_handler.cpp | 6 | 0 | 4 | 2 | 0 |
| src/server/session_api_handler.cpp | 6 | 5 | 0 | 1 | 0 |
| src/server/chunked_response_writer.cpp | 5 | 0 | 0 | 5 | 0 |
| src/server/compliance_reporting_api_handler.cpp | 5 | 1 | 1 | 3 | 0 |
| src/server/http3_session.cpp | 5 | 0 | 0 | 5 | 0 |
| src/server/mqtt_session.cpp | 5 | 0 | 0 | 5 | 0 |
| src/server/cache_admin_api_handler.cpp | 4 | 1 | 0 | 3 | 0 |
| src/server/distributed_txn_api_handler.cpp | 4 | 0 | 1 | 3 | 0 |
| src/server/import_api_handler.cpp | 4 | 0 | 0 | 4 | 0 |
| src/server/policy_manager_api_handler.cpp | 4 | 1 | 1 | 2 | 0 |
| src/server/policy_template_api_handler.cpp | 4 | 1 | 1 | 2 | 0 |
| src/server/policy_validation_api_handler.cpp | 4 | 1 | 1 | 2 | 0 |
| src/server/review_scheduling_api_handler.cpp | 4 | 1 | 1 | 2 | 0 |
| src/server/saga_api_handler.cpp | 4 | 0 | 0 | 4 | 0 |
| src/server/tenant_manager.cpp | 4 | 0 | 0 | 4 | 0 |
| src/server/timeseries_api_handler.cpp | 4 | 0 | 0 | 4 | 0 |
| src/server/wal_grpc_service.cpp | 4 | 0 | 0 | 4 | 0 |
| src/server/api_key_mgmt_handler.cpp | 3 | 0 | 0 | 3 | 0 |
| src/server/graphql_api_handler.cpp | 3 | 0 | 0 | 3 | 0 |
| src/server/prompt_engineering_grpc_service.cpp | 3 | 0 | 0 | 3 | 0 |
| src/server/reports_api_handler.cpp | 3 | 0 | 0 | 3 | 0 |
| src/server/response_transformer.cpp | 3 | 0 | 0 | 3 | 0 |
| src/server/retention_api_handler.cpp | 3 | 0 | 0 | 3 | 0 |
| src/server/smart_routing.cpp | 3 | 0 | 0 | 3 | 0 |
| src/server/spatial_api_handler.cpp | 3 | 0 | 0 | 3 | 0 |
| src/server/themis_core_grpc_service.cpp | 3 | 0 | 0 | 3 | 0 |
| src/server/transaction_api_handler.cpp | 3 | 0 | 0 | 3 | 0 |
| src/server/branch_api_handler.cpp | 2 | 0 | 0 | 2 | 0 |
| src/server/classification_api_handler.cpp | 2 | 0 | 0 | 2 | 0 |
| src/server/error_api_handler.cpp | 2 | 0 | 0 | 2 | 0 |
| src/server/export_api_handler.cpp | 2 | 0 | 0 | 2 | 0 |
| src/server/health_error_service.cpp | 2 | 0 | 0 | 2 | 0 |
| src/server/openapi_route_registry.cpp | 2 | 0 | 0 | 2 | 0 |
| src/server/pii_api_handler.cpp | 2 | 0 | 0 | 2 | 0 |
| src/server/pitr_grpc_service.cpp | 2 | 0 | 0 | 2 | 0 |
| src/server/rate_limiter.cpp | 2 | 0 | 0 | 2 | 0 |
| src/server/rate_limiter_v2.cpp | 2 | 0 | 1 | 1 | 0 |
| src/server/rpc/snapshot_transfer_handler.cpp | 2 | 0 | 0 | 2 | 0 |
| src/server/serverless_function_api_handler.cpp | 2 | 0 | 0 | 2 | 0 |
| src/server/sharding_metrics_handler.cpp | 2 | 0 | 2 | 0 | 0 |
| src/server/snapshot_api_handler.cpp | 2 | 0 | 0 | 2 | 0 |
| src/server/wal_api_handler.cpp | 2 | 0 | 0 | 2 | 0 |
| src/server/adaptive_rate_limiter.cpp | 1 | 0 | 0 | 1 | 0 |
| src/server/api_security_audit.cpp | 1 | 0 | 0 | 1 | 0 |
| src/server/api_version.cpp | 1 | 0 | 1 | 0 | 0 |
| src/server/continuous_query_api_handler.cpp | 1 | 0 | 0 | 1 | 0 |
| src/server/hot_reload_api_handler.cpp | 1 | 0 | 0 | 1 | 0 |
| src/server/http_type_adapter.cpp | 1 | 0 | 0 | 1 | 0 |
| src/server/import_wizard_builder.cpp | 1 | 0 | 0 | 1 | 0 |
| src/server/merge_api_handler.cpp | 1 | 0 | 0 | 1 | 0 |
| src/server/opa_adapter.cpp | 1 | 0 | 0 | 1 | 0 |
| src/server/pitr_api_handler.cpp | 1 | 0 | 0 | 1 | 0 |
| src/server/prompt_api_handler.cpp | 1 | 0 | 0 | 1 | 0 |
| src/server/rate_limiting_middleware.cpp | 1 | 0 | 0 | 1 | 0 |
| src/server/saml_auth_provider.cpp | 1 | 0 | 0 | 1 | 0 |
| src/server/sse_connection_manager.cpp | 1 | 0 | 0 | 1 | 0 |
| src/server/udf_api_handler.cpp | 1 | 0 | 0 | 1 | 0 |
| src/server/websocket_session.cpp | 1 | 0 | 0 | 1 | 0 |
| include/server/examples/workload_fingerprint_example.cpp | 0 | 0 | 0 | 0 | 0 |
| src/server/api_auth_config.cpp | 0 | 0 | 0 | 0 | 0 |
| src/server/buffer_api_handler.cpp | 0 | 0 | 0 | 0 | 0 |
| src/server/buffer_binary_protocol.cpp | 0 | 0 | 0 | 0 | 0 |
| src/server/cache_api_handler.cpp | 0 | 0 | 0 | 0 | 0 |
| src/server/cdn_cache_middleware.cpp | 0 | 0 | 0 | 0 | 0 |
| src/server/cost_based_rate_limiter.cpp | 0 | 0 | 0 | 0 | 0 |
| src/server/diff_api_handler.cpp | 0 | 0 | 0 | 0 | 0 |
| src/server/grpc_web_proxy_handler.cpp | 0 | 0 | 0 | 0 | 0 |
| src/server/http3_datagram.cpp | 0 | 0 | 0 | 0 | 0 |
| src/server/mvcc_api_handler.cpp | 0 | 0 | 0 | 0 | 0 |
| src/server/oauth2_provider.cpp | 0 | 0 | 0 | 0 | 0 |
| src/server/policy_api_handler.cpp | 0 | 0 | 0 | 0 | 0 |
| src/server/request_coalescing.cpp | 0 | 0 | 0 | 0 | 0 |
| src/server/rpc/blob_transfer_handler.cpp | 0 | 0 | 0 | 0 | 0 |
| src/server/update_api_handler.cpp | 0 | 0 | 0 | 0 | 0 |
| src/server/workload_fingerprint_engine.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/server/llm_api_handler.cpp
Total findings: 92

- Line 192: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: return handleLoadModel(req);
  Confidence: band=very_high; score=0.99
- Line 194: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: return handleUnloadModel(req);
  Confidence: band=very_high; score=0.99
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
- Line 17: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: #include "llm/async_inference_engine.h"
  Confidence: band=very_high; score=0.9
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
- Line 2076: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string{"Inference failed: "} + e.what(),
  Confidence: band=very_high; score=0.9
- Line 2084: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto err = llm::OpenAICompatAdapter::buildError("Inference failed", "server_error");
  Confidence: band=very_high; score=0.9
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
- Line 531: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rag_context.documents.push_back(std::move(document));
  Confidence: band=high; score=0.74
- Line 661: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: embedding_vector.push_back(val);
  Confidence: band=high; score=0.74
- Line 709: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (raw[i] == '+') { decoded += ' '; ++i; }
  Confidence: band=high; score=0.74
- Line 903: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: models.push_back(json{{"model_id", model_id}});
  Confidence: band=high; score=0.74
- Line 1141: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: loras.push_back(lora_obj);
  Confidence: band=high; score=0.74
- Line 1543: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: docs_array.push_back({
  Confidence: band=high; score=0.74
- Line 1925: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: feedback_array.push_back(feedback.toJson());
  Confidence: band=high; score=0.74
- Line 2003: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> header_map;
  Confidence: band=medium; score=0.66
- Line 2069: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: plugin_mgr.generate(llm_request);
  Confidence: band=high; score=0.74
- Line 2117: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: llm_response = plugin_mgr.generate(llm_request);
  Confidence: band=high; score=0.74
- Line 2171: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: models_arr.push_back(json{
  Confidence: band=high; score=0.74

### src/server/http_server.cpp
Total findings: 65

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
- Line 3005: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward compatibility alias
  Confidence: band=high; score=0.8
- Line 1900: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: SSL_CTX_set_cipher_list(ssl_ctx_->native_handle(),
  Confidence: band=high; score=0.74
- Line 2010: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads_.emplace_back([this, i] {
  Confidence: band=high; score=0.74
- Line 2238: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: SSL_CTX_set_cipher_list(new_ctx->native_handle(),
  Confidence: band=high; score=0.74
- Line 3682: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> req_headers;
  Confidence: band=high; score=0.74
- Line 3755: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: header_bytes += field.name_string().size() + field.value().size() + 4; // 2 for ": " + 2 for "\r\n"
  Confidence: band=high; score=0.74
- Line 3887: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> headers;
  Confidence: band=medium; score=0.66
- Line 4506: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: docs_array.push_back({
  Confidence: band=high; score=0.74
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
- Line 8396: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
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
- Line 9032: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: static std::unordered_map<std::string, std::string> parseQuery(const std::string& target) {
  Confidence: band=high; score=0.74
- Line 9216: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
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
- Line 10170: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response["interactions"].push_back(interaction.toJson());
  Confidence: band=high; score=0.74
- Line 10408: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(std::move(j));
  Confidence: band=high; score=0.74
- Line 10438: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resp.push_back({{"pk", pk}, {"score", score}});
  Confidence: band=high; score=0.74
- Line 10494: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resp.push_back({
  Confidence: band=high; score=0.74
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
- Line 10926: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = body["weights"].begin(); it != body["weights"].end(); ++it) {
  Confidence: band=high; score=0.74
- Line 11127: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto configObj = body["config"];
  Confidence: band=high; score=0.74
- Line 11132: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (s.is_string()) config.stopwords.push_back(s.get<std::string>());
  Confidence: band=high; score=0.74
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
- Line 12434: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response.push_back(suggestion.toJson());
  Confidence: band=high; score=0.74
- Line 12476: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response.push_back(pattern.toJson());
  Confidence: band=high; score=0.74

### src/server/query_api_handler.cpp
Total findings: 64

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
- Line 2200: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Provide "result" alias for compatibility with older clients/tests
  Confidence: band=high; score=0.8
- Line 2839: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: const auto it = mp.find(a.var);
  Confidence: band=very_high; score=0.9
- Line 3269: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Provide "result" alias for compatibility
  Confidence: band=high; score=0.8
- Line 210: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: preds.push_back({p["column"].get<std::string>(), p["value"].get<std::string>()});
  Confidence: band=high; score=0.74
- Line 230: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rpreds.push_back(std::move(pr));
  Confidence: band=high; score=0.74
- Line 290: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
  Confidence: band=high; score=0.74
- Line 294: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan_json["estimates"].push_back({
  Confidence: band=high; score=0.74
- Line 314: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
  Confidence: band=high; score=0.74
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
- Line 496: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto coll = schema["collections"][table];
  Confidence: band=high; score=0.74
- Line 511: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: try { obj = nlohmann::json::parse(e.toJson()); } catch (...) { entities.push_back(e.toJson()); continue; }
  Confidence: band=high; score=0.74
- Line 511: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: try { obj = nlohmann::json::parse(e.toJson()); } catch (...) { entities.push_back(e.toJson()); continue; }
  Confidence: band=high; score=0.74
- Line 522: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto enc_meta_str = obj[f + "_encrypted"].get<std::string>();
  Confidence: band=high; score=0.74
- Line 811: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::shared_ptr<themis::query::Expression>> letMap;
  Confidence: band=medium; score=0.66
- Line 820: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: std::vector<std::string> parts; parts.push_back(fa->field);
  Confidence: band=high; score=0.74
- Line 825: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: std::string col; for (auto it = parts.rbegin(); it != parts.rend(); ++it) { if (!col.empty()) col += "."; col += *it; }
  Confidence: band=high; score=0.74
- Line 1735: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (retVar == var1) out.push_back(l); else out.push_back(e);
  Confidence: band=high; score=0.74
- Line 1735: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (retVar == var1) out.push_back(l); else out.push_back(e);
  Confidence: band=high; score=0.74
- Line 1942: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resultVertices.push_back(node);
  Confidence: band=high; score=0.74
- Line 2039: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: res["entities"].push_back(entity.toJson());
  Confidence: band=high; score=0.74
- Line 2054: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: res["entities"].push_back(edgeEnt.toJson());
  Confidence: band=high; score=0.74
- Line 2070: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vertices.push_back(cur);
  Confidence: band=high; score=0.74
- Line 2095: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: jpath["vertices"].push_back(ent.toJson());
  Confidence: band=high; score=0.74
- Line 2110: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: jpath["edges"].push_back(eent.toJson());
  Confidence: band=high; score=0.74
- Line 2187: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(nlohmann::json::parse(entity.toJson()));
  Confidence: band=high; score=0.74
- Line 2242: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: parts.push_back(fa->field);
  Confidence: band=high; score=0.74
- Line 2254: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!col.empty()) col += ".";
  Confidence: band=high; score=0.74
- Line 2254: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!col.empty()) col += ".";
  Confidence: band=high; score=0.74
- Line 2271: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cq.predicates.push_back({col, litToString(lit->value)});
  Confidence: band=high; score=0.74
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
- Line 2382: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(projected);
  Confidence: band=high; score=0.74
- Line 2382: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(projected);
  Confidence: band=high; score=0.74
- Line 2612: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
  Confidence: band=high; score=0.74
- Line 2616: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan_json["estimates"].push_back({
  Confidence: band=high; score=0.74
- Line 2757: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!col.empty()) col += ".";
  Confidence: band=high; score=0.74
- Line 2781: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: aggs.push_back({a.varName, func, col});
  Confidence: band=high; score=0.74
- Line 2786: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::unordered_map<std::string, AggState>> acc;
  Confidence: band=medium; score=0.66
- Line 2848: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: groups.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 2848: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: groups.push_back(std::move(row));
  Confidence: band=high; score=0.74
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
- Line 3177: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& e : sliced) entities.push_back(e.toJson());
  Confidence: band=high; score=0.74
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
- Line 3612: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_fwd = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/postgres_session.cpp
Total findings: 37

- Line 301: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Handle schema queries (pg_catalog, information_schema) for BI tool compatibility
  Confidence: band=high; score=0.8
- Line 1626: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Standard PostgreSQL types required for BI tool compatibility
  Confidence: band=high; score=0.8
- Line 32: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += "''";  // PostgreSQL escapes single quotes by doubling
  Confidence: band=high; score=0.74
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
- Line 599: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: row_vals.push_back(
  Confidence: band=high; score=0.74
- Line 599: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: row_vals.push_back(
  Confidence: band=high; score=0.74
- Line 721: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back({"?column?", 0, 0, 25, -1, -1, 0}); // text type
  Confidence: band=high; score=0.74
- Line 784: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back({"?column?", 0, 0, 25, -1, -1, 0});
  Confidence: band=high; score=0.74
- Line 911: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: field += '"';
  Confidence: band=high; score=0.74
- Line 1032: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void PostgresSession::sendReadyForQuery(char transactionStatus) {
  Confidence: band=high; score=0.74
- Line 1047: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: payload.push_back(0);
  Confidence: band=high; score=0.74
- Line 1094: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: payload.push_back((len >> 24) & 0xFF);
  Confidence: band=high; score=0.74
- Line 1119: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: payload.push_back((len >> 24) & 0xFF);
  Confidence: band=high; score=0.74
- Line 1162: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: payload.push_back((typeOid >> 24) & 0xFF);
  Confidence: band=high; score=0.74
- Line 1196: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: payload.push_back((format >> 8) & 0xFF);
  Confidence: band=high; score=0.74
- Line 1217: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: payload.push_back((format >> 8) & 0xFF);
  Confidence: band=high; score=0.74
- Line 1238: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: payload.push_back((format >> 8) & 0xFF);
  Confidence: band=high; score=0.74
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
- Line 1597: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool PostgresSession::isSchemaQuery(const std::string& query) {
  Confidence: band=high; score=0.74
- Line 1610: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void PostgresSession::handleSchemaQuery(const std::string& query) {
  Confidence: band=high; score=0.74
- Line 1761: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: PostgresSession::QueryInfo PostgresSession::parseSelectQuery(const std::string& query) {
  Confidence: band=high; score=0.74
- Line 1920: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) cypher += ", ";
  Confidence: band=high; score=0.74
- Line 1959: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) cypher += ", ";
  Confidence: band=high; score=0.74
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
- Line 2089: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string PostgresSession::parseUpdateQuery(const std::string& query) {
  Confidence: band=high; score=0.74
- Line 2152: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: assignments.push_back(cypherSetClause.substr(start, i - start));
  Confidence: band=high; score=0.74
- Line 2183: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) cypherSetClause += ", ";
  Confidence: band=high; score=0.74
- Line 2194: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string PostgresSession::parseDeleteQuery(const std::string& query) {
  Confidence: band=high; score=0.74
- Line 2240: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string PostgresSession::translateQuery(const std::string& postgresQuery) {
  Confidence: band=high; score=0.74

### src/server/monitoring_api_handler.cpp
Total findings: 24

- Line 287: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: modules_compiled.push_back(module_info);
  Confidence: band=high; score=0.74
- Line 337: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: supported.push_back({
  Confidence: band=high; score=0.74
- Line 631: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "# HELP themis_build_info ThemisDB build information\n";
  Confidence: band=high; score=0.74
- Line 674: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "# HELP themis_continuous_learning_loop_signal_value Latest loop signal value\n";
  Confidence: band=high; score=0.74
- Line 704: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "themis_continuous_learning_loop_signal_value" + labels +
  Confidence: band=high; score=0.74
- Line 761: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = r["files_per_level"].begin(); it != r["files_per_level"].end(); ++it) {
  Confidence: band=high; score=0.74
- Line 764: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: level_rows.emplace_back(it.key(), val);
  Confidence: band=high; score=0.74
- Line 785: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "rocksdb_files_level{level=\"" + level + "\"} " + std::to_string(val) + "\n";
  Confidence: band=high; score=0.74
- Line 785: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "rocksdb_files_level{level=\"" + level + "\"} " + std::to_string(val) + "\n";
  Confidence: band=high; score=0.74
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
- Line 842: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "themis_plugin_reloads_total{plugin=\"" + plugin_name + "\"} "
  Confidence: band=high; score=0.74
- Line 850: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "themis_plugin_errors_total{plugin=\"" + plugin_name + "\"} "
  Confidence: band=high; score=0.74
- Line 858: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "themis_plugin_function_calls_total{plugin=\"" + plugin_name + "\"} "
  Confidence: band=high; score=0.74
- Line 867: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "themis_plugin_load_duration_seconds_sum{plugin=\"" + plugin_name + "\"} "
  Confidence: band=high; score=0.74
- Line 876: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "themis_plugin_memory_bytes{plugin=\"" + plugin_name + "\"} "
  Confidence: band=high; score=0.74
- Line 885: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "themis_plugin_call_latency_milliseconds{plugin=\"" + plugin_name
  Confidence: band=high; score=0.74
- Line 986: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sorted_plugin_names.push_back(plugin_name);
  Confidence: band=high; score=0.74
- Line 1197: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(a);
  Confidence: band=high; score=0.74
- Line 1384: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '&': escaped += "&amp;"; break;
  Confidence: band=high; score=0.74
- Line 1388: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: default: escaped.push_back(ch); break;
  Confidence: band=high; score=0.74
- Line 1413: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: html += "<tr><td>" + name + "</td><td class=\"val\">" + val + "</td></tr>\n";
  Confidence: band=high; score=0.74
- Line 1510: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: html += "<tr>";
  Confidence: band=high; score=0.74

### src/server/mcp_server.cpp
Total findings: 23

- Line 862: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function query without trace point
  Context: {"description", "Search query (e.g., 'gpu', 'lora', 'storage')"}
  Confidence: band=very_high; score=0.9
- Line 108: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: new_cfg.denied_collections.push_back(c.as<std::string>());
  Confidence: band=high; score=0.74
- Line 114: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: new_cfg.allowed_collections.push_back(c.as<std::string>());
  Confidence: band=high; score=0.74
- Line 524: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: json McpServer::handleInitialize(const json& params) {
  Confidence: band=medium; score=0.66
- Line 553: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tools_list.push_back({
  Confidence: band=high; score=0.74
- Line 596: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resources_list.push_back({
  Confidence: band=high; score=0.74
- Line 641: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: prompts_list.push_back({
  Confidence: band=high; score=0.74
- Line 862: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: {"description", "Search query (e.g., 'gpu', 'lora', 'storage')"}
  Confidence: band=high; score=0.74
- Line 887: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: json McpServer::toolQuery(const json& args) {
  Confidence: band=high; score=0.74
- Line 1301: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ft_config = args["fulltext_config"];
  Confidence: band=high; score=0.74
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
- Line 1802: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: modes_arr.push_back(std::move(entry));
  Confidence: band=high; score=0.74
- Line 1849: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors_json.push_back(error.toJSON());
  Confidence: band=high; score=0.74
- Line 1877: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors_json.push_back(error.toJSON());
  Confidence: band=high; score=0.74
- Line 2020: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: answer += fmt::format("**{}** ({} error types)\n", category, errors.size());
  Confidence: band=high; score=0.74
- Line 2046: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) docs_str += ", ";
  Confidence: band=high; score=0.74
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
- Line 2778: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void StdioTransport::start() {
  Confidence: band=medium; score=0.66
- Line 2996: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void SseTransport::start() {
  Confidence: band=medium; score=0.66

### src/server/rpc/rpc_service_impl.cpp
Total findings: 21

- Line 3314: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto auth_result = auth.authorize(token, required_scope);
  Confidence: band=very_high; score=0.99
- Line 3283: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // for backward compatibility. In production, auth should always be configured.
  Confidence: band=high; score=0.8
- Line 662: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys_to_delete.push_back(child_key);
  Confidence: band=high; score=0.74
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
- Line 2705: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: projected.push_back(proj_doc);
  Confidence: band=high; score=0.74
- Line 2786: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: collections_array.push_back({
  Confidence: band=high; score=0.74
- Line 3024: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: models_array.push_back({
  Confidence: band=high; score=0.74

### src/server/policy_engine.cpp
Total findings: 18

- Line 243: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: PolicyEngine::Decision PolicyEngine::authorize(const std::string& user_id,
  Confidence: band=very_high; score=0.99
- Line 66: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& r : n["resources"]) p.resources.push_back(r.as<std::string>());
  Confidence: band=high; score=0.74
- Line 66: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& r : n["resources"]) p.resources.push_back(r.as<std::string>());
  Confidence: band=high; score=0.74
- Line 70: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto eff = n["effect"].as<std::string>("allow");
  Confidence: band=high; score=0.74
- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& ip : n["allowed_ip_prefixes"]) p.allowed_ip_prefixes.push_back(ip.as<std::string>());
  Confidence: band=high; score=0.74
- Line 80: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& ua : n["allowed_user_agent_patterns"]) p.allowed_user_agent_patterns.push_back(ua.as<std::string>());
  Confidence: band=high; score=0.74
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
- Line 392: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (j.contains("allowed_ip_prefixes")) for (const auto& ip : j["allowed_ip_prefixes"]) p.allowed_ip_prefixes.push_back(ip.get<std::string>());
  Confidence: band=high; score=0.74
- Line 395: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (j.contains("allowed_user_agent_patterns")) for (const auto& ua : j["allowed_user_agent_patterns"]) p.allowed_user_agent_patterns.push_back(ua.get<std::string>());
  Confidence: band=high; score=0.74

### src/server/task_scheduler_api_handler.cpp
Total findings: 18

- Line 145: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(taskToJson(t));
  Confidence: band=high; score=0.74
- Line 313: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(r.toJson());
  Confidence: band=high; score=0.74
- Line 414: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: params.sort_by = scheduler::AuditQueryParams::SortBy::TIMESTAMP_DESC;
  Confidence: band=high; score=0.74
- Line 437: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: html += "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n";
  Confidence: band=high; score=0.74
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

### src/server/distributed_gateway.cpp
Total findings: 15

- Line 163: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: seen.insert(node.node_id);
  Confidence: band=very_high; score=0.99
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
- Line 90: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cfg.routes.push_back(GatewayRouteConfig::fromJson(r));
  Confidence: band=high; score=0.74
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
- Line 446: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: raft_cfg.cluster_members.push_back(n.node_id);
  Confidence: band=high; score=0.74
- Line 485: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto upgrade = req[http::field::upgrade];
  Confidence: band=high; score=0.74
- Line 496: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto accept = req[http::field::accept];
  Confidence: band=high; score=0.74

### src/server/voice_api_handler.cpp
Total findings: 15

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
- Line 1325: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: steps.push_back(parseStep(sj));
  Confidence: band=high; score=0.74
- Line 1436: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(macroInfoToResponseJson(m));
  Confidence: band=high; score=0.74
- Line 1488: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: steps.push_back(parseStep(sj));
  Confidence: band=high; score=0.74
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
- Line 2129: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: audio_samples.push_back(std::move(decoded_sample));
  Confidence: band=high; score=0.74
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

### src/server/vector_api_handler.cpp
Total findings: 14

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
- Line 191: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back({{"pk", results[i].pk}, {"distance", results[i].distance}});
  Confidence: band=high; score=0.74
- Line 207: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resultJson.push_back({{"pk", result.pk}, {"distance", result.distance}});
  Confidence: band=high; score=0.74
- Line 302: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto coll = schema_json["collections"][object_name];
  Confidence: band=high; score=0.74
- Line 304: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ecfg = coll["encryption"];
  Confidence: band=high; score=0.74
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
- Line 347: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vec.push_back(v.get<float>());
  Confidence: band=high; score=0.74
- Line 356: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto fit = it["fields"].begin(); fit != it["fields"].end(); ++fit) {
  Confidence: band=high; score=0.74
- Line 765: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 790: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/content_api_handler.cpp
Total findings: 12

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
- Line 503: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resp.push_back({
  Confidence: band=high; score=0.74
- Line 722: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = body["weights"].begin(); it != body["weights"].end(); ++it) {
  Confidence: band=high; score=0.74

### src/server/llm_grpc_service.cpp
Total findings: 12

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

### src/server/schema_api_handler.cpp
Total findings: 12

- Line 136: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response["tables"].push_back(table_info);
  Confidence: band=high; score=0.74
- Line 666: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(col.toJSON());
  Confidence: band=high; score=0.74
- Line 679: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats_arr.push_back(s.toJSON());
  Confidence: band=high; score=0.74
- Line 683: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats_arr.push_back(s.toJSON());
  Confidence: band=high; score=0.74
- Line 827: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: c_arr.push_back(c.toJSON());
  Confidence: band=high; score=0.74
- Line 1034: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rec_arr.push_back(r.toJSON());
  Confidence: band=high; score=0.74
- Line 1034: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rec_arr.push_back(r.toJSON());
  Confidence: band=high; score=0.74
- Line 1055: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rec_arr.push_back(r.toJSON());
  Confidence: band=high; score=0.74
- Line 1155: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back({{"error", "Table schema missing 'name' field"}});
  Confidence: band=high; score=0.74
- Line 1259: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: valid_rows.push_back({{"index", row_index}, {"row", row_json}});
  Confidence: band=high; score=0.74
- Line 1263: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: invalid_rows.push_back({{"index", row_index}, {"row", row_json},
  Confidence: band=high; score=0.74
- Line 1390: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entry.source_columns.push_back(
  Confidence: band=high; score=0.74

### src/server/async_job_api_handler.cpp
Total findings: 11

- Line 576: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return makeJsonResponse(http::status::conflict,
  Confidence: band=very_high; score=0.99
- Line 142: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = jobs_.begin(); it != jobs_.end(); ) {
  Confidence: band=very_high; score=0.9
- Line 156: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: std::shared_ptr<AsyncJobRecord> AsyncJobRegistry::get(const std::string& id) const {
  Confidence: band=very_high; score=0.9
- Line 226: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = jobs_.begin(); it != jobs_.end(); ) {
  Confidence: band=very_high; score=0.9
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
- Line 431: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures_.push_back(std::move(fut));
  Confidence: band=high; score=0.74
- Line 446: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_hdr = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/changefeed_api_handler.cpp
Total findings: 11

- Line 1025: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto auth_result = auth_->authorize(*token, required_scope);
  Confidence: band=very_high; score=0.99
- Line 1120: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto auth_result = auth_->authorize(*token, required_scope);
  Confidence: band=very_high; score=0.99
- Line 204: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response["events"].push_back(event.toJson());
  Confidence: band=high; score=0.74
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

### src/server/feedback_api_handler.cpp
Total findings: 11

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
- Line 521: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: auto stats = storage_service.getStatistics(adapter_id);
  Confidence: band=high; score=0.74
- Line 579: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: llm::lora::FeedbackFilter FeedbackAPIHandler::parseFilterFromQuery(const std::string& query) const {
  Confidence: band=high; score=0.74

### src/server/profiling_api_handler.cpp
Total findings: 11

- Line 126: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(profile->toJSON());
  Confidence: band=high; score=0.74
- Line 149: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(profile->toJSON());
  Confidence: band=high; score=0.74
- Line 191: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: query_json.push_back(profile->toJSON());
  Confidence: band=high; score=0.74
- Line 196: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: storage_json.push_back(stats.toJSON());
  Confidence: band=high; score=0.74
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

### src/server/entity_api_handler.cpp
Total findings: 10

- Line 155: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto authz = auth_->authorize(*token_opt, scope);
  Confidence: band=very_high; score=0.99
- Line 108: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 142: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 247: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto coll = schema["collections"][table];
  Confidence: band=high; score=0.74
- Line 267: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto enc_meta_str = entity_json[f + "_encrypted"].get<std::string>();
  Confidence: band=high; score=0.74
- Line 394: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto coll = schema["collections"][table];
  Confidence: band=high; score=0.74
- Line 406: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (f.is_string()) fields.push_back(f.get<std::string>());
  Confidence: band=high; score=0.74
- Line 850: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back({
  Confidence: band=high; score=0.74
- Line 956: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back({
  Confidence: band=high; score=0.74
- Line 1208: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back({
  Confidence: band=high; score=0.74

### src/server/mqtt_client_service.cpp
Total findings: 10

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
- Line 150: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vheader.push_back(qos & 0x03);
  Confidence: band=high; score=0.74
- Line 170: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pkt.push_back(0xA2); // UNSUBSCRIBE
  Confidence: band=high; score=0.74
- Line 236: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void MqttClientService::start() {
  Confidence: band=medium; score=0.66
- Line 775: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: SSL_set_tlsext_host_name(asio_->ssl_stream->native_handle(),
  Confidence: band=high; score=0.74
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

### src/server/prompt_engineering_api_handler.cpp
Total findings: 10

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
- Line 378: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response.push_back(entry.toJson());
  Confidence: band=high; score=0.74
- Line 427: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response.push_back(version.toJson());
  Confidence: band=high; score=0.74

### src/server/rpc/differential_update_engine.cpp
Total findings: 10

- Line 38: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: boundaries.push_back(i);
  Confidence: band=high; score=0.74
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

### src/server/shard_repair_api_handler.cpp
Total findings: 10

- Line 128: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: << "input{padding:10px 12px;border-radius:999px;border:1px solid var(--line);min-width:220px;background:#fff}"
  Confidence: band=very_high; score=0.99
- Line 144: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: << "<input id=\"shardId\" placeholder=\"Shard ID for targeted repair\">"
  Confidence: band=very_high; score=0.99
- Line 206: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto ar = auth.authorize(*token, required_scope);
  Confidence: band=very_high; score=0.99
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
- Line 255: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: body["shards"].push_back(shardReportToJson(report));
  Confidence: band=high; score=0.74

### src/server/auth_middleware.cpp
Total findings: 9

- Line 182: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: AuthMiddleware::AuthResult AuthMiddleware::authorize(std::string_view token, std::string_view required_scope) const {
  Confidence: band=very_high; score=0.99
- Line 612: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: auto claims = mtls_auth.authenticate(std::string(cert_pem));
  Confidence: band=very_high; score=0.99
- Line 199: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // inputs (which would require zero-padding and may confuse static
  Confidence: band=very_high; score=0.9
- Line 158: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::string>> mapping)
  Confidence: band=medium; score=0.66
- Line 341: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> granted_scopes(claims.scopes.begin(),
  Confidence: band=medium; score=0.66
- Line 527: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) roles_str += ", ";
  Confidence: band=high; score=0.74
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

### src/server/bpmn_api_handler.cpp
Total findings: 9

- Line 113: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: // auth_->authorize() which checks that the token contains the required scope.
  Confidence: band=very_high; score=0.99
- Line 114: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto ar = auth_->authorize(*token, scope);
  Confidence: band=very_high; score=0.99
- Line 478: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: const auto tsIt = token.visit_timestamps.find(node);
  Confidence: band=very_high; score=0.9
- Line 59: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 94: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74
- Line 251: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_tasks.push_back(instance_id + ":" + token.current_node);
  Confidence: band=high; score=0.74
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

### src/server/pki_api_handler.cpp
Total findings: 9

- Line 57: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(chars[(val>>valb)&0x3F]);
  Confidence: band=high; score=0.74
- Line 77: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back((uint8_t)((val>>valb)&0xFF));
  Confidence: band=high; score=0.74
- Line 77: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back((uint8_t)((val>>valb)&0xFF));
  Confidence: band=high; score=0.74
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

### src/server/ethics_api_handler.cpp
Total findings: 8

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
- Line 551: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rows.push_back(nlohmann::json::parse(entity.toJson()));
  Confidence: band=high; score=0.74
- Line 558: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rows.push_back(nlohmann::json::parse(entity.toJson()));
  Confidence: band=high; score=0.74

### src/server/graph_api_handler.cpp
Total findings: 8

- Line 380: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: body += "themis_graph_latency_ms_bucket{le=\"";
  Confidence: band=high; score=0.74
- Line 813: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (v.is_string()) qc.forbidden_vertices.push_back(v.get<std::string>());
  Confidence: band=high; score=0.74
- Line 818: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (v.is_string()) qc.required_vertices.push_back(v.get<std::string>());
  Confidence: band=high; score=0.74
- Line 823: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (v.is_string()) qc.node_labels.push_back(v.get<std::string>());
  Confidence: band=high; score=0.74
- Line 862: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: alt_arr.push_back({{"algorithm", algo_name(alt_algo)}, {"estimated_cost", alt_cost}});
  Confidence: band=high; score=0.74
- Line 867: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_arr.push_back(sid);
  Confidence: band=high; score=0.74
- Line 988: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (pv.is_string()) pverts.push_back(pv.get<std::string>());
  Confidence: band=high; score=0.74
- Line 996: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pedges.emplace_back(pe[0].get<std::string>(), pe[1].get<std::string>());
  Confidence: band=high; score=0.74

### src/server/lora_api_handler.cpp
Total findings: 8

- Line 15: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "llm/lora_framework/lora_storage_service.h"
  Confidence: band=high; score=0.74
- Line 16: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "llm/lora_framework/lora_training_service.h"
  Confidence: band=high; score=0.74
- Line 616: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered_adapters.push_back(adapter);
  Confidence: band=high; score=0.74
- Line 626: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adapters.push_back(filtered_adapters[i].toJSON());
  Confidence: band=high; score=0.74
- Line 1292: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto created_ns = metadata_json["created_at"].get<uint64_t>();
  Confidence: band=high; score=0.74
- Line 1300: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto updated_ns = metadata_json["updated_at"].get<uint64_t>();
  Confidence: band=high; score=0.74
- Line 1457: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(e.toJSON());
  Confidence: band=high; score=0.74
- Line 1490: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(s.toJSON());
  Confidence: band=high; score=0.74

### src/server/rope_api_handler.cpp
Total findings: 8

- Line 891: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: // auth_->authorize(); deny with HTTP 403 when the scope is not granted.
  Confidence: band=very_high; score=0.99
- Line 904: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto ar = auth_->authorize(*token, permission);
  Confidence: band=very_high; score=0.99
- Line 365: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vec.push_back(v.get<float>());
  Confidence: band=high; score=0.74
- Line 489: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vec.push_back(v.get<float>());
  Confidence: band=high; score=0.74
- Line 580: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: query_vector.push_back(val.get<float>());
  Confidence: band=high; score=0.74
- Line 608: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results_array.push_back({
  Confidence: band=high; score=0.74
- Line 731: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vec.push_back(v.get<float>());
  Confidence: band=high; score=0.74
- Line 892: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/api_gateway.cpp
Total findings: 7

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
- Line 776: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> labels = {
  Confidence: band=high; score=0.74

### src/server/index_api_handler.cpp
Total findings: 7

- Line 74: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto configObj = body["config"];
  Confidence: band=high; score=0.74
- Line 79: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (s.is_string()) config.stopwords.push_back(s.get<std::string>());
  Confidence: band=high; score=0.74
- Line 119: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: columns.push_back(c.get<std::string>());
  Confidence: band=high; score=0.74
- Line 254: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resp.push_back(stat_obj);
  Confidence: band=high; score=0.74
- Line 333: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats_array.push_back({
  Confidence: band=high; score=0.74
- Line 394: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response.push_back(suggestion.toJson());
  Confidence: band=high; score=0.74
- Line 436: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response.push_back(pattern.toJson());
  Confidence: band=high; score=0.74

### src/server/ranger_adapter.cpp
Total findings: 7

- Line 148: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (resource_prefixes.empty()) resource_prefixes.push_back("/");
  Confidence: band=high; score=0.74
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
- Line 207: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& u : p.subjects) item["users"].push_back(u);
  Confidence: band=high; score=0.74
- Line 211: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(std::move(rp));
  Confidence: band=high; score=0.74

### src/server/wasm_handler_registry.cpp
Total findings: 7

- Line 147: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: const char* pos = std::find(kBase64Chars,
  Confidence: band=very_high; score=0.9
- Line 137: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(char3[j]);
  Confidence: band=high; score=0.74
- Line 137: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
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
- Line 155: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(char3[j]);
  Confidence: band=high; score=0.74
- Line 273: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry.toJson());
  Confidence: band=high; score=0.74

### src/server/audit_api_handler.cpp
Total findings: 6

- Line 37: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: AuditLogEntry::toJson()
  Context: nlohmann::json AuditLogEntry::toJson() const {
  Confidence: band=medium; score=0.56
- Line 76: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ciphertext_b64 = payload["ciphertext_b64"].get<std::string>();
  Confidence: band=high; score=0.74
- Line 189: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort by timestamp descending (newest first)
  Confidence: band=high; score=0.74
- Line 210: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result["entries"].push_back(all_entries[i].toJson());
  Confidence: band=high; score=0.74
- Line 238: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"') escaped += "\"\"";
  Confidence: band=high; score=0.74
- Line 238: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"') escaped += "\"\"";
  Confidence: band=high; score=0.74

### src/server/geo_topology_api_handler.cpp
Total findings: 6

- Line 131: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: std::find(zones_arr.begin(), zones_arr.end(), s.zone) == zones_arr.end()) {
  Confidence: band=very_high; score=0.9
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
- Line 189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failed_regions.push_back(region);
  Confidence: band=high; score=0.74

### src/server/http2_session.cpp
Total findings: 6

- Line 113: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void Http2Session::start() {
  Confidence: band=medium; score=0.66
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
- Line 562: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nva.push_back({
  Confidence: band=high; score=0.74
- Line 678: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response_nva.push_back({
  Confidence: band=high; score=0.74

### src/server/maintenance_api_handler.cpp
Total findings: 6

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
- Line 252: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back({{"task_type", task_type}, {"handler", handler_name}});
  Confidence: band=high; score=0.74

### src/server/policy_versioning_api_handler.cpp
Total findings: 6

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
- Line 285: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: json_array.push_back(entry.toJson());
  Confidence: band=high; score=0.74
- Line 321: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conflicts_arr.push_back(c.toJson());
  Confidence: band=high; score=0.74
- Line 355: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/replication_topology_api_handler.cpp
Total findings: 6

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

### src/server/session_api_handler.cpp
Total findings: 6

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
- Line 193: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(std::move(j));
  Confidence: band=high; score=0.74

### src/server/chunked_response_writer.cpp
Total findings: 5

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
- Line 155: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fragments.push_back(std::move(chunk_data));
  Confidence: band=high; score=0.74

### src/server/compliance_reporting_api_handler.cpp
Total findings: 5

- Line 313: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto auth_result = auth.authorize(*token, required_scope);
  Confidence: band=very_high; score=0.99
- Line 284: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward compatibility: If no auth configured or disabled, allow access but log a warning
  Confidence: band=high; score=0.8
- Line 118: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: json_array.push_back(gap.toJson());
  Confidence: band=high; score=0.74
- Line 198: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back(
  Confidence: band=high; score=0.74
- Line 293: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/http3_session.cpp
Total findings: 5

- Line 135: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void Http3Handler::start() {
  Confidence: band=medium; score=0.66
- Line 386: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void Http3Session::start() {
  Confidence: band=medium; score=0.66
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

### src/server/mqtt_session.cpp
Total findings: 5

- Line 51: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void MqttSession::start() {
  Confidence: band=medium; score=0.66
- Line 312: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: packet.push_back(static_cast<uint8_t>(0x20u)); // CONNACK packet type
  Confidence: band=high; score=0.74
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

### src/server/cache_admin_api_handler.cpp
Total findings: 4

- Line 167: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto ar = auth_->authorize(*token, required_scope);
  Confidence: band=very_high; score=0.99
- Line 102: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: output.push_back(static_cast<char>((val >> valb) & 0xFF));
  Confidence: band=high; score=0.74
- Line 102: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: output.push_back(static_cast<char>((val >> valb) & 0xFF));
  Confidence: band=high; score=0.74
- Line 153: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74

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

### src/server/import_api_handler.cpp
Total findings: 4

- Line 589: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dst.push_back(entry.get<std::string>());
  Confidence: band=high; score=0.74
- Line 603: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: warn_arr.push_back(message);
  Confidence: band=high; score=0.74
- Line 612: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: warn_arr.push_back("Circular reference detected: " + c.get<std::string>());
  Confidence: band=high; score=0.74
- Line 657: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: validated.push_back(entry);
  Confidence: band=high; score=0.74

### src/server/policy_manager_api_handler.cpp
Total findings: 4

- Line 462: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto auth_result = auth.authorize(*token, required_scope);
  Confidence: band=very_high; score=0.99
- Line 433: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward compatibility: If no auth configured or disabled, allow access but log a warning
  Confidence: band=high; score=0.8
- Line 76: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: json_array.push_back(rule.toJson());
  Confidence: band=high; score=0.74
- Line 442: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/policy_template_api_handler.cpp
Total findings: 4

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
- Line 222: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/policy_validation_api_handler.cpp
Total findings: 4

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
- Line 159: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/review_scheduling_api_handler.cpp
Total findings: 4

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
- Line 220: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/saga_api_handler.cpp
Total findings: 4

- Line 106: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["steps"].push_back(step_json);
  Confidence: band=high; score=0.74
- Line 147: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto sig = j["signature"];
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result["batches"].push_back(info.toJson());
  Confidence: band=high; score=0.74
- Line 225: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hash.push_back(byte.get<uint8_t>());
  Confidence: band=high; score=0.74

### src/server/tenant_manager.cpp
Total findings: 4

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

### src/server/timeseries_api_handler.cpp
Total findings: 4

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

### src/server/wal_grpc_service.cpp
Total findings: 4

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

### src/server/api_key_mgmt_handler.cpp
Total findings: 3

- Line 123: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (p.is_string()) permissions.push_back(p.get<std::string>());
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(recordToJson(rec));
  Confidence: band=high; score=0.74
- Line 221: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (p.is_string()) rec.permissions.push_back(p.get<std::string>());
  Confidence: band=high; score=0.74

### src/server/graphql_api_handler.cpp
Total findings: 3

- Line 61: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(serializeValue(item));
  Confidence: band=high; score=0.74
- Line 117: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors_array.push_back({{"message", pe.toString()}});
  Confidence: band=high; score=0.74
- Line 189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors_array.push_back({
  Confidence: band=high; score=0.74

### src/server/prompt_engineering_grpc_service.cpp
Total findings: 3

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

### src/server/reports_api_handler.cpp
Total findings: 3

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

### src/server/response_transformer.cpp
Total findings: 3

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

### src/server/retention_api_handler.cpp
Total findings: 3

- Line 88: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered.push_back(policy);
  Confidence: band=high; score=0.74
- Line 102: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(policyToJson(filtered[static_cast<size_t>(i)]));
  Confidence: band=high; score=0.74
- Line 189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(actionToJson(action));
  Confidence: band=high; score=0.74

### src/server/smart_routing.cpp
Total findings: 3

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

### src/server/spatial_api_handler.cpp
Total findings: 3

- Line 64: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto cfg = j["config"];
  Confidence: band=high; score=0.74
- Line 66: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto bounds = cfg["total_bounds"];
  Confidence: band=high; score=0.74
- Line 363: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::unordered_map<std::string, std::string> SpatialApiHandler::parseQuery(const std::string& target) {
  Confidence: band=high; score=0.74

### src/server/themis_core_grpc_service.cpp
Total findings: 3

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

### src/server/transaction_api_handler.cpp
Total findings: 3

- Line 154: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors_array.push_back({{"index", i}, {"error", "Missing 'type' field"}});
  Confidence: band=high; score=0.74
- Line 571: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: locks_json.push_back({{"key", lock.key}, {"lock_type", lock.lock_type}});
  Confidence: band=high; score=0.74
- Line 576: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: write_set_json.push_back({{"key", entry.key}, {"operation", entry.operation}});
  Confidence: band=high; score=0.74

### src/server/branch_api_handler.cpp
Total findings: 2

- Line 146: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(branch.toJson());
  Confidence: band=high; score=0.74
- Line 323: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resolutions.push_back(std::move(res_item));
  Confidence: band=high; score=0.74

### src/server/classification_api_handler.cpp
Total findings: 2

- Line 49: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back({
  Confidence: band=high; score=0.74
- Line 111: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: detected_entities.push_back({
  Confidence: band=high; score=0.74

### src/server/error_api_handler.cpp
Total findings: 2

- Line 36: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors_json.push_back(error.toJSON());
  Confidence: band=high; score=0.74
- Line 136: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors_json.push_back(error.toJSON());
  Confidence: band=high; score=0.74

### src/server/export_api_handler.cpp
Total findings: 2

- Line 384: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string ExportApiHandler::buildAqlQuery(const json& request_json) {
  Confidence: band=high; score=0.74
- Line 505: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto auth_header = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/server/health_error_service.cpp
Total findings: 2

- Line 2: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: * ThemisDB | File: health_error_service.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
  Confidence: band=high; score=0.74
- Line 10: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "server/health_error_service.h"
  Confidence: band=high; score=0.74

### src/server/openapi_route_registry.cpp
Total findings: 2

- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: params.push_back(std::move(param));
  Confidence: band=high; score=0.74
- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: params.push_back(std::move(param));
  Confidence: band=high; score=0.74

### src/server/pii_api_handler.cpp
Total findings: 2

- Line 145: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out_items.push_back(j);
  Confidence: band=high; score=0.74
- Line 162: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: csv += r.value("original_uuid", ""); csv += ",";
  Confidence: band=high; score=0.74

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

### src/server/rate_limiter.cpp
Total findings: 2

- Line 28: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: TokenBucket::refill()
  Context: void TokenBucket::refill() {
  Confidence: band=medium; score=0.56
- Line 338: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RateLimiter::reset()
  Context: void RateLimiter::reset() {
  Confidence: band=medium; score=0.56

### src/server/rate_limiter_v2.cpp
Total findings: 2

- Line 193: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [prio, bucket] : buckets_) {
  Confidence: band=very_high; score=0.9
- Line 270: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: redis_pool_.available.push_back(static_cast<size_t>(i));
  Confidence: band=high; score=0.74

### src/server/rpc/snapshot_transfer_handler.cpp
Total findings: 2

- Line 140: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: files.push_back(entry.path());
  Confidence: band=high; score=0.74
- Line 732: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: files.push_back(entry.path());
  Confidence: band=high; score=0.74

### src/server/serverless_function_api_handler.cpp
Total findings: 2

- Line 367: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(fn.toJson());
  Confidence: band=high; score=0.74
- Line 600: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(snap.toJson());
  Confidence: band=high; score=0.74

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

### src/server/snapshot_api_handler.cpp
Total findings: 2

- Line 101: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: sort_by = "timestamp";
  Confidence: band=high; score=0.74
- Line 115: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(snapshot.toJson());
  Confidence: band=high; score=0.74

### src/server/wal_api_handler.cpp
Total findings: 2

- Line 118: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back(std::move(e));
  Confidence: band=high; score=0.74
- Line 203: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hex.push_back(hex_digits[(result[i] >> 4) & 0x0F]);
  Confidence: band=high; score=0.74

### src/server/adaptive_rate_limiter.cpp
Total findings: 1

- Line 188: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: latencies.push_back(s.latency_ms.count());
  Confidence: band=high; score=0.74

### src/server/api_security_audit.cpp
Total findings: 1

- Line 53: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: findings.push_back({severity, endpoint_pattern, http_method, finding, recommendation});
  Confidence: band=high; score=0.74

### src/server/api_version.cpp
Total findings: 1

- Line 105: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Support current major version and previous major version for backward compatibility
  Confidence: band=high; score=0.8

### src/server/continuous_query_api_handler.cpp
Total findings: 1

- Line 234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(infoToJson(info));
  Confidence: band=high; score=0.74

### src/server/hot_reload_api_handler.cpp
Total findings: 1

- Line 248: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response_json.push_back(point);
  Confidence: band=high; score=0.74

### src/server/http_type_adapter.cpp
Total findings: 1

- Line 42: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += ' ';
  Confidence: band=high; score=0.74

### src/server/import_wizard_builder.cpp
Total findings: 1

- Line 268: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: html += "    var p=document.getElementById('panel-'+i);\n";
  Confidence: band=high; score=0.74

### src/server/merge_api_handler.cpp
Total findings: 1

- Line 215: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: options.manual_resolutions.push_back(resolution);
  Confidence: band=high; score=0.74

### src/server/opa_adapter.cpp
Total findings: 1

- Line 28: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void ensure_curl_global_init() {
  Confidence: band=medium; score=0.66

### src/server/pitr_api_handler.cpp
Total findings: 1

- Line 208: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: options.tables.push_back(table.get<std::string>());
  Confidence: band=high; score=0.74

### src/server/prompt_api_handler.cpp
Total findings: 1

- Line 76: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(t.toJson());
  Confidence: band=high; score=0.74

### src/server/rate_limiting_middleware.cpp
Total findings: 1

- Line 61: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: override_limiters_.push_back(
  Confidence: band=high; score=0.74

### src/server/saml_auth_provider.cpp
Total findings: 1

- Line 147: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: attrs[name].push_back(value);
  Confidence: band=high; score=0.74

### src/server/sse_connection_manager.cpp
Total findings: 1

- Line 352: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_conns.push_back(PollTarget{
  Confidence: band=high; score=0.74

### src/server/udf_api_handler.cpp
Total findings: 1

- Line 174: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(d.toJson());
  Confidence: band=high; score=0.74

### src/server/websocket_session.cpp
Total findings: 1

- Line 791: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cdc_sessions.push_back(session);
  Confidence: band=high; score=0.74

### include/server/examples/workload_fingerprint_example.cpp
Total findings: 0


### src/server/api_auth_config.cpp
Total findings: 0


### src/server/buffer_api_handler.cpp
Total findings: 0


### src/server/buffer_binary_protocol.cpp
Total findings: 0


### src/server/cache_api_handler.cpp
Total findings: 0


### src/server/cdn_cache_middleware.cpp
Total findings: 0


### src/server/cost_based_rate_limiter.cpp
Total findings: 0


### src/server/diff_api_handler.cpp
Total findings: 0


### src/server/grpc_web_proxy_handler.cpp
Total findings: 0


### src/server/http3_datagram.cpp
Total findings: 0


### src/server/mvcc_api_handler.cpp
Total findings: 0


### src/server/oauth2_provider.cpp
Total findings: 0


### src/server/policy_api_handler.cpp
Total findings: 0


### src/server/request_coalescing.cpp
Total findings: 0


### src/server/rpc/blob_transfer_handler.cpp
Total findings: 0


### src/server/update_api_handler.cpp
Total findings: 0


### src/server/workload_fingerprint_engine.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
